#include <stdio.h>
#include <unistd.h>
#include "cu_test.h"
#include "core_http.h"
#include "aiot_http_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_state_api.h"

char test_data_token_expired[] = \
                                 "HTTP/1.1 200\r\n"      \
                                 "Date: Fri, 20 Dec 2019 10:15:51 GMT\r\n"       \
                                 "Content-Type: application/json\r\n"        \
                                 "Content-Length: 85\r\n"        \
                                 "Connection: close\r\n"     \
                                 "Server: Tengine\r\n"       \
                                 "EagleEye-TraceId: 0bc5deef15768369512723115e5703\r\n"      \
                                 "Timing-Allow-Origin: *\r\n"        \
                                 "\r\n"  \
                                 "{\"code\":20001,\"info\":{\"messageId\":1207967842654323713},\"message\":\"token is expired\"}";

char test_data_code_no_exist_and_ugly_header[] = \
        "HTTP/1.1 200\r\n"      \
        "Date: Fri, 20 Dec 2019 10:15:51 GMT\r\n"       \
        ": application/json\r\n"        \
        "Content-Length: 85\r\n"        \
        "Connection: close\r\n"     \
        "Server: \r\n"       \
        "EagleEye-TraceId:\r\n"      \
        "Timing-Allow-Origin: *\r\n"        \
        "\r\n"  \
        "{\"code\": \"abc\",\"info\":{\"messageId\":1207967842654323713},\"message\":\"token is expired\"}";

char test_data_ugly_start_line[] = \
                                   "HTTP/1.1 abc\r\n"      \
                                   "Date: Fri, 20 Dec 2019 10:15:51 GMT\r\n"       \
                                   "Content-Type: application/json\r\n"        \
                                   "Content-Length: 85\r\n"        \
                                   "Connection: close\r\n"     \
                                   "Server: Tengine\r\n"       \
                                   "EagleEye-TraceId: 0bc5deef15768369512723115e5703\r\n"      \
                                   "Timing-Allow-Origin: *\r\n"        \
                                   "\r\n"  \
                                   "{\"code\": \"abc\",\"info\":{\"messageId\":1207967842654323713},\"message\":\"token is expired\"}";

char test_data_rsp_status_code_4xx[] = \
                                       "HTTP/1.1 401\r\n"      \
                                       "Date: Fri, 20 Dec 2019 10:15:51 GMT\r\n"       \
                                       "Content-Type: application/json\r\n"        \
                                       "Content-Length: 85\r\n"        \
                                       "Connection: close\r\n"     \
                                       "Server: Tengine\r\n"       \
                                       "EagleEye-TraceId: 0bc5deef15768369512723115e5703\r\n"      \
                                       "Timing-Allow-Origin: *\r\n"        \
                                       "\r\n"  \
                                       "{\"code\": \"abc\",\"info\":{\"messageId\":1207967842654323713},\"message\":\"token is expired\"}";

FILE *fd = NULL;

static int32_t aiot_http_test_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

extern int32_t core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                        core_sysdep_addr_t *addr);
extern int32_t core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr);

static int32_t test_sysdep_network_read(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                        core_sysdep_addr_t *addr)
{
    return -1;
}
static int32_t test_sysdep_network_read1(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    if (fd != NULL) {
        return fread(buffer, 1, len, fd);
    }
    return -1;
}
static int32_t test_sysdep_network_write(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    return -1;
}

void test_http_event_handler(void *handle, const aiot_http_event_t *event, void *user_data)
{

}

void core_aiot_http_event_handler(void *handle, const aiot_http_event_t *event, void *userdata)
{
    switch(event->type) {
        case AIOT_HTTPEVT_TOKEN_INVALID: {
            printf("AIOT_HTTPEVT_TOKEN_INVALID\n");
        }
        break;
        default: {

        }
        break;
    }
}

