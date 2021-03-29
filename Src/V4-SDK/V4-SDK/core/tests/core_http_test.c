#include "stdio.h"
#include "cu_test.h"
#include "core_http.h"
#include "aiot_http_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_state_api.h"

extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
extern void *core_sysdep_network_init(void);
extern int32_t core_sysdep_network_establish(void *handle);
extern int32_t core_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data);
extern int32_t core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                        core_sysdep_addr_t *addr);
extern int32_t core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr);

static int32_t aiot_http_test_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

void *_test_sysdep_network_init(void)
{
    return NULL;
}

int32_t _test_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data)
{
    return -1;
}

static int32_t test_sysdep_network_establish()
{
    return -1;
}

int32_t _test_sysdep_network_read(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                  core_sysdep_addr_t *addr)
{
    return -1;
}

int32_t _test_sysdep_network_write(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                   core_sysdep_addr_t *addr)
{
    return -1;
}

void core_http_recv_handler(void *handle, const aiot_http_recv_t *packet, void *userdata)
{
    core_http_handle_t *http_handle = (core_http_handle_t *)handle;
    core_http_response_t *response = (core_http_response_t *)userdata;

    switch (packet->type) {
        case AIOT_HTTPRECV_STATUS_CODE: {
            response->code = packet->data.status_code.code;
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

DATA(CORE_HTTP)
{
    void *http_handle;
};

SETUP(CORE_HTTP)
{
    uint16_t port = 80;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    aiot_state_set_logcb(aiot_http_test_logcb);

    data->http_handle = core_http_init();
    core_http_setopt(data->http_handle, CORE_HTTPOPT_PORT, &port);
}

TEARDOWN(CORE_HTTP)
{
    core_http_deinit(&data->http_handle);

    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    g_aiot_sysdep_portfile.core_sysdep_network_establish = core_sysdep_network_establish;
    g_aiot_sysdep_portfile.core_sysdep_network_send = core_sysdep_network_send;
    g_aiot_sysdep_portfile.core_sysdep_network_recv = core_sysdep_network_recv;
}

CASE(CORE_HTTP, case_01_core_http_init_without_portfile)
{
    aiot_sysdep_set_portfile(NULL);

    void *http_handle = core_http_init();
    ASSERT_NULL(http_handle);
}

CASE(CORE_HTTP, case_02_core_http_init_with_portfile)
{
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);

    void *http_handle = core_http_init();
    ASSERT_NOT_NULL(http_handle);

    core_http_deinit(&http_handle);
}

CASE(CORE_HTTP, case_03_core_http_setopt_null_handle)
{
    void *http_handle = NULL;
    int32_t res;

    res = core_http_setopt(http_handle, CORE_HTTPOPT_HOST, "www.aiot.com");
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASEs(CORE_HTTP, case_04_core_http_setopt_error_option)
{
    int32_t res;
    int32_t opt_data = 0;

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_MAX, &opt_data);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);
}

CASEs(CORE_HTTP, case_05_core_http_setopt_null_data)
{
    int32_t res = core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASEs(CORE_HTTP, case_06_core_http_setopt_normal_set_cred)
{
    int32_t res;
    aiot_sysdep_network_cred_t cred = {
        .option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_PSK,
    };

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_NETWORK_CRED, &cred);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_NETWORK_CRED, &cred);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(CORE_HTTP, case_07_core_http_setopt_normal_set_host)
{
    int32_t res;
    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST, "www.aiot.com");
    ASSERT_EQ(res, STATE_SUCCESS);

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST, "www.aiot.com");
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(CORE_HTTP, case_08_core_http_setopt_normal_set_numeric_data)
{
    int32_t res;
    uint16_t port = 80;
    uint32_t timeout = 5000;

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_PORT, &port);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_SEND_TIMEOUT_MS, &timeout);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_RECV_TIMEOUT_MS, &timeout);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(CORE_HTTP, case_09_core_http_setopt_normal_set_event_handler)
{
    int32_t res;
    uint32_t user_data = 1;

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_USERDATA, &user_data);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_RECV_HANDLER, core_http_recv_handler);
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASE(CORE_HTTP, case_10_core_http_send_null_handle)
{
    void *http_handle = NULL;
    core_http_request_t request = {
        .method = "POST",
        .path = "path",
    };
    int32_t res;

    res = core_http_send(http_handle, &request);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASEs(CORE_HTTP, case_11_core_http_send_invalid_params)
{
    core_http_request_t request = {
        .method = NULL,
        .path = NULL,
        .header = NULL,
        .content = NULL,
        .content_len = 0,
    };
    int32_t res;

    res = core_http_send(data->http_handle, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    request.method = "POST";
    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    request.method = NULL;
    request.path = "/auth";
    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASEs(CORE_HTTP, case_12_core_http_send_missing_option)
{
    core_http_request_t request = {
        .method = "POST",
        .path = "/auth",
        .header = NULL,
        .content = NULL,
        .content_len = 0,
    };
    int32_t res;

    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, STATE_USER_INPUT_MISSING_HOST);
}

CASEs(CORE_HTTP, case_13_core_http_send_while_network_est_failed)
{
    core_http_request_t request = {
        .method = "POST",
        .path = "/auth",
        .header = NULL,
        .content = NULL,
        .content_len = 0,
    };
    int32_t res;

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST,
                           "iot-as-http.cn-shanghai.aliyuncs.com");
    ASSERT_EQ(res, STATE_SUCCESS);

    /* hack the network est */
    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_establish = test_sysdep_network_establish;

    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);
}

