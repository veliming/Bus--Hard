/**
 * @file aiot_mqtt_api.c
 * @brief MQTT模块实现, 其中包含了连接到物联网平台和收发数据的API接口
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 */

#include "core_list.h"
#include "core_mqtt.h"
#include "aiot_at_mqtt_api.h"
void user_send_data_with_delay(char *buffer);
int user_get_data_with_delay(char *buffer, unsigned int length, unsigned int delay);
static int32_t _core_mqtt_sysdep_return(int32_t sysdep_code, int32_t core_code)
{
    if (sysdep_code >= (STATE_PORT_BASE - 0x00FF) && sysdep_code < (STATE_PORT_BASE)) {
        return sysdep_code;
    } else {
        return core_code;
    }
}

static void _core_mqtt_event_notify(core_mqtt_handle_t *mqtt_handle, aiot_mqtt_event_type_t type)
{
    if (mqtt_handle->event_handler) {
        aiot_mqtt_event_t event;
        memset(&event, 0, sizeof(aiot_mqtt_event_t));
        event.type = type;

        mqtt_handle->event_handler((void *)mqtt_handle, &event, mqtt_handle->userdata);
    }
}

static void _core_mqtt_connect_event_notify(core_mqtt_handle_t *mqtt_handle)
{
    mqtt_handle->disconnected = 0;
    if (mqtt_handle->has_connected == 0) {
        mqtt_handle->has_connected = 1;
        _core_mqtt_event_notify(mqtt_handle, AIOT_MQTTEVT_CONNECT);
    } else {
        _core_mqtt_event_notify(mqtt_handle, AIOT_MQTTEVT_RECONNECT);
    }
}

static void _core_mqtt_disconnect_event_notify(core_mqtt_handle_t *mqtt_handle,
        aiot_mqtt_disconnect_event_type_t disconnect)
{
    if (mqtt_handle->has_connected == 1 && mqtt_handle->disconnected == 0) {
        mqtt_handle->disconnected = 1;
        if (mqtt_handle->event_handler) {
            aiot_mqtt_event_t event;
            memset(&event, 0, sizeof(aiot_mqtt_event_t));
            event.type = AIOT_MQTTEVT_DISCONNECT;
            event.data.disconnect = disconnect;

            mqtt_handle->event_handler((void *)mqtt_handle, &event, mqtt_handle->userdata);
        }
    }
}

static void _core_mqtt_exec_inc(core_mqtt_handle_t *mqtt_handle)
{
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    mqtt_handle->exec_count++;
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
}

static void _core_mqtt_exec_dec(core_mqtt_handle_t *mqtt_handle)
{
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    mqtt_handle->exec_count--;
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
}

static void _core_mqtt_sign_clean(core_mqtt_handle_t *mqtt_handle)
{
    if (mqtt_handle->username) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->username);
        mqtt_handle->username = NULL;
    }
    if (mqtt_handle->password) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->password);
        mqtt_handle->password = NULL;
    }
    if (mqtt_handle->clientid) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->clientid);
        mqtt_handle->clientid = NULL;
    }
}

static int32_t _core_mqtt_handlerlist_insert(core_mqtt_handle_t *mqtt_handle, core_mqtt_sub_node_t *sub_node,
        aiot_mqtt_recv_handler_t handler, void *userdata)
{
    core_mqtt_sub_handler_node_t *node = NULL;

    core_list_for_each_entry(node, &sub_node->handle_list, linked_node) {
        if (node->handler == handler) {
            /* exist handler, replace userdata */
            node->userdata = userdata;
            return STATE_SUCCESS;
        }
    }

    if (&node->linked_node == &sub_node->handle_list) {
        /* new handler */
        node = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(core_mqtt_sub_handler_node_t), CORE_MQTT_MODULE_NAME);
        if (node == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node, 0, sizeof(core_mqtt_sub_handler_node_t));
        CORE_INIT_LIST_HEAD(&node->linked_node);
        node->handler = handler;
        node->userdata = userdata;

        core_list_add_tail(&node->linked_node, &sub_node->handle_list);
    }

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_sublist_insert(core_mqtt_handle_t *mqtt_handle, aiot_mqtt_buff_t *topic,
        aiot_mqtt_recv_handler_t handler, void *userdata)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_sub_node_t *node = NULL;

    core_list_for_each_entry(node, &mqtt_handle->sub_list, linked_node) {
        if ((strlen(node->topic) == topic->len) && memcmp(node->topic, topic->buffer, topic->len) == 0) {
            /* exist topic */
            if (handler != NULL) {
                return _core_mqtt_handlerlist_insert(mqtt_handle, node, handler, userdata);
            } else {
                return STATE_SUCCESS;
            }
        }
    }

    if (&node->linked_node == &mqtt_handle->sub_list) {
        /* new topic */
        node = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(core_mqtt_sub_node_t), CORE_MQTT_MODULE_NAME);
        if (node == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node, 0, sizeof(core_mqtt_sub_node_t));
        CORE_INIT_LIST_HEAD(&node->linked_node);
        CORE_INIT_LIST_HEAD(&node->handle_list);

        node->topic = mqtt_handle->sysdep->core_sysdep_malloc(topic->len + 1, CORE_MQTT_MODULE_NAME);
        if (node->topic == NULL) {
            mqtt_handle->sysdep->core_sysdep_free(node);
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node->topic, 0, topic->len + 1);
        memcpy(node->topic, topic->buffer, topic->len);

        if (handler != NULL) {
            res = _core_mqtt_handlerlist_insert(mqtt_handle, node, handler, userdata);
            if (res < STATE_SUCCESS) {
                mqtt_handle->sysdep->core_sysdep_free(node->topic);
                mqtt_handle->sysdep->core_sysdep_free(node);
                return res;
            }
        }

        core_list_add_tail(&node->linked_node, &mqtt_handle->sub_list);
    }
    return res;
}

static void _core_mqtt_sublist_handlerlist_destroy(core_mqtt_handle_t *mqtt_handle, struct core_list_head *list)
{
    core_mqtt_sub_handler_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, list, linked_node) {
        core_list_del(&node->linked_node);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }
}

static void _core_mqtt_sublist_remove(core_mqtt_handle_t *mqtt_handle, aiot_mqtt_buff_t *topic)
{
    core_mqtt_sub_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handle->sub_list, linked_node) {
        if ((strlen(node->topic) == topic->len) && memcmp(node->topic, topic->buffer, topic->len) == 0) {
            core_list_del(&node->linked_node);
            _core_mqtt_sublist_handlerlist_destroy(mqtt_handle, &node->handle_list);
            mqtt_handle->sysdep->core_sysdep_free(node->topic);
            mqtt_handle->sysdep->core_sysdep_free(node);
        }
    }
}

static void _core_mqtt_sublist_destroy(core_mqtt_handle_t *mqtt_handle)
{
    core_mqtt_sub_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handle->sub_list, linked_node) {
        core_list_del(&node->linked_node);
        _core_mqtt_sublist_handlerlist_destroy(mqtt_handle, &node->handle_list);
        mqtt_handle->sysdep->core_sysdep_free(node->topic);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }
}