void core_aiot_http_recv_handler(void *handle, const aiot_http_recv_t *packet, void *userdata)
{
    core_http_handle_t *http_handle = (core_http_handle_t *)handle;
    core_http_response_t *response = (core_http_response_t *)userdata;

    switch (packet->type) {
        case AIOT_HTTPRECV_STATUS_CODE: {
            response->code = packet->data.status_code.code;
            printf("response->code: %d\n", response->code);
        }
        break;
        case AIOT_HTTPRECV_HEADER: {
            if ((strlen(packet->data.header.key) == strlen("Content-Length")) &&
                (memcmp(packet->data.header.key, "Content-Length", strlen(packet->data.header.key)) == 0)) {
                core_str2uint(packet->data.header.value, (uint8_t)strlen(packet->data.header.value), &response->content_total_len);
                printf("response->content_total_len: %d\n", response->content_total_len);
            }
        }
        break;
        case AIOT_HTTPRECV_BODY: {
            uint8_t *content = http_handle->sysdep->core_sysdep_malloc(response->content_len + packet->data.body.len + 1,
                               CORE_HTTP_MODULE_NAME);
            if (content == NULL) {
                return;
            }
            memset(content, 0, response->content_len + packet->data.body.len + 1);
            if (response->content != NULL) {
                memcpy(content, response->content, response->content_len);
                http_handle->sysdep->core_sysdep_free(response->content);
            }
            memcpy(content + response->content_len, packet->data.body.buffer, packet->data.body.len);
            response->content = content;
            response->content_len = response->content_len + packet->data.body.len;
        }
        break;
        default: {

        }
        break;
    }
}

DATA(AIOT_HTTP)
{
    void *http_handle;
    char *product_key;
    char *device_name;
    char *device_secret;
};

SETUP(AIOT_HTTP)
{
    uint16_t port = 443;
    extern const char *ali_ca_crt;
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_network_cred_t cred;

    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    aiot_state_set_logcb(aiot_http_test_logcb);

    data->http_handle = aiot_http_init();
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PORT, &port);

    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_RSA;
    cred.x509_server_cert = ali_ca_crt;
    cred.x509_server_cert_len = strlen(ali_ca_crt);
    cred.max_tls_fragment = 16384;
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_NETWORK_CRED, &cred);
}

TEARDOWN(AIOT_HTTP)
{
    aiot_http_deinit(&data->http_handle);

    /* recover network */
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    g_aiot_sysdep_portfile.core_sysdep_network_send = core_sysdep_network_send;
    g_aiot_sysdep_portfile.core_sysdep_network_recv = core_sysdep_network_recv;
}

CASE(AIOT_HTTP, case_01_aiot_http_init_without_portfile)
{
    aiot_sysdep_set_portfile(NULL);

    void *http_handle = aiot_http_init();
    ASSERT_NULL(http_handle);
}

CASE(AIOT_HTTP, case_02_aiot_http_init_with_portfile)
{
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);

    void *http_handle = aiot_http_init();
    ASSERT_NOT_NULL(http_handle);

    aiot_http_deinit(&http_handle);
}

