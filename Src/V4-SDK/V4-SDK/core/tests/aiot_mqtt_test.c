#include <unistd.h>
#include <pthread.h>
#include "cu_test.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "core_list.h"
#include "core_string.h"
#include "core_mqtt.h"

extern const char *ali_ca_crt;
int32_t core_sysdep_network_establish(void *handle);
int32_t core_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data);
int32_t core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                 core_sysdep_addr_t *addr);
int32_t core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                  core_sysdep_addr_t *addr);

static int32_t aiot_mqtt_test_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

CASE(AIOT_MQTT, case_01_aiot_mqtt_init_without_portfile)
{
    void *mqtt_handle = NULL;

    aiot_sysdep_set_portfile(NULL);
    mqtt_handle = aiot_mqtt_init();
    ASSERT_NULL(mqtt_handle);
}

CASE(AIOT_MQTT, case_02_aiot_mqtt_init_with_portfile)
{
    void *mqtt_handle = NULL;

    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    mqtt_handle = aiot_mqtt_init();
    ASSERT_NOT_NULL(mqtt_handle);

    aiot_mqtt_deinit(&mqtt_handle);
}

DATA(AIOT_MQTT)
{
    void *mqtt_handle;
    char *host;
    uint16_t port;
    char *product_key;
    char *device_name;
    char *device_secret;
};

SETUP(AIOT_MQTT)
{
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    aiot_state_set_logcb(aiot_mqtt_test_logcb);

    data->mqtt_handle = aiot_mqtt_init();

    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_mqtt_test";
    data->device_secret = "ggYWoavsOokye6vNz8DgtF4XwzYjSGaF";
    data->host = "a18wPzZJzNG.iot-as-mqtt.cn-shanghai.aliyuncs.com";
    data->port = 443;
}

TEARDOWN(AIOT_MQTT)
{
    aiot_mqtt_deinit(&data->mqtt_handle);
}

CASEs(AIOT_MQTT, case_03_aiot_mqtt_setopt_AIOT_MQTTOPT_HOST)
{
    int32_t res = STATE_SUCCESS;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->host, data->host);
}

CASEs(AIOT_MQTT, case_04_aiot_mqtt_setopt_AIOT_MQTTOPT_PORT)
{
    int32_t res = STATE_SUCCESS;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->port, data->port);
}

static void username_password_client_set(void *mqtt_handle)
{
    int32_t res = STATE_SUCCESS;

    char *username = "username";
    char *password = "password";
    char *clientid = "clientid";

    res = aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_USERNAME, (void *)username);
    ASSERT_EQ(res, STATE_SUCCESS);
    ASSERT_STR_EQ(((core_mqtt_handle_t *)mqtt_handle)->username, username);

    res = aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PASSWORD, (void *)password);
    ASSERT_EQ(res, STATE_SUCCESS);
    ASSERT_STR_EQ(((core_mqtt_handle_t *)mqtt_handle)->password, password);

    res = aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_CLIENTID, (void *)clientid);
    ASSERT_EQ(res, STATE_SUCCESS);
    ASSERT_STR_EQ(((core_mqtt_handle_t *)mqtt_handle)->clientid, clientid);
}

static void username_password_client_check(void *mqtt_handle)
{
    ASSERT_EQ(((core_mqtt_handle_t *)mqtt_handle)->username, NULL);
    ASSERT_EQ(((core_mqtt_handle_t *)mqtt_handle)->password, NULL);
    ASSERT_EQ(((core_mqtt_handle_t *)mqtt_handle)->clientid, NULL);
}

CASEs(AIOT_MQTT, case_05_aiot_mqtt_setopt_AIOT_MQTTOPT_PRODUCT_KEY)
{
    int32_t res = STATE_SUCCESS;

    username_password_client_set(data->mqtt_handle);

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->product_key, data->product_key);

    username_password_client_check(data->mqtt_handle);
}

CASEs(AIOT_MQTT, case_06_aiot_mqtt_setopt_AIOT_MQTTOPT_DEVICE_NAME)
{
    int32_t res = STATE_SUCCESS;

    username_password_client_set(data->mqtt_handle);

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->device_name, data->device_name);

    username_password_client_check(data->mqtt_handle);
}

CASEs(AIOT_MQTT, case_07_aiot_mqtt_setopt_AIOT_MQTTOPT_DEVICE_SECRET)
{
    int32_t res = STATE_SUCCESS;

    username_password_client_set(data->mqtt_handle);

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->device_secret, data->device_secret);

    username_password_client_check(data->mqtt_handle);
}

CASEs(AIOT_MQTT, case_08_aiot_mqtt_setopt_AIOT_MQTTOPT_EXTEND_CLIENTID)
{
    int32_t res = STATE_SUCCESS;
    char *extend_clientid = "authtype=id2";

    username_password_client_set(data->mqtt_handle);

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_EXTEND_CLIENTID, (void *)extend_clientid);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->extend_clientid, extend_clientid);

    username_password_client_check(data->mqtt_handle);
}

CASEs(AIOT_MQTT, case_09_aiot_mqtt_setopt_AIOT_MQTTOPT_SECURITY_MODE)
{
    int32_t res = STATE_SUCCESS;
    char *security_mode = "8";

    username_password_client_set(data->mqtt_handle);

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_SECURITY_MODE, (void *)security_mode);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->security_mode, security_mode);

    username_password_client_check(data->mqtt_handle);
}

CASEs(AIOT_MQTT, case_10_aiot_mqtt_setopt_AIOT_MQTTOPT_USERNAME)
{
    int32_t res = STATE_SUCCESS;
    char *username = "username";

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_USERNAME, (void *)username);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->username, username);
}

CASEs(AIOT_MQTT, case_11_aiot_mqtt_setopt_AIOT_MQTTOPT_PASSWORD)
{
    int32_t res = STATE_SUCCESS;
    char *password = "password";

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PASSWORD, (void *)password);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->password, password);
}

CASEs(AIOT_MQTT, case_12_aiot_mqtt_setopt_AIOT_MQTTOPT_CLIENTID)
{
    int32_t res = STATE_SUCCESS;
    char *clientid = "clientid";

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_CLIENTID, (void *)clientid);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->clientid, clientid);
}

CASEs(AIOT_MQTT, case_13_aiot_mqtt_setopt_AIOT_MQTTOPT_KEEPALIVE_SEC)
{
    int32_t res = STATE_SUCCESS;
    uint16_t keep_alive_s = 60;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_KEEPALIVE_SEC, (void *)&keep_alive_s);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->keep_alive_s, keep_alive_s);
}

CASEs(AIOT_MQTT, case_14_aiot_mqtt_setopt_AIOT_MQTTOPT_NETWORK_CRED)
{
    int32_t res = STATE_SUCCESS;
    aiot_sysdep_network_cred_t cred;

    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.x509_server_cert = "";
    cred.x509_client_cert = "";
    cred.x509_client_privkey = "";

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->cred->option, cred.option);
    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->cred->sni_enabled, cred.sni_enabled);
    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->cred->max_tls_fragment, cred.max_tls_fragment);
    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->cred->x509_server_cert, cred.x509_server_cert);
    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->cred->x509_server_cert_len, cred.x509_server_cert_len);
    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->cred->x509_client_cert, cred.x509_client_cert);
    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->cred->x509_client_cert_len, cred.x509_client_cert_len);
    ASSERT_STR_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->cred->x509_client_privkey, cred.x509_client_privkey);
    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->cred->x509_client_privkey_len, cred.x509_client_privkey_len);
}

CASEs(AIOT_MQTT, case_15_aiot_mqtt_setopt_AIOT_MQTTOPT_HEARTBEAT_INTERVAL_MS)
{
    int32_t res = STATE_SUCCESS;
    uint32_t heartbeat_interval_ms = 5 * 1000;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HEARTBEAT_INTERVAL_MS, (void *)&heartbeat_interval_ms);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->heartbeat_params.interval_ms, heartbeat_interval_ms);
}

CASEs(AIOT_MQTT, case_16_aiot_mqtt_setopt_AIOT_MQTTOPT_HEARTBEAT_MAX_LOST)
{
    int32_t res = STATE_SUCCESS;
    uint8_t heartbeat_max_lost_times = 10;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HEARTBEAT_MAX_LOST, (void *)&heartbeat_max_lost_times);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->heartbeat_params.max_lost_times, heartbeat_max_lost_times);
}

CASEs(AIOT_MQTT, case_17_aiot_mqtt_setopt_AIOT_MQTTOPT_RECONN_ENABLED)
{
    int32_t res = STATE_SUCCESS;
    uint8_t reconn_enabled = 0;

    reconn_enabled = 2;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECONN_ENABLED, (void *)&reconn_enabled);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);

    reconn_enabled = 0;
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECONN_ENABLED, (void *)&reconn_enabled);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->reconnect_params.enabled, reconn_enabled);
}

CASEs(AIOT_MQTT, case_18_aiot_mqtt_setopt_AIOT_MQTTOPT_RECONN_INTERVAL_MS)
{
    int32_t res = STATE_SUCCESS;
    uint32_t reconn_interval_ms = 5000;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECONN_INTERVAL_MS, (void *)&reconn_interval_ms);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->reconnect_params.interval_ms, reconn_interval_ms);
}

CASEs(AIOT_MQTT, case_19_aiot_mqtt_setopt_AIOT_MQTTOPT_SEND_TIMEOUT_MS)
{
    int32_t res = STATE_SUCCESS;
    uint32_t send_timeout_ms = 10000;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_SEND_TIMEOUT_MS, (void *)&send_timeout_ms);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->send_timeout_ms, send_timeout_ms);
}