CASEs(CORE_HTTP, case_14_core_http_send_normal_content_len_0byte)
{
    int32_t res = STATE_SUCCESS;
    core_http_response_t response;
    core_http_request_t request = {
        .method = "POST",
        .path = "/auth",
        .header = NULL,
        .content = NULL,
        .content_len = 0,
    };

    memset(&response, 0, sizeof(core_http_response_t));

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST,
                           "iot-as-http.cn-shanghai.aliyuncs.com");
    ASSERT_EQ(res, STATE_SUCCESS);

    core_http_connect(data->http_handle);

    core_http_setopt(data->http_handle, CORE_HTTPOPT_RECV_HANDLER, (void *)core_http_recv_handler);
    core_http_setopt(data->http_handle, CORE_HTTPOPT_USERDATA, (void *)&response);

    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, 0);

    res = core_http_recv(data->http_handle);
    printf("recv content: %.*s\n", response.content_len, response.content);
    ASSERT_EQ(res, 38);     /* cloud return fixed 38 bytes */

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
}

CASEs(CORE_HTTP, case_15_core_http_send_normal_content_len_nbytes)
{
    int32_t res = STATE_SUCCESS;
    core_http_response_t response;
    uint8_t content[] = "{\"version\": \"1.0.0\"}";
    core_http_request_t request = {
        .method = "POST",
        .path = "/auth",
        .header = NULL,
        .content = content,
        .content_len = sizeof(content),
    };

    memset(&response, 0, sizeof(core_http_response_t));

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST,
                           "iot-as-http.cn-shanghai.aliyuncs.com");
    ASSERT_EQ(res, STATE_SUCCESS);

    core_http_connect(data->http_handle);

    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, sizeof(content));

    res = core_http_recv(data->http_handle);
    printf("recv content: %.*s\n", response.content_len, response.content);
    ASSERT_EQ(res, 38);     /* cloud return fixed 38 bytes */

    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
    }
}