static int32_t _core_mqtt_topic_is_valid(char *topic, uint32_t len)
{
    uint32_t idx = 0;

    /* topic should start with '/' */
    if (topic[0] != '/') {
        return STATE_MQTT_TOPIC_INVALID;
    }

    for (idx = 0; idx < len; idx++) {
        if (topic[idx] == '+') {
            /* topic should contain '/+/' in the middle or '/+' in the end */
            if ((topic[idx - 1] != '/') ||
                    ((idx + 1 < len) && (topic[idx + 1] != '/'))) {
                return STATE_MQTT_TOPIC_INVALID;
            }
        }
        if (topic[idx] == '#') {
            /* topic should contain '/#' in the end */
            if ((topic[idx - 1] != '/') ||
                    (idx + 1 < len)) {
                return STATE_MQTT_TOPIC_INVALID;
            }
        }
    }

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_append_topic_map(core_mqtt_handle_t *mqtt_handle, aiot_mqtt_topic_map_t *map)
{
    int32_t res = STATE_SUCCESS;

    if (map->topic.buffer == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (map->topic.len == 0) {
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if ((res = _core_mqtt_topic_is_valid((char *)map->topic.buffer, map->topic.len)) < STATE_SUCCESS) {
        return res;
    }

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->sub_mutex);
    res = _core_mqtt_sublist_insert(mqtt_handle, &map->topic, map->handler, map->userdata);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->sub_mutex);

    return res;
}


#if 0
static void _core_mqtt_set_utf8_encoded_str(uint8_t *input, uint16_t input_len, uint8_t *output)
{
    uint32_t idx = 0, input_idx = 0;

    /* String Length MSB */
    output[idx++] = (uint8_t)((input_len >> 8) & 0x00FF);

    /* String Length LSB */
    output[idx++] = (uint8_t)((input_len) & 0x00FF);

    /* UTF-8 Encoded Character Data */
    for (input_idx = 0; input_idx < input_len; input_idx++) {
        output[idx++] = input[input_idx];
    }
}
static void _core_mqtt_remain_len_encode(uint32_t input, uint8_t *output, uint32_t *output_idx)
{
    uint8_t encoded_byte = 0, idx = 0;

    do {
        encoded_byte = input % 128;
        input /= 128;
        if (input > 0) {
            encoded_byte |= 128;
        }
        output[idx++] = encoded_byte;
    } while (input > 0);

    *output_idx += idx;
}
static int32_t _core_mqtt_conn_pkt(core_mqtt_handle_t *mqtt_handle, uint8_t **pkt, uint32_t *pkt_len)
{
    uint32_t idx = 0, conn_paylaod_len = 0, conn_remainlen = 0, conn_pkt_len = 0;
    uint8_t *pos = NULL;
    const uint8_t conn_fixed_header = CORE_MQTT_CONN_PKT_TYPE;
    const uint8_t conn_protocol_name[] = {0x00, 0x04, 0x4D, 0x51, 0x54, 0x54};
    const uint8_t conn_protocol_level = 0x04;
    const uint8_t conn_connect_flag = 0xC0 | (mqtt_handle->clean_session << 1);

    /* Payload Length */
    conn_paylaod_len = (uint32_t)(strlen(mqtt_handle->clientid) + strlen(mqtt_handle->username)
                                  + strlen(mqtt_handle->password) + 3 * CORE_MQTT_UTF8_STR_EXTRA_LEN);

    /* Remain-Length Value */
    conn_remainlen = CORE_MQTT_CONN_REMAINLEN_FIXED_LEN + conn_paylaod_len;

    /* Total Packet Length */
    conn_pkt_len = CORE_MQTT_CONN_FIXED_HEADER_TOTAL_LEN + conn_paylaod_len;

    pos = mqtt_handle->sysdep->core_sysdep_malloc(conn_pkt_len, CORE_MQTT_MODULE_NAME);
    if (pos == NULL) {
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }
    memset(pos, 0, conn_pkt_len);

    /* Fixed Header */
    pos[idx++] = conn_fixed_header;

    /* Remain Length */
    _core_mqtt_remain_len_encode(conn_remainlen, pos + idx, &idx);

    /* Protocol Name */
    memcpy(pos + idx, conn_protocol_name, CORE_MQTT_CONN_PROTOCOL_NAME_LEN);
    idx += CORE_MQTT_CONN_PROTOCOL_NAME_LEN;

    /* Protocol Level */
    pos[idx++] = conn_protocol_level;

    /* Connect Flag */
    pos[idx++] = conn_connect_flag;

    /* Keep Alive MSB */
    pos[idx++] = (uint8_t)((mqtt_handle->keep_alive_s >> 8) & 0x00FF);

    /* Keep Alive LSB */
    pos[idx++] = (uint8_t)((mqtt_handle->keep_alive_s) & 0x00FF);

    /* Payload: clientid, username, password */
    _core_mqtt_set_utf8_encoded_str((uint8_t *)mqtt_handle->clientid, strlen(mqtt_handle->clientid), pos + idx);
    idx += CORE_MQTT_UTF8_STR_EXTRA_LEN + strlen(mqtt_handle->clientid);
    _core_mqtt_set_utf8_encoded_str((uint8_t *)mqtt_handle->username, strlen(mqtt_handle->username), pos + idx);
    idx += CORE_MQTT_UTF8_STR_EXTRA_LEN + strlen(mqtt_handle->username);
    _core_mqtt_set_utf8_encoded_str((uint8_t *)mqtt_handle->password, strlen(mqtt_handle->password), pos + idx);
    idx += CORE_MQTT_UTF8_STR_EXTRA_LEN + strlen(mqtt_handle->password);

    *pkt = pos;
    *pkt_len = idx;

    return STATE_SUCCESS;
}
#endif
#if 0
static int32_t _core_mqtt_connack_handle(core_mqtt_handle_t *mqtt_handle, uint8_t *connack)
{
    int32_t res = STATE_SUCCESS;
    const uint8_t connack_byte1 = CORE_MQTT_CONNACK_PKT_TYPE;
    const uint8_t connack_byte2 = 0x02;
    const uint8_t connack_byte3 = 0x00;

    if (connack[0] != connack_byte1 || connack[1] != connack_byte2 ||
            connack[2] != connack_byte3) {
        return STATE_MQTT_CONNACK_FMT_ERROR;
    }

    if (connack[3] == CORE_MQTT_CONNACK_RCODE_ACCEPTED) {
        res = STATE_SUCCESS;
    } else if (connack[3] == CORE_MQTT_CONNACK_RCODE_UNACCEPTABLE_PROTOCOL_VERSION) {
        core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT invalid protocol version, disconnect\r\n");
        res = STATE_MQTT_CONNACK_RCODE_UNACCEPTABLE_PROTOCOL_VERSION;
    } else if (connack[3] == CORE_MQTT_CONNACK_RCODE_SERVER_UNAVAILABLE) {
        core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT server unavailable, disconnect\r\n");
        res = STATE_MQTT_CONNACK_RCODE_SERVER_UNAVAILABLE;
    } else if (connack[3] == CORE_MQTT_CONNACK_RCODE_BAD_USERNAME_PASSWORD) {
        core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT bad username or password, disconnect\r\n");
        res = STATE_MQTT_CONNACK_RCODE_BAD_USERNAME_PASSWORD;
    } else if (connack[3] == CORE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED) {
        core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT authorize fail, disconnect\r\n");
        res = STATE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED;
    } else {
        res = STATE_MQTT_CONNACK_RCODE_UNKNOWN;
    }

    return res;
}
#endif

static int32_t _core_mqtt_read(core_mqtt_handle_t *mqtt_handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms)
{
    int32_t res = STATE_SUCCESS;

    if (mqtt_handle->network_handle != NULL) {
        res = mqtt_handle->sysdep->core_sysdep_network_recv(mqtt_handle->network_handle, buffer, len, timeout_ms, NULL);
        if (res < STATE_SUCCESS) {
            mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT network error when receving data, disconnect\r\n");
            res = _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_CLOSED);
        } else if (res != len) {
            res = STATE_SYS_DEPEND_NWK_READ_LESSDATA;
        }
    } else {
        res = STATE_SYS_DEPEND_NWK_CLOSED;
    }

    return res;
}

static int32_t _core_mqtt_write(core_mqtt_handle_t *mqtt_handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms)
{
    int32_t res = STATE_SUCCESS;

    if (mqtt_handle->network_handle != NULL) {
        res = mqtt_handle->sysdep->core_sysdep_network_send(mqtt_handle->network_handle, buffer, len, timeout_ms, NULL);
        if (res < STATE_SUCCESS) {
            mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT network error when sending data, disconnect\r\n");
            res = _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_CLOSED);
        } else if (res != len) {
            res = STATE_SYS_DEPEND_NWK_WRITE_LESSDATA;
        }
    } else {
        res = STATE_SYS_DEPEND_NWK_CLOSED;
    }

    return res;
}