CASEs(AIOT_MQTT, case_20_aiot_mqtt_setopt_AIOT_MQTTOPT_RECV_TIMEOUT_MS)
{
    int32_t res = STATE_SUCCESS;
    uint32_t recv_timeout_ms = 10000;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&recv_timeout_ms);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->recv_timeout_ms, recv_timeout_ms);
}

CASEs(AIOT_MQTT, case_21_aiot_mqtt_setopt_AIOT_MQTTOPT_REPUB_TIMEOUT_MS)
{
    int32_t res = STATE_SUCCESS;
    uint32_t repub_timeout_ms = 10000;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_REPUB_TIMEOUT_MS, (void *)&repub_timeout_ms);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->repub_timeout_ms, repub_timeout_ms);
}

CASEs(AIOT_MQTT, case_22_aiot_mqtt_setopt_AIOT_MQTTOPT_DEINIT_TIMEOUT_MS)
{
    int32_t res = STATE_SUCCESS;
    uint32_t deinit_timeout_ms = 10000;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEINIT_TIMEOUT_MS, (void *)&deinit_timeout_ms);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->deinit_timeout_ms, deinit_timeout_ms);
}

static void case23_aiot_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{

}

CASEs(AIOT_MQTT, case_23_aiot_mqtt_setopt_AIOT_MQTTOPT_RECV_HANDLER)
{
    int32_t res = STATE_SUCCESS;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case23_aiot_mqtt_recv_handler);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->recv_handler, case23_aiot_mqtt_recv_handler);
}

static void case_24_aiot_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{

}

CASEs(AIOT_MQTT, case_24_aiot_mqtt_setopt_AIOT_MQTTOPT_EVENT_HANDLER)
{
    int32_t res = STATE_SUCCESS;

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)case_24_aiot_mqtt_event_handler);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->event_handler, case_24_aiot_mqtt_event_handler);
}

static void case25_aiot_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{

}

CASEs(AIOT_MQTT, case_25_aiot_mqtt_setopt_AIOT_MQTTOPT_APPEND_TOPIC_MAP)
{
    int32_t res = STATE_SUCCESS;
    char *topic = "/sys/a1X2bEnP82z/example_core/thing/event/property/post_reply";
    char *sub_topic_invalid1 = "abc";
    char *userdata = "123";

    aiot_mqtt_topic_map_t map = {
        .topic = {
            .buffer = NULL,
            .len = 0
        },
        .handler = case25_aiot_mqtt_recv_handler,
        .userdata = userdata
    };

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    map.topic.buffer = (uint8_t *)sub_topic_invalid1;
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);

    map.topic.len = (uint32_t)strlen(sub_topic_invalid1);
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    map.topic.buffer = (uint8_t *)topic;
    map.topic.len = (uint32_t)strlen(topic);
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map);
    ASSERT_EQ(res, STATE_SUCCESS);

    {
        uint8_t count = 0;
        core_mqtt_sub_node_t *sub_node = NULL;
        core_mqtt_sub_handler_node_t *handler_node = NULL;

        core_list_for_each_entry(sub_node, &((core_mqtt_handle_t *)data->mqtt_handle)->sub_list, linked_node) {
            count++;
            break;
        }

        ASSERT_EQ(count, 1);
        ASSERT_STR_EQ(sub_node->topic, topic);

        count = 0;
        core_list_for_each_entry(handler_node, &sub_node->handle_list, linked_node) {
            count++;
            break;
        }

        ASSERT_EQ(count, 1);
        ASSERT_EQ(handler_node->handler, case25_aiot_mqtt_recv_handler);
        ASSERT_EQ(handler_node->userdata, userdata);
    }
}

CASEs(AIOT_MQTT, case_26_aiot_mqtt_setopt_AIOT_MQTTOPT_USERDATA)
{
    int32_t res = STATE_SUCCESS;
    char *userdata = "123";

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_USERDATA, (void *)userdata);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->userdata, userdata);
}

CASEs(AIOT_MQTT, case_27_aiot_mqtt_setopt_invalid_user_input)
{
    int32_t res = STATE_SUCCESS;
    char *userdata = "123";

    res = aiot_mqtt_setopt(NULL, AIOT_MQTTOPT_HOST, (void *)data->host);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_MAX, (void *)userdata);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);
}

static void case_28_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{

}

CASEs(AIOT_MQTT, case_28_aiot_mqtt_connect)
{
    int32_t res = STATE_SUCCESS;
    aiot_sysdep_network_cred_t cred;

    data->device_name = "aiot_mqtt_test_case_28";
    data->device_secret = "pl8TNHBxXeuIit3nivDVTCKIR2eJfYsS";

    res = aiot_mqtt_connect(NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_MISSING_HOST);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_MISSING_PRODUCT_KEY);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_MISSING_DEVICE_NAME);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_MISSING_DEVICE_SECRET);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    res = aiot_mqtt_connect(data->mqtt_handle);
    printf("-0x%02X\n", -res);
    ASSERT_EQ(res, STATE_SUCCESS);

    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.x509_server_cert = "";
    cred.x509_client_cert = "";
    cred.x509_client_privkey = "";
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_SECURITY_MODE, (void *)"8");
    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_SECURITY_MODE, (void *)"3");
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_USERNAME, (void *)"username");
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PASSWORD, (void *)"password");
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_CLIENTID, (void *)"clientid");
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)case_28_event_handler);
    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED);
}

static void case_29_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{

}

CASEs(AIOT_MQTT, case_29_aiot_mqtt_disconnect)
{
    int32_t res = STATE_SUCCESS;

    data->device_name = "aiot_mqtt_test_case_29";
    data->device_secret = "sncmaL34jYJZgwiHuQYfrSHxRnc59BfZ";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)case_29_event_handler);

    aiot_mqtt_connect(data->mqtt_handle);

    res = aiot_mqtt_disconnect(NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_disconnect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(AIOT_MQTT, case_30_aiot_mqtt_process)
{
    int32_t res = STATE_SUCCESS;

    data->device_name = "aiot_mqtt_test_case_30";
    data->device_secret = "OaYRzIvH6y70d8o6fvAK4eA3BTsrMmBR";

    res = aiot_mqtt_process(NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_process(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_mqtt_connect(data->mqtt_handle);
    res = aiot_mqtt_process(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);
}

static void case_31_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_PUB: {

        }
        break;
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {

        }
        break;
        case AIOT_MQTTRECV_SUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_UNSUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_PUB_ACK: {
            *(uint8_t *)userdata = 1;
        }
        break;
        default: {

        }
    }
}

CASEs(AIOT_MQTT, case_31_aiot_mqtt_pub)
{
    int32_t res = STATE_SUCCESS;
    uint8_t puback_received = 0, recv_count = 3;
    char *topic = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_31/thing/event/property/post";
    char *payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";
    aiot_mqtt_buff_t topic_buff = {
        .buffer = NULL,
        .len = 0
    };
    aiot_mqtt_buff_t payload_buff = {
        .buffer = NULL,
        .len = 0
    };

    data->device_name = "aiot_mqtt_test_case_31";
    data->device_secret = "cK04fvgEX0PH3SLg1jyrKlQGfhnrcIZk";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case_31_mqtt_recv_handler);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_USERDATA, (void *)&puback_received);

    aiot_mqtt_connect(data->mqtt_handle);

    res = aiot_mqtt_pub(NULL, NULL, NULL, 0);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_pub(data->mqtt_handle, NULL, NULL, 0);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, NULL, 0);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, NULL, 0);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 0);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    topic_buff.buffer = (uint8_t *)topic;
    payload_buff.buffer = NULL;
    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 0);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    topic_buff.buffer = NULL;
    payload_buff.buffer = (uint8_t *)payload;
    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 0);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    topic_buff.buffer = (uint8_t *)topic;
    payload_buff.buffer = (uint8_t *)payload;
    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 0);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);

    topic_buff.len = (uint32_t)strlen(topic);
    payload_buff.len = (uint32_t)strlen(payload);

    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 3);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);
    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 0);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 1);
    ASSERT_EQ(res, 1);

    while ((puback_received == 0) && (recv_count-- > 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }
    ASSERT_EQ(puback_received, 1);

    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 1);
    ASSERT_EQ(res, 2);

    ((core_mqtt_handle_t *)data->mqtt_handle)->packet_id = 1;
    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 1);
    ASSERT_EQ(res, STATE_MQTT_PUBLIST_PACKET_ID_ROLL);

    ((core_mqtt_handle_t *)data->mqtt_handle)->packet_id = 0xFFFF;
    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 1);
    ASSERT_EQ(res, 1);

    aiot_mqtt_disconnect(data->mqtt_handle);
    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 0);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);
}

typedef struct {
    uint8_t is_default_handler_received;
    uint8_t is_sub_handler_received;
    uint16_t packet_id;
} case_32_userdata;

static void case_32_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_PUB: {
            ((case_32_userdata *)userdata)->is_default_handler_received = 1;
        }
        break;
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {

        }
        break;
        case AIOT_MQTTRECV_SUB_ACK: {
            ((case_32_userdata *)userdata)->packet_id = packet->data.sub_ack.packet_id;
        }
        break;
        case AIOT_MQTTRECV_UNSUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_PUB_ACK: {

        }
        break;
        default: {

        }
    }
}