CASEs(CORE_HTTP, case_16_core_http_send_normal_several_times)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_response_t response;
    uint8_t content[] = "{\"version\": \"1.0.0\"}";
    core_http_request_t request = {
        .method = "POST",
        .path = "/auth",
        .header = NULL,
        .content = content,
        .content_len = sizeof(content),
    };

    memset(&response, 0, sizeof(core_http_response_t));

    core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST, (void *)host);
    core_http_setopt(data->http_handle, CORE_HTTPOPT_RECV_HANDLER, (void *)core_http_recv_handler);
    core_http_setopt(data->http_handle, CORE_HTTPOPT_USERDATA, (void *)&response);

    res = core_http_connect(data->http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, sizeof(content));

    res = core_http_recv(data->http_handle);
    printf("recv content: %.*s\n", response.content_len, response.content);
    ASSERT_EQ(res, 38);     /* cloud return fixed 38 bytes */
    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
        response.content = NULL;
    }
    memset(&response, 0, sizeof(core_http_response_t));

    printf("send second time\n");
    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, sizeof(content));

    memset(content, 0, sizeof(content));
    res = core_http_recv(data->http_handle);
    printf("recv content: %.*s\n", response.content_len, response.content);
    ASSERT_EQ(res, 38);     /* cloud return fixed 38 bytes */
    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
        response.content = NULL;
    }
    memset(&response, 0, sizeof(core_http_response_t));
}

CASE(CORE_HTTP, case_17_core_http_recv_null_handle)
{
    void *http_handle = NULL;
    int32_t res;

    res = core_http_recv(http_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASEs(CORE_HTTP, case_18_core_http_recv_invalid_params)
{
    void *http_handle = NULL;
    int32_t res;

    res = core_http_recv(http_handle);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = core_http_recv(data->http_handle);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_CLOSED);
}

CASEs(CORE_HTTP, case_19_core_http_recv_serveral_times)
{
    int32_t res = STATE_SUCCESS;
    core_http_response_t response;
    uint8_t content[] = "{\"version\": \"1.0.0\"}";
    core_http_request_t request = {
        .method = "POST",
        .path = "/auth",
        .header = NULL,
        .content = content,
        .content_len = sizeof(content),
    };

    memset(&response, 0, sizeof(core_http_response_t));

    core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST,
                     "iot-as-http.cn-shanghai.aliyuncs.com");
    core_http_setopt(data->http_handle, CORE_HTTPOPT_USERDATA, &response);
    core_http_setopt(data->http_handle, CORE_HTTPOPT_RECV_HANDLER, core_http_recv_handler);

    res = core_http_connect(data->http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, sizeof(content));

    res = core_http_recv(data->http_handle);
    printf("res: %d, response.content_total_len: %d\n", res, response.content_total_len);
    ASSERT_EQ(response.content_total_len, 38);     /* cloud return fixed 38 bytes */
    printf("rsp content len = %d\n", response.content_total_len);
    printf("res = %d, recv content: %.*s\n", res, res, response.content);
    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
        response.content = NULL;
        memset(&response, 0, sizeof(core_http_response_t));
    }
}

CASEs(CORE_HTTP, case_20_core_http_recv_overtime)
{
    int32_t res = STATE_SUCCESS;
    core_http_response_t response;
    uint8_t content[] = "{\"version\": \"1.0.0\"}";
    core_http_request_t request = {
        .method = "POST",
        .path = "/auth",
        .header = NULL,
        .content = content,
        .content_len = sizeof(content),
    };
    uint32_t recv_timeout = 2000;

    memset(&response, 0, sizeof(core_http_response_t));

    core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST,
                     "iot-as-http.cn-shanghai.aliyuncs.com");
    core_http_setopt(data->http_handle, CORE_HTTPOPT_USERDATA, &response);
    core_http_setopt(data->http_handle, CORE_HTTPOPT_RECV_HANDLER, core_http_recv_handler);
    core_http_setopt(data->http_handle, CORE_HTTPOPT_RECV_TIMEOUT_MS, &recv_timeout);

    res = core_http_connect(data->http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, sizeof(content));

    res = core_http_recv(data->http_handle);
    ASSERT_EQ(response.content_total_len, 38);     /* cloud return fixed 38 bytes */
    printf("rsp content len = %d\n", response.content_total_len);
    printf("res = %d, recv content: %.*s\n", res, res, response.content);
    if (response.content != NULL) {
        ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_free(response.content);
        response.content = NULL;
        memset(&response, 0, sizeof(core_http_response_t));
    }
}