static int32_t _core_mqtt_connect(core_mqtt_handle_t *mqtt_handle)
{
    int32_t res = 0;
    core_sysdep_socket_type_t socket_type = CORE_SYSDEP_SOCKET_TCP_CLIENT;
    char *secure_mode = (mqtt_handle->cred == NULL) ? ("3") : ("2");

    if (mqtt_handle->host == NULL) {
        return STATE_USER_INPUT_MISSING_HOST;
    }

    if (mqtt_handle->security_mode != NULL) {
        secure_mode = mqtt_handle->security_mode;
    }

    if (mqtt_handle->cred && \
            mqtt_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_NONE && \
            mqtt_handle->security_mode == NULL) {
        secure_mode = "3";
    }

    if (mqtt_handle->username == NULL || mqtt_handle->password == NULL ||
            mqtt_handle->clientid == NULL) {
        /* no valid username, password or clientid, check pk/dn/ds */
        if (mqtt_handle->product_key == NULL) {
            return STATE_USER_INPUT_MISSING_PRODUCT_KEY;
        }
        if (mqtt_handle->device_name == NULL) {
            return STATE_USER_INPUT_MISSING_DEVICE_NAME;
        }
        if (mqtt_handle->device_secret == NULL) {
            return STATE_USER_INPUT_MISSING_DEVICE_SECRET;
        }
        _core_mqtt_sign_clean(mqtt_handle);

        if ((res = core_auth_mqtt_username(mqtt_handle->sysdep, &mqtt_handle->username, mqtt_handle->product_key,
                                           mqtt_handle->device_name, CORE_MQTT_MODULE_NAME)) < STATE_SUCCESS ||
                (res = core_auth_mqtt_password(mqtt_handle->sysdep, &mqtt_handle->password, mqtt_handle->product_key,
                                               mqtt_handle->device_name, mqtt_handle->device_secret, CORE_MQTT_MODULE_NAME)) < STATE_SUCCESS ||
                (res = core_auth_mqtt_clientid(mqtt_handle->sysdep, &mqtt_handle->clientid, mqtt_handle->product_key,
                                               mqtt_handle->device_name, secure_mode, mqtt_handle->extend_clientid, CORE_MQTT_MODULE_NAME)) < STATE_SUCCESS) {
            _core_mqtt_sign_clean(mqtt_handle);
            return res;
        }
        core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_USERNAME, "%s\r\n", (void *)mqtt_handle->username);
        core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_PASSWORD, "%s\r\n", (void *)mqtt_handle->password);
        core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_CLIENTID, "%s\r\n", (void *)mqtt_handle->clientid);
    }



    if (mqtt_handle->network_handle != NULL) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
    }

    mqtt_handle->network_handle = mqtt_handle->sysdep->core_sysdep_network_init();
    if (mqtt_handle->network_handle == NULL) {
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }

    if ((res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_SOCKET_TYPE,
               &socket_type)) < STATE_SUCCESS ||
            (res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_HOST,
                    mqtt_handle->host)) < STATE_SUCCESS ||
            (res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_PORT,
                    &mqtt_handle->port)) < STATE_SUCCESS ||
            (res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle,
                    CORE_SYSDEP_NETWORK_CONNECT_TIMEOUT_MS,
                    &mqtt_handle->connect_timeout_ms)) < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
        return _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_INVALID_OPTION);
    }

    if (mqtt_handle->cred != NULL) {
        if ((res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_CRED,
                   mqtt_handle->cred)) < STATE_SUCCESS) {
            mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            return _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_INVALID_OPTION);
        }
        if (mqtt_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_SVRCERT_PSK) {
            char *psk_id = NULL, psk[65] = {0};
            core_sysdep_psk_t sysdep_psk;

            res = core_auth_tls_psk(mqtt_handle->sysdep, &psk_id, psk, mqtt_handle->product_key, mqtt_handle->device_name,
                                    mqtt_handle->device_secret, CORE_MQTT_MODULE_NAME);
            if (res < STATE_SUCCESS) {
                return res;
            }

            memset(&sysdep_psk, 0, sizeof(core_sysdep_psk_t));
            sysdep_psk.psk_id = psk_id;
            sysdep_psk.psk = psk;
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_TLS_PSK, "%s\r\n", sysdep_psk.psk_id);
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_TLS_PSK, "%s\r\n", sysdep_psk.psk);
            res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_PSK,
                    (void *)&sysdep_psk);
            mqtt_handle->sysdep->core_sysdep_free(psk_id);
            if (res < STATE_SUCCESS) {
                return _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_INVALID_OPTION);
            }
        }
    }

    // n720 send at+mqttconnparam cmd
    {
        char *at_mqttconnparam_cmd = NULL;
        char *src[] = {"AT+MQTTCONNPARAM=\"", mqtt_handle->clientid, "\",\"", mqtt_handle->username, "\",\"", mqtt_handle->password, "\"\r\n"};
        uint8_t topic_len = sizeof(src) / sizeof(char *);
        res = core_sprintf(mqtt_handle->sysdep, &at_mqttconnparam_cmd, "%s%s%s%s%s%s%s", src, topic_len, CORE_MQTT_MODULE_NAME);
        if (res < 0) {
            return -1;
        }

        int iter = 0;
        while (iter++ < 10) {
            user_send_data_with_delay(at_mqttconnparam_cmd);
            char buffer[AT_CMD_DEFAULT_BUFFER_SIZE] = {0};
            int ret = user_get_data_with_delay(buffer, AT_CMD_DEFAULT_BUFFER_SIZE, 100);
            if (ret == 0) {
                break;
            }
        }

        if (NULL != at_mqttconnparam_cmd) {
            mqtt_handle->sysdep->core_sysdep_free(at_mqttconnparam_cmd);
        }
    }


    // n720 send at+mqttconn cmd
    {
        char *at_mqttconn_cmd = NULL;
        char *src[] = {"AT+MQTTCONN=", mqtt_handle->host, ":1883,0,171\r\n"};
        uint8_t topic_len = sizeof(src) / sizeof(char *);
        res = core_sprintf(mqtt_handle->sysdep, &at_mqttconn_cmd, "%s%s%s", src, topic_len, CORE_MQTT_MODULE_NAME);
        if (res < 0) {
            return -1;
        }

        int iter = 0;
        while (iter++ < 10) {
            user_send_data_with_delay(at_mqttconn_cmd);
            char buffer[50] = {0};
            int ret = user_get_data_with_delay(buffer, 50, 0x100);
            if (ret == 0) {
                break;
            }
        }

        if (NULL != at_mqttconn_cmd) {
            mqtt_handle->sysdep->core_sysdep_free(at_mqttconn_cmd);
        }
    }




#if 0
    if ((res = mqtt_handle->sysdep->core_sysdep_network_establish(mqtt_handle->network_handle)) < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
        return _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_EST_FAILED);
    }

    /* Get MQTT Connect Packet */
    res = _core_mqtt_conn_pkt(mqtt_handle, &conn_pkt, &conn_pkt_len);
    if (res < STATE_SUCCESS) {
        return res;
    }

    /* Send MQTT Connect Packet */
    res = _core_mqtt_write(mqtt_handle, conn_pkt, conn_pkt_len, mqtt_handle->send_timeout_ms);
    mqtt_handle->sysdep->core_sysdep_free(conn_pkt);
    if (res < STATE_SUCCESS) {
        if (res == STATE_SYS_DEPEND_NWK_WRITE_LESSDATA) {
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT_TIMEOUT, "MQTT connect packet send timeout: %d\r\n",
                      &mqtt_handle->send_timeout_ms);
        }
        return res;
    }

    /* Receive MQTT Connect ACK Packet */
    res = _core_mqtt_read(mqtt_handle, connack_pkt, CORE_MQTT_CONNACK_FIXED_HEADER_TOTAL_LEN, mqtt_handle->recv_timeout_ms);
    if (res < STATE_SUCCESS) {
        if (res == STATE_SYS_DEPEND_NWK_READ_LESSDATA) {
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT_TIMEOUT, "MQTT connack packet recv timeout: %d\r\n",
                      &mqtt_handle->recv_timeout_ms);
        }
        return res;
    }

    res = _core_mqtt_connack_handle(mqtt_handle, connack_pkt);
    if (res < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
        return res;
    }
#endif

    return STATE_MQTT_CONNECT_SUCCESS;
}

static int32_t _core_mqtt_disconnect(core_mqtt_handle_t *mqtt_handle)
{
    int res = STATE_SUCCESS;
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    {
        int iter = 0;
        while (iter++ < 10) {
            user_send_data_with_delay("AT+MQTTDISCONN\r\n");
            char buffer[AT_CMD_DEFAULT_BUFFER_SIZE] = {0};
            int ret = user_get_data_with_delay(buffer, AT_CMD_DEFAULT_BUFFER_SIZE, 100);
            if (ret == 0) {
                break;
            }
        }
    }

    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    if (res < STATE_SUCCESS) {
        return res;
    }

    return STATE_SUCCESS;
}