static void case_32_mqtt_sub_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_PUB: {
            ((case_32_userdata *)userdata)->is_sub_handler_received = 1;
        }
        break;
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {

        }
        break;
        case AIOT_MQTTRECV_SUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_UNSUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_PUB_ACK: {

        }
        break;
        default: {

        }
    }
}

CASEs(AIOT_MQTT, case_32_aiot_mqtt_sub)
{
    int32_t res = STATE_SUCCESS;
    uint16_t recv_count = 3;
    char *sub_topic = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_32/thing/event/+/post_reply";
    char *pub_topic = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_32/thing/event/property/post";
    char *pub_payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";
    char *sub_topic_invalid1 = "abc";
    char *sub_topic_invalid2 = "/abc+/def";
    char *sub_topic_invalid3 = "/abc/+def";
    char *sub_topic_invalid4 = "/abc/d+ef";
    char *sub_topic_invalid5 = "/abc/#def";
    char *sub_topic_invalid6 = "/abc/d#ef";
    char *sub_topic_invalid7 = "/abc#/def";
    aiot_mqtt_buff_t sub_topic_buff = {
        .buffer = NULL,
        .len = 0
    };
    aiot_mqtt_buff_t pub_topic_buff = {
        .buffer = (uint8_t *)pub_topic,
        .len = (uint32_t)strlen(pub_topic)
    };
    aiot_mqtt_buff_t pub_payload_buff = {
        .buffer = (uint8_t *)pub_payload,
        .len = (uint32_t)strlen(pub_payload)
    };
    case_32_userdata userdata;

    memset(&userdata, 0, sizeof(case_32_userdata));

    data->device_name = "aiot_mqtt_test_case_32";
    data->device_secret = "GnuVXBRv1Tc1irE0Eejj5d3i8YBCHhJl";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case_32_mqtt_recv_handler);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_USERDATA, (void *)&userdata);

    aiot_mqtt_connect(data->mqtt_handle);

    res = aiot_mqtt_sub(NULL, NULL, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_sub(data->mqtt_handle, NULL, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    sub_topic_buff.buffer = (uint8_t *)sub_topic;
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);

    sub_topic_buff.len = (uint32_t)strlen(sub_topic);
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 3, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);

    sub_topic_buff.buffer = (uint8_t *)sub_topic_invalid1;
    sub_topic_buff.len = (uint32_t)strlen(sub_topic_invalid1);
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    sub_topic_buff.buffer = (uint8_t *)sub_topic_invalid2;
    sub_topic_buff.len = (uint32_t)strlen(sub_topic_invalid2);
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    sub_topic_buff.buffer = (uint8_t *)sub_topic_invalid3;
    sub_topic_buff.len = (uint32_t)strlen(sub_topic_invalid3);
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    sub_topic_buff.buffer = (uint8_t *)sub_topic_invalid4;
    sub_topic_buff.len = (uint32_t)strlen(sub_topic_invalid4);
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    sub_topic_buff.buffer = (uint8_t *)sub_topic_invalid5;
    sub_topic_buff.len = (uint32_t)strlen(sub_topic_invalid5);
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    sub_topic_buff.buffer = (uint8_t *)sub_topic_invalid6;
    sub_topic_buff.len = (uint32_t)strlen(sub_topic_invalid6);
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    sub_topic_buff.buffer = (uint8_t *)sub_topic_invalid7;
    sub_topic_buff.len = (uint32_t)strlen(sub_topic_invalid7);
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    sub_topic_buff.buffer = (uint8_t *)sub_topic;
    sub_topic_buff.len = (uint32_t)strlen(sub_topic);
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, 1);

    while ((userdata.packet_id != 1) && (recv_count-- > 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }
    ASSERT_EQ(userdata.packet_id, 1);
    recv_count = 3;

    res = aiot_mqtt_pub(data->mqtt_handle, &pub_topic_buff, &pub_payload_buff, 0);
    ASSERT_EQ(res, STATE_SUCCESS);

    while ((userdata.is_default_handler_received != 1) && (recv_count-- > 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }
    ASSERT_EQ(userdata.is_default_handler_received, 1);
    recv_count = 3;

    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, case_32_mqtt_sub_recv_handler, 0, (void *)&userdata);
    ASSERT_EQ(res, 2);

    while ((userdata.packet_id != 2) && (recv_count-- > 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }
    ASSERT_EQ(userdata.packet_id, 2);
    recv_count = 3;

    res = aiot_mqtt_pub(data->mqtt_handle, &pub_topic_buff, &pub_payload_buff, 0);
    ASSERT_EQ(res, STATE_SUCCESS);

    while ((userdata.is_sub_handler_received != 1) && (recv_count-- > 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }
    ASSERT_EQ(userdata.is_sub_handler_received, 1);
    recv_count = 3;

    aiot_mqtt_disconnect(data->mqtt_handle);
    res = aiot_mqtt_sub(data->mqtt_handle, &sub_topic_buff, case_32_mqtt_sub_recv_handler, 0, (void *)&userdata);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);
}

typedef struct {
    uint8_t is_default_handler_received;
    uint8_t is_sub_handler_received;
    uint16_t packet_id;
} case_33_userdata;

static void case_33_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_PUB: {
            ((case_32_userdata *)userdata)->is_default_handler_received = 1;
        }
        break;
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {

        }
        break;
        case AIOT_MQTTRECV_SUB_ACK: {
            ((case_32_userdata *)userdata)->packet_id = packet->data.sub_ack.packet_id;
        }
        break;
        case AIOT_MQTTRECV_UNSUB_ACK: {
            ((case_32_userdata *)userdata)->packet_id = packet->data.unsub_ack.packet_id;
        }
        break;
        case AIOT_MQTTRECV_PUB_ACK: {

        }
        break;
        default: {

        }
    }
}