CASE(AIOT_HTTP, case_03_aiot_http_setopt_null_handle)
{
    void *http_handle = NULL;
    int32_t res;

    res = aiot_http_setopt(http_handle, AIOT_HTTPOPT_HOST, "www.aiot.com");
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASEs(AIOT_HTTP, case_04_aiot_http_setopt_invalid_params)
{
    int32_t res;
    int32_t opt_data = 0;

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_MAX, &opt_data);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASEs(AIOT_HTTP, case_05_aiot_http_setopt_set_cred)
{
    int32_t res;
    aiot_sysdep_network_cred_t cred = {
        .option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_PSK,
    };

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_NETWORK_CRED, &cred);
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_NETWORK_CRED, &cred);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(AIOT_HTTP, case_06_aiot_http_setopt_set_numeric_data)
{
    int32_t res;
    uint16_t port = 80;
    uint32_t timeout = 5000;
    uint8_t persistent_conn = 1;

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PORT, &port);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_SEND_TIMEOUT_MS, &timeout);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_TIMEOUT_MS, &timeout);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_LONG_CONNECTION, &persistent_conn);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(AIOT_HTTP, case_07_aiot_http_setopt_set_string_data_twice)
{
    int32_t res;

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, "www.iot.com");
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, "www.iot.com");
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, "this_is_a_pk");
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, "this_is_a_pk");
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, "this_is_device_name");
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, "this_is_device_name");
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, "this_is_device_secret");
    ASSERT_EQ(res, STATE_SUCCESS);
    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, "this_is_device_secret");
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(AIOT_HTTP, case_08_aiot_http_setopt_set_event_handler)
{
    uint32_t user_data = 0;
    int32_t res;

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, &user_data);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_EVENT_HANDLER, test_http_event_handler);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASE(AIOT_HTTP, case_09_aiot_http_auth_null_handle)
{
    void *handle = NULL;
    int32_t res;

    res = aiot_http_auth(handle);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASEs(AIOT_HTTP, case_10_aiot_http_auth_missing_mandatory_option)
{
    int32_t res;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_10";
    data->device_secret = "2fKToVR1IBiGqvu9p3EaPV4eZ6MI3AE7";

    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_MISSING_PRODUCT_KEY);

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_MISSING_DEVICE_NAME);

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_MISSING_DEVICE_SECRET);

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_MISSING_HOST);

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    res = aiot_http_auth(data->http_handle);
    printf("res: 0x%04X\n", -res);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(AIOT_HTTP, case_11_aiot_http_auth_with_invalid_device_secret)
{
    int32_t res;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_11";
    data->device_secret = "4EInzMrwChWCSo1jLZgi3XKukzjavIZvb";

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);

    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_HTTP_AUTH_TOKEN_FAILED);
}

CASE(AIOT_HTTP, case_12_aiot_http_send_null_handle)
{
    void *http_handle = NULL;
    int32_t res;

    res = aiot_http_send(http_handle, "/pk/dn/user/update", (uint8_t *)"hello world", strlen("hello world"));
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASEs(AIOT_HTTP, case_13_aiot_http_send_invalid_params)
{
    int32_t res;

    res = aiot_http_send(data->http_handle, NULL, (uint8_t *)"hello world", strlen("hello world"));
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_http_send(data->http_handle, "/pk/dn/user/update", NULL, strlen("hello world"));
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_http_send(data->http_handle, "/pk/dn/user/update", (uint8_t *)"hello world", 0);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);
}

CASEs(AIOT_HTTP, case_14_aiot_http_send_without_auth)
{
    int32_t res;

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_11/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    ASSERT_EQ(res, STATE_HTTP_NEED_AUTH);
}

CASEs(AIOT_HTTP, case_15_aiot_http_send_invalid_path)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_15";
    data->device_secret = "76ZhU62baGtE9fjYyoE97lefxFPLTOsG";

    memset(&response, 0, sizeof(core_http_response_t));

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_HANDLER, (void *)core_aiot_http_recv_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, (void *)&response);

    res = aiot_http_auth(data->http_handle);
    printf("send res: -0x%04x\n", -res);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_send(data->http_handle, "/error/path", (uint8_t *)"hello world", strlen("hello world"));
    printf("send res: %d\n", res);
    ASSERT_GE(res, STATE_SUCCESS);

    res = aiot_http_recv(data->http_handle);
    ASSERT_GE(res, STATE_SUCCESS);
    printf("recv res: %d, msg: %.*s\n", res, response.content_len, response.content);
    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
        memset(&response, 0, sizeof(core_http_response_t));
    }
}