#if 0
static uint16_t _core_mqtt_packet_id(core_mqtt_handle_t *mqtt_handle)
{
    uint16_t packet_id = 0;

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    if ((uint16_t)(mqtt_handle->packet_id + 1) == 0) {
        mqtt_handle->packet_id = 0;
    }
    packet_id = ++mqtt_handle->packet_id;
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);

    return packet_id;
}
#endif
#if 0
static int32_t _core_mqtt_publist_insert(core_mqtt_handle_t *mqtt_handle, uint8_t *packet, uint32_t len,
        uint16_t packet_id)
{
    core_mqtt_pub_node_t *node = NULL;

    core_list_for_each_entry(node, &mqtt_handle->pub_list, linked_node) {
        if (node->packet_id == packet_id) {
            return STATE_MQTT_PUBLIST_PACKET_ID_ROLL;
        }
    }

    node = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(core_mqtt_pub_node_t), CORE_MQTT_MODULE_NAME);
    if (node == NULL) {
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }
    memset(node, 0, sizeof(core_mqtt_pub_node_t));
    CORE_INIT_LIST_HEAD(&node->linked_node);
    node->packet_id = packet_id;
    node->packet = mqtt_handle->sysdep->core_sysdep_malloc(len, CORE_MQTT_MODULE_NAME);
    if (node->packet == NULL) {
        mqtt_handle->sysdep->core_sysdep_free(node);
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }
    memset(node->packet, 0, len);
    memcpy(node->packet, packet, len);
    node->len = len;
    node->last_send_time = mqtt_handle->sysdep->core_sysdep_time();

    core_list_add_tail(&node->linked_node, &mqtt_handle->pub_list);

    return STATE_SUCCESS;
}
#endif


static void _core_mqtt_publist_remove(core_mqtt_handle_t *mqtt_handle, uint16_t packet_id)
{
    core_mqtt_pub_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handle->pub_list, linked_node) {
        if (node->packet_id == packet_id) {
            core_list_del(&node->linked_node);
            mqtt_handle->sysdep->core_sysdep_free(node->packet);
            mqtt_handle->sysdep->core_sysdep_free(node);
            return;
        }
    }
}

static void _core_mqtt_publist_destroy(core_mqtt_handle_t *mqtt_handle)
{
    core_mqtt_pub_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handle->pub_list, linked_node) {
        core_list_del(&node->linked_node);
        mqtt_handle->sysdep->core_sysdep_free(node->packet);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }
}

static int32_t _core_mqtt_subunsub(core_mqtt_handle_t *mqtt_handle, char *topic, uint16_t topic_len, uint8_t qos,
                                   uint8_t pkt_type)
{
    int ret = STATE_SUCCESS;
    int res = 0;
    // n720 send at+mqttconnparam cmd
    if (pkt_type == CORE_MQTT_SUB_PKT_TYPE) {
        {
            void *pkt = NULL;
            pkt = mqtt_handle->sysdep->core_sysdep_malloc(topic_len + 1, CORE_MQTT_MODULE_NAME);
            if (NULL == pkt) {
                return STATE_SYS_DEPEND_MALLOC_FAILED;
            }
            memset(pkt, 0, topic_len + 1);
            memcpy(pkt, topic, topic_len);

            char *at_mqttsub = NULL;
            char dbg_buffer[100] = {0};
            char *src[] = {"AT+mqttsub=\"", pkt, "\",0\r\n"};
            uint8_t list_len = sizeof(src) / sizeof(char *);
            res = core_sprintf(mqtt_handle->sysdep, &at_mqttsub, "%s%s%s", src, list_len, CORE_MQTT_MODULE_NAME);
            if(NULL != pkt) {
                mqtt_handle->sysdep->core_sysdep_free(pkt);
            }
            if (res < 0) {
                return -1;
            }

            int iter = 0;
            int test_len = strlen(at_mqttsub);

            while (iter++ < 10) {
                user_send_data_with_delay(at_mqttsub);
                memcpy(dbg_buffer, at_mqttsub, strlen(at_mqttsub));
                char dd = dbg_buffer[0];
                char buffer[6] = {0};
                ret = user_get_data_with_delay(buffer, 6, 2000);
                if (ret == 0) {
                    break;
                }
            }

            if (NULL != at_mqttsub) {
                mqtt_handle->sysdep->core_sysdep_free(at_mqttsub);
            }
            return ret;
        }
    }
    if (pkt_type == CORE_MQTT_UNSUB_PKT_TYPE) {
        {
            void *pkt = NULL;
            pkt = mqtt_handle->sysdep->core_sysdep_malloc(topic_len + 1, CORE_MQTT_MODULE_NAME);
            if (NULL == pkt) {
                return STATE_SYS_DEPEND_MALLOC_FAILED;
            }
            memset(pkt, 0, topic_len + 1);
            memcpy(pkt, topic, topic_len);

            char *at_mqttunsub = NULL;
            char *src[] = {"AT+MQTTUNSUB=\"", pkt, "\"\r\n"};
            uint8_t topic_len = sizeof(src) / sizeof(char *);
            res = core_sprintf(mqtt_handle->sysdep, &at_mqttunsub, "%s%s%s", src, topic_len, CORE_MQTT_MODULE_NAME);
            if(NULL != pkt) {
                mqtt_handle->sysdep->core_sysdep_free(pkt);
            }
            if (res < 0) {
                return -1;
            }

            int iter = 0;
            while (iter++ < 10) {
                user_send_data_with_delay(at_mqttunsub);
                char buffer[AT_CMD_DEFAULT_BUFFER_SIZE] = {0};
                ret = user_get_data_with_delay(buffer, AT_CMD_DEFAULT_BUFFER_SIZE, 100);
                if (ret == 0) {
                    break;
                }
            }

            if (NULL != at_mqttunsub) {
                mqtt_handle->sysdep->core_sysdep_free(at_mqttunsub);
            }
            return ret;
        }
    }
}

static int32_t _core_mqtt_heartbeat(core_mqtt_handle_t *mqtt_handle)
{
    int32_t res = 0;
    uint8_t pingreq_pkt[2] = { CORE_MQTT_PINGREQ_PKT_TYPE, 0x00 };

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    res = _core_mqtt_write(mqtt_handle, pingreq_pkt, 2, mqtt_handle->send_timeout_ms);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    if (res < STATE_SUCCESS) {
        return res;
    }

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_repub(core_mqtt_handle_t *mqtt_handle)
{
    int32_t res = 0;
    uint64_t time_now = 0;
    core_mqtt_pub_node_t *node = NULL;

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->pub_mutex);
    core_list_for_each_entry(node, &mqtt_handle->pub_list, linked_node) {
        time_now = mqtt_handle->sysdep->core_sysdep_time();
        if (time_now < node->last_send_time) {
            node->last_send_time = time_now;
        }
        if ((time_now - node->last_send_time) >= mqtt_handle->repub_timeout_ms) {
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
            res = _core_mqtt_write(mqtt_handle, node->packet, node->len, mqtt_handle->send_timeout_ms);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
            if (res < STATE_SUCCESS) {
                return res;
            }
        }
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->pub_mutex);

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_process_handlerlist_insert(core_mqtt_handle_t *mqtt_handler,
        core_mqtt_process_data_t *process_handler)
{
    core_mqtt_process_handler_node_t *node = NULL;

    core_list_for_each_entry(node, &mqtt_handler->process_handler_list, linked_node) {
        if (node->process_handler.handler == process_handler->handler) {
            node->process_handler.context = process_handler->context;
            return STATE_SUCCESS;
        }
    }

    if (&node->linked_node == &mqtt_handler->process_handler_list) {
        node = mqtt_handler->sysdep->core_sysdep_malloc(sizeof(core_mqtt_process_handler_node_t), CORE_MQTT_MODULE_NAME);
        if (node == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node, 0, sizeof(core_mqtt_process_handler_node_t));
        CORE_INIT_LIST_HEAD(&node->linked_node);
        memcpy(node, process_handler, sizeof(core_mqtt_process_data_t));

        core_list_add_tail(&node->linked_node, &mqtt_handler->process_handler_list);
    }

    return STATE_SUCCESS;
}

static void _core_mqtt_process_handlerlist_destroy(core_mqtt_handle_t *mqtt_handler)
{
    core_mqtt_process_handler_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handler->process_handler_list, linked_node) {
        core_list_del(&node->linked_node);
        mqtt_handler->sysdep->core_sysdep_free(node);
    }
}