CASEs(AIOT_MQTT, case_33_aiot_mqtt_unsub)
{
    int32_t res = STATE_SUCCESS;
    uint16_t recv_count = 1, test_retry = 3;
    char *unsub_topic = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_33/thing/event/+/post_reply";
    char *pub_topic = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_33/thing/event/property/post";
    char *pub_payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";
    char *unsub_topic_invalid1 = "abc";
    char *unsub_topic_invalid2 = "/abc+/def";
    char *unsub_topic_invalid3 = "/abc/+def";
    char *unsub_topic_invalid4 = "/abc/d+ef";
    char *unsub_topic_invalid5 = "/abc/#def";
    char *unsub_topic_invalid6 = "/abc/d#ef";
    char *unsub_topic_invalid7 = "/abc#/def";
    aiot_mqtt_buff_t unsub_topic_buff = {
        .buffer = NULL,
        .len = 0
    };
    aiot_mqtt_buff_t pub_topic_buff = {
        .buffer = (uint8_t *)pub_topic,
        .len = (uint32_t)strlen(pub_topic)
    };
    aiot_mqtt_buff_t pub_payload_buff = {
        .buffer = (uint8_t *)pub_payload,
        .len = (uint32_t)strlen(pub_payload)
    };
    case_33_userdata userdata;

    memset(&userdata, 0, sizeof(case_33_userdata));

    data->device_name = "aiot_mqtt_test_case_33";
    data->device_secret = "m3GACFnYU2boZa8Q0TZeDMP2AZdCG6Gf";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case_33_mqtt_recv_handler);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_USERDATA, (void *)&userdata);

    aiot_mqtt_connect(data->mqtt_handle);

    res = aiot_mqtt_unsub(NULL, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_unsub(data->mqtt_handle, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    unsub_topic_buff.buffer = (uint8_t *)unsub_topic;
    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);

    unsub_topic_buff.buffer = (uint8_t *)unsub_topic_invalid1;
    unsub_topic_buff.len = (uint32_t)strlen(unsub_topic_invalid1);
    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    unsub_topic_buff.buffer = (uint8_t *)unsub_topic_invalid2;
    unsub_topic_buff.len = (uint32_t)strlen(unsub_topic_invalid2);
    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    unsub_topic_buff.buffer = (uint8_t *)unsub_topic_invalid3;
    unsub_topic_buff.len = (uint32_t)strlen(unsub_topic_invalid3);
    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    unsub_topic_buff.buffer = (uint8_t *)unsub_topic_invalid4;
    unsub_topic_buff.len = (uint32_t)strlen(unsub_topic_invalid4);
    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    unsub_topic_buff.buffer = (uint8_t *)unsub_topic_invalid5;
    unsub_topic_buff.len = (uint32_t)strlen(unsub_topic_invalid5);
    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    unsub_topic_buff.buffer = (uint8_t *)unsub_topic_invalid6;
    unsub_topic_buff.len = (uint32_t)strlen(unsub_topic_invalid6);
    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    unsub_topic_buff.buffer = (uint8_t *)unsub_topic_invalid7;
    unsub_topic_buff.len = (uint32_t)strlen(unsub_topic_invalid7);
    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, STATE_MQTT_TOPIC_INVALID);

    unsub_topic_buff.buffer = (uint8_t *)unsub_topic;
    unsub_topic_buff.len = (uint32_t)strlen(unsub_topic);
    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, 1);

    while ((userdata.packet_id != 1) && (recv_count-- > 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }
    ASSERT_EQ(userdata.packet_id, 1);
    recv_count = 1;

    while (test_retry-- > 0) {
        res = aiot_mqtt_pub(data->mqtt_handle, &pub_topic_buff, &pub_payload_buff, 0);
        ASSERT_EQ(res, STATE_SUCCESS);

        while (recv_count-- > 0) {
            aiot_mqtt_recv(data->mqtt_handle);
        }
        recv_count = 1;
        if (userdata.is_default_handler_received == 1) {
            userdata.is_default_handler_received = 0;
        } else {
            break;
        }
    }
    test_retry = 3;

    ASSERT_EQ(userdata.is_default_handler_received, 0);
    recv_count = 1;

    res = aiot_mqtt_sub(data->mqtt_handle, &unsub_topic_buff, NULL, 0, NULL);
    ASSERT_EQ(res, 2);

    while ((userdata.packet_id != 2) && (recv_count-- > 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }
    ASSERT_EQ(userdata.packet_id, 2);
    recv_count = 1;

    res = aiot_mqtt_pub(data->mqtt_handle, &pub_topic_buff, &pub_payload_buff, 0);
    ASSERT_EQ(res, STATE_SUCCESS);

    while (test_retry-- > 0) {
        res = aiot_mqtt_pub(data->mqtt_handle, &pub_topic_buff, &pub_payload_buff, 0);
        ASSERT_EQ(res, STATE_SUCCESS);

        while ((userdata.is_default_handler_received != 1) && (recv_count-- > 0)) {
            aiot_mqtt_recv(data->mqtt_handle);
        }
        recv_count = 1;
        if (userdata.is_default_handler_received == 1) {
            break;
        }
    }
    test_retry = 3;

    res = aiot_mqtt_unsub(data->mqtt_handle, &unsub_topic_buff);
    ASSERT_EQ(res, 3);

    recv_count = 2;
    while ((userdata.packet_id != 3) && (recv_count-- > 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }
    ASSERT_EQ(userdata.packet_id, 3);
    recv_count = 1;

    ASSERT_EQ(userdata.is_default_handler_received, 1);
    recv_count = 1;
}

CASEs(AIOT_MQTT, case_34_aiot_mqtt_deinit)
{
    int32_t res = STATE_SUCCESS;
    void *dummy_handle = NULL;

    res = aiot_mqtt_deinit(NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_deinit(&dummy_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_deinit(&data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);
}

void *case35_process_thread(void *params)
{
    int32_t res = STATE_SUCCESS;

    while (1) {
        res = aiot_mqtt_process(params);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        usleep(10 * 1000);
    }

    return NULL;
}

void *case35_sub_thread(void *params)
{
    int32_t res = STATE_SUCCESS;
    char *sub_topic = "/sys/a18wPzZJzNG/aiot_mqtt_test/thing/event/+/post_reply";
    aiot_mqtt_buff_t sub_topic_buff = {
        .buffer = (uint8_t *)sub_topic,
        .len = (uint32_t)strlen(sub_topic)
    };

    while (1) {
        res = aiot_mqtt_sub(params, &sub_topic_buff, NULL, 0, NULL);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        usleep(10 * 1000);
    }

    return NULL;
}

void *case35_pub_thread(void *params)
{
    int32_t res = STATE_SUCCESS;
    char *pub_topic = "/sys/a18wPzZJzNG/aiot_mqtt_test/thing/event/property/post";
    char *pub_payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";
    aiot_mqtt_buff_t pub_topic_buff = {
        .buffer = (uint8_t *)pub_topic,
        .len = (uint32_t)strlen(pub_topic)
    };
    aiot_mqtt_buff_t pub_payload_buff = {
        .buffer = (uint8_t *)pub_payload,
        .len = (uint32_t)strlen(pub_payload)
    };

    while (1) {
        res = aiot_mqtt_pub(params, &pub_topic_buff, &pub_payload_buff, 0);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        usleep(10 * 1000);
    }

    return NULL;
}

void *case35_recv_thread(void *params)
{
    int32_t res = STATE_SUCCESS;

    while (1) {
        res = aiot_mqtt_recv(params);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
    }

    return NULL;
}

CASEs(AIOT_MQTT, case_35_multithread)
{
    int32_t res = STATE_SUCCESS;
    static pthread_t process_thread, sub_thread, pub1_thread, pub2_thread, recv_thread;

    data->device_name = "aiot_mqtt_test_case_35";
    data->device_secret = "sSPmHMoWeGFaIMNKdIKwuPFSBjGe3gQH";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_mqtt_connect(data->mqtt_handle);

    pthread_create(&process_thread, NULL, case35_process_thread, data->mqtt_handle);
    pthread_create(&sub_thread, NULL, case35_sub_thread, data->mqtt_handle);
    pthread_create(&pub1_thread, NULL, case35_pub_thread, data->mqtt_handle);
    pthread_create(&pub2_thread, NULL, case35_pub_thread, data->mqtt_handle);
    pthread_create(&recv_thread, NULL, case35_recv_thread, data->mqtt_handle);

    res = aiot_mqtt_recv(NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    usleep(100 * 1000);

    res = aiot_mqtt_deinit(&data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    pthread_join(process_thread, NULL);
    pthread_join(sub_thread, NULL);
    pthread_join(pub1_thread, NULL);
    pthread_join(pub2_thread, NULL);
    pthread_join(recv_thread, NULL);
}

CASEs(AIOT_MQTT, case_36_aiot_mqtt_connect_tls_psk)
{
    int32_t res = STATE_SUCCESS;
    aiot_sysdep_network_cred_t cred;

    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_PSK;
    cred.max_tls_fragment = 512;
    cred.sni_enabled = 1;

    data->host = "a18wPzZJzNG.itls.cn-shanghai.aliyuncs.com";
    data->port = 1883;
    data->device_name = "aiot_mqtt_test_case_36";
    data->device_secret = "ZVgd0fPBKoUsSfK3Fk7xGvcllfAZArut";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);

    res = aiot_mqtt_connect(data->mqtt_handle);
    printf("aiot_mqtt_connect: -0x%04X\n", -res);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(AIOT_MQTT, case_37_aiot_mqtt_process_repub)
{
    int32_t res = STATE_SUCCESS;
    char *topic = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_37/thing/event/property/post";
    char *payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";
    aiot_mqtt_buff_t topic_buff = {
        .buffer = (uint8_t *)topic,
        .len = (uint32_t)strlen(topic)
    };
    aiot_mqtt_buff_t payload_buff = {
        .buffer = (uint8_t *)payload,
        .len = (uint32_t)strlen(payload)
    };

    data->device_name = "aiot_mqtt_test_case_37";
    data->device_secret = "G15mj38G3n4k9C5WKC4M8KZLodpTJKbF";

    res = aiot_mqtt_process(NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_process(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_mqtt_connect(data->mqtt_handle);

    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 1);
    ASSERT_EQ(res, 1);

    sleep(3);

    res = aiot_mqtt_process(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);
}

void case_37_core_mqtt_process_handler(void *context)
{

}

CASEs(AIOT_MQTT, case_37_core_mqtt_process)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_process_data_t process_data = {
        .handler = NULL,
        .context = NULL
    };

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    res = core_mqtt_setopt(NULL, CORE_MQTTOPT_APPEND_PROCESS_HANDLER, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = core_mqtt_setopt(data->mqtt_handle, CORE_MQTTOPT_APPEND_PROCESS_HANDLER, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = core_mqtt_setopt(data->mqtt_handle, CORE_MQTTOPT_MAX, (void *)&process_data);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);

    res = core_mqtt_setopt(data->mqtt_handle, CORE_MQTTOPT_APPEND_PROCESS_HANDLER, (void *)&process_data);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    process_data.handler = case_37_core_mqtt_process_handler;
    core_mqtt_setopt(data->mqtt_handle, CORE_MQTTOPT_APPEND_PROCESS_HANDLER, (void *)&process_data);
    core_mqtt_setopt(data->mqtt_handle, CORE_MQTTOPT_APPEND_PROCESS_HANDLER, (void *)&process_data);

    res = aiot_mqtt_process(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);

    aiot_mqtt_recv(data->mqtt_handle);
}

static void case_38_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_PUB: {

        }
        break;
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            *(uint8_t *)userdata = 1;
        }
        break;
        case AIOT_MQTTRECV_SUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_UNSUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_PUB_ACK: {

        }
        break;
        default: {

        }
    }
}

CASEs(AIOT_MQTT, case_38_aiot_mqtt_process_heartbeat)
{
    uint32_t count = 2;
    uint8_t heartbeat = 0;
    uint32_t heartbeat_interval_ms = 100;
    uint8_t heartbeat_max_lost = 1;

    data->device_name = "aiot_mqtt_test_case_38";
    data->device_secret = "5NkLXQ8ptiEzk0CcISaPpbfhScr6OiFC";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case_38_mqtt_recv_handler);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_USERDATA, (void *)&heartbeat);

    aiot_mqtt_connect(data->mqtt_handle);

    while ((heartbeat != 1) && (count-- > 0)) {
        aiot_mqtt_process(data->mqtt_handle);
        aiot_mqtt_recv(data->mqtt_handle);
        sleep(1);
    }

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HEARTBEAT_INTERVAL_MS, (void *)&heartbeat_interval_ms);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HEARTBEAT_MAX_LOST, (void *)&heartbeat_max_lost);

    count = 2;
    while (count-- > 0) {
        aiot_mqtt_process(data->mqtt_handle);
        usleep(200 * 1000);
    }
    aiot_mqtt_recv(data->mqtt_handle);

    ((core_mqtt_handle_t *)data->mqtt_handle)->heartbeat_params.last_send_time = ((core_mqtt_handle_t *)
            data->mqtt_handle)->sysdep->core_sysdep_time() + 5000;
    count = 2;
    while (count-- > 0) {
        aiot_mqtt_process(data->mqtt_handle);
        usleep(200 * 1000);
    }
}