CASEs(AIOT_HTTP, case_16_aiot_http_send_without_network)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_16";
    data->device_secret = "eOxqBAnqH8vLpYjHu46pA4yYCBO83zs0";

    memset(&response, 0, sizeof(core_http_response_t));

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_HANDLER, (void *)core_aiot_http_recv_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, (void *)&response);

    res = aiot_http_auth(data->http_handle);
    printf("send res: -0x%04x\n", res);
    ASSERT_EQ(res, STATE_SUCCESS);

    /* hack the network write */
    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_send = test_sysdep_network_write;

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_16/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    ASSERT_GE(res, STATE_SYS_DEPEND_NWK_CLOSED);
    printf("send res: %d\n", res);

    res = aiot_http_recv(data->http_handle);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);
    printf("recv res: %d\n", res);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
        memset(&response, 0, sizeof(core_http_response_t));
    }
}

CASEs(AIOT_HTTP, case_17_aiot_http_send_normal)
{
    int32_t res;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_17";
    data->device_secret = "6CXtP4qln3ONvRJ7bxbROloG6rcN5qp4";

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);

    res = aiot_http_auth(data->http_handle);
    printf("send res: -0x%04x\n", -res);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_17/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    ASSERT_GE(res, STATE_SUCCESS);
    printf("send res: %d\n", res);
}

CASE(AIOT_HTTP, case_18_aiot_http_recv_null_handle)
{
    int32_t res;

    res = aiot_http_recv(NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASEs(AIOT_HTTP, case_19_aiot_http_recv_invalid_params)
{
    int32_t res;
    void *http_handle = NULL;

    res = aiot_http_recv(http_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = aiot_http_recv(data->http_handle);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);
}

CASEs(AIOT_HTTP, case_20_aiot_http_recv_without_network)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_20";
    data->device_secret = "newDU2XawtaD3NdF1Pnqr4aETk0pnsPY";

    memset(&response, 0, sizeof(core_http_response_t));

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_HANDLER, (void *)core_aiot_http_recv_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, (void *)&response);

    res = aiot_http_auth(data->http_handle);
    printf("send res: -0x%04x\n", -res);
    ASSERT_GE(res, STATE_SUCCESS);

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_20/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    printf("send res: %d\n", res);
    ASSERT_GE(res, 0);

    /* hack the network read */
    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_recv = test_sysdep_network_read;

    res = aiot_http_recv(data->http_handle);
    printf("recv res: -0x%04X\n", -res);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
        memset(&response, 0, sizeof(core_http_response_t));
    }
}