static int32_t _core_mqtt_append_process_handler(core_mqtt_handle_t *mqtt_handler,
        core_mqtt_process_data_t *process_handler)
{
    int32_t res = STATE_SUCCESS;

    if (process_handler->handler == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    mqtt_handler->sysdep->core_sysdep_mutex_lock(mqtt_handler->process_handler_mutex);
    res = _core_mqtt_process_handlerlist_insert(mqtt_handler, process_handler);
    mqtt_handler->sysdep->core_sysdep_mutex_unlock(mqtt_handler->process_handler_mutex);

    return res;
}

static void _core_mqtt_process_handler_process(core_mqtt_handle_t *mqtt_handler)
{
    core_mqtt_process_handler_node_t *node = NULL;

    mqtt_handler->sysdep->core_sysdep_mutex_lock(mqtt_handler->process_handler_mutex);
    core_list_for_each_entry(node, &mqtt_handler->process_handler_list, linked_node) {
        node->process_handler.handler(node->process_handler.context);
    }
    mqtt_handler->sysdep->core_sysdep_mutex_unlock(mqtt_handler->process_handler_mutex);
}

static int32_t _core_mqtt_reconnect(core_mqtt_handle_t *mqtt_handle)
{
    int32_t res = STATE_SYS_DEPEND_NWK_CLOSED;
    uint64_t time_now = 0;

    if (mqtt_handle->network_handle != NULL) {
        return STATE_SUCCESS;
    }

    core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_RECONNECTING, "MQTT network disconnect, try to reconnecting...\r\n");

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
    time_now = mqtt_handle->sysdep->core_sysdep_time();
    if (time_now < mqtt_handle->reconnect_params.last_retry_time) {
        mqtt_handle->reconnect_params.last_retry_time = time_now;
    }
    if (time_now >= (mqtt_handle->reconnect_params.last_retry_time + mqtt_handle->reconnect_params.interval_ms)) {
        mqtt_handle->reconnect_params.last_retry_time = time_now;
        res = _core_mqtt_connect(mqtt_handle);
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);

    return res;
}

static int32_t _core_mqtt_read_remainlen(core_mqtt_handle_t *mqtt_handle, uint32_t *remainlen)
{
    int32_t res = 0;
    uint8_t ch = 0;
    uint32_t multiplier = 1;
    uint32_t mqtt_remainlen = 0;

    do {
        res = _core_mqtt_read(mqtt_handle, &ch, 1, mqtt_handle->recv_timeout_ms);
        if (res < STATE_SUCCESS) {
            return res;
        }
        mqtt_remainlen += (ch & 127) * multiplier;
        multiplier *= 128;
        if (multiplier > 128 * 128 * 128) {
            return STATE_MQTT_MALFORMED_REMAINING_LEN;
        }
    } while ((ch & 128) != 0);

    *remainlen = mqtt_remainlen;

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_read_remainbytes(core_mqtt_handle_t *mqtt_handle, uint32_t remainlen, uint8_t **output)
{
    int32_t res = 0;
    uint8_t *remain = NULL;

    if (remainlen > 0) {
        remain = mqtt_handle->sysdep->core_sysdep_malloc(remainlen, CORE_MQTT_MODULE_NAME);
        if (remain == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(remain, 0, remainlen);

        res = _core_mqtt_read(mqtt_handle, remain, remainlen, mqtt_handle->recv_timeout_ms);
        if (res < STATE_SUCCESS) {
            mqtt_handle->sysdep->core_sysdep_free(remain);
            if (res == STATE_SYS_DEPEND_NWK_READ_LESSDATA) {
                return STATE_MQTT_MALFORMED_REMAINING_BYTES;
            } else {
                return res;
            }
        }
    }
    *output = remain;

    return STATE_SUCCESS;
}

static void _core_mqtt_pingresp_handler(core_mqtt_handle_t *mqtt_handle)
{
    aiot_mqtt_recv_t packet;

    memset(&packet, 0, sizeof(aiot_mqtt_recv_t));

    if (mqtt_handle->recv_handler == NULL) {
        return;
    }

    packet.type = AIOT_MQTTRECV_HEARTBEAT_RESPONSE;
    mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
}

static int32_t _core_mqtt_puback_send(core_mqtt_handle_t *mqtt_handle, uint16_t packet_id)
{
    int32_t res = 0;
    uint8_t pkt[4] = {0};

    /* Packet Type */
    pkt[0] = CORE_MQTT_PUBACK_PKT_TYPE;

    /* Remaining Lengh */
    pkt[1] = 2;

    /* Packet Id */
    pkt[2] = (uint16_t)((packet_id >> 8) & 0x00FF);
    pkt[3] = (uint16_t)((packet_id) & 0x00FF);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    res = _core_mqtt_write(mqtt_handle, pkt, 4, mqtt_handle->send_timeout_ms);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    if (res < STATE_SUCCESS) {
        return res;
    }

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_topic_compare(char *topic, uint32_t topic_len, char *cmp_topic, uint32_t cmp_topic_len)
{
    uint32_t idx = 0, cmp_idx = 0;

    for (idx = 0, cmp_idx = 0; idx < topic_len; idx++) {
        /* compare topic alreay out of bounds */
        if (cmp_idx >= cmp_topic_len) {
            /* compare success only in case of the left string of topic is "/#" */
            if ((topic_len - idx == 2) && (memcmp(&topic[idx], "/#", 2) == 0)) {
                return STATE_SUCCESS;
            } else {
                return STATE_MQTT_TOPIC_COMPARE_FIALED;
            }
        }

        /* if topic reach the '#', compare success */
        if (topic[idx] == '#') {
            return STATE_SUCCESS;
        }

        if (topic[idx] == '+') {
            /* wildcard + exist */
            for (; cmp_idx < cmp_topic_len; cmp_idx++) {
                if (cmp_topic[cmp_idx] == '/') {
                    /* if topic already reach the bound, compare topic should not contain '/' */
                    if (idx + 1 == topic_len) {
                        return STATE_MQTT_TOPIC_COMPARE_FIALED;
                    } else {
                        break;
                    }
                }
            }
        } else {
            /* compare each character */
            if (topic[idx] != cmp_topic[cmp_idx]) {
                return STATE_MQTT_TOPIC_COMPARE_FIALED;
            }
            cmp_idx++;
        }
    }

    /* compare topic should be reach the end */
    if (cmp_idx < cmp_topic_len) {
        return STATE_MQTT_TOPIC_COMPARE_FIALED;
    }
    return STATE_SUCCESS;
}

static void _core_mqtt_handlerlist_append(core_mqtt_handle_t *mqtt_handle, struct core_list_head *dest,
        struct core_list_head *src, uint8_t *found)
{
    core_mqtt_sub_handler_node_t *node = NULL, *copy_node = NULL;

    core_list_for_each_entry(node, src, linked_node) {
        copy_node = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(core_mqtt_sub_handler_node_t), CORE_MQTT_MODULE_NAME);
        if (copy_node == NULL) {
            continue;
        }
        memset(copy_node, 0, sizeof(core_mqtt_sub_handler_node_t));
        CORE_INIT_LIST_HEAD(&copy_node->linked_node);
        copy_node->handler = node->handler;
        copy_node->userdata = node->userdata;

        core_list_add_tail(&copy_node->linked_node, dest);
        *found = 1;
    }
}

static void _core_mqtt_handlerlist_destroy(core_mqtt_handle_t *mqtt_handle, struct core_list_head *list)
{
    core_mqtt_sub_handler_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, list, linked_node) {
        core_list_del(&node->linked_node);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }
}

static void _core_mqtt_pub_handler(core_mqtt_handle_t *mqtt_handle, uint8_t *input, uint32_t len, uint8_t qos)
{
    uint8_t sub_found = 0;
    uint32_t idx = 0;
    uint16_t packet_id = 0;
    uint16_t utf8_strlen = 0;
    aiot_mqtt_recv_t packet;
    void *userdata;
    core_mqtt_sub_node_t *sub_node = NULL;
    core_mqtt_sub_handler_node_t *handler_node = NULL;
    struct core_list_head handler_list_copy;

    if (input == NULL || len == 0 || qos > CORE_MQTT_QOS1) {
        return;
    }

    memset(&packet, 0, sizeof(aiot_mqtt_recv_t));
    packet.type = AIOT_MQTTRECV_PUB;

    /* Topic Length */
    // 目前接收到的是"topic",payload"的形式, 因此有必要把",作为一个判断报文完整性的判断条件
    char *topic_end = strstr((const char *)input, "\",");

    if(NULL == topic_end) {
        return;
    }

    utf8_strlen = topic_end -input -1;

    packet.data.pub.topic = (char *)&input[idx+1];
    packet.data.pub.topic_len = utf8_strlen;

    idx += utf8_strlen;

    /* Packet Id For QOS1 */
    if (qos == CORE_MQTT_QOS1) {
    }

    /* Payload Len */
    if ((int64_t)len - (int64_t)packet.data.pub.topic_len < 0) {
        return;
    }
    packet.data.pub.payload = &input[idx+3];
    //payload中有"topic",payload"的形式,去掉引号和逗号,因此要减去4
    packet.data.pub.payload_len = len - packet.data.pub.topic_len - 4;

    if(len - packet.data.pub.topic_len - packet.data.pub.payload_len <= 0) {
        return;
    }

    /* Publish Ack For QOS1 */
    if (qos == CORE_MQTT_QOS1) {
    }

    /* debug */
    core_log2(mqtt_handle->sysdep, STATE_MQTT_LOG_TOPIC, "pub: %.*s\r\n", &packet.data.pub.topic_len,
              packet.data.pub.topic);
    //core_log_hexdump(STATE_MQTT_LOG_HEXDUMP, '<', packet.data.pub.payload, packet.data.pub.payload_len);

    /* Search Packet Handler In sublist */
    CORE_INIT_LIST_HEAD(&handler_list_copy);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->sub_mutex);
    core_list_for_each_entry(sub_node, &mqtt_handle->sub_list, linked_node) {
        if (_core_mqtt_topic_compare(sub_node->topic, (uint32_t)(strlen(sub_node->topic)), packet.data.pub.topic,
                                     packet.data.pub.topic_len) == STATE_SUCCESS) {
            _core_mqtt_handlerlist_append(mqtt_handle, &handler_list_copy, &sub_node->handle_list, &sub_found);
        }
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->sub_mutex);

    core_list_for_each_entry(handler_node, &handler_list_copy, linked_node) {
        userdata = (handler_node->userdata == NULL) ? (mqtt_handle->userdata) : (handler_node->userdata);
        handler_node->handler(mqtt_handle, &packet, userdata);
    }

    _core_mqtt_handlerlist_destroy(mqtt_handle, &handler_list_copy);

    /* User Data Default Packet Handler */
    if (mqtt_handle->recv_handler && sub_found == 0) {
        mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
    }
}