static int32_t case_39_core_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data)
{
    return STATE_PORT_NETWORK_UNKNOWN_OPTION;
}

static int32_t case_39_core_sysdep_network_establish(void *handle)
{
    return STATE_PORT_NETWORK_DNS_FAILED;
}

static int32_t case_39_1_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    static uint8_t count = 0;
    if (count == 0) {
        char connack[] = {CORE_MQTT_CONNACK_PKT_TYPE + 1, 0x02, 0x00, CORE_MQTT_CONNACK_RCODE_ACCEPTED};
        memcpy(buffer, connack, 4);
        count = 1;
    } else if (count == 1) {
        char connack[] = {CORE_MQTT_CONNACK_PKT_TYPE, 0x02, 0x00, CORE_MQTT_CONNACK_RCODE_ACCEPTED};
        memcpy(buffer, connack, 4);
        count = 2;
    } else if (count == 2) {
        char connack[] = {CORE_MQTT_CONNACK_PKT_TYPE, 0x02, 0x00, CORE_MQTT_CONNACK_RCODE_UNACCEPTABLE_PROTOCOL_VERSION};
        memcpy(buffer, connack, 4);
        count = 3;
    } else if (count == 3) {
        char connack[] = {CORE_MQTT_CONNACK_PKT_TYPE, 0x02, 0x00, CORE_MQTT_CONNACK_RCODE_SERVER_UNAVAILABLE};
        memcpy(buffer, connack, 4);
        count = 4;
    } else if (count == 4) {
        char connack[] = {CORE_MQTT_CONNACK_PKT_TYPE, 0x02, 0x00, CORE_MQTT_CONNACK_RCODE_BAD_USERNAME_PASSWORD};
        memcpy(buffer, connack, 4);
        count = 5;
    } else if (count == 5) {
        char connack[] = {CORE_MQTT_CONNACK_PKT_TYPE, 0x02, 0x00, CORE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED};
        memcpy(buffer, connack, 4);
        count = 6;
    } else if (count == 6) {
        char connack[] = {CORE_MQTT_CONNACK_PKT_TYPE, 0x02, 0x00, CORE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED + 1};
        memcpy(buffer, connack, 4);
        count = 7;
    } else if (count == 7) {
        return -1;
    }

    return 4;
}

static int32_t case_39_2_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    return 1;
}

static int32_t case_39_core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    return -1;
}

static int32_t case_39_2_core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    return 1;
}

CASEs(AIOT_MQTT, case_39_aiot_mqtt_connect_error)
{
    int32_t res = STATE_SUCCESS;

    data->device_name = "aiot_mqtt_test_case_39";
    data->device_secret = "VWRwyDTfovMASOvJ1P7oLlBtBFOM5jKV";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_39_1_core_sysdep_network_recv;

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_CONNACK_FMT_ERROR);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_CONNACK_RCODE_UNACCEPTABLE_PROTOCOL_VERSION);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_CONNACK_RCODE_SERVER_UNAVAILABLE);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_CONNACK_RCODE_BAD_USERNAME_PASSWORD);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_CONNACK_RCODE_UNKNOWN);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_39_2_core_sysdep_network_recv;
    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_READ_LESSDATA);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_send = case_39_core_sysdep_network_send;

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_send = case_39_2_core_sysdep_network_send;
    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_WRITE_LESSDATA);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_establish =
                case_39_core_sysdep_network_establish;
    res = aiot_mqtt_connect(data->mqtt_handle);
    printf("-0x%04X\n", -res);
    ASSERT_EQ(res, STATE_PORT_NETWORK_DNS_FAILED);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_setopt = case_39_core_sysdep_network_setopt;
    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_PORT_NETWORK_UNKNOWN_OPTION);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_setopt = core_sysdep_network_setopt;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_establish = core_sysdep_network_establish;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_send = core_sysdep_network_send;
}

static int32_t case_40_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x32};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xA3};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x01};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
        char pub[] = {
            0x00, 0x3A, 0x2F, 0x73, 0x79, 0x73, 0x2F, 0x61, 0x31, 0x38, 0x77, 0x50, 0x7A, 0x5A, 0x4A, 0x7A,
            0x4E, 0x47, 0x2F, 0x61, 0x69, 0x6F, 0x74, 0x5F, 0x6D, 0x71, 0x74, 0x74, 0x5F, 0x74, 0x65, 0x73,
            0x74, 0x2F, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x2F, 0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x2F,
            0x70, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x2F, 0x73, 0x65, 0x74, 0x00, 0x00, 0x7B, 0x22, 0x6D, 0x65,
            0x74, 0x68, 0x6F, 0x64, 0x22, 0x3A, 0x22, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x2E, 0x73, 0x65, 0x72,
            0x76, 0x69, 0x63, 0x65, 0x2E, 0x70, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x2E, 0x73, 0x65,
            0x74, 0x22, 0x2C, 0x22, 0x69, 0x64, 0x22, 0x3A, 0x22, 0x39, 0x39, 0x37, 0x33, 0x35, 0x31, 0x33,
            0x35, 0x38, 0x22, 0x2C, 0x22, 0x70, 0x61, 0x72, 0x61, 0x6D, 0x73, 0x22, 0x3A, 0x7B, 0x22, 0x4C,
            0x69, 0x67, 0x68, 0x74, 0x53, 0x77, 0x69, 0x74, 0x63, 0x68, 0x22, 0x3A, 0x31, 0x7D, 0x2C, 0x22,
            0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x22, 0x3A, 0x22, 0x31, 0x2E, 0x30, 0x2E, 0x30, 0x22,
            0x7D
        };
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 4;
    }
    return res;
}

void case_40_aiot_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{

}

CASEs(AIOT_MQTT, case_40_aiot_mqtt_recv_qos1_pub)
{
    int32_t res = STATE_SUCCESS;

    data->device_name = "aiot_mqtt_test_case_40";
    data->device_secret = "h1aBKJIeEQ8ub4SnCOk474J5jvAEJIB7";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case_40_aiot_mqtt_recv_handler);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_40_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
}

void case_41_update_qos1_timestamp(core_mqtt_handle_t *mqtt_handle)
{
    core_mqtt_pub_node_t *node = NULL;

    core_list_for_each_entry(node, &mqtt_handle->pub_list, linked_node) {
        node->last_send_time += 5000;
        break;
    }
}

CASEs(AIOT_MQTT, case_41_aiot_mqtt_recv_qos1_pub_time_roll)
{
    int32_t res = STATE_SUCCESS;
    char *topic = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_41/thing/event/property/post";
    char *payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";
    aiot_mqtt_buff_t topic_buff = {
        .buffer = (uint8_t *)topic,
        .len = (uint32_t)strlen(topic)
    };
    aiot_mqtt_buff_t payload_buff = {
        .buffer = (uint8_t *)payload,
        .len = (uint32_t)strlen(payload)
    };

    data->device_name = "aiot_mqtt_test_case_41";
    data->device_secret = "JG43or4W0786EDCjFHYJoIYGVRKqeGMP";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 1);
    ASSERT_EQ(res, 1);

    case_41_update_qos1_timestamp((core_mqtt_handle_t *)data->mqtt_handle);

    aiot_mqtt_process(data->mqtt_handle);
}

CASEs(AIOT_MQTT, case_42_aiot_mqtt_recv_reconnect_time_roll)
{
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    ((core_mqtt_handle_t *)data->mqtt_handle)->reconnect_params.last_retry_time = ((core_mqtt_handle_t *)
            data->mqtt_handle)->sysdep->core_sysdep_time() + 5000;
    aiot_mqtt_recv(data->mqtt_handle);
}

static int32_t case_43_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x32};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xA3 | 0x80};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x01 | 0x80};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
        char pub[] = {0x01 | 0x80};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 4;
    } else if (count == 4) {
        char pub[] = {0x01 | 0x80};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 5;
    } else if (count == 5) {
        char pub[] = {0x01 | 0x80};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 6;
    }
    return res;
}

CASEs(AIOT_MQTT, case_43_aiot_mqtt_recv_malformed_packet_len)
{
    int32_t res = STATE_SUCCESS;

    data->device_name = "aiot_mqtt_test_case_43";
    data->device_secret = "PncgrkkVDNTdBSWZju4ae4p9ZPq79qcu";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_43_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_MALFORMED_REMAINING_LEN);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
}

static void case_44_aiot_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_PUB: {
            *(uint8_t *)userdata = 1;
        }
        break;
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {

        }
        break;
        case AIOT_MQTTRECV_SUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_UNSUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_PUB_ACK: {

        }
        break;
        default: {

        }
    }
}