CASEs(AIOT_HTTP, case_21_aiot_http_recv_rsp_exception)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_21";
    data->device_secret = "efYeuZ5t9j49oK8k1fSzceqGQTMWsuxv";

    memset(&response, 0, sizeof(core_http_response_t));

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_EVENT_HANDLER, test_http_event_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_HANDLER, (void *)core_aiot_http_recv_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, (void *)&response);

    res = aiot_http_auth(data->http_handle);
    printf("send res: -0x%04x\n", -res);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_21/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    printf("send res: %d\n", res);
    ASSERT_GE(res, 0);

    /* token expired test */
    fd = fopen(".test_data", "wb+");
    if (fd == NULL) {
        printf("fopen failed\r");
        ASSERT_FAIL();
    }
    fwrite(test_data_token_expired, 1, sizeof(test_data_token_expired), fd);
    fclose(fd);
    fd = fopen(".test_data", "rb");

    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_recv = test_sysdep_network_read1;

    res = aiot_http_recv(data->http_handle);
    printf("recv res: -0x%04X\n", -res);
    ASSERT_GE(res, 0);
    ASSERT_EQ(response.code, 200);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));

    fclose(fd);
    res = system("rm -rf .test_data");

    sleep(1);
    /* ugly header and response code not exist test */
    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_21/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    printf("send res: %d\n", res);
    ASSERT_GE(res, 0);

    fd = fopen(".test_data", "wb+");
    if (fd == NULL) {
        printf("fopen failed\r");
        ASSERT_FAIL();
    }
    fwrite(test_data_code_no_exist_and_ugly_header, 1, sizeof(test_data_code_no_exist_and_ugly_header), fd);
    fclose(fd);
    fd = fopen(".test_data", "rb");

    res = aiot_http_recv(data->http_handle);
    printf("recv res: %d\n", res);
    ASSERT_EQ(res, STATE_SUCCESS);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));

    fclose(fd);
    res = system("rm -rf .test_data");

    sleep(1);
    /* ugly start line test */
    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_21/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    printf("send res: %d\n", res);
    ASSERT_GE(res, 0);

    /* hack the network read */
    fd = fopen(".test_data", "wb+");
    if (fd == NULL) {
        printf("fopen failed\r");
        ASSERT_FAIL();
    }
    fwrite(test_data_ugly_start_line, 1, sizeof(test_data_ugly_start_line), fd);
    fclose(fd);
    fd = fopen(".test_data", "rb");

    res = aiot_http_recv(data->http_handle);
    printf("recv res: %d\n", res);
    ASSERT_EQ(res, STATE_SUCCESS);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));

    fclose(fd);
    res = system("rm -rf .test_data");

    sleep(1);
    /* rsp status code 4xx */
    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_21/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    printf("send res: %d\n", res);
    ASSERT_GE(res, 0);

    /* hack the network read */
    fd = fopen(".test_data", "wb+");
    if (fd == NULL) {
        printf("fopen failed\r");
        ASSERT_FAIL();
    }
    fwrite(test_data_rsp_status_code_4xx, 1, sizeof(test_data_rsp_status_code_4xx), fd);
    fclose(fd);
    fd = fopen(".test_data", "rb");

    res = aiot_http_recv(data->http_handle);
    printf("recv res: %d\n", res);
    ASSERT_EQ(res, STATE_SUCCESS);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));

    fclose(fd);
    res = system("rm -rf .test_data");
}

CASEs(AIOT_HTTP, case_22_aiot_http_normal)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_22";
    data->device_secret = "qQemYa0sefddfVvLDxbxGIOdRQiE39oX";

    memset(&response, 0, sizeof(core_http_response_t));

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_HANDLER, (void *)core_aiot_http_recv_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, (void *)&response);

    res = aiot_http_auth(data->http_handle);
    printf("send res: -0x%04x\n", -res);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_22/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    ASSERT_GE(res, STATE_SUCCESS);
    printf("send res: %d\n", res);

    res = aiot_http_recv(data->http_handle);
    ASSERT_GE(res, STATE_SUCCESS);
    printf("recv res: %d, msg: %.*s\n", res, res, response.content);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));
}

CASEs(AIOT_HTTP, case_23_aiot_http_normal)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    uint8_t is_persistant_conn = 1;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_23";
    data->device_secret = "2CJoc8jHCTF80jvMzGSa3SG2zWvJ6Huw";

    memset(&response, 0, sizeof(core_http_response_t));

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_LONG_CONNECTION, &is_persistant_conn);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_HANDLER, (void *)core_aiot_http_recv_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, (void *)&response);

    res = aiot_http_auth(data->http_handle);
    printf("send res: -0x%04x\n", -res);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_23/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    ASSERT_GE(res, STATE_SUCCESS);
    printf("send res: %d\n", res);

    res = aiot_http_recv(data->http_handle);
    ASSERT_GE(res, STATE_SUCCESS);
    printf("recv res: %d, msg: %.*s\n", res, res, response.content);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));
}

CASEs(AIOT_HTTP, case_24_aiot_http_auth_multi_times)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    uint32_t body_buffer_len = 8;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_24";
    data->device_secret = "2R5KeXFT8uCNgTjH5m3MdIAIvXFsfbfJ";

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_BODY_BUFFER_LEN, (void *)&body_buffer_len);

    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(AIOT_HTTP, case_25_aiot_http_send_multi_times)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    uint32_t body_buffer_len = 8;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_25";
    data->device_secret = "T1s22gOKQWG6zaGlccLeumitH9hozE8P";

    memset(&response, 0, sizeof(core_http_response_t));

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_BODY_BUFFER_LEN, (void *)&body_buffer_len);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_HANDLER, (void *)core_aiot_http_recv_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, (void *)&response);

    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_25/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    ASSERT_GE(res, STATE_SUCCESS);

    res = aiot_http_recv(data->http_handle);
    ASSERT_GE(res, STATE_SUCCESS);
    printf("recv res: %d, msg: %.*s\n", res, res, response.content);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));
}