static void _core_mqtt_puback_handler(core_mqtt_handle_t *mqtt_handle, uint8_t *input, uint32_t len)
{
    aiot_mqtt_recv_t packet;

    if (input == NULL || len != 2) {
        return;
    }

    memset(&packet, 0, sizeof(aiot_mqtt_recv_t));
    packet.type = AIOT_MQTTRECV_PUB_ACK;

    /* Packet Id */
    packet.data.pub_ack.packet_id = input[0] << 8;
    packet.data.pub_ack.packet_id |= input[1];

    /* Remove Packet From republist */
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->pub_mutex);
    _core_mqtt_publist_remove(mqtt_handle, packet.data.pub_ack.packet_id);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->pub_mutex);

    /* User Ctrl Packet Handler */
    if (mqtt_handle->recv_handler) {
        mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
    }
}

static void _core_mqtt_subunsuback_handler(core_mqtt_handle_t *mqtt_handle, uint8_t *input, uint32_t len,
        uint8_t packet_type)
{
    uint32_t idx = 0;
    aiot_mqtt_recv_t packet;

    if (input == NULL || len == 0 ||
            (packet_type == CORE_MQTT_SUBACK_PKT_TYPE && len % 3 != 0) ||
            (packet_type == CORE_MQTT_UNSUBACK_PKT_TYPE && len % 2 != 0)) {
        return;
    }

    for (idx = 0; idx < len;) {
        memset(&packet, 0, sizeof(aiot_mqtt_recv_t));
        if (packet_type == CORE_MQTT_SUBACK_PKT_TYPE) {
            packet.type = AIOT_MQTTRECV_SUB_ACK;
            packet.data.sub_ack.packet_id = input[idx] << 8;
            packet.data.sub_ack.packet_id |= input[idx + 1];
        } else {
            packet.type = AIOT_MQTTRECV_UNSUB_ACK;
            packet.data.unsub_ack.packet_id = input[idx] << 8;
            packet.data.unsub_ack.packet_id |= input[idx + 1];
        }

        if (packet_type == CORE_MQTT_SUBACK_PKT_TYPE) {
            if (input[idx + 2] == CORE_MQTT_SUBACK_RCODE_MAXQOS0 ||
                    input[idx + 2] == CORE_MQTT_SUBACK_RCODE_MAXQOS1 ||
                    input[idx + 2] == CORE_MQTT_SUBACK_RCODE_MAXQOS2) {
                packet.data.sub_ack.res = STATE_SUCCESS;
                packet.data.sub_ack.max_qos = input[idx + 2];
            } else if (input[idx + 2] == CORE_MQTT_SUBACK_RCODE_FAILURE) {
                packet.data.sub_ack.res = STATE_MQTT_SUBACK_RCODE_FAILURE;
            } else {
                packet.data.sub_ack.res = STATE_MQTT_SUBACK_RCODE_UNKNOWN;
            }
            idx += 3;
        } else {
            idx += 2;
        }

        if (mqtt_handle->recv_handler) {
            mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
        }
    }
}

int32_t core_mqtt_setopt(void *handle, core_mqtt_option_t option, void *data)
{
    int32_t res = 0;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL || data == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (option >= CORE_MQTTOPT_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    switch (option) {
    case CORE_MQTTOPT_APPEND_PROCESS_HANDLER: {
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
        res = _core_mqtt_append_process_handler(mqtt_handle, (core_mqtt_process_data_t *)data);
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    }
    break;
    default: {
        res = STATE_USER_INPUT_UNKNOWN_OPTION;
    }
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);

    return res;
}

void *aiot_mqtt_init(void)
{
    core_mqtt_handle_t *mqtt_handle = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return NULL;
    }

    mqtt_handle = sysdep->core_sysdep_malloc(sizeof(core_mqtt_handle_t), CORE_MQTT_MODULE_NAME);
    if (mqtt_handle == NULL) {
        return NULL;
    }
    memset(mqtt_handle, 0, sizeof(core_mqtt_handle_t));

    mqtt_handle->sysdep = sysdep;
    mqtt_handle->keep_alive_s = CORE_MQTT_DEFAULT_KEEPALIVE_S;
    mqtt_handle->clean_session = CORE_MQTT_DEFAULT_CLEAN_SESSION;
    mqtt_handle->connect_timeout_ms = CORE_MQTT_DEFAULT_CONNECT_TIMEOUT_MS;
    mqtt_handle->heartbeat_params.interval_ms = CORE_MQTT_DEFAULT_HEARTBEAT_INTERVAL_MS;
    mqtt_handle->heartbeat_params.max_lost_times = CORE_MQTT_DEFAULT_HEARTBEAT_MAX_LOST_TIMES;
    mqtt_handle->reconnect_params.enabled = CORE_MQTT_DEFAULT_RECONN_ENABLED;
    mqtt_handle->reconnect_params.interval_ms = CORE_MQTT_DEFAULT_RECONN_INTERVAL_MS;
    mqtt_handle->send_timeout_ms = CORE_MQTT_DEFAULT_SEND_TIMEOUT_MS;
    mqtt_handle->recv_timeout_ms = CORE_MQTT_DEFAULT_RECV_TIMEOUT_MS;
    mqtt_handle->repub_timeout_ms = CORE_MQTT_DEFAULT_REPUB_TIMEOUT_MS;
    mqtt_handle->deinit_timeout_ms = CORE_MQTT_DEFAULT_DEINIT_TIMEOUT_MS;

    mqtt_handle->data_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->send_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->recv_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->sub_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->pub_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->process_handler_mutex = sysdep->core_sysdep_mutex_init();

    CORE_INIT_LIST_HEAD(&mqtt_handle->sub_list);
    CORE_INIT_LIST_HEAD(&mqtt_handle->pub_list);
    CORE_INIT_LIST_HEAD(&mqtt_handle->process_handler_list);

    mqtt_handle->exec_enabled = 1;

    return mqtt_handle;
}