CASEs(AIOT_MQTT, case_44_aiot_mqtt_sub_topic_wildcard)
{
    int32_t res = STATE_SUCCESS;
    uint8_t is_received = 0, count = 3;
    char *sub_topic = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_44/thing/event/#";
    aiot_mqtt_topic_map_t map = {
        .topic = {
            .buffer = (uint8_t *)sub_topic,
            .len = (uint32_t)strlen(sub_topic)
        },
        .handler = case_44_aiot_mqtt_recv_handler,
        .userdata = &is_received
    };
    char *sub_topic1 = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_44/thing/event/property/post_reply/#";
    aiot_mqtt_topic_map_t map1 = {
        .topic = {
            .buffer = (uint8_t *)sub_topic1,
            .len = (uint32_t)strlen(sub_topic1)
        },
        .handler = case_44_aiot_mqtt_recv_handler,
        .userdata = &is_received
    };
    char *sub_topic2 = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_44/thing/event/property/post_reply/abc/#";
    aiot_mqtt_topic_map_t map2 = {
        .topic = {
            .buffer = (uint8_t *)sub_topic2,
            .len = (uint32_t)strlen(sub_topic2)
        },
        .handler = case_44_aiot_mqtt_recv_handler,
        .userdata = &is_received
    };
    char *sub_topic3 = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_44/thing/event/+";
    aiot_mqtt_topic_map_t map3 = {
        .topic = {
            .buffer = (uint8_t *)sub_topic3,
            .len = (uint32_t)strlen(sub_topic3)
        },
        .handler = case_44_aiot_mqtt_recv_handler,
        .userdata = &is_received
    };
    char *sub_topic4 = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_44/thing/event/abc";
    aiot_mqtt_topic_map_t map4 = {
        .topic = {
            .buffer = (uint8_t *)sub_topic4,
            .len = (uint32_t)strlen(sub_topic4)
        },
        .handler = case_44_aiot_mqtt_recv_handler,
        .userdata = &is_received
    };
    char *sub_topic5 = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_44/thing/event/property/post_repl";
    aiot_mqtt_topic_map_t map5 = {
        .topic = {
            .buffer = (uint8_t *)sub_topic5,
            .len = (uint32_t)strlen(sub_topic5)
        },
        .handler = case_44_aiot_mqtt_recv_handler,
        .userdata = &is_received
    };
    char *sub_topic6 = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_44/thing/event/property/post_reply";
    aiot_mqtt_topic_map_t map6 = {
        .topic = {
            .buffer = (uint8_t *)sub_topic6,
            .len = (uint32_t)strlen(sub_topic6)
        },
        .handler = case_44_aiot_mqtt_recv_handler,
        .userdata = &is_received
    };
    char *topic = "/sys/a18wPzZJzNG/aiot_mqtt_test_case_44/thing/event/property/post";
    char *payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";
    aiot_mqtt_buff_t topic_buff = {
        .buffer = (uint8_t *)topic,
        .len = (uint32_t)strlen(topic)
    };
    aiot_mqtt_buff_t payload_buff = {
        .buffer = (uint8_t *)payload,
        .len = (uint32_t)strlen(payload)
    };

    data->device_name = "aiot_mqtt_test_case_44";
    data->device_secret = "7L1co9AtaAkwC7lTxz5cJCqA8VAVUjtf";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map);
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map1);
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map2);
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map3);
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map4);
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map5);
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&map6);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 0);
    ASSERT_EQ(res, STATE_SUCCESS);

    while ((is_received != 1) && (count-- > 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }
    ASSERT_EQ(is_received, 1);
}

static int32_t case_45_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x36};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xA3};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x01};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
        char pub[] = {
            0x00, 0x3A, 0x2F, 0x73, 0x79, 0x73, 0x2F, 0x61, 0x31, 0x38, 0x77, 0x50, 0x7A, 0x5A, 0x4A, 0x7A,
            0x4E, 0x47, 0x2F, 0x61, 0x69, 0x6F, 0x74, 0x5F, 0x6D, 0x71, 0x74, 0x74, 0x5F, 0x74, 0x65, 0x73,
            0x74, 0x2F, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x2F, 0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x2F,
            0x70, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x2F, 0x73, 0x65, 0x74, 0x00, 0x00, 0x7B, 0x22, 0x6D, 0x65,
            0x74, 0x68, 0x6F, 0x64, 0x22, 0x3A, 0x22, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x2E, 0x73, 0x65, 0x72,
            0x76, 0x69, 0x63, 0x65, 0x2E, 0x70, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x2E, 0x73, 0x65,
            0x74, 0x22, 0x2C, 0x22, 0x69, 0x64, 0x22, 0x3A, 0x22, 0x39, 0x39, 0x37, 0x33, 0x35, 0x31, 0x33,
            0x35, 0x38, 0x22, 0x2C, 0x22, 0x70, 0x61, 0x72, 0x61, 0x6D, 0x73, 0x22, 0x3A, 0x7B, 0x22, 0x4C,
            0x69, 0x67, 0x68, 0x74, 0x53, 0x77, 0x69, 0x74, 0x63, 0x68, 0x22, 0x3A, 0x31, 0x7D, 0x2C, 0x22,
            0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x22, 0x3A, 0x22, 0x31, 0x2E, 0x30, 0x2E, 0x30, 0x22,
            0x7D
        };
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 4;
    }
    return res;
}

void case45_aiot_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{

}

CASEs(AIOT_MQTT, case_45_aiot_mqtt_recv_qos2_pub)
{
    int32_t res = STATE_SUCCESS;

    data->device_name = "aiot_mqtt_test_case_45";
    data->device_secret = "B1dsu3lfWlo825gVw93LW1EQsabhcK8I";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case45_aiot_mqtt_recv_handler);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_45_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
}

static int32_t case_46_1_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x32};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xA3};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x01};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
        char pub[] = {
            0x00, 0x3A, 0x2F, 0x73, 0x79, 0x73, 0x2F, 0x61, 0x31, 0x38, 0x77, 0x50, 0x7A, 0x5A, 0x4A, 0x7A,
            0x4E, 0x47, 0x2F, 0x61, 0x69, 0x6F, 0x74, 0x5F, 0x6D, 0x71, 0x74, 0x74, 0x5F, 0x74, 0x65, 0x73,
            0x74, 0x2F, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x2F, 0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x2F,
            0x70, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x2F, 0x73, 0x65, 0x74, 0x00, 0x00, 0x7B, 0x22, 0x6D, 0x65,
            0x74, 0x68, 0x6F, 0x64, 0x22, 0x3A, 0x22, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x2E, 0x73, 0x65, 0x72,
            0x76, 0x69, 0x63, 0x65, 0x2E, 0x70, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x2E, 0x73, 0x65,
            0x74, 0x22, 0x2C, 0x22, 0x69, 0x64, 0x22, 0x3A, 0x22, 0x39, 0x39, 0x37, 0x33, 0x35, 0x31, 0x33,
            0x35, 0x38, 0x22, 0x2C, 0x22, 0x70, 0x61, 0x72, 0x61, 0x6D, 0x73, 0x22, 0x3A, 0x7B, 0x22, 0x4C,
            0x69, 0x67, 0x68, 0x74, 0x53, 0x77, 0x69, 0x74, 0x63, 0x68, 0x22, 0x3A, 0x31, 0x7D, 0x2C, 0x22,
            0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x22, 0x3A, 0x22, 0x31, 0x2E, 0x30, 0x2E, 0x30, 0x22,
            0x7D
        };
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char) - 1;
        count = 4;
    }
    return res;
}

static int32_t case_46_2_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x32};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xA3};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x01};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
        res = -1;
        count = 4;
    }
    return res;
}

void case_46_aiot_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{

}

CASEs(AIOT_MQTT, case_46_aiot_mqtt_recv_malformed_remaining_bytes)
{
    int32_t res = STATE_SUCCESS;

    data->device_name = "aiot_mqtt_test_case_46";
    data->device_secret = "PSTcYd2BPcZJ6wMlnVoKrjxFR2EcjaMM";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case_46_aiot_mqtt_recv_handler);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_46_1_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_MALFORMED_REMAINING_BYTES);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_46_2_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    printf("aiot_mqtt_recv -0x%04X\n", -res);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_READ_LESSDATA);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
}

static int32_t case_47_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x40};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0x01};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x00};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    }
    return res;
}

void case_47_aiot_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{

}

CASEs(AIOT_MQTT, case_47_aiot_mqtt_recv_malformed_puback)
{
    int32_t res = STATE_SUCCESS;

    data->device_name = "aiot_mqtt_test_case_47";
    data->device_secret = "mnmdaYuhDX743Kzh0Hj3Wpdyr9u5puIP";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case_47_aiot_mqtt_recv_handler);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_47_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    printf("aiot_mqtt_recv -0x%04X\n", -res);
    ASSERT_EQ(res, STATE_SUCCESS);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
}

static int32_t case_48_1_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x90};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x00, 0x01};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    }
    return res;
}

static int32_t case_48_2_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x90};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0x03};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x00, 0x01, CORE_MQTT_SUBACK_RCODE_MAXQOS1};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    }
    return res;
}

static int32_t case_48_3_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x90};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0x03};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x00, 0x01, CORE_MQTT_SUBACK_RCODE_MAXQOS2};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    }
    return res;
}

static int32_t case_48_4_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x90};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0x03};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x00, 0x01, CORE_MQTT_SUBACK_RCODE_FAILURE};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    }
    return res;
}

static int32_t case_48_5_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x90};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0x03};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x00, 0x01, CORE_MQTT_SUBACK_RCODE_FAILURE + 1};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    }
    return res;
}

CASEs(AIOT_MQTT, case_48_aiot_mqtt_recv_malformed_suback)
{
    int32_t res = STATE_SUCCESS;

    data->device_name = "aiot_mqtt_test_case_48";
    data->device_secret = "j4O5G2OPo1RBcNbpLFxkUXTBYHCIUKvt";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_48_1_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    printf("[%d] aiot_mqtt_recv -0x%04X\n", __LINE__, -res);
    ASSERT_EQ(res, STATE_SUCCESS);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_48_2_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    printf("[%d] aiot_mqtt_recv -0x%04X\n", __LINE__, -res);
    ASSERT_EQ(res, STATE_SUCCESS);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_48_3_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    printf("[%d] aiot_mqtt_recv -0x%04X\n", __LINE__, -res);
    ASSERT_EQ(res, STATE_SUCCESS);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_48_4_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    printf("[%d] aiot_mqtt_recv -0x%04X\n", __LINE__, -res);
    ASSERT_EQ(res, STATE_SUCCESS);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_48_5_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    printf("[%d] aiot_mqtt_recv -0x%04X\n", __LINE__, -res);
    ASSERT_EQ(res, STATE_SUCCESS);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
}