CASEs(AIOT_HTTP, case_26_aiot_http_send_token_error)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    uint32_t body_buffer_len = 8;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_26";
    data->device_secret = "xz18qnFy3fUzMY5hmrNgJMHHFjT7lc9z";

    memset(&response, 0, sizeof(core_http_response_t));

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_BODY_BUFFER_LEN, (void *)&body_buffer_len);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_HANDLER, (void *)core_aiot_http_recv_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_EVENT_HANDLER, (void *)core_aiot_http_event_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, (void *)&response);

    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    if (((core_http_handle_t *)(data->http_handle))->token != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(((core_http_handle_t *)(data->http_handle))->token);
        ((core_http_handle_t *)(data->http_handle))->token = "1234";
    }

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_26/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    ASSERT_GE(res, STATE_SUCCESS);

    res = aiot_http_recv(data->http_handle);
    ASSERT_GE(res, STATE_SUCCESS);
    printf("recv res: %d, msg: %.*s\n", res, res, response.content);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));

    ((core_http_handle_t *)(data->http_handle))->token = NULL;
}

CASEs(AIOT_HTTP, case_27_aiot_http_setopt_AIOT_HTTPOPT_AUTH_TIMEOUT_MS)
{
    int32_t res = STATE_SUCCESS;
    uint32_t auth_timeout_ms = 8;

    res = aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_AUTH_TIMEOUT_MS, (void *)&auth_timeout_ms);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(AIOT_HTTP, case_28_aiot_http_setopt_AIOT_HTTPOPT_LONG_CONNECTION)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    uint8_t long_connection = 0;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_28";
    data->device_secret = "1f2gedES8eZVLi5GUEzQ0nPuilO9m8nN";

    memset(&response, 0, sizeof(core_http_response_t));

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_LONG_CONNECTION, (void *)&long_connection);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_HANDLER, (void *)core_aiot_http_recv_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_EVENT_HANDLER, (void *)core_aiot_http_event_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, (void *)&response);

    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_28/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    ASSERT_GE(res, STATE_SUCCESS);

    res = aiot_http_recv(data->http_handle);
    ASSERT_GE(res, STATE_SUCCESS);
    printf("recv res: %d, msg: %.*s\n", res, res, response.content);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));

    res = aiot_http_send(data->http_handle, "/a18wPzZJzNG/aiot_http_test_case_28/user/update", (uint8_t *)"hello world",
                         strlen("hello world"));
    ASSERT_GE(res, STATE_SUCCESS);

    res = aiot_http_recv(data->http_handle);
    ASSERT_GE(res, STATE_SUCCESS);
    printf("recv res: %d, msg: %.*s\n", res, res, response.content);

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));
}

int32_t case_29_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms, core_sysdep_addr_t *addr)
{
    int32_t res = STATE_SUCCESS;
    static int count = 0;

    if (count == 0) {
        char header[] = {'\r'};
        count = 1;
        memcpy(buffer, header, sizeof(header)/sizeof(char));
        res = sizeof(header)/sizeof(char);
    } else if (count == 1) {
        char header[] = {'a'};
        count = 2;
        memcpy(buffer, header, sizeof(header)/sizeof(char));
        res = sizeof(header)/sizeof(char);
        count = 0;
    }

    return res;
}