int32_t aiot_mqtt_setopt(void *handle, aiot_mqtt_option_t option, void *data)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL || data == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (option >= AIOT_MQTTOPT_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    switch (option) {
    case AIOT_MQTTOPT_HOST: {
        res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->host, (char *)data, CORE_MQTT_MODULE_NAME);
    }
    break;
    case AIOT_MQTTOPT_PORT: {
        mqtt_handle->port = *(uint16_t *)data;
    }
    break;
    case AIOT_MQTTOPT_PRODUCT_KEY: {
        res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->product_key, (char *)data, CORE_MQTT_MODULE_NAME);
        _core_mqtt_sign_clean(mqtt_handle);
    }
    break;
    case AIOT_MQTTOPT_DEVICE_NAME: {
        res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->device_name, (char *)data, CORE_MQTT_MODULE_NAME);
        _core_mqtt_sign_clean(mqtt_handle);
    }
    break;
    case AIOT_MQTTOPT_DEVICE_SECRET: {
        res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->device_secret, (char *)data, CORE_MQTT_MODULE_NAME);
        _core_mqtt_sign_clean(mqtt_handle);
    }
    break;
    case AIOT_MQTTOPT_EXTEND_CLIENTID: {
        res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->extend_clientid, (char *)data, CORE_MQTT_MODULE_NAME);
        _core_mqtt_sign_clean(mqtt_handle);
    }
    break;
    case AIOT_MQTTOPT_SECURITY_MODE: {
        res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->security_mode, (char *)data, CORE_MQTT_MODULE_NAME);
        _core_mqtt_sign_clean(mqtt_handle);
    }
    break;
    case AIOT_MQTTOPT_USERNAME: {
        res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->username, (char *)data, CORE_MQTT_MODULE_NAME);
    }
    break;
    case AIOT_MQTTOPT_PASSWORD: {
        res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->password, (char *)data, CORE_MQTT_MODULE_NAME);
    }
    break;
    case AIOT_MQTTOPT_CLIENTID: {
        res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->clientid, (char *)data, CORE_MQTT_MODULE_NAME);
    }
    break;
    case AIOT_MQTTOPT_KEEPALIVE_SEC: {
        mqtt_handle->keep_alive_s = *(uint16_t *)data;
    }
    break;
    case AIOT_MQTTOPT_CLEAN_SESSION: {
        if (*(uint8_t *)data != 0 && *(uint8_t *)data != 1) {
            res = STATE_USER_INPUT_OUT_RANGE;
        }
        mqtt_handle->clean_session = *(uint8_t *)data;
    }
    break;
    case AIOT_MQTTOPT_NETWORK_CRED: {
        if (mqtt_handle->cred != NULL) {
            mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->cred);
            mqtt_handle->cred = NULL;
        }
        mqtt_handle->cred = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(aiot_sysdep_network_cred_t), CORE_MQTT_MODULE_NAME);
        if (mqtt_handle->cred == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(mqtt_handle->cred, 0, sizeof(aiot_sysdep_network_cred_t));
        memcpy(mqtt_handle->cred, data, sizeof(aiot_sysdep_network_cred_t));
    }
    break;
    case AIOT_MQTTOPT_CONNECT_TIMEOUT_MS: {
        mqtt_handle->connect_timeout_ms = *(uint32_t *)data;
    }
    break;
    case AIOT_MQTTOPT_HEARTBEAT_INTERVAL_MS: {
        mqtt_handle->heartbeat_params.interval_ms = *(uint32_t *)data;
    }
    break;
    case AIOT_MQTTOPT_HEARTBEAT_MAX_LOST: {
        mqtt_handle->heartbeat_params.max_lost_times = *(uint8_t *)data;
    }
    break;
    case AIOT_MQTTOPT_RECONN_ENABLED: {
        if (*(uint8_t *)data != 0 && *(uint8_t *)data != 1) {
            res = STATE_USER_INPUT_OUT_RANGE;
        }
        mqtt_handle->reconnect_params.enabled = *(uint8_t *)data;
    }
    break;
    case AIOT_MQTTOPT_RECONN_INTERVAL_MS: {
        mqtt_handle->reconnect_params.interval_ms = *(uint32_t *)data;
    }
    break;
    case AIOT_MQTTOPT_SEND_TIMEOUT_MS: {
        mqtt_handle->send_timeout_ms = *(uint32_t *)data;
    }
    break;
    case AIOT_MQTTOPT_RECV_TIMEOUT_MS: {
        mqtt_handle->recv_timeout_ms = *(uint32_t *)data;
    }
    break;
    case AIOT_MQTTOPT_REPUB_TIMEOUT_MS: {
        mqtt_handle->repub_timeout_ms = *(uint32_t *)data;
    }
    break;
    case AIOT_MQTTOPT_DEINIT_TIMEOUT_MS: {
        mqtt_handle->deinit_timeout_ms = *(uint32_t *)data;
    }
    break;
    case AIOT_MQTTOPT_RECV_HANDLER: {
        mqtt_handle->recv_handler = (aiot_mqtt_recv_handler_t)data;
    }
    break;
    case AIOT_MQTTOPT_EVENT_HANDLER: {
        mqtt_handle->event_handler = (aiot_mqtt_event_handler_t)data;
    }
    break;
    case AIOT_MQTTOPT_APPEND_TOPIC_MAP: {
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
        res = _core_mqtt_append_topic_map(mqtt_handle, (aiot_mqtt_topic_map_t *)data);
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    }
    break;
    case AIOT_MQTTOPT_USERDATA: {
        mqtt_handle->userdata = data;
    }
    break;
    default: {
        res = STATE_USER_INPUT_UNKNOWN_OPTION;
    }
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

int32_t aiot_mqtt_deinit(void **handle)
{
    uint32_t deinit_timeout_ms = 0;
    core_mqtt_handle_t *mqtt_handle = NULL;

    if (handle == NULL || *handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    mqtt_handle = *(core_mqtt_handle_t **)handle;

    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    mqtt_handle->exec_enabled = 0;
    deinit_timeout_ms = mqtt_handle->deinit_timeout_ms;
    do {
        if (mqtt_handle->exec_count == 0) {
            break;
        }
        mqtt_handle->sysdep->core_sysdep_sleep(CORE_MQTT_DEINIT_INTERVAL_MS);
    } while ((deinit_timeout_ms > CORE_MQTT_DEINIT_INTERVAL_MS) && (deinit_timeout_ms - CORE_MQTT_DEINIT_INTERVAL_MS > 0));

    if (mqtt_handle->exec_count != 0) {
        return STATE_MQTT_DEINIT_TIMEOUT;
    }

    *handle = NULL;

    if (mqtt_handle->network_handle != NULL) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
    }
    if (mqtt_handle->host != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->host);
    }
    if (mqtt_handle->product_key != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->product_key);
    }
    if (mqtt_handle->device_name != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->device_name);
    }
    if (mqtt_handle->device_secret != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->device_secret);
    }
    if (mqtt_handle->username != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->username);
    }
    if (mqtt_handle->password != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->password);
    }
    if (mqtt_handle->clientid != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->clientid);
    }
    if (mqtt_handle->extend_clientid != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->extend_clientid);
    }
    if (mqtt_handle->security_mode != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->security_mode);
    }
    if (mqtt_handle->cred != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->cred);
    }

    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->data_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->recv_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->sub_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->pub_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->process_handler_mutex);

    _core_mqtt_sublist_destroy(mqtt_handle);
    _core_mqtt_publist_destroy(mqtt_handle);
    _core_mqtt_process_handlerlist_destroy(mqtt_handle);

    mqtt_handle->sysdep->core_sysdep_free(mqtt_handle);

    return STATE_SUCCESS;
}

int32_t aiot_mqtt_connect(void *handle)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;
    uint64_t time_ent_ms = 0;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    time_ent_ms = mqtt_handle->sysdep->core_sysdep_time();

    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    mqtt_handle->disconnect_api_called = 0;

    /* if network connection exist, close first */
    //core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT, "MQTT user calls aiot_mqtt_connect api, connect\r\n");
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
    res = _core_mqtt_connect(mqtt_handle);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);

    if (res == STATE_MQTT_CONNECT_SUCCESS) {
        uint64_t time_ms = mqtt_handle->sysdep->core_sysdep_time();
        uint64_t time_delta = (time_ms - time_ent_ms);
        core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT, "MQTT connect success in %d ms\r\n", (void *)&time_delta);
        _core_mqtt_connect_event_notify(mqtt_handle);
        res = STATE_SUCCESS;
    } else {
        _core_mqtt_disconnect_event_notify(mqtt_handle, AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT);
    }

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

