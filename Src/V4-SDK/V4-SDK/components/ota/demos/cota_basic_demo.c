#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_ota_api.h"

char *product_key       = "a1dhWKuKqX5";
char *device_name       = "tmmv2990";
char *device_secret     = "EmEcYq0GXTDRgGWPN3hz8gyTWtfh7wq2";

extern const char *ali_ca_crt;
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

int32_t demo_state_logcb(int32_t code, char *message)
{
    if (STATE_HTTP_LOG_RECV_CONTENT != code) {
        printf("%s", message);
    }
    return 0;
}

void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *const event, void *userdata)
{
    switch (event->type) {
        case AIOT_MQTTEVT_CONNECT: {
            printf("AIOT_MQTTEVT_CONNECT\r\n");
        }
        break;
        case AIOT_MQTTEVT_RECONNECT: {
            printf("AIOT_MQTTEVT_RECONNECT\r\n");
        }
        break;
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            printf("AIOT_MQTTEVT_DISCONNECT: %s\r\n", cause);
        }
        break;
        default: {

        }
    }
}

void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *const packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            printf("heartbeat response\r\n");
        }
        break;
        case AIOT_MQTTRECV_SUB_ACK: {
            printf("suback, res: -0x%04X, packet id: %d, max qos: %d\r\n",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
        }
        break;
        case AIOT_MQTTRECV_PUB: {
            printf("pub, qos: %d, topic: %.*s\r\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            printf("pub, payload: %.*s\r\n", packet->data.pub.payload_len, packet->data.pub.payload);
        }
        break;
        case AIOT_MQTTRECV_PUB_ACK: {
            printf("puback, packet id: %d\r\n", packet->data.pub_ack.packet_id);
        }
        break;
        default: {

        }
    }
}

void user_download_recv_handler(void *handle, int32_t percent, const aiot_download_recv_t *packet, void *userdata)
{
    if (AIOT_DLRECV_HTTPBODY != packet->type) {
        return;
    }
    uint8_t *src_buffer = packet->data.buffer;
    uint32_t src_buffer_len = packet->data.len;

    if (percent < 0) {
        /* 收包异常或者digest校验错误 */
        printf("exception happend, percent is %d\r\n", percent);
        return;
    }

    printf("config len is %d, config content is %.*s\r\n", src_buffer_len, src_buffer_len, (char *)src_buffer);
}

void *demo_ota_download_loop(void *dl_handle)
{
    aiot_download_send_request(dl_handle);
    while (1) {
        int32_t ret = aiot_download_recv(dl_handle);

        if (STATE_DOWNLOAD_FINISHED == ret) {
            printf("finished download\r\n");
            goto exit;
        }
    }
exit:
    aiot_download_deinit(&dl_handle);
    return NULL;
}

void user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_COTA: {
            uint32_t res = 0;
            uint16_t port = 443;
            uint32_t max_buffer_len = 2048;
            aiot_sysdep_network_cred_t cred;
            void *dl_handle = aiot_download_init();

            if (NULL == dl_handle) {
                return;
            }
            memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
            cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_RSA;
            cred.max_tls_fragment = 16384;
            cred.x509_server_cert = ali_ca_crt;
            cred.x509_server_cert_len = strlen(ali_ca_crt);
            res = aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_CRED, (void *)(&cred));
            if (res != STATE_SUCCESS) {
                goto exit;
            }
            res = aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            if (res != STATE_SUCCESS) {
                goto exit;
            }
            res = aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            if (res != STATE_SUCCESS) {
                goto exit;
            }
            res = aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(user_download_recv_handler));
            if (res != STATE_SUCCESS) {
                goto exit;
            }
            res = aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len));
            if (res != STATE_SUCCESS) {
                goto exit;
            }
            demo_ota_download_loop(dl_handle);
            break;
exit:
            aiot_download_deinit(&dl_handle);
            break;
        }
        default:
            break;
    }
}

int main(int argc, char *argv[])
{
    int32_t res = STATE_SUCCESS;
    void *mqtt_handle = NULL;
    char *url = "iot-as-mqtt.cn-shanghai.aliyuncs.com";
    char host[100] = {0};
    uint16_t port = 1883;
    void *ota_handle = NULL;

    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    aiot_state_set_logcb(demo_state_logcb);

    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        return -1;
    }

    snprintf(host, 100, "%s.%s", product_key, url);
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)host);
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    ota_handle = aiot_ota_init();
    if (NULL == ota_handle) {
        goto exit;
    }
    /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(ota_handle, AIOT_OTAOPT_MQTT_HANDLE, mqtt_handle);
    /* 设置ota_handle的recv_handler */
    aiot_ota_setopt(ota_handle, AIOT_OTAOPT_RECV_HANDLER, user_ota_recv_handler);

    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_connect failed: -0x%04X\r\n", -res);
        goto exit;
    }

    {
        char *topic_string = "/sys/a1dhWKuKqX5/tmmv2990/thing/config/get";
        char *payload_string = "{\"id\":\"123\",\"params\":{\"configScope\":\"product\", \"getType\":\"file\"}}";

        aiot_mqtt_buff_t topic_buff = {
            .buffer = (uint8_t *)topic_string,
            .len = (uint32_t)strlen(topic_string)
        };

        aiot_mqtt_buff_t payload_buff = {
            .buffer = (uint8_t *)payload_string,
            .len = (uint32_t)strlen(payload_string)
        };

        aiot_mqtt_pub(mqtt_handle, &topic_buff, &payload_buff, 0);
    }

    while (1) {
        aiot_mqtt_process(mqtt_handle);
        res = aiot_mqtt_recv(mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
    }

    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_disconnect failed: -0x%04X\r\n", -res);
        goto exit;
    }

exit:
    while (1) {
        res = aiot_mqtt_deinit(&mqtt_handle);

        if (res < STATE_SUCCESS) {
            printf("aiot_mqtt_deinit failed: -0x%04X\r\n", -res);
            return -1;
        } else {
            break;
        }
    }

    aiot_ota_deinit(&ota_handle);

    return 0;
}