static int32_t case_49_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x50};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0x03};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x00, 0x01, CORE_MQTT_SUBACK_RCODE_MAXQOS1};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    }
    return res;
}

CASEs(AIOT_MQTT, case_49_aiot_mqtt_recv_malformed_packet)
{
    int32_t res = STATE_SUCCESS;

    data->device_name = "aiot_mqtt_test_case_49";
    data->device_secret = "UlPa9FhLuJg4LAguJQvI5Ds4zVX0tvCb";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_49_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    ASSERT_EQ(res, STATE_MQTT_PACKET_TYPE_UNKNOWN);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
}

CASEs(AIOT_MQTT, case_50_aiot_mqtt_setopt_AIOT_MQTTOPT_CLEAN_SESSION)
{
    int32_t res = STATE_SUCCESS;
    uint8_t clean_session = 0;

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->clean_session, 1);

    clean_session = 2;
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_CLEAN_SESSION, (void *)&clean_session);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);

    clean_session = 0;
    res = aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_CLEAN_SESSION, (void *)&clean_session);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((core_mqtt_handle_t *)data->mqtt_handle)->clean_session, clean_session);
}

static void case_51_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_PUB: {

        }
        break;
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            *(uint8_t *)userdata = 1;
        }
        break;
        case AIOT_MQTTRECV_SUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_UNSUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_PUB_ACK: {

        }
        break;
        default: {

        }
    }
}

CASEs(AIOT_MQTT, case_51_aiot_mqtt_heartbeat)
{
    int32_t res = STATE_SUCCESS;
    uint8_t is_received = 0, count = 2;

    data->device_name = "aiot_mqtt_test_case_51";
    data->device_secret = "takQac7mHO9BkkD8w41ZIBo9AXwRdAY7";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case_51_mqtt_recv_handler);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_USERDATA, (void *)&is_received);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_mqtt_heartbeat(NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_mqtt_heartbeat(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    while ((is_received != 1) && (count-- > 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }
    ASSERT_EQ(is_received, 1);
}

char *case_52_client_cert = {
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDhzCCAm+gAwIBAgIHUITEfMLu7TANBgkqhkiG9w0BAQsFADBTMSgwJgYDVQQD\r\n"
    "DB9BbGliYWJhIENsb3VkIElvVCBPcGVyYXRpb24gQ0ExMRowGAYDVQQKDBFBbGli\r\n"
    "YWJhIENsb3VkIElvVDELMAkGA1UEBhMCQ04wIBcNMjAwMTE2MTA1MDEwWhgPMjEy\r\n"
    "MDAxMTYxMDUwMTBaMFExJjAkBgNVBAMMHUFsaWJhYmEgQ2xvdWQgSW9UIENlcnRp\r\n"
    "ZmljYXRlMRowGAYDVQQKDBFBbGliYWJhIENsb3VkIElvVDELMAkGA1UEBhMCQ04w\r\n"
    "ggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCMhUnaflJtjkFtjviHvaXQ\r\n"
    "+2MNQHL1fZH2C5XkHCU6mADJBAG0n1OSjc265kqDQ72ClDVtWL2ehvf7s/uf0Z/h\r\n"
    "1Uw5f7A+68DVIL+5425MzX1Fxpjrq8x6myxX28vDwOJ23f1fRfOd7tOxB3WYYqy+\r\n"
    "hFF4+ALTyZ1nqkAzXz9ALpqgez2IYSe2hS8iWnCEbgXkjMagMvt5JO69pVP6ijbE\r\n"
    "/V50Vu0cZo6U0XQszr1yHABT0j1gtKXHIIIzlIR6xT/nr7DXfoQ/atdI2Y9fC3z3\r\n"
    "iONV0NVE0SOZeLqlJghiVUAp+IvlgG37oSexZIq0PBeD0XHuNhjYxCil4Sb0U20J\r\n"
    "AgMBAAGjYDBeMB8GA1UdIwQYMBaAFIo3m6hwzdX5SMiXfiGfWW9JjiQRMB0GA1Ud\r\n"
    "DgQWBBQoVpJXc2b3Nsa80dHw2fAVXsm6QzAOBgNVHQ8BAf8EBAMCA/gwDAYDVR0T\r\n"
    "AQH/BAIwADANBgkqhkiG9w0BAQsFAAOCAQEAKiruKgx1anvAR2K56jubJwYRwsHW\r\n"
    "4VTYXMU1Zk0j338CsDI5r7mPxzE/mfs7nT87Um65Bb0lw2/eA+q+Nh7A4cG28jxV\r\n"
    "b/qyeNyhdqRe8SraPycir3VKdxZ7hglrKyeNohy+AAGqmp94/TA1moumMIPBqOGu\r\n"
    "Llk/y5XES6Sl7lfVd3UdB7T56LyUpBOXPTOa/nvr8MZu+itze3PnW0YxtWL1MshO\r\n"
    "qTO4L3qAoRUpKAN7QG9gdfRvYFjCT29P1r3PCFY1zYuwp13+EZJMwY6j8GX9LlPV\r\n"
    "zkR8+aYF8ljYC9PCJmNInZx3UtxF9egfycL1nFrSCIFONBstrZbRh6t0hg==\r\n"
    "-----END CERTIFICATE-----\r\n"
};

char *case_52_client_privkey = {
    "-----BEGIN RSA PRIVATE KEY-----\r\n"
    "MIIEowIBAAKCAQEAjIVJ2n5SbY5BbY74h72l0PtjDUBy9X2R9guV5BwlOpgAyQQB\r\n"
    "tJ9Tko3NuuZKg0O9gpQ1bVi9nob3+7P7n9Gf4dVMOX+wPuvA1SC/ueNuTM19RcaY\r\n"
    "66vMepssV9vLw8Didt39X0Xzne7TsQd1mGKsvoRRePgC08mdZ6pAM18/QC6aoHs9\r\n"
    "iGEntoUvIlpwhG4F5IzGoDL7eSTuvaVT+oo2xP1edFbtHGaOlNF0LM69chwAU9I9\r\n"
    "YLSlxyCCM5SEesU/56+w136EP2rXSNmPXwt894jjVdDVRNEjmXi6pSYIYlVAKfiL\r\n"
    "5YBt+6EnsWSKtDwXg9Fx7jYY2MQopeEm9FNtCQIDAQABAoIBAGe9c0OSJMpqzlTS\r\n"
    "yxpzYTpCjOLYpMYl+R8beIJaYQW7+EBu689sHKfCdpK3t2TnGr6PKk5ayEqDvAof\r\n"
    "2vEnMhDohoiggv5A0DDIJ6NVizW6MvTTZEwAnkoZywfl5a3T9Zzp0EeI/gynp7M3\r\n"
    "HZLtrjhMuVVES9oNK16/6vvCIpD/fed5/RbQTYwXQDT9KS7HoD1/ldnpFOaUMoKs\r\n"
    "4eo+BEib4dT8zm2nDsZd+RV5PbYdHeNGDOemiaP/gVyvRxdcBI1A2ouaZ358vNdz\r\n"
    "h4u5pw1m2Xz9ifb9dFBJQUnl1ZdzdaXQUUnUlJwVfn/JiEHH4v7VqtXXGkOE+i66\r\n"
    "gn9KZL0CgYEAzpuOLObvq/D4jQgVA4QO17b92mxamAqFQvsfE0GWJjz26HPxWFXk\r\n"
    "jNt9LHjcX3TaLH7Gwno2YIV70G0UgOP3Nc6bJ+upbWIzm9RX/SVMTa9LRwqFA8KN\r\n"
    "JtqKIq16ElDaMC1WwcOYdsZl2AwTmW7oumSTQ4c7Q1i8R53U33ty6esCgYEArh0x\r\n"
    "EDlpoSrNGpcyoCPwjxbgTwm1ChqhzYM20ySQflxqPQsboV2oEvRxYPtDkv5pWMes\r\n"
    "raC7m9/aKMQpzUGs7vP/eNQaHIP+6XEbdplszETvKkAS1VTe8lGDl/osfC4/jCbo\r\n"
    "foQvGNnCIt7xCRWexMQQGkm9z55PgEpOD/ris9sCgYBiirsDG5qQrbw+t+4d4Syb\r\n"
    "IoJtXWTQQ6RP0CqAKrYMwuMY98PS2BTMQhuvzG1/ceJleooeU9//pWrqfDxdRV2x\r\n"
    "YjuKjNIgg8gNuPfGm7WLD+KdnZzXsEFWmMFtzMP+XGXUuKs6e6oKbJJCu2/VrJp/\r\n"
    "3FyIxsUzV+1lUxgnB2BFxQKBgGBG9g1hod4ju7x7ZNwWY3vMC4bI+Fm49kWcy+Ef\r\n"
    "M+MbmQdeMA4fqsOodVVKO0h586jK6Notqe/8bTbjmbXNHiIXu+mFZ1bjSj6tc6E2\r\n"
    "H2ooqBM10PQz9QALQPf1t0mHjU4jaaYj06MrLmV339LFKMDnJXxT0GiQwqdO7zF4\r\n"
    "6ojHAoGBALmFgdBSpJfUZvmLzg/dHeqW8Bx8YlqyIgkN8+SIa1ucKQ14Uy5nghXT\r\n"
    "WGNZoFVOjc2z2pCbsnTsDx/CwZW/T1lHz5WWPyF8ZHEjd9FD7Twf29ZE/ewSuk7u\r\n"
    "6d6lycHmyi1EOEjsxv6RnhaIKZJdwBzZpyRNuSpa04AZ3abJQCkx\r\n"
    "-----END RSA PRIVATE KEY-----\r\n"
};

static void case_52_mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_PUB: {
            char *pk_key = "productKey", *dn_key = "deviceName";
            char *pk_value = NULL, *dn_value = NULL;
            uint32_t pk_value_len = 0, dn_value_len = 0;

            core_json_value((char *)packet->data.pub.payload, packet->data.pub.payload_len, pk_key, strlen(pk_key), &pk_value, &pk_value_len);
            core_json_value((char *)packet->data.pub.payload, packet->data.pub.payload_len, dn_key, strlen(dn_key), &dn_value, &dn_value_len);

            if (pk_value != NULL && dn_value != NULL) {
                *(uint8_t *)userdata = 1;
            }
        }
        break;
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {

        }
        break;
        case AIOT_MQTTRECV_SUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_UNSUB_ACK: {

        }
        break;
        case AIOT_MQTTRECV_PUB_ACK: {

        }
        break;
        default: {

        }
    }
}