int32_t aiot_mqtt_disconnect(void *handle)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    mqtt_handle->disconnect_api_called = 1;

    /* send mqtt disconnect packet to mqtt broker first */
    _core_mqtt_disconnect(handle);

    /* close socket connect with mqtt broker */
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
    if (mqtt_handle->network_handle != NULL) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);

    _core_mqtt_disconnect_event_notify(mqtt_handle, AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT);
    core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT user calls aiot_mqtt_disconnect api, disconnect\r\n");

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

int32_t aiot_mqtt_heartbeat(void *handle)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    return _core_mqtt_heartbeat(mqtt_handle);
}

extern int n720_check_mqttonline();
extern int do_sub(void *handle);
extern int do_unsub(void *mqtt_handle);

int32_t aiot_mqtt_process(void *handle)
{
    int32_t res = STATE_SUCCESS;
    uint64_t time_now = 0;
    static int iter = 0;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    /* mqtt PINREQ packet */
    time_now = mqtt_handle->sysdep->core_sysdep_time();
    if (time_now < mqtt_handle->heartbeat_params.last_send_time) {
        mqtt_handle->heartbeat_params.last_send_time = time_now;
    }

    iter++;
    if(iter >= 50) {
        iter=0;
    }

    while(iter ==2 && n720_check_mqttonline() != 0) {
        int rett=0;
        aiot_mqtt_disconnect(mqtt_handle);
        aiot_mqtt_connect(mqtt_handle);
        res = do_unsub(mqtt_handle);
        printf("mqtt disconnected during loop, unsub res is %d\n", res);
        res = do_sub(mqtt_handle);
        printf("mqtt disconnected during loop, sub res is %d\n", res);
    }

    /* mqtt process handler process */
    _core_mqtt_process_handler_process(mqtt_handle);

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}
extern void user_send_data_with_no_delay(char *buffer);
#define SEND_LEN 256
int32_t aiot_mqtt_pub(void *handle, aiot_mqtt_buff_t *topic, aiot_mqtt_buff_t *payload, uint8_t qos)
{
    int ret  =0;
    {
        core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *) handle;
        int iter = 0;
        char buffer[SEND_LEN] = {0};

        char *at_mqttpub = NULL;
        char *src[] = {"at+mqttpub=0,0,\"", (char *)topic->buffer, "\",\"", (char *)payload->buffer, "\"\r\n"};
        uint8_t topic_len = sizeof(src) / sizeof(char *);
        int res = core_sprintf(mqtt_handle->sysdep, &at_mqttpub, "%s%s%s%s%s", src, topic_len, CORE_MQTT_MODULE_NAME);
        if (res < 0) {
            return -1;
        }
        core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_TOPIC, at_mqttpub);

        while (iter++ < 10) {
            user_send_data_with_delay(at_mqttpub);
            ret = user_get_data_with_delay(buffer, SEND_LEN, 100);
            {
                char *ref = "+MQTTSUB:";
                char *pub_data = strstr(buffer, ref);
                if(NULL != pub_data) {
                    char *start_data = pub_data + 11;  /* 报文格式是 +MQTTSUB:0,"$TOPIC 这样的形式,因此要跳过逗号前面(保留逗号)的11个字符 */
                    if(pub_data) {
                        _core_mqtt_pub_handler(handle, start_data, strlen(start_data), 0);
                    }
                }
            }

            if (ret == 0) {
                break;
            }
        }

        if (NULL != at_mqttpub) {
            mqtt_handle->sysdep->core_sysdep_free(at_mqttpub);
        }
    }
    return ret;
}

int32_t aiot_mqtt_sub(void *handle, aiot_mqtt_buff_t *topic, aiot_mqtt_recv_handler_t handler, uint8_t qos,
                      void *userdata)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL || topic == NULL || topic->buffer == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (topic->len == 0 || qos > CORE_MQTT_QOS_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if ((res = _core_mqtt_topic_is_valid((char *)topic->buffer, topic->len)) < STATE_SUCCESS) {
        return STATE_MQTT_TOPIC_INVALID;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    core_log2(mqtt_handle->sysdep, STATE_MQTT_LOG_TOPIC, "sub: %.*s\r\n", &topic->len, topic->buffer);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->sub_mutex);
    res = _core_mqtt_sublist_insert(mqtt_handle, topic, handler, userdata);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->sub_mutex);

    if (res < STATE_SUCCESS) {
        return res;
    }

    /* send subscribe packet */
    res = _core_mqtt_subunsub(mqtt_handle, (char *)topic->buffer, topic->len, qos, CORE_MQTT_SUB_PKT_TYPE);

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

int32_t aiot_mqtt_unsub(void *handle, aiot_mqtt_buff_t *topic)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL || topic == NULL || topic->buffer == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (topic->len == 0) {
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if ((res = _core_mqtt_topic_is_valid((char *)topic->buffer, topic->len)) < STATE_SUCCESS) {
        return STATE_MQTT_TOPIC_INVALID;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    core_log2(mqtt_handle->sysdep, STATE_MQTT_LOG_TOPIC, "unsub: %.*s\r\n", &topic->len, topic->buffer);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->sub_mutex);
    _core_mqtt_sublist_remove(mqtt_handle, topic);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->sub_mutex);

    res = _core_mqtt_subunsub(mqtt_handle, (char *)topic->buffer, topic->len, 0, CORE_MQTT_UNSUB_PKT_TYPE);

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

int32_t aiot_mqtt_recv(void *handle)
{
    int32_t res = STATE_SUCCESS;
    uint32_t mqtt_remainlen = 0;
    uint8_t mqtt_fixed_header[CORE_MQTT_FIXED_HEADER_LEN] = {0};
    uint8_t mqtt_pkt_type = 0, mqtt_pkt_reserved = 0;
    uint8_t *remain = NULL;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    /* network error reconnect */
    if (mqtt_handle->network_handle == NULL) {
        _core_mqtt_disconnect_event_notify(mqtt_handle, AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT);
    }
    if (mqtt_handle->reconnect_params.enabled == 1 && mqtt_handle->disconnect_api_called == 0) {
        res = _core_mqtt_reconnect(mqtt_handle);
        if (res < STATE_SUCCESS) {
            if (res == STATE_MQTT_CONNECT_SUCCESS) {
                mqtt_handle->heartbeat_params.lost_times = 0;
                core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT, "MQTT network reconnect success\r\n");
                _core_mqtt_connect_event_notify(mqtt_handle);
                res = STATE_SUCCESS;
            } else {
                _core_mqtt_exec_dec(mqtt_handle);
                return res;
            }
        }
    }

    /* heartbeat missing reconnect */
    if (mqtt_handle->heartbeat_params.lost_times > mqtt_handle->heartbeat_params.max_lost_times) {
        _core_mqtt_disconnect_event_notify(mqtt_handle, AIOT_MQTTDISCONNEVT_HEARTBEAT_DISCONNECT);
        if (mqtt_handle->reconnect_params.enabled == 1 && mqtt_handle->disconnect_api_called == 0) {
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_RECONNECTING, "MQTT heartbeat lost %d times, try to reconnecting...\r\n",
                      &mqtt_handle->heartbeat_params.lost_times);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
            res = _core_mqtt_connect(mqtt_handle);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
            if (res < STATE_SUCCESS) {
                if (res == STATE_MQTT_CONNECT_SUCCESS) {
                    mqtt_handle->heartbeat_params.lost_times = 0;
                    core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT, "MQTT network reconnect success\r\n");
                    _core_mqtt_connect_event_notify(mqtt_handle);
                    res = STATE_SUCCESS;
                } else {
                    _core_mqtt_exec_dec(mqtt_handle);
                    return res;
                }
            }
        }
    }

    char buffer[AT_RECV_DEFAULT_BUFFER_SIZE] = {0};
    user_get_data_with_delay(buffer, AT_RECV_DEFAULT_BUFFER_SIZE, 200);
    char *ref = "+MQTTSUB:";
    char *pub_data = strstr(buffer, ref);
    if(NULL == pub_data) {
        return -1;
    }
    char *start_data = pub_data + 11;  /* 报文格式是 +MQTTSUB:0,"$TOPIC 这样的形式,因此要跳过逗号前面(保留逗号)的11个字符 */

    if(pub_data) {
        _core_mqtt_pub_handler(handle, start_data, strlen(start_data), 0);
    }

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