CASE(CORE_HTTP, case_21_core_http_deinit_invalid_params)
{
    void *handle = NULL;
    int32_t res;

    res = core_http_deinit(NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);

    res = core_http_deinit(&handle);
    ASSERT_EQ(res, STATE_USER_INPUT_NULL_POINTER);
}

CASE(CORE_HTTP, case_22_core_http_deinit_normal)
{
    int32_t res = STATE_SUCCESS;;
    void *http_handle = NULL;
    uint8_t content[] = "{\"version\": \"1.0.0\"}";
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    uint16_t port = 80;
    core_http_request_t request = {
        .method = "POST",
        .path = "/auth",
        .header = NULL,
        .content = content,
        .content_len = sizeof(content),
    };
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);

    http_handle = core_http_init();

    core_http_setopt(http_handle, CORE_HTTPOPT_HOST, (void *)host);
    core_http_setopt(http_handle, CORE_HTTPOPT_PORT, (void *)&port);

    res = core_http_connect(http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);


    res = core_http_send(http_handle, &request);
    ASSERT_EQ(res, sizeof(content));

    res = core_http_deinit(&http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);
}

int32_t case_23_1_core_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data)
{
    return STATE_PORT_NETWORK_UNKNOWN_OPTION;
}

int32_t case_23_2_core_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data)
{
    switch (option) {
        case CORE_SYSDEP_NETWORK_SOCKET_TYPE:
        case CORE_SYSDEP_NETWORK_HOST:
        case CORE_SYSDEP_NETWORK_PORT: {
            return STATE_SUCCESS;
        }
        break;
        case CORE_SYSDEP_NETWORK_CRED : {
            return STATE_PORT_NETWORK_UNKNOWN_OPTION;
        }
        break;
        default: {

        }
        break;
    }
    return STATE_PORT_NETWORK_UNKNOWN_OPTION;
}

int32_t case_23_core_sysdep_network_establish(void *handle)
{
    return STATE_PORT_NETWORK_CONNECT_FAILED;
}

CASEs(CORE_HTTP, case_23_core_http_connect_failed)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    aiot_sysdep_network_cred_t cred;

    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));

    core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST, (void *)host);
    core_http_setopt(data->http_handle, CORE_HTTPOPT_NETWORK_CRED, (void *)&cred);

    ((core_http_handle_t *)(data->http_handle))->network_handle = ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_init();
    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_setopt = case_23_1_core_sysdep_network_setopt;
    res = core_http_connect(data->http_handle);
    ASSERT_EQ(res, STATE_PORT_NETWORK_UNKNOWN_OPTION);

    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_setopt = case_23_2_core_sysdep_network_setopt;
    res = core_http_connect(data->http_handle);
    ASSERT_EQ(res, STATE_PORT_NETWORK_UNKNOWN_OPTION);

    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_setopt = core_sysdep_network_setopt;
    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_establish = case_23_core_sysdep_network_establish;
    res = core_http_connect(data->http_handle);
    ASSERT_EQ(res, STATE_PORT_NETWORK_CONNECT_FAILED);

    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_establish = core_sysdep_network_establish;
    res = core_http_connect(data->http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);
}

int32_t case_24_core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms, core_sysdep_addr_t *addr)
{
    return 0;
}