CASEs(AIOT_HTTP, case_29_aiot_http_auth_header_invalid)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    uint8_t long_connection = 0;
    data->product_key = "a18wPzZJzNG";
    data->device_name = "aiot_http_test_case_29";
    data->device_secret = "lQHyWikbvUVjwhntXzDWhdMRalpbWnUw";

    memset(&response, 0, sizeof(core_http_response_t));

    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_HOST, (void *)host);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_PRODUCT_KEY, data->product_key);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_NAME, data->device_name);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_DEVICE_SECRET, data->device_secret);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_LONG_CONNECTION, (void *)&long_connection);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_RECV_HANDLER, (void *)core_aiot_http_recv_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_EVENT_HANDLER, (void *)core_aiot_http_event_handler);
    aiot_http_setopt(data->http_handle, AIOT_HTTPOPT_USERDATA, (void *)&response);

    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_recv = case_29_core_sysdep_network_recv;

    res = aiot_http_auth(data->http_handle);
    ASSERT_EQ(res, STATE_HTTP_AUTH_CODE_FAILED);

    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
    memset(&response, 0, sizeof(core_http_response_t));
}

SUITE(AIOT_HTTP) = {
    ADD_CASE(AIOT_HTTP, case_01_aiot_http_init_without_portfile),
    ADD_CASE(AIOT_HTTP, case_02_aiot_http_init_with_portfile),
    ADD_CASE(AIOT_HTTP, case_03_aiot_http_setopt_null_handle),
    ADD_CASE(AIOT_HTTP, case_04_aiot_http_setopt_invalid_params),
    ADD_CASE(AIOT_HTTP, case_05_aiot_http_setopt_set_cred),
    ADD_CASE(AIOT_HTTP, case_06_aiot_http_setopt_set_numeric_data),
    ADD_CASE(AIOT_HTTP, case_07_aiot_http_setopt_set_string_data_twice),
    ADD_CASE(AIOT_HTTP, case_08_aiot_http_setopt_set_event_handler),
    ADD_CASE(AIOT_HTTP, case_09_aiot_http_auth_null_handle),
    ADD_CASE(AIOT_HTTP, case_10_aiot_http_auth_missing_mandatory_option),
    ADD_CASE(AIOT_HTTP, case_11_aiot_http_auth_with_invalid_device_secret),
    ADD_CASE(AIOT_HTTP, case_12_aiot_http_send_null_handle),
    ADD_CASE(AIOT_HTTP, case_13_aiot_http_send_invalid_params),
    ADD_CASE(AIOT_HTTP, case_14_aiot_http_send_without_auth),
    ADD_CASE(AIOT_HTTP, case_15_aiot_http_send_invalid_path),
    ADD_CASE(AIOT_HTTP, case_16_aiot_http_send_without_network),
    ADD_CASE(AIOT_HTTP, case_17_aiot_http_send_normal),
    ADD_CASE(AIOT_HTTP, case_18_aiot_http_recv_null_handle),
    ADD_CASE(AIOT_HTTP, case_19_aiot_http_recv_invalid_params),
    ADD_CASE(AIOT_HTTP, case_20_aiot_http_recv_without_network),
    ADD_CASE(AIOT_HTTP, case_21_aiot_http_recv_rsp_exception),
    ADD_CASE(AIOT_HTTP, case_22_aiot_http_normal),
    ADD_CASE(AIOT_HTTP, case_23_aiot_http_normal),
    ADD_CASE(AIOT_HTTP, case_24_aiot_http_auth_multi_times),
    ADD_CASE(AIOT_HTTP, case_25_aiot_http_send_multi_times),
    ADD_CASE(AIOT_HTTP, case_26_aiot_http_send_token_error),
    ADD_CASE(AIOT_HTTP, case_27_aiot_http_setopt_AIOT_HTTPOPT_AUTH_TIMEOUT_MS),
    ADD_CASE(AIOT_HTTP, case_28_aiot_http_setopt_AIOT_HTTPOPT_LONG_CONNECTION),
    ADD_CASE(AIOT_HTTP, case_29_aiot_http_auth_header_invalid),
    ADD_CASE_NULL
};