CASEs(AIOT_MQTT, case_52_aiot_mqtt_connect_tls_x509)
{
    int32_t res = STATE_SUCCESS;
    aiot_sysdep_network_cred_t cred;
    uint8_t count = 3, recv_triple = 0;

    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_RSA;
    cred.max_tls_fragment = 4096;
    cred.sni_enabled = 1;
    cred.x509_server_cert = ali_ca_crt;
    cred.x509_server_cert_len = strlen(ali_ca_crt);
    cred.x509_client_cert = case_52_client_cert;
    cred.x509_client_cert_len = strlen(case_52_client_cert);
    cred.x509_client_privkey = case_52_client_privkey;
    cred.x509_client_privkey_len = strlen(case_52_client_privkey);


    data->host = "x509.itls.cn-shanghai.aliyuncs.com";
    data->port = 1883;
    data->product_key = "";
    data->device_name = "";
    data->device_secret = "";

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)case_52_mqtt_recv_handler);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_USERDATA, (void *)&recv_triple);

    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    while((count-- > 0) && (recv_triple == 0)) {
        aiot_mqtt_recv(data->mqtt_handle);
    }

    ASSERT_EQ(recv_triple, 1);
}

SUITE(AIOT_MQTT) = {
    ADD_CASE(AIOT_MQTT, case_01_aiot_mqtt_init_without_portfile),
    ADD_CASE(AIOT_MQTT, case_02_aiot_mqtt_init_with_portfile),
    ADD_CASE(AIOT_MQTT, case_03_aiot_mqtt_setopt_AIOT_MQTTOPT_HOST),
    ADD_CASE(AIOT_MQTT, case_04_aiot_mqtt_setopt_AIOT_MQTTOPT_PORT),
    ADD_CASE(AIOT_MQTT, case_05_aiot_mqtt_setopt_AIOT_MQTTOPT_PRODUCT_KEY),
    ADD_CASE(AIOT_MQTT, case_06_aiot_mqtt_setopt_AIOT_MQTTOPT_DEVICE_NAME),
    ADD_CASE(AIOT_MQTT, case_07_aiot_mqtt_setopt_AIOT_MQTTOPT_DEVICE_SECRET),
    ADD_CASE(AIOT_MQTT, case_08_aiot_mqtt_setopt_AIOT_MQTTOPT_EXTEND_CLIENTID),
    ADD_CASE(AIOT_MQTT, case_09_aiot_mqtt_setopt_AIOT_MQTTOPT_SECURITY_MODE),
    ADD_CASE(AIOT_MQTT, case_10_aiot_mqtt_setopt_AIOT_MQTTOPT_USERNAME),
    ADD_CASE(AIOT_MQTT, case_11_aiot_mqtt_setopt_AIOT_MQTTOPT_PASSWORD),
    ADD_CASE(AIOT_MQTT, case_12_aiot_mqtt_setopt_AIOT_MQTTOPT_CLIENTID),
    ADD_CASE(AIOT_MQTT, case_13_aiot_mqtt_setopt_AIOT_MQTTOPT_KEEPALIVE_SEC),
    ADD_CASE(AIOT_MQTT, case_14_aiot_mqtt_setopt_AIOT_MQTTOPT_NETWORK_CRED),
    ADD_CASE(AIOT_MQTT, case_15_aiot_mqtt_setopt_AIOT_MQTTOPT_HEARTBEAT_INTERVAL_MS),
    ADD_CASE(AIOT_MQTT, case_16_aiot_mqtt_setopt_AIOT_MQTTOPT_HEARTBEAT_MAX_LOST),
    ADD_CASE(AIOT_MQTT, case_17_aiot_mqtt_setopt_AIOT_MQTTOPT_RECONN_ENABLED),
    ADD_CASE(AIOT_MQTT, case_18_aiot_mqtt_setopt_AIOT_MQTTOPT_RECONN_INTERVAL_MS),
    ADD_CASE(AIOT_MQTT, case_19_aiot_mqtt_setopt_AIOT_MQTTOPT_SEND_TIMEOUT_MS),
    ADD_CASE(AIOT_MQTT, case_20_aiot_mqtt_setopt_AIOT_MQTTOPT_RECV_TIMEOUT_MS),
    ADD_CASE(AIOT_MQTT, case_21_aiot_mqtt_setopt_AIOT_MQTTOPT_REPUB_TIMEOUT_MS),
    ADD_CASE(AIOT_MQTT, case_22_aiot_mqtt_setopt_AIOT_MQTTOPT_DEINIT_TIMEOUT_MS),
    ADD_CASE(AIOT_MQTT, case_23_aiot_mqtt_setopt_AIOT_MQTTOPT_RECV_HANDLER),
    ADD_CASE(AIOT_MQTT, case_24_aiot_mqtt_setopt_AIOT_MQTTOPT_EVENT_HANDLER),
    ADD_CASE(AIOT_MQTT, case_25_aiot_mqtt_setopt_AIOT_MQTTOPT_APPEND_TOPIC_MAP),
    ADD_CASE(AIOT_MQTT, case_26_aiot_mqtt_setopt_AIOT_MQTTOPT_USERDATA),
    ADD_CASE(AIOT_MQTT, case_27_aiot_mqtt_setopt_invalid_user_input),
    ADD_CASE(AIOT_MQTT, case_28_aiot_mqtt_connect),
    ADD_CASE(AIOT_MQTT, case_29_aiot_mqtt_disconnect),
    ADD_CASE(AIOT_MQTT, case_30_aiot_mqtt_process),
    ADD_CASE(AIOT_MQTT, case_31_aiot_mqtt_pub),
    ADD_CASE(AIOT_MQTT, case_32_aiot_mqtt_sub),
    ADD_CASE(AIOT_MQTT, case_33_aiot_mqtt_unsub),
    ADD_CASE(AIOT_MQTT, case_34_aiot_mqtt_deinit),
    ADD_CASE(AIOT_MQTT, case_35_multithread),
    ADD_CASE(AIOT_MQTT, case_36_aiot_mqtt_connect_tls_psk),
    ADD_CASE(AIOT_MQTT, case_37_aiot_mqtt_process_repub),
    ADD_CASE(AIOT_MQTT, case_37_core_mqtt_process),
    ADD_CASE(AIOT_MQTT, case_38_aiot_mqtt_process_heartbeat),
    ADD_CASE(AIOT_MQTT, case_39_aiot_mqtt_connect_error),
    ADD_CASE(AIOT_MQTT, case_40_aiot_mqtt_recv_qos1_pub),
    ADD_CASE(AIOT_MQTT, case_41_aiot_mqtt_recv_qos1_pub_time_roll),
    ADD_CASE(AIOT_MQTT, case_42_aiot_mqtt_recv_reconnect_time_roll),
    ADD_CASE(AIOT_MQTT, case_43_aiot_mqtt_recv_malformed_packet_len),
    ADD_CASE(AIOT_MQTT, case_44_aiot_mqtt_sub_topic_wildcard),
    ADD_CASE(AIOT_MQTT, case_45_aiot_mqtt_recv_qos2_pub),
    ADD_CASE(AIOT_MQTT, case_46_aiot_mqtt_recv_malformed_remaining_bytes),
    ADD_CASE(AIOT_MQTT, case_47_aiot_mqtt_recv_malformed_puback),
    ADD_CASE(AIOT_MQTT, case_48_aiot_mqtt_recv_malformed_suback),
    ADD_CASE(AIOT_MQTT, case_49_aiot_mqtt_recv_malformed_packet),
    ADD_CASE(AIOT_MQTT, case_50_aiot_mqtt_setopt_AIOT_MQTTOPT_CLEAN_SESSION),
    ADD_CASE(AIOT_MQTT, case_51_aiot_mqtt_heartbeat),
    ADD_CASE(AIOT_MQTT, case_52_aiot_mqtt_connect_tls_x509),
    ADD_CASE_NULL
};