CASEs(CORE_HTTP, case_24_core_http_send_less_data)
{
    int32_t res = STATE_SUCCESS;
    char *host = "iot-as-http.cn-shanghai.aliyuncs.com";
    core_http_request_t request;

    memset(&request, 0, sizeof(core_http_request_t));
    request.path = "/";
    request.method = "GET";

    core_http_setopt(data->http_handle, CORE_HTTPOPT_HOST, (void *)host);

    res = core_http_connect(data->http_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ((core_http_handle_t *)(data->http_handle))->sysdep->core_sysdep_network_send = case_24_core_sysdep_network_send;

    res = core_http_send(data->http_handle, &request);
    ASSERT_EQ(res, STATE_SYS_DEPEND_NWK_WRITE_LESSDATA);
}

CASEs(CORE_HTTP, case_25_core_http_setopt_normal)
{
    int32_t res = STATE_SUCCESS;
    uint32_t deinit_timeout_ms = 5000, header_line_max_len = 256, body_buffer_max_len = 256;

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_DEINIT_TIMEOUT_MS, (void *)&deinit_timeout_ms);
    ASSERT_EQ(res, STATE_SUCCESS);
    ASSERT_EQ(((core_http_handle_t *)(data->http_handle))->deinit_timeout_ms, deinit_timeout_ms);

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_HEADER_LINE_MAX_LEN, (void *)&header_line_max_len);
    ASSERT_EQ(res, STATE_SUCCESS);
    ASSERT_EQ(((core_http_handle_t *)(data->http_handle))->header_line_max_len, header_line_max_len);

    res = core_http_setopt(data->http_handle, CORE_HTTPOPT_BODY_BUFFER_MAX_LEN, (void *)&body_buffer_max_len);
    ASSERT_EQ(res, STATE_SUCCESS);
    ASSERT_EQ(((core_http_handle_t *)(data->http_handle))->body_buffer_max_len, body_buffer_max_len);
}

SUITE(CORE_HTTP) = {
    ADD_CASE(CORE_HTTP, case_01_core_http_init_without_portfile),
    ADD_CASE(CORE_HTTP, case_02_core_http_init_with_portfile),
    ADD_CASE(CORE_HTTP, case_03_core_http_setopt_null_handle),
    ADD_CASE(CORE_HTTP, case_04_core_http_setopt_error_option),
    ADD_CASE(CORE_HTTP, case_05_core_http_setopt_null_data),
    ADD_CASE(CORE_HTTP, case_06_core_http_setopt_normal_set_cred),
    ADD_CASE(CORE_HTTP, case_07_core_http_setopt_normal_set_host),
    ADD_CASE(CORE_HTTP, case_08_core_http_setopt_normal_set_numeric_data),
    ADD_CASE(CORE_HTTP, case_09_core_http_setopt_normal_set_event_handler),
    ADD_CASE(CORE_HTTP, case_10_core_http_send_null_handle),
    ADD_CASE(CORE_HTTP, case_11_core_http_send_invalid_params),
    ADD_CASE(CORE_HTTP, case_12_core_http_send_missing_option),
    ADD_CASE(CORE_HTTP, case_13_core_http_send_while_network_est_failed),
    ADD_CASE(CORE_HTTP, case_14_core_http_send_normal_content_len_0byte),
    ADD_CASE(CORE_HTTP, case_15_core_http_send_normal_content_len_nbytes),
    ADD_CASE(CORE_HTTP, case_16_core_http_send_normal_several_times),
    ADD_CASE(CORE_HTTP, case_17_core_http_recv_null_handle),
    ADD_CASE(CORE_HTTP, case_18_core_http_recv_invalid_params),
    ADD_CASE(CORE_HTTP, case_19_core_http_recv_serveral_times),
    ADD_CASE(CORE_HTTP, case_20_core_http_recv_overtime),
    ADD_CASE(CORE_HTTP, case_21_core_http_deinit_invalid_params),
    ADD_CASE(CORE_HTTP, case_22_core_http_deinit_normal),
    ADD_CASE(CORE_HTTP, case_23_core_http_connect_failed),
    ADD_CASE(CORE_HTTP, case_24_core_http_send_less_data),
    ADD_CASE(CORE_HTTP, case_25_core_http_setopt_normal),
    ADD_CASE_NULL
};

