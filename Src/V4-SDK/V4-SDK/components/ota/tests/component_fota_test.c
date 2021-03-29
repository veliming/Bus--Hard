#include <unistd.h>
#include <pthread.h>
#include "cu_test.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "core_list.h"
#include "core_mqtt.h"
#include "aiot_ota_api.h"
#include "aiot_http_api.h"
#include "core_http.h"
#define RUN_DOWNLOAD_DEMO

int32_t core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                 core_sysdep_addr_t *addr);
static int32_t aiot_mqtt_test_logcb(int32_t code, char *message)
{
    if (STATE_HTTP_LOG_RECV_CONTENT != code) {
        printf("%s", message);
    }
    return 0;
}

CASE(COMPONENT_FOTA, case_01_aiot_ota_init_without_portfile)
{
    void *ota_handle = NULL;

    aiot_sysdep_set_portfile(NULL);
    ota_handle = aiot_ota_init();
    ASSERT_NULL(ota_handle);
}

CASE(COMPONENT_FOTA, case_02_aiot_ota_init_with_portfile)
{
    void *ota_handle = NULL;

    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    ota_handle = aiot_ota_init();
    ASSERT_NOT_NULL(ota_handle);
    aiot_ota_deinit(&ota_handle);
}

DATA(COMPONENT_FOTA)
{
    void *ota_handle;
    void *mqtt_handle;
    void *download_handle;
    char *host;
    uint16_t port;
    char *product_key;
    char *device_name;
    char *device_secret;
};

SETUP(COMPONENT_FOTA)
{
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    aiot_state_set_logcb(aiot_mqtt_test_logcb);

    data->ota_handle = aiot_ota_init();
    data->mqtt_handle = aiot_mqtt_init();
    data->download_handle = aiot_download_init();

    data->product_key = "a1Zd7n5yTt8";
    data->device_name = "aiot_sha256_v2";
    data->device_secret = "jSgDO7ouLFLj02hYM1mfNapB7YZbyfK5";
    data->host = "a1Zd7n5yTt8.iot-as-mqtt.cn-shanghai.aliyuncs.com";
    data->port = 443;
}

TEARDOWN(COMPONENT_FOTA)
{
    aiot_ota_deinit(&data->ota_handle);
}


CASEs(COMPONENT_FOTA, case_03_aiot_ota_setopt_mqtt_handle)
{
    int32_t res = STATE_SUCCESS;

    res = aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE, (void *)data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((ota_handle_t *)data->ota_handle)->mqtt_handle, data->mqtt_handle);
}

int case_16_main_loop_finished = 0;
int case_18_main_loop_finished = 0;
int case_04_main_loop_finished = 0;
int case_22_main_loop_finished = 0;
int case_31_main_loop_finished = 0;
int case_32_main_loop_finished = 0;
int case_35_main_loop_finished = 0;

CASEs(COMPONENT_FOTA, case_05_aiot_ota_report_version_success)
{

    int32_t res;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    res = aiot_mqtt_connect(data->mqtt_handle);

    char *cur_version = "1.0.0";
    res = aiot_ota_report_version(data->ota_handle,
                                  cur_version);             /* 上报当前固件版本号 */
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(COMPONENT_FOTA, case_06_aiot_ota_setopt_userdata)
{
    int32_t userdata = 100;
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_USER_DATA, (void *)&userdata);
    ASSERT_EQ(((ota_handle_t *)(data->ota_handle))->userdata, &userdata);
}

CASEs(COMPONENT_FOTA, case_07_aiot_download_setopt_recv_timeout_ms)
{
    uint32_t timeout = 100;
    aiot_download_setopt(data->download_handle, AIOT_DLOPT_RECV_TIMEOUT_MS, (void *)&timeout);
    ASSERT_EQ((((core_http_handle_t *)((download_handle_t *)(data->download_handle))->http_handle))->recv_timeout_ms,
              timeout);
}

CASEs(COMPONENT_FOTA, case_08_aiot_download_setopt_user_data)
{
    uint32_t userdata = 100;
    aiot_download_setopt(data->download_handle, AIOT_DLOPT_USER_DATA, (void *)&userdata);
    ASSERT_EQ((((download_handle_t *)(data->download_handle))->userdata), &userdata);
}

CASEs(COMPONENT_FOTA, case_10_aiot_download_setopt_range_start)
{
    uint32_t range_start = 100;
    aiot_download_setopt(data->download_handle, AIOT_DLOPT_RANGE_START, (void *)&range_start);
    ASSERT_EQ((((download_handle_t *)(data->download_handle))->range_start), range_start);
}

CASEs(COMPONENT_FOTA, case_11_aiot_download_setopt_range_end)
{
    uint32_t range_end = 100;
    aiot_download_setopt(data->download_handle, AIOT_DLOPT_RANGE_END, (void *)&range_end);
    ASSERT_EQ((((download_handle_t *)(data->download_handle))->range_end), range_end);
}

CASEs(COMPONENT_FOTA, case_12_aiot_download_setopt_network_port)
{
    uint32_t port = 100;
    aiot_download_setopt(data->download_handle, AIOT_DLOPT_NETWORK_PORT, (void *)&port);
    ASSERT_EQ((((core_http_handle_t *)((download_handle_t *)(data->download_handle))->http_handle))->port, port);
}


CASEs(COMPONENT_FOTA, case_13_aiot_ota_setopt_max_num)
{
    int32_t res = STATE_SUCCESS;

    res = aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MAX, (void *)data->mqtt_handle);
    printf("res is %d\r\n", res);
    ASSERT_EQ(res, STATE_USER_INPUT_UNKNOWN_OPTION);
}

CASEs(COMPONENT_FOTA, case_14_aiot_download_setopt_null_input)
{
    void *indata = NULL;
    int32_t res;
    res = aiot_download_setopt(data->download_handle, AIOT_DLOPT_RANGE_END, (void *)indata);
    ASSERT_EQ(res, STATE_DOWNLOAD_SETOPT_DATA_IS_NULL);
}

CASEs(COMPONENT_FOTA, case_15_aiot_download_setopt_max_range)
{
    int indata = 32;
    int32_t res;
    res = aiot_download_setopt(data->download_handle, AIOT_DLOPT_MAX, (void *)&indata);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);
}


#if 1
int case_16_ota_download_finished = 0;
static void *case_16_demo_ota_download_loop(void *dl_handle)
{
    download_handle_t *downloader = (download_handle_t *)dl_handle;
    downloader->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    printf("debug switch to legacy read, finish is %d\r\n", case_16_main_loop_finished);
    while (0 == case_16_main_loop_finished) {
        int32_t res = aiot_download_recv(dl_handle);
        if (res == STATE_DOWNLOAD_FINISHED) {
            case_16_ota_download_finished = 1;
            goto exit;
        }
    }
exit:

    if (NULL != dl_handle) {
        aiot_download_deinit(&dl_handle);
    }
    return NULL;
}


static int32_t case_16_2_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;
    printf("now count is %d\r\n", count);

    if (count == 0) {
        char pub[] = {0x30};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xea};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
#if 1
        char pub[] = {
            0x0,  0x2e, 0x2f, 0x6f, 0x74, 0x61, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2f, 0x75, 0x70, 0x67,
            0x72, 0x61, 0x64, 0x65, 0x2f, 0x61, 0x31, 0x5a, 0x64, 0x37, 0x6e, 0x35, 0x79, 0x54, 0x74, 0x38,
            0x2f, 0x61, 0x69, 0x6f, 0x74, 0x5f, 0x73, 0x68, 0x61, 0x32, 0x35, 0x36, 0x5f, 0x76, 0x32, 0x7b,
            0x22, 0x63, 0x6f, 0x64, 0x65, 0x22, 0x3a, 0x22, 0x31, 0x30, 0x30, 0x30, 0x22, 0x2c, 0x22, 0x64,
            0x61, 0x74, 0x61, 0x22, 0x3a, 0x7b, 0x22, 0x73, 0x69, 0x7a, 0x65, 0x22, 0x3a, 0x32, 0x30, 0x39,
            0x37, 0x31, 0x35, 0x32, 0x30, 0x2c, 0x22, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x3a, 0x22, 0x38, 0x66,
            0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65, 0x34, 0x31, 0x34, 0x66, 0x66, 0x39,
            0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63, 0x62, 0x61, 0x38, 0x63, 0x22, 0x2c,
            0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x22, 0x61, 0x70, 0x70, 0x2d, 0x31,
            0x2e, 0x30, 0x2e, 0x31, 0x2d, 0x32, 0x30, 0x31, 0x38, 0x30, 0x31, 0x30, 0x31, 0x2e, 0x31, 0x30,
            0x30, 0x30, 0x22, 0x2c, 0x22, 0x75, 0x72, 0x6c, 0x22, 0x3a, 0x22,  /* 0x75, 0x72, 0x6c 表示 url */
#if 1
            0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x6c, 0x69, 0x6e, 0x6b, 0x6b, 0x69, 0x74, 0x2d,
            0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x73, 0x73, 0x2d, 0x63, 0x6e, 0x2d, 0x73, 0x68,
            0x61, 0x6e, 0x67, 0x68, 0x61, 0x69, 0x2e, 0x61, 0x6c, 0x69, 0x79, 0x75, 0x6e, 0x63, 0x73, 0x2e,
            0x63, 0x6f, 0x6d, 0x2f, 0x6b, 0x75, 0x61, 0x6e, 0x6a, 0x75, 0x2f, 0x63, 0x6a, 0x6f, 0x36, 0x76,
            0x61, 0x64, 0x71, 0x31, 0x30, 0x30, 0x30, 0x64, 0x33, 0x67, 0x37, 0x34, 0x37, 0x6c, 0x6b, 0x61,
            0x39, 0x33, 0x36, 0x78, 0x2e, 0x62, 0x69, 0x6e, 0x22, 0x2c,
#endif
            0x22, 0x73, 0x69, 0x67, 0x6e, 0x4d,
            0x65, 0x74, 0x68, 0x6f, 0x64, 0x22, 0x3a, 0x22, 0x4d, 0x64, 0x35, 0x22, 0x2c, 0x22, 0x6d, 0x64,
            0x35, 0x22, 0x3a, 0x22, 0x38, 0x66, 0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65,
            0x34, 0x31, 0x34, 0x66, 0x66, 0x39, 0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63,
            0x62, 0x61, 0x38, 0x63, 0x22, 0x7d, 0x2c, 0x22, 0x69, 0x64, 0x22, 0x3a, 0x31, 0x35, 0x37, 0x37,
            0x32, 0x35, 0x33, 0x39, 0x30, 0x35, 0x32, 0x37, 0x33, 0x2c, 0x22, 0x6d, 0x65, 0x73, 0x73, 0x61,
            0x67, 0x65, 0x22, 0x3a, 0x22, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x22, 0x7d
        };
#endif
        printf("len of pub is %lu\r\n", sizeof(pub));
        printf("len of pub is %d\r\n", 0xed - 0x80 + 256);
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        count = 4;
        res = sizeof(pub) / sizeof(char);
    }
    return res;
}
void case_16_user_download_recv_handler(void *handle, int32_t percent, const aiot_download_recv_t *packet,
                                        void *userdata)
{
    switch (packet->type) {
        case AIOT_DLRECV_HTTPBODY: {

            if (percent < 0) {
                /* 收包异常或者digest校验错误 */
                printf("exception happend, percent is %d\r\n", percent);
                break;
            }

            if (percent == 100) {
            }
        }
        break;
        default:
            break;
    }
}

static pthread_t case_16_ota_thread;
static void case_16_user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            uint32_t max_buffer_len = 2048;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(case_16_user_download_recv_handler));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len));
            pthread_create(&case_16_ota_thread, NULL, case_16_demo_ota_download_loop, dl_handle);
            break;
        }
        case AIOT_OTARECV_COTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_FOTA, case_16_aiot_download_success_with_mock_data)
{
    int32_t res;
    uint32_t count = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    case_16_user_ota_recv_handler);           /* 设置ota_handle的recv_handler */
    res = aiot_mqtt_connect(data->mqtt_handle);

    case_16_main_loop_finished = 0;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_16_2_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    sleep(1);

    while (count < 10) {
        printf("looping in case 16, count is %d\r\n", count);
        aiot_mqtt_process(data->mqtt_handle);
        res = aiot_mqtt_recv(data->mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        count++;
        if (case_16_ota_download_finished) {
            break;
        }
    }
    printf("exit case 16, count is %d\r\n", count);
    case_16_main_loop_finished = 1;
    pthread_join(case_16_ota_thread, NULL);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    ASSERT_EQ(case_16_ota_download_finished, 1);
}
#endif

static int32_t case_17_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;
    printf("now count is %d\r\n", count);

    if (count == 0) {
        char pub[] = {0x30};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xea};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
#if 1
        char pub[] = {
            0x0,  0x2e, 0x2f, 0x6f, 0x74, 0x61, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2f, 0x75, 0x70, 0x67,
            0x72, 0x61, 0x64, 0x65, 0x2f, 0x61, 0x31, 0x5a, 0x64, 0x37, 0x6e, 0x35, 0x79, 0x54, 0x74, 0x38,
            0x2f, 0x61, 0x69, 0x6f, 0x74, 0x5f, 0x73, 0x68, 0x61, 0x32, 0x35, 0x36, 0x5f, 0x76, 0x32, 0x7b,
            0x22, 0x63, 0x6f, 0x64, 0x65, 0x22, 0x3a, 0x22, 0x31, 0x30, 0x30, 0x30, 0x22, 0x2c, 0x22, 0x64,
            0x61, 0x74, 0x61, 0x22, 0x3a, 0x7b, 0x22, 0x73, 0x69, 0x7a, 0x65, 0x22, 0x3a, 0x32, 0x30, 0x39,
            0x37, 0x31, 0x35, 0x32, 0x30, 0x2c, 0x22, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x3a, 0x22, 0x38, 0x66,
            0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65, 0x34, 0x31, 0x34, 0x66, 0x66, 0x39,
            0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63, 0x62, 0x61, 0x38, 0x63, 0x22, 0x2c,
            0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x22, 0x61, 0x70, 0x70, 0x2d, 0x31,
            0x2e, 0x30, 0x2e, 0x31, 0x2d, 0x32, 0x30, 0x31, 0x38, 0x30, 0x31, 0x30, 0x31, 0x2e, 0x31, 0x30,
            0x30, 0x30, 0x22, 0x2c, 0x22, 0x75, 0x72, 0x6c, 0x22, 0x3a, 0x22,
#if 1
            0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x6c, 0x69, 0x6e, 0x6b, 0x6b, 0x69, 0x74, 0x2d,
            0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x73, 0x73, 0x2d, 0x63, 0x6e, 0x2d, 0x73, 0x68,
            0x61, 0x6e, 0x67, 0x68, 0x61, 0x69, 0x2e, 0x61, 0x6c, 0x69, 0x79, 0x75, 0x6e, 0x63, 0x73, 0x2e,
            0x63, 0x6f, 0x6d, 0x2f, 0x6b, 0x75, 0x61, 0x6e, 0x6a, 0x75, 0x2f, 0x63, 0x6a, 0x6f, 0x36, 0x76,
            0x61, 0x64, 0x71, 0x31, 0x30, 0x30, 0x30, 0x64, 0x33, 0x67, 0x37, 0x34, 0x37, 0x6c, 0x6b, 0x61,
            0x39, 0x33, 0x36, 0x78, 0x2e, 0x62, 0x69, 0x6e, 0x22, 0x2c,
#endif
            0x22, 0x73, 0x69, 0x67, 0x6e, 0x4d,
            0x65, 0x74, 0x68, 0x6f, 0x64, 0x22, 0x3a, 0x22, 0x4d, 0x64, 0x31, 0x22, 0x2c, 0x22, 0x6d, 0x64,  //0x4d, 0x64, 0x35 means md5
            0x35, 0x22, 0x3a, 0x22, 0x38, 0x66, 0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65,
            0x34, 0x31, 0x34, 0x66, 0x66, 0x39, 0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63,
            0x62, 0x61, 0x38, 0x63, 0x22, 0x7d, 0x2c, 0x22, 0x69, 0x64, 0x22, 0x3a, 0x31, 0x35, 0x37, 0x37,
            0x32, 0x35, 0x33, 0x39, 0x30, 0x35, 0x32, 0x37, 0x33, 0x2c, 0x22, 0x6d, 0x65, 0x73, 0x73, 0x61,
            0x67, 0x65, 0x22, 0x3a, 0x22, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x22, 0x7d
        };
#endif
        printf("len of pub is %lu\r\n", sizeof(pub));
        printf("len of pub is %d\r\n", 0xed - 0x80 + 256);
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        count = 4;
        res = sizeof(pub) / sizeof(char);
    }
    return res;
}

static int case_17_ota_handle_triggered = 0;
static void case_17_user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;
            case_17_ota_handle_triggered = 1;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            break;
        }
        case AIOT_OTARECV_COTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_FOTA, case_17_aiot_download_with_maltformed_sign_method)
{
    int32_t res;
    uint32_t count = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    case_17_user_ota_recv_handler);           /* 设置ota_handle的recv_handler */
    res = aiot_mqtt_connect(data->mqtt_handle);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_17_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    sleep(1);

    while (count < 5) {
        aiot_mqtt_process(data->mqtt_handle);
        res = aiot_mqtt_recv(data->mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        count++;
    }
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    ASSERT_EQ(case_17_ota_handle_triggered, 0);
}

#if 1
static int case_18_ota_handle_triggered = 0;
static void *case_18_demo_ota_download_loop(void *dl_handle)
{
    download_handle_t *downloader = (download_handle_t *)dl_handle;
    downloader->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    printf("debug switch to legacy read, finish is %d\r\n", case_18_main_loop_finished);
    while (0 == case_18_main_loop_finished) {
        int32_t res = aiot_download_recv(dl_handle);
        if (res == STATE_DOWNLOAD_FINISHED) {
            case_18_ota_handle_triggered = 1;
            goto exit;
        }
    }
exit:

    printf("case 18 subloop exited\r\n");
    if (NULL != dl_handle) {
        aiot_download_deinit(&dl_handle);
    }
    return NULL;
}


static int32_t case_18_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;
    printf("now count is %d\r\n", count);

    if (count == 0) {
        char pub[] = {0x30};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xea};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
#if 1
        char pub[] = {
            0x0,  0x2e, 0x2f, 0x6f, 0x74, 0x61, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2f, 0x75, 0x70, 0x67,
            0x72, 0x61, 0x64, 0x65, 0x2f, 0x61, 0x31, 0x5a, 0x64, 0x37, 0x6e, 0x35, 0x79, 0x54, 0x74, 0x38,
            0x2f, 0x61, 0x69, 0x6f, 0x74, 0x5f, 0x73, 0x68, 0x61, 0x32, 0x35, 0x36, 0x5f, 0x76, 0x32, 0x7b,
            0x22, 0x63, 0x6f, 0x64, 0x65, 0x22, 0x3a, 0x22, 0x31, 0x30, 0x30, 0x30, 0x22, 0x2c, 0x22, 0x64,
            0x61, 0x74, 0x61, 0x22, 0x3a, 0x7b, 0x22, 0x73, 0x69, 0x7a, 0x65, 0x22, 0x3a, 0x32, 0x30, 0x39,
            0x37, 0x31, 0x35, 0x32, 0x30, 0x2c, 0x22, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x3a, 0x22, 0x38, 0x66,
            0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65, 0x34, 0x31, 0x34, 0x66, 0x66, 0x39,
            0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63, 0x62, 0x61, 0x38, 0x63, 0x22, 0x2c,
            0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x22, 0x61, 0x70, 0x70, 0x2d, 0x31,
            0x2e, 0x30, 0x2e, 0x31, 0x2d, 0x32, 0x30, 0x31, 0x38, 0x30, 0x31, 0x30, 0x31, 0x2e, 0x31, 0x30,
            0x30, 0x30, 0x22, 0x2c, 0x22, 0x75, 0x72, 0x6c, 0x22, 0x3a, 0x22,
#if 1
            0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x6c, 0x69, 0x6e, 0x6b, 0x6b, 0x69, 0x74, 0x2d,
            0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x73, 0x79, 0x2d, 0x63, 0x6e, 0x2d, 0x73, 0x68, /* 0x6f, 0x73, 0x73 表OSS */
            0x61, 0x6e, 0x67, 0x68, 0x61, 0x69, 0x2e, 0x61, 0x6c, 0x69, 0x79, 0x75, 0x6e, 0x63, 0x73, 0x2e,
            0x63, 0x6f, 0x6d, 0x2f, 0x6b, 0x75, 0x61, 0x6e, 0x6a, 0x75, 0x2f, 0x63, 0x6a, 0x6f, 0x36, 0x76,
            0x61, 0x64, 0x71, 0x31, 0x30, 0x30, 0x30, 0x64, 0x33, 0x67, 0x37, 0x34, 0x37, 0x6c, 0x6b, 0x61,
            0x39, 0x33, 0x36, 0x78, 0x2e, 0x62, 0x69, 0x6e, 0x22, 0x2c,
#endif
            0x22, 0x73, 0x69, 0x67, 0x6e, 0x4d,
            0x65, 0x74, 0x68, 0x6f, 0x64, 0x22, 0x3a, 0x22, 0x4d, 0x64, 0x35, 0x22, 0x2c, 0x22, 0x6d, 0x64,
            0x35, 0x22, 0x3a, 0x22, 0x38, 0x66, 0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65,
            0x34, 0x31, 0x34, 0x66, 0x66, 0x39, 0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63,
            0x62, 0x61, 0x38, 0x63, 0x22, 0x7d, 0x2c, 0x22, 0x69, 0x64, 0x22, 0x3a, 0x31, 0x35, 0x37, 0x37,
            0x32, 0x35, 0x33, 0x39, 0x30, 0x35, 0x32, 0x37, 0x33, 0x2c, 0x22, 0x6d, 0x65, 0x73, 0x73, 0x61,
            0x67, 0x65, 0x22, 0x3a, 0x22, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x22, 0x7d
        };
#endif
        printf("len of pub is %lu\r\n", sizeof(pub));
        printf("len of pub is %d\r\n", 0xed - 0x80 + 256);
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        count = 4;
        res = sizeof(pub) / sizeof(char);
    }
    return res;
}
static pthread_t case_18_ota_thread;
static void case_18_user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            pthread_create(&case_18_ota_thread, NULL, case_18_demo_ota_download_loop, dl_handle);
            break;
        }
        case AIOT_OTARECV_COTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_FOTA, case_18_aiot_download_with_malformed_url)
{
    int32_t res;
    uint32_t count = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    case_18_user_ota_recv_handler);           /* 设置ota_handle的recv_handler */
    res = aiot_mqtt_connect(data->mqtt_handle);

    case_18_main_loop_finished = 0;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_18_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    sleep(1);

    while (count < 5) {
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
        }
        sleep(1);
        count++;
    }
    printf("case 18 main exited\r\n");
    case_18_main_loop_finished = 1;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    pthread_join(case_18_ota_thread, NULL);
    ASSERT_EQ(case_18_ota_handle_triggered, 0);
}
#endif

CASEs(COMPONENT_FOTA, case_19_aiot_download_with_cred)
{
    extern const char *ali_ca_crt;
    aiot_sysdep_network_cred_t cred;
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_RSA;
    cred.max_tls_fragment = 16384;
    cred.x509_server_cert = ali_ca_crt;
    cred.x509_server_cert_len = strlen(ali_ca_crt);
    aiot_download_setopt(data->download_handle, AIOT_DLOPT_NETWORK_CRED, (void *)(&cred));
    ASSERT_NOT_NULL((((core_http_handle_t *)((download_handle_t *)(data->download_handle))->http_handle))->cred);
}


int case_04_ota_download_finished = 0;
static void *case_04_demo_ota_download_loop(void *dl_handle)
{
    download_handle_t *downloader = (download_handle_t *)dl_handle;
    downloader->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    printf("debug switch to legacy read, finish is %d\r\n", case_04_main_loop_finished);
    while (0 == case_04_main_loop_finished) {
        int32_t res = aiot_download_recv(dl_handle);
        case_04_ota_download_finished = 1;
        if (res == STATE_DOWNLOAD_FINISHED) {
            goto exit;
        }
    }
exit:
    if (NULL != dl_handle) {
        aiot_download_deinit(&dl_handle);
    }
    return NULL;
}


static int32_t case_04_2_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;
    printf("now count is %d\r\n", count);

    if (count == 0) {
        char pub[] = {0x30};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xea};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
#if 1
        char pub[] = {
            0x0,  0x2e, 0x2f, 0x6f, 0x74, 0x61, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2f, 0x75, 0x70, 0x67,
            0x72, 0x61, 0x64, 0x65, 0x2f, 0x61, 0x31, 0x5a, 0x64, 0x37, 0x6e, 0x35, 0x79, 0x54, 0x74, 0x38,
            0x2f, 0x61, 0x69, 0x6f, 0x74, 0x5f, 0x73, 0x68, 0x61, 0x32, 0x35, 0x36, 0x5f, 0x76, 0x32, 0x7b,
            0x22, 0x63, 0x6f, 0x64, 0x65, 0x22, 0x3a, 0x22, 0x31, 0x30, 0x30, 0x30, 0x22, 0x2c, 0x22, 0x64,
            0x61, 0x74, 0x61, 0x22, 0x3a, 0x7b, 0x22, 0x73, 0x69, 0x7a, 0x65, 0x22, 0x3a, 0x32, 0x30, 0x39,
            0x37, 0x31, 0x35, 0x32, 0x30, 0x2c, 0x22, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x3a, 0x22, 0x38, 0x66,
            0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65, 0x34, 0x31, 0x34, 0x66, 0x66, 0x39,
            0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63, 0x62, 0x61, 0x38, 0x63, 0x22, 0x2c,
            0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x22, 0x61, 0x70, 0x70, 0x2d, 0x31,
            0x2e, 0x30, 0x2e, 0x31, 0x2d, 0x32, 0x30, 0x31, 0x38, 0x30, 0x31, 0x30, 0x31, 0x2e, 0x31, 0x30,
            0x30, 0x30, 0x22, 0x2c, 0x22, 0x75, 0x72, 0x6d, 0x22, 0x3a, 0x22,  /* 0x75, 0x72, 0x6c 表示 url */
#if 1
            0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x6c, 0x69, 0x6e, 0x6b, 0x6b, 0x69, 0x74, 0x2d,
            0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x73, 0x73, 0x2d, 0x63, 0x6e, 0x2d, 0x73, 0x68,
            0x61, 0x6e, 0x67, 0x68, 0x61, 0x69, 0x2e, 0x61, 0x6c, 0x69, 0x79, 0x75, 0x6e, 0x63, 0x73, 0x2e,
            0x63, 0x6f, 0x6d, 0x2f, 0x6b, 0x75, 0x61, 0x6e, 0x6a, 0x75, 0x2f, 0x63, 0x6a, 0x6f, 0x36, 0x76,
            0x61, 0x64, 0x71, 0x31, 0x30, 0x30, 0x30, 0x64, 0x33, 0x67, 0x37, 0x34, 0x37, 0x6c, 0x6b, 0x61,
            0x39, 0x33, 0x36, 0x78, 0x2e, 0x62, 0x69, 0x6e, 0x22, 0x2c,
#endif
            0x22, 0x73, 0x69, 0x67, 0x6e, 0x4d,
            0x65, 0x74, 0x68, 0x6f, 0x64, 0x22, 0x3a, 0x22, 0x4d, 0x64, 0x35, 0x22, 0x2c, 0x22, 0x6d, 0x64,
            0x35, 0x22, 0x3a, 0x22, 0x38, 0x66, 0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65,
            0x34, 0x31, 0x34, 0x66, 0x66, 0x39, 0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63,
            0x62, 0x61, 0x38, 0x63, 0x22, 0x7d, 0x2c, 0x22, 0x69, 0x64, 0x22, 0x3a, 0x31, 0x35, 0x37, 0x37,
            0x32, 0x35, 0x33, 0x39, 0x30, 0x35, 0x32, 0x37, 0x33, 0x2c, 0x22, 0x6d, 0x65, 0x73, 0x73, 0x61,
            0x67, 0x65, 0x22, 0x3a, 0x22, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x22, 0x7d
        };
#endif
        printf("len of pub is %lu\r\n", sizeof(pub));
        printf("len of pub is %d\r\n", 0xed - 0x80 + 256);
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        count = 4;
        res = sizeof(pub) / sizeof(char);
    }
    return res;
}
static pthread_t case_04_ota_thread;
static void case_04_user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            pthread_create(&case_04_ota_thread, NULL, case_04_demo_ota_download_loop, dl_handle);
            break;
        }
        case AIOT_OTARECV_COTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_FOTA, case_04_aiot_download_with_maltform_url)
{
    int32_t res;
    uint32_t count = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    case_04_user_ota_recv_handler);           /* 设置ota_handle的recv_handler */
    res = aiot_mqtt_connect(data->mqtt_handle);

    case_04_main_loop_finished = 0;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_04_2_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    sleep(1);

    while (count < 5) {
        aiot_mqtt_process(data->mqtt_handle);
        res = aiot_mqtt_recv(data->mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        count++;
    }
    case_04_main_loop_finished = 1;
    // pthread_join(case_04_ota_thread, NULL);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    ASSERT_EQ(case_04_ota_download_finished, 0);
}

uint32_t case_20_ota_handle_triggered = 0;
static int32_t case_20_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;
    printf("now count is %d\r\n", count);

    if (count == 0) {
        char pub[] = {0x30};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xea};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
#if 1
        char pub[] = {
            0x0,  0x2e, 0x2f, 0x6f, 0x74, 0x61, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2f, 0x75, 0x70, 0x67,
            0x72, 0x61, 0x64, 0x65, 0x2f, 0x61, 0x31, 0x5a, 0x64, 0x37, 0x6e, 0x35, 0x79, 0x54, 0x74, 0x38,
            0x2f, 0x61, 0x69, 0x6f, 0x74, 0x5f, 0x73, 0x68, 0x61, 0x32, 0x35, 0x36, 0x5f, 0x76, 0x32, 0x7b,
            0x22, 0x63, 0x6f, 0x64, 0x65, 0x22, 0x3a, 0x22, 0x31, 0x30, 0x30, 0x30, 0x22, 0x2c, 0x22, 0x64,
            0x61, 0x74, 0x69, 0x22, 0x3a, 0x7b, 0x22, 0x73, 0x69, 0x7a, 0x65, 0x22, 0x3a, 0x32, 0x30, 0x39, /*61, 74, 61表示data */
            0x37, 0x31, 0x35, 0x32, 0x30, 0x2c, 0x22, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x3a, 0x22, 0x38, 0x66,
            0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65, 0x34, 0x31, 0x34, 0x66, 0x66, 0x39,
            0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63, 0x62, 0x61, 0x38, 0x63, 0x22, 0x2c,
            0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x22, 0x61, 0x70, 0x70, 0x2d, 0x31,
            0x2e, 0x30, 0x2e, 0x31, 0x2d, 0x32, 0x30, 0x31, 0x38, 0x30, 0x31, 0x30, 0x31, 0x2e, 0x31, 0x30,
            0x30, 0x30, 0x22, 0x2c, 0x22, 0x75, 0x72, 0x6c, 0x22, 0x3a, 0x22,
#if 1
            0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x6c, 0x69, 0x6e, 0x6b, 0x6b, 0x69, 0x74, 0x2d,
            0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x73, 0x79, 0x2d, 0x63, 0x6e, 0x2d, 0x73, 0x68, /* 0x6f, 0x73, 0x73 表OSS */
            0x61, 0x6e, 0x67, 0x68, 0x61, 0x69, 0x2e, 0x61, 0x6c, 0x69, 0x79, 0x75, 0x6e, 0x63, 0x73, 0x2e,
            0x63, 0x6f, 0x6d, 0x2f, 0x6b, 0x75, 0x61, 0x6e, 0x6a, 0x75, 0x2f, 0x63, 0x6a, 0x6f, 0x36, 0x76,
            0x61, 0x64, 0x71, 0x31, 0x30, 0x30, 0x30, 0x64, 0x33, 0x67, 0x37, 0x34, 0x37, 0x6c, 0x6b, 0x61,
            0x39, 0x33, 0x36, 0x78, 0x2e, 0x62, 0x69, 0x6e, 0x22, 0x2c,
#endif
            0x22, 0x73, 0x69, 0x67, 0x6e, 0x4d,
            0x65, 0x74, 0x68, 0x6f, 0x64, 0x22, 0x3a, 0x22, 0x4d, 0x64, 0x35, 0x22, 0x2c, 0x22, 0x6d, 0x64,
            0x35, 0x22, 0x3a, 0x22, 0x38, 0x66, 0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65,
            0x34, 0x31, 0x34, 0x66, 0x66, 0x39, 0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63,
            0x62, 0x61, 0x38, 0x63, 0x22, 0x7d, 0x2c, 0x22, 0x69, 0x64, 0x22, 0x3a, 0x31, 0x35, 0x37, 0x37,
            0x32, 0x35, 0x33, 0x39, 0x30, 0x35, 0x32, 0x37, 0x33, 0x2c, 0x22, 0x6d, 0x65, 0x73, 0x73, 0x61,
            0x67, 0x65, 0x22, 0x3a, 0x22, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x22, 0x7d
        };
#endif
        printf("len of pub is %lu\r\n", sizeof(pub));
        printf("len of pub is %d\r\n", 0xed - 0x80 + 256);
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        count = 4;
        res = sizeof(pub) / sizeof(char);
    }
    return res;
}
static void case_20_user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            case_20_ota_handle_triggered = 1;
            break;
        }
        case AIOT_OTARECV_COTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_FOTA, case_20_aiot_ota_maltformed_json_without_data_keyword)
{
    int32_t res;
    uint32_t count = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    case_20_user_ota_recv_handler);           /* 设置ota_handle的recv_handler */
    res = aiot_mqtt_connect(data->mqtt_handle);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_20_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);

    while (count < 5) {
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
        }
        sleep(1);
        count++;
    }
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    ASSERT_EQ(case_20_ota_handle_triggered, 0);
}


int case_21_ota_download_finished = 0;

static int32_t case_21_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;
    printf("now count is %d\r\n", count);

    if (count == 0) {
        char pub[] = {0x30};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xea};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
#if 1
        char pub[] = {
            0x0,  0x2e, 0x2f, 0x6f, 0x74, 0x61, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2f, 0x75, 0x70, 0x67,
            0x72, 0x61, 0x64, 0x65, 0x2f, 0x61, 0x31, 0x5a, 0x64, 0x37, 0x6e, 0x35, 0x79, 0x54, 0x74, 0x38,
            0x2f, 0x61, 0x69, 0x6f, 0x74, 0x5f, 0x73, 0x68, 0x61, 0x32, 0x35, 0x36, 0x5f, 0x76, 0x32, 0x7b,
            0x22, 0x63, 0x6f, 0x64, 0x65, 0x22, 0x3a, 0x22, 0x31, 0x30, 0x30, 0x30, 0x22, 0x2c, 0x22, 0x64,
            0x61, 0x74, 0x61, 0x22, 0x3a, 0x7b, 0x22, 0x73, 0x69, 0x7a, 0x65, 0x22, 0x3a, 0x32, 0x30, 0x39,
            0x37, 0x31, 0x35, 0x32, 0x30, 0x2c, 0x22, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x3a, 0x22, 0x38, 0x66,
            0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65, 0x34, 0x31, 0x34, 0x66, 0x66, 0x39,
            0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63, 0x62, 0x61, 0x38, 0x63, 0x22, 0x2c,
            0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x61, 0x22, 0x3a, 0x22, 0x61, 0x70, 0x70, 0x2d, 0x31,
            0x2e, 0x30, 0x2e, 0x31, 0x2d, 0x32, 0x30, 0x31, 0x38, 0x30, 0x31, 0x30, 0x31, 0x2e, 0x31, 0x30,
            0x30, 0x30, 0x22, 0x2c, 0x22, 0x75, 0x72, 0x6c, 0x22, 0x3a, 0x22,  /* 0x75, 0x72, 0x6c 表示 url */
#if 1
            0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x6c, 0x69, 0x6e, 0x6b, 0x6b, 0x69, 0x74, 0x2d,
            0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x73, 0x73, 0x2d, 0x63, 0x6e, 0x2d, 0x73, 0x68,
            0x61, 0x6e, 0x67, 0x68, 0x61, 0x69, 0x2e, 0x61, 0x6c, 0x69, 0x79, 0x75, 0x6e, 0x63, 0x73, 0x2e,
            0x63, 0x6f, 0x6d, 0x2f, 0x6b, 0x75, 0x61, 0x6e, 0x6a, 0x75, 0x2f, 0x63, 0x6a, 0x6f, 0x36, 0x76,
            0x61, 0x64, 0x71, 0x31, 0x30, 0x30, 0x30, 0x64, 0x33, 0x67, 0x37, 0x34, 0x37, 0x6c, 0x6b, 0x61,
            0x39, 0x33, 0x36, 0x78, 0x2e, 0x62, 0x69, 0x6e, 0x22, 0x2c,
#endif
            0x22, 0x73, 0x69, 0x67, 0x6e, 0x4d,
            0x65, 0x74, 0x68, 0x6f, 0x64, 0x22, 0x3a, 0x22, 0x4d, 0x64, 0x35, 0x22, 0x2c, 0x22, 0x6d, 0x64,
            0x35, 0x22, 0x3a, 0x22, 0x38, 0x66, 0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65,
            0x34, 0x31, 0x34, 0x66, 0x66, 0x39, 0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63,
            0x62, 0x61, 0x38, 0x63, 0x22, 0x7d, 0x2c, 0x22, 0x69, 0x64, 0x22, 0x3a, 0x31, 0x35, 0x37, 0x37,
            0x32, 0x35, 0x33, 0x39, 0x30, 0x35, 0x32, 0x37, 0x33, 0x2c, 0x22, 0x6d, 0x65, 0x73, 0x73, 0x61,
            0x67, 0x65, 0x22, 0x3a, 0x22, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x22, 0x7d
        };
#endif
        printf("len of pub is %lu\r\n", sizeof(pub));
        printf("len of pub is %d\r\n", 0xed - 0x80 + 256);
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        count = 4;
        res = sizeof(pub) / sizeof(char);
    }
    return res;
}
static void case_21_user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;
            case_21_ota_download_finished = 1;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            break;
        }
        case AIOT_OTARECV_COTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_FOTA, case_21_aiot_ota_maltformed_json_without_version)
{
    int32_t res;
    uint32_t count = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    case_21_user_ota_recv_handler);           /* 设置ota_handle的recv_handler */
    res = aiot_mqtt_connect(data->mqtt_handle);

    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_21_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    sleep(1);

    while (count < 3) {
        aiot_mqtt_process(data->mqtt_handle);
        res = aiot_mqtt_recv(data->mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        count++;
    }
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    ASSERT_EQ(case_21_ota_download_finished, 0);
}

int case_22_faked = 0;
int case_22_ota_download_finished = 0;
static int32_t case_22_2_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;

    if (count == 0) {
        char pub[] = {0x30};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xea};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
#if 1
        char pub[] = {
            0x0,  0x2e, 0x2f, 0x6f, 0x74, 0x61, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2f, 0x75, 0x70, 0x67,
            0x72, 0x61, 0x64, 0x65, 0x2f, 0x61, 0x31, 0x5a, 0x64, 0x37, 0x6e, 0x35, 0x79, 0x54, 0x74, 0x38,
            0x2f, 0x61, 0x69, 0x6f, 0x74, 0x5f, 0x73, 0x68, 0x61, 0x32, 0x35, 0x36, 0x5f, 0x76, 0x32, 0x7b,
            0x22, 0x63, 0x6f, 0x64, 0x65, 0x22, 0x3a, 0x22, 0x31, 0x30, 0x30, 0x30, 0x22, 0x2c, 0x22, 0x64,
            0x61, 0x74, 0x61, 0x22, 0x3a, 0x7b, 0x22, 0x73, 0x69, 0x7a, 0x65, 0x22, 0x3a, 0x32, 0x30, 0x39,
            0x37, 0x31, 0x35, 0x32, 0x30, 0x2c, 0x22, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x3a, 0x22, 0x38, 0x66,
            0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65, 0x34, 0x31, 0x34, 0x66, 0x66, 0x39,
            0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63, 0x62, 0x61, 0x38, 0x63, 0x22, 0x2c,
            0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x22, 0x61, 0x70, 0x70, 0x2d, 0x31,
            0x2e, 0x30, 0x2e, 0x31, 0x2d, 0x32, 0x30, 0x31, 0x38, 0x30, 0x31, 0x30, 0x31, 0x2e, 0x31, 0x30,
            0x30, 0x30, 0x22, 0x2c, 0x22, 0x75, 0x72, 0x6c, 0x22, 0x3a, 0x22,  /* 0x75, 0x72, 0x6c 表示 url */
#if 1
            0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x6c, 0x69, 0x6e, 0x6b, 0x6b, 0x69, 0x74, 0x2d,
            0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x73, 0x73, 0x2d, 0x63, 0x6e, 0x2d, 0x73, 0x68,
            0x61, 0x6e, 0x67, 0x68, 0x61, 0x69, 0x2e, 0x61, 0x6c, 0x69, 0x79, 0x75, 0x6e, 0x63, 0x73, 0x2e,
            0x63, 0x6f, 0x6d, 0x2f, 0x6b, 0x75, 0x61, 0x6e, 0x6a, 0x75, 0x2f, 0x63, 0x6a, 0x6f, 0x36, 0x76,
            0x61, 0x64, 0x71, 0x31, 0x30, 0x30, 0x30, 0x64, 0x33, 0x67, 0x37, 0x34, 0x37, 0x6c, 0x6b, 0x61,
            0x39, 0x33, 0x36, 0x78, 0x2e, 0x62, 0x69, 0x6e, 0x22, 0x2c,
#endif
            0x22, 0x73, 0x69, 0x67, 0x6e, 0x4d,
            0x65, 0x74, 0x68, 0x6f, 0x64, 0x22, 0x3a, 0x22, 0x4d, 0x64, 0x35, 0x22, 0x2c, 0x22, 0x6d, 0x64,
            0x35, 0x22, 0x3a, 0x22, 0x38, 0x66, 0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65,
            0x34, 0x31, 0x34, 0x66, 0x66, 0x39, 0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63,
            0x62, 0x61, 0x38, 0x63, 0x22, 0x7d, 0x2c, 0x22, 0x69, 0x64, 0x22, 0x3a, 0x31, 0x35, 0x37, 0x37,
            0x32, 0x35, 0x33, 0x39, 0x30, 0x35, 0x32, 0x37, 0x33, 0x2c, 0x22, 0x6d, 0x65, 0x73, 0x73, 0x61,
            0x67, 0x65, 0x22, 0x3a, 0x22, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x22, 0x7d
        };
#endif
        printf("len of pub is %lu\r\n", sizeof(pub));
        printf("len of pub is %d\r\n", 0xed - 0x80 + 256);
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        count = 4;
        res = sizeof(pub) / sizeof(char);
    }
    return res;
}

int case_22_finished_in_advance = 0;
int g_case_22_percent = 0;
static void *case_22_demo_ota_download_loop(void *dl_handle)
{
    download_handle_t *downloader = (download_handle_t *)dl_handle;
    downloader->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    printf("debug switch to legacy read, finish is %d\r\n", case_22_main_loop_finished);
    while (0 == case_22_main_loop_finished) {
        if (case_22_faked > 0) {
            printf("recv with faking\r\n");
        }
        int32_t res = aiot_download_recv(dl_handle);

        aiot_sysdep_portfile_t *sysdep = NULL;
        sysdep = aiot_sysdep_get_portfile();
        /* TODO: user code, burn it into flash */
        if (g_case_22_percent > 50 && case_22_faked == 0) {
            printf("ready to fake\r\n");
            sysdep->core_sysdep_network_recv = case_22_2_core_sysdep_network_recv;
            case_22_faked++;
        }
        if (case_22_faked > 0) {
            printf("faking\r\n");
            case_22_faked++;
        }

        if (case_22_faked == 20) {
            printf("outof fake\r\n");
            sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
        }

        /* report new version */
        if (STATE_DOWNLOAD_FINISHED == res) {
            case_22_ota_download_finished = 1;
            case_22_finished_in_advance = 1;
            goto exit;
        }
    }
exit:

    if (NULL != dl_handle) {
        aiot_download_deinit(&dl_handle);
    }
    return NULL;
}


void case_22_user_download_recv_handler(void *handle, int32_t percent, const aiot_download_recv_t *packet,
                                        void *userdata)
{
    switch (packet->type) {
        case AIOT_DLRECV_HTTPBODY: {
            if (percent < 0) {
                /* 收包异常或者digest校验错误 */
                printf("exception happend, percent is %d\r\n", percent);
                break;
            }
            g_case_22_percent = percent;

        }
        break;
        default:
            break;
    }
}

static pthread_t case_22_ota_thread;
static void case_22_user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            uint32_t max_buffer_len = 2048;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(case_22_user_download_recv_handler));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len));
            pthread_create(&case_22_ota_thread, NULL, case_22_demo_ota_download_loop, dl_handle);
            break;
        }
        case AIOT_OTARECV_COTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_FOTA, case_22_aiot_download_with_mock_data_stop_and_renewal)
{
    uint32_t count = 0;
    int res;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    case_22_user_ota_recv_handler);           /* 设置ota_handle的recv_handler */

    aiot_mqtt_connect(data->mqtt_handle);

    case_22_main_loop_finished = 0;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_22_2_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    sleep(1);

    while (count < 25) {
        aiot_mqtt_process(data->mqtt_handle);
        res = aiot_mqtt_recv(data->mqtt_handle);
        sleep(1);
        printf("looping in case 22, count is %d, %d\r\n", count, res);
        count++;
        if (case_22_ota_download_finished == 1) {
            break;
        }
    }
    printf("exit case 22, count is %d\r\n", count);
    case_22_main_loop_finished = 1;
    pthread_join(case_22_ota_thread, NULL);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    ASSERT_EQ(case_22_ota_download_finished, 1);
}

CASE(COMPONENT_FOTA, case_23_aiot_download_init_without_portfile)
{
    void *download_handle = NULL;

    aiot_sysdep_set_portfile(NULL);
    download_handle = aiot_download_init();
    ASSERT_NULL(download_handle);
}

#if 1
CASEs(COMPONENT_FOTA, case_24_aiot_download_send_request_null_task_desc)
{
    download_handle_t dl_handle = {0};
    int res = aiot_download_send_request(&dl_handle);
    printf("res = %d\r\n", res);
    ASSERT_EQ(res, STATE_DOWNLOAD_REQUEST_TASK_DESC_IS_NULL);
}

CASEs(COMPONENT_FOTA, case_25_aiot_ota_report_version_with_ota_handle)
{
    char *version = NULL;
    int res = aiot_ota_report_version(NULL, version);
    printf("res = %d\r\n", res);
    ASSERT_EQ(res, STATE_OTA_REPORT_HANDLE_IS_NULL);
}


CASEs(COMPONENT_FOTA, case_26_aiot_ota_report_version_with_null_mqtt_handle)
{
    char *version = "hello";
    ota_handle_t ota_handle = {0};
    int res = aiot_ota_report_version(&ota_handle, version);
    printf("res = %d\r\n", res);
    ASSERT_EQ(res, STATE_OTA_REPORT_MQTT_HANDLE_IS_NULL);
}

CASEs(COMPONENT_FOTA, case_27_aiot_ota_setopt_null_handle)
{
    ota_handle_t ota_handle = {0};
    int res = aiot_ota_setopt(&ota_handle, AIOT_OTAOPT_RECV_HANDLER, NULL);
    printf("res = %d\r\n", res);
    ASSERT_EQ(res, STATE_OTA_SETOPT_DATA_IS_NULL);
}

CASEs(COMPONENT_FOTA, case_28_aiot_download_report_progress_null_handle)
{
    int res = aiot_download_report_progress(NULL, 0);
    printf("res = %d\r\n", res);
    ASSERT_EQ(res, STATE_DOWNLOAD_REPORT_HANDLE_IS_NULL);
}

CASEs(COMPONENT_FOTA, case_29_aiot_download_recv_null_hanndle)
{
    int res = aiot_download_recv(NULL);
    printf("res = %d\r\n", res);
    ASSERT_EQ(res, STATE_DOWNLOAD_RECV_HANDLE_IS_NULL);
}
CASEs(COMPONENT_FOTA, case_30_aiot_download_send_request_null_url)
{
    download_handle_t dl_handle = {0};
    aiot_download_task_desc_t task_desc = {0};
    dl_handle.task_desc = &task_desc;
    int res = aiot_download_send_request(&dl_handle);
    printf("res = %d\r\n", res);
    ASSERT_EQ(res, STATE_DOWNLOAD_REQUEST_URL_IS_NULL);
}
#endif


int case_31_ota_download_finished = 0;
static void *case_31_demo_ota_download_loop(void *dl_handle)
{
    download_handle_t *downloader = (download_handle_t *)dl_handle;
    downloader->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    printf("debug switch to legacy read, finish is %d\r\n", case_31_main_loop_finished);
    while (0 == case_31_main_loop_finished) {
        int32_t len = aiot_download_recv(dl_handle);

        if (downloader->range_end + 1 == downloader->size_fetched) {
            case_31_ota_download_finished = 1;
        } else {
            case_31_ota_download_finished = 0;
        }

        if (len == STATE_DOWNLOAD_FINISHED) {
            goto exit;
        }
    }
exit:

    if (NULL != dl_handle) {
        aiot_download_deinit(&dl_handle);
    }
    return NULL;
}


static int32_t case_31_2_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;
    printf("now count is %d\r\n", count);

    if (count == 0) {
        char pub[] = {0x30};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xea};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
#if 1
        char pub[] = {
            0x0,  0x2e, 0x2f, 0x6f, 0x74, 0x61, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2f, 0x75, 0x70, 0x67,
            0x72, 0x61, 0x64, 0x65, 0x2f, 0x61, 0x31, 0x5a, 0x64, 0x37, 0x6e, 0x35, 0x79, 0x54, 0x74, 0x38,
            0x2f, 0x61, 0x69, 0x6f, 0x74, 0x5f, 0x73, 0x68, 0x61, 0x32, 0x35, 0x36, 0x5f, 0x76, 0x32, 0x7b,
            0x22, 0x63, 0x6f, 0x64, 0x65, 0x22, 0x3a, 0x22, 0x31, 0x30, 0x30, 0x30, 0x22, 0x2c, 0x22, 0x64,
            0x61, 0x74, 0x61, 0x22, 0x3a, 0x7b, 0x22, 0x73, 0x69, 0x7a, 0x65, 0x22, 0x3a, 0x32, 0x30, 0x39,
            0x37, 0x31, 0x35, 0x32, 0x30, 0x2c, 0x22, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x3a, 0x22, 0x38, 0x66,
            0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65, 0x34, 0x31, 0x34, 0x66, 0x66, 0x39,
            0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63, 0x62, 0x61, 0x38, 0x63, 0x22, 0x2c,
            0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x22, 0x61, 0x70, 0x70, 0x2d, 0x31,
            0x2e, 0x30, 0x2e, 0x31, 0x2d, 0x32, 0x30, 0x31, 0x38, 0x30, 0x31, 0x30, 0x31, 0x2e, 0x31, 0x30,
            0x30, 0x30, 0x22, 0x2c, 0x22, 0x75, 0x72, 0x6c, 0x22, 0x3a, 0x22,  /* 0x75, 0x72, 0x6c 表示 url */
#if 1
            0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x6c, 0x69, 0x6e, 0x6b, 0x6b, 0x69, 0x74, 0x2d,
            0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x73, 0x73, 0x2d, 0x63, 0x6e, 0x2d, 0x73, 0x68,
            0x61, 0x6e, 0x67, 0x68, 0x61, 0x69, 0x2e, 0x61, 0x6c, 0x69, 0x79, 0x75, 0x6e, 0x63, 0x73, 0x2e,
            0x63, 0x6f, 0x6d, 0x2f, 0x6b, 0x75, 0x61, 0x6e, 0x6a, 0x75, 0x2f, 0x63, 0x6a, 0x6f, 0x36, 0x76,
            0x61, 0x64, 0x71, 0x31, 0x30, 0x30, 0x30, 0x64, 0x33, 0x67, 0x37, 0x34, 0x37, 0x6c, 0x6b, 0x61,
            0x39, 0x33, 0x36, 0x78, 0x2e, 0x62, 0x69, 0x6e, 0x22, 0x2c,
#endif
            0x22, 0x73, 0x69, 0x67, 0x6e, 0x4d,
            0x65, 0x74, 0x68, 0x6f, 0x64, 0x22, 0x3a, 0x22, 0x4d, 0x64, 0x35, 0x22, 0x2c, 0x22, 0x6d, 0x64,
            0x35, 0x22, 0x3a, 0x22, 0x38, 0x66, 0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65,
            0x34, 0x31, 0x34, 0x66, 0x66, 0x39, 0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63,
            0x62, 0x61, 0x38, 0x63, 0x22, 0x7d, 0x2c, 0x22, 0x69, 0x64, 0x22, 0x3a, 0x31, 0x35, 0x37, 0x37,
            0x32, 0x35, 0x33, 0x39, 0x30, 0x35, 0x32, 0x37, 0x33, 0x2c, 0x22, 0x6d, 0x65, 0x73, 0x73, 0x61,
            0x67, 0x65, 0x22, 0x3a, 0x22, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x22, 0x7d
        };
#endif
        printf("len of pub is %lu\r\n", sizeof(pub));
        printf("len of pub is %d\r\n", 0xed - 0x80 + 256);
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        count = 4;
        res = sizeof(pub) / sizeof(char);
    }
    return res;
}
void case_31_user_download_recv_handler(void *handle, int32_t percent, const aiot_download_recv_t *packet,
                                        void *userdata)
{
    switch (packet->type) {
        case AIOT_DLRECV_HTTPBODY: {

            if (percent < 0) {
                /* 收包异常或者digest校验错误 */
                printf("exception happend, percent is %d\r\n", percent);
                break;
            }

            if (percent == 100) {
            }
        }
        break;
        default:
            break;
    }
}

static pthread_t case_31_ota_thread;
static void case_31_user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            uint32_t max_buffer_len = 2048;
            uint32_t range_end = 2097152;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(case_31_user_download_recv_handler));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_RANGE_END, (void *)(&range_end));
            pthread_create(&case_31_ota_thread, NULL, case_31_demo_ota_download_loop, dl_handle);
            break;
        }
        case AIOT_OTARECV_COTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_FOTA, case_31_aiot_download_small_range_download_test)
{
    int32_t res;
    uint32_t count = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    case_31_user_ota_recv_handler);           /* 设置ota_handle的recv_handler */
    res = aiot_mqtt_connect(data->mqtt_handle);

    case_31_main_loop_finished = 0;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_31_2_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    sleep(1);

    while (count < 10) {
        aiot_mqtt_process(data->mqtt_handle);
        res = aiot_mqtt_recv(data->mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        count++;
        if (case_31_ota_download_finished) {
            break;
        }
    }
    case_31_main_loop_finished = 1;
    pthread_join(case_31_ota_thread, NULL);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    ASSERT_EQ(case_31_ota_download_finished, 1);
}


int case_32_ota_download_finished = 0;
static void *case_32_demo_ota_download_loop(void *dl_handle)
{
    download_handle_t *downloader = (download_handle_t *)dl_handle;
    downloader->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    printf("debug switch to legacy read, finish is %d\r\n", case_32_main_loop_finished);
    while (0 == case_32_main_loop_finished) {
        int32_t res = aiot_download_recv(dl_handle);
        if (res == STATE_DOWNLOAD_FINISHED) {
            goto exit;
        }

    }
exit:

    if (NULL != dl_handle) {
        aiot_download_deinit(&dl_handle);
    }
    return NULL;
}


static int32_t case_32_2_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;
    printf("now count is %d\r\n", count);

    if (count == 0) {
        char pub[] = {0x30};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xea};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
#if 1
        char pub[] = {
            0x0,  0x2e, 0x2f, 0x6f, 0x74, 0x61, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2f, 0x75, 0x70, 0x67,
            0x72, 0x61, 0x64, 0x65, 0x2f, 0x61, 0x31, 0x5a, 0x64, 0x37, 0x6e, 0x35, 0x79, 0x54, 0x74, 0x38,
            0x2f, 0x61, 0x69, 0x6f, 0x74, 0x5f, 0x73, 0x68, 0x61, 0x32, 0x35, 0x36, 0x5f, 0x76, 0x32, 0x7b,
            0x22, 0x63, 0x6f, 0x64, 0x65, 0x22, 0x3a, 0x22, 0x31, 0x30, 0x30, 0x30, 0x22, 0x2c, 0x22, 0x64,
            0x61, 0x74, 0x61, 0x22, 0x3a, 0x7b, 0x22, 0x73, 0x69, 0x7a, 0x65, 0x22, 0x3a, 0x32, 0x30, 0x39,
            0x37, 0x31, 0x35, 0x32, 0x30, 0x2c, 0x22, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x3a, 0x22, 0x28, 0x76, /* change from 0x38, 0x66 to normal 0x28, 0x76*/
            0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65, 0x34, 0x31, 0x34, 0x66, 0x66, 0x39,
            0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63, 0x62, 0x61, 0x38, 0x63, 0x22, 0x2c,
            0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x22, 0x61, 0x70, 0x70, 0x2d, 0x31,
            0x2e, 0x30, 0x2e, 0x31, 0x2d, 0x32, 0x30, 0x31, 0x38, 0x30, 0x31, 0x30, 0x31, 0x2e, 0x31, 0x30,
            0x30, 0x30, 0x22, 0x2c, 0x22, 0x75, 0x72, 0x6c, 0x22, 0x3a, 0x22,  /* 0x75, 0x72, 0x6c 表示 url */
#if 1
            0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x6c, 0x69, 0x6e, 0x6b, 0x6b, 0x69, 0x74, 0x2d,
            0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x73, 0x73, 0x2d, 0x63, 0x6e, 0x2d, 0x73, 0x68,
            0x61, 0x6e, 0x67, 0x68, 0x61, 0x69, 0x2e, 0x61, 0x6c, 0x69, 0x79, 0x75, 0x6e, 0x63, 0x73, 0x2e,
            0x63, 0x6f, 0x6d, 0x2f, 0x6b, 0x75, 0x61, 0x6e, 0x6a, 0x75, 0x2f, 0x63, 0x6a, 0x6f, 0x36, 0x76,
            0x61, 0x64, 0x71, 0x31, 0x30, 0x30, 0x30, 0x64, 0x33, 0x67, 0x37, 0x34, 0x37, 0x6c, 0x6b, 0x61,
            0x39, 0x33, 0x36, 0x78, 0x2e, 0x62, 0x69, 0x6e, 0x22, 0x2c,
#endif
            0x22, 0x73, 0x69, 0x67, 0x6e, 0x4d,
            0x65, 0x74, 0x68, 0x6f, 0x64, 0x22, 0x3a, 0x22, 0x4d, 0x64, 0x35, 0x22, 0x2c, 0x22, 0x6d, 0x64,
            0x35, 0x22, 0x3a, 0x22, 0x38, 0x66, 0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65,
            0x34, 0x31, 0x34, 0x66, 0x66, 0x39, 0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63,
            0x62, 0x61, 0x38, 0x63, 0x22, 0x7d, 0x2c, 0x22, 0x69, 0x64, 0x22, 0x3a, 0x31, 0x35, 0x37, 0x37,
            0x32, 0x35, 0x33, 0x39, 0x30, 0x35, 0x32, 0x37, 0x33, 0x2c, 0x22, 0x6d, 0x65, 0x73, 0x73, 0x61,
            0x67, 0x65, 0x22, 0x3a, 0x22, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x22, 0x7d
        };
#endif
        printf("len of pub is %lu\r\n", sizeof(pub));
        printf("len of pub is %d\r\n", 0xed - 0x80 + 256);
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        count = 4;
        res = sizeof(pub) / sizeof(char);
    }
    return res;
}
void case_32_user_download_recv_handler(void *handle, int32_t percent, const aiot_download_recv_t *packet,
                                        void *userdata)
{
    switch (packet->type) {
        case AIOT_DLRECV_HTTPBODY: {

            if (percent < 0) {
                /* 收包异常或者digest校验错误 */
                printf("exception happend, percent is %d\r\n", percent);
                /* report new version */
                if (percent == AIOT_OTAERR_CHECKSUM_MISMATCH) {
                    case_32_ota_download_finished = 1;
                }
                break;
            }

            if (percent == 100) {
            }
        }
        break;
        default:
            break;
    }
}

static pthread_t case_32_ota_thread;
static void case_32_user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            uint32_t max_buffer_len = 2048;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(case_32_user_download_recv_handler));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len));
            pthread_create(&case_32_ota_thread, NULL, case_32_demo_ota_download_loop, dl_handle);
            break;
        }
        case AIOT_OTARECV_COTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_FOTA, case_32_aiot_download_with_mock_data_with_maltformed_md5)
{
    int32_t res;
    uint32_t count = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    case_32_user_ota_recv_handler);           /* 设置ota_handle的recv_handler */
    res = aiot_mqtt_connect(data->mqtt_handle);

    case_32_main_loop_finished = 0;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_32_2_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    sleep(1);

    while (count < 10) {
        aiot_mqtt_process(data->mqtt_handle);
        res = aiot_mqtt_recv(data->mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        count++;
        if (case_32_ota_download_finished) {
            break;
        }
    }
    case_32_main_loop_finished = 1;
    pthread_join(case_32_ota_thread, NULL);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    ASSERT_EQ(case_32_ota_download_finished, 1);
}

CASEs(COMPONENT_FOTA, case_33_aiot_ota_report_version_ext)
{
    int32_t res;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    res = aiot_mqtt_connect(data->mqtt_handle);

    char *cur_version = "1.0.0";
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_report_version_ext(data->ota_handle, "pk", "dn",
                                cur_version);             /* 上报当前固件版本号 */
    ASSERT_EQ(res, STATE_SUCCESS);
}

CASEs(COMPONENT_FOTA, case_34_aiot_ota_report_version_ext_null_input)
{
    char *cur_version = "1.0.0";
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    int32_t res = aiot_ota_report_version_ext(data->ota_handle, NULL, "dn",
                  cur_version);             /* 上报当前固件版本号 */
    ASSERT_EQ(res, STATE_OTA_REPORT_EXT_PRODUCT_KEY_IS_NULL);
}

static int32_t case_35_test_sysdep_network_write(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    return -1;
}
extern int32_t core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                        core_sysdep_addr_t *addr);

int case_35_ota_download_finished = 0;
static void *case_35_demo_ota_download_loop(void *dl_handle)
{
    int ret = 0;
    download_handle_t *downloader = (download_handle_t *)dl_handle;
    downloader->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    downloader->sysdep->core_sysdep_network_send = case_35_test_sysdep_network_write;
    printf("debug switch to legacy read, finish is %d\r\n", case_35_main_loop_finished);
    ret = aiot_download_send_request(dl_handle);
    if (ret == STATE_DOWNLOAD_SEND_REQUEST_FAILED) {
        case_35_ota_download_finished = 1;
    }
    downloader->sysdep->core_sysdep_network_send = core_sysdep_network_send;
    while (0 == case_35_main_loop_finished) {
        int32_t res = aiot_download_recv(dl_handle);
        if (res == STATE_DOWNLOAD_FINISHED) {
            goto exit;
        }
    }
exit:

    if (NULL != dl_handle) {
        aiot_download_deinit(&dl_handle);
    }
    return NULL;
}


static int32_t case_35_2_core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
        core_sysdep_addr_t *addr)
{
    int32_t res = 0;
    static uint8_t count = 0;
    printf("now count is %d\r\n", count);

    if (count == 0) {
        char pub[] = {0x30};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 1;
    } else if (count == 1) {
        char pub[] = {0xea};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 2;
    } else if (count == 2) {
        char pub[] = {0x02};
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        res = sizeof(pub) / sizeof(char);
        count = 3;
    } else if (count == 3) {
#if 1
        char pub[] = {
            0x0,  0x2e, 0x2f, 0x6f, 0x74, 0x61, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2f, 0x75, 0x70, 0x67,
            0x72, 0x61, 0x64, 0x65, 0x2f, 0x61, 0x31, 0x5a, 0x64, 0x37, 0x6e, 0x35, 0x79, 0x54, 0x74, 0x38,
            0x2f, 0x61, 0x69, 0x6f, 0x74, 0x5f, 0x73, 0x68, 0x61, 0x32, 0x35, 0x36, 0x5f, 0x76, 0x32, 0x7b,
            0x22, 0x63, 0x6f, 0x64, 0x65, 0x22, 0x3a, 0x22, 0x31, 0x30, 0x30, 0x30, 0x22, 0x2c, 0x22, 0x64,
            0x61, 0x74, 0x61, 0x22, 0x3a, 0x7b, 0x22, 0x73, 0x69, 0x7a, 0x65, 0x22, 0x3a, 0x32, 0x30, 0x39,
            0x37, 0x31, 0x35, 0x32, 0x30, 0x2c, 0x22, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x3a, 0x22, 0x38, 0x66,
            0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65, 0x34, 0x31, 0x34, 0x66, 0x66, 0x39,
            0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63, 0x62, 0x61, 0x38, 0x63, 0x22, 0x2c,
            0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x22, 0x61, 0x70, 0x70, 0x2d, 0x31,
            0x2e, 0x30, 0x2e, 0x31, 0x2d, 0x32, 0x30, 0x31, 0x38, 0x30, 0x31, 0x30, 0x31, 0x2e, 0x31, 0x30,
            0x30, 0x30, 0x22, 0x2c, 0x22, 0x75, 0x72, 0x6c, 0x22, 0x3a, 0x22,  /* 0x75, 0x72, 0x6c 表示 url */
            0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x6c, 0x69, 0x6e, 0x6b, 0x6b, 0x69, 0x74, 0x2d,
            0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x2e, 0x6f, 0x73, 0x73, 0x2d, 0x63, 0x6e, 0x2d, 0x73, 0x68,
            0x61, 0x6e, 0x67, 0x68, 0x61, 0x69, 0x2e, 0x61, 0x6c, 0x69, 0x79, 0x75, 0x6e, 0x63, 0x73, 0x2e,
            0x63, 0x6f, 0x6d, 0x2f, 0x6b, 0x75, 0x61, 0x6e, 0x6a, 0x75, 0x2f, 0x63, 0x6a, 0x6f, 0x36, 0x76,
            0x61, 0x64, 0x71, 0x31, 0x30, 0x30, 0x30, 0x64, 0x33, 0x67, 0x37, 0x34, 0x37, 0x6c, 0x6b, 0x61,
            0x39, 0x33, 0x36, 0x78, 0x2e, 0x62, 0x69, 0x6e, 0x22, 0x2c,
            0x22, 0x73, 0x69, 0x67, 0x6e, 0x4d,
            0x65, 0x74, 0x68, 0x6f, 0x64, 0x22, 0x3a, 0x22, 0x4d, 0x64, 0x35, 0x22, 0x2c, 0x22, 0x6d, 0x64,
            0x35, 0x22, 0x3a, 0x22, 0x38, 0x66, 0x34, 0x65, 0x33, 0x33, 0x66, 0x33, 0x64, 0x63, 0x33, 0x65,
            0x34, 0x31, 0x34, 0x66, 0x66, 0x39, 0x34, 0x65, 0x35, 0x66, 0x62, 0x36, 0x39, 0x30, 0x35, 0x63,
            0x62, 0x61, 0x38, 0x63, 0x22, 0x7d, 0x2c, 0x22, 0x69, 0x64, 0x22, 0x3a, 0x31, 0x35, 0x37, 0x37,
            0x32, 0x35, 0x33, 0x39, 0x30, 0x35, 0x32, 0x37, 0x33, 0x2c, 0x22, 0x6d, 0x65, 0x73, 0x73, 0x61,
            0x67, 0x65, 0x22, 0x3a, 0x22, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x22, 0x7d
        };
#endif
        printf("len of pub is %lu\r\n", sizeof(pub));
        printf("len of pub is %d\r\n", 0xed - 0x80 + 256);
        memcpy(buffer, pub, sizeof(pub) / sizeof(char));
        count = 4;
        res = sizeof(pub) / sizeof(char);
    }
    return res;
}

void case_35_user_download_recv_handler(void *handle, int32_t percent, const aiot_download_recv_t *packet,
                                        void *userdata)
{
    switch (packet->type) {
        case AIOT_DLRECV_HTTPBODY: {

            if (percent < 0) {
                /* 收包异常或者digest校验错误 */
                printf("exception happend, percent is %d\r\n", percent);
                break;
            }

            if (percent == 100) {
            }
        }
        break;
        default:
            break;
    }
}

static pthread_t case_35_ota_thread;
static void case_35_user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            uint32_t max_buffer_len = 2048;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(case_35_user_download_recv_handler));
            aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len));
            pthread_create(&case_35_ota_thread, NULL, case_35_demo_ota_download_loop, dl_handle);
            break;
        }
        case AIOT_OTARECV_COTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_FOTA, case_35_aiot_download_with_aritifical_data_send_failed)
{
    int32_t res;
    uint32_t count = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    case_35_user_ota_recv_handler);           /* 设置ota_handle的recv_handler */
    res = aiot_mqtt_connect(data->mqtt_handle);

    case_35_main_loop_finished = 0;
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = case_35_2_core_sysdep_network_recv;
    res = aiot_mqtt_recv(data->mqtt_handle);
    sleep(1);

    while (count < 10) {
        aiot_mqtt_process(data->mqtt_handle);
        res = aiot_mqtt_recv(data->mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        count++;
    }
    case_35_main_loop_finished = 1;
    pthread_join(case_35_ota_thread, NULL);
    sleep(1);
    ((core_mqtt_handle_t *)data->mqtt_handle)->sysdep->core_sysdep_network_recv = core_sysdep_network_recv;
    ASSERT_EQ(case_35_ota_download_finished, 1);
}



CASEs(COMPONENT_FOTA, case_36_aiot_report_version_ext_null_ota_handle_input)
{
    void *ota_handle = NULL;
    char *cur_version = "test";
    int32_t res = aiot_ota_report_version_ext(ota_handle, "pk", "dn",
                  cur_version);             /* 上报当前固件版本号 */
    ASSERT_EQ(res, STATE_OTA_REPORT_EXT_HANELD_IS_NULL);
}

CASEs(COMPONENT_FOTA, case_37_aiot_report_version_ext_null_mqtt_handle)
{
    ota_handle_t ota_handle = {0};
    char *cur_version = "test";
    int32_t res = aiot_ota_report_version_ext(&ota_handle, "pk", "dn",
                  cur_version);             /* 上报当前固件版本号 */
    ASSERT_EQ(res, STATE_OTA_REPORT_EXT_MQTT_HANDLE_IS_NULL);
}

CASEs(COMPONENT_FOTA, case_09_aiot_download_report_progress_null_desc_fail)
{
    download_handle_t download_handle = {0};
    int res = aiot_download_report_progress(&download_handle, 1);
    ASSERT_EQ(res, STATE_DOWNLOAD_REPORT_TASK_DESC_IS_NULL);
}

SUITE(COMPONENT_FOTA) = {
    ADD_CASE(COMPONENT_FOTA, case_01_aiot_ota_init_without_portfile),
    ADD_CASE(COMPONENT_FOTA, case_02_aiot_ota_init_with_portfile),
    ADD_CASE(COMPONENT_FOTA, case_03_aiot_ota_setopt_mqtt_handle),
    ADD_CASE(COMPONENT_FOTA, case_04_aiot_download_with_maltform_url),
    ADD_CASE(COMPONENT_FOTA, case_05_aiot_ota_report_version_success),
    ADD_CASE(COMPONENT_FOTA, case_06_aiot_ota_setopt_userdata),
    ADD_CASE(COMPONENT_FOTA, case_07_aiot_download_setopt_recv_timeout_ms),
    ADD_CASE(COMPONENT_FOTA, case_08_aiot_download_setopt_user_data),
    ADD_CASE(COMPONENT_FOTA, case_09_aiot_download_report_progress_null_desc_fail),
    ADD_CASE(COMPONENT_FOTA, case_10_aiot_download_setopt_range_start),
    ADD_CASE(COMPONENT_FOTA, case_11_aiot_download_setopt_range_end),
    ADD_CASE(COMPONENT_FOTA, case_12_aiot_download_setopt_network_port),
    ADD_CASE(COMPONENT_FOTA, case_13_aiot_ota_setopt_max_num),
    ADD_CASE(COMPONENT_FOTA, case_14_aiot_download_setopt_null_input),
    ADD_CASE(COMPONENT_FOTA, case_15_aiot_download_setopt_max_range),
    ADD_CASE(COMPONENT_FOTA, case_16_aiot_download_success_with_mock_data),
    ADD_CASE(COMPONENT_FOTA, case_17_aiot_download_with_maltformed_sign_method),
    ADD_CASE(COMPONENT_FOTA, case_18_aiot_download_with_malformed_url),
    ADD_CASE(COMPONENT_FOTA, case_19_aiot_download_with_cred),
    ADD_CASE(COMPONENT_FOTA, case_20_aiot_ota_maltformed_json_without_data_keyword),
    ADD_CASE(COMPONENT_FOTA, case_21_aiot_ota_maltformed_json_without_version),
    ADD_CASE(COMPONENT_FOTA, case_22_aiot_download_with_mock_data_stop_and_renewal),
    ADD_CASE(COMPONENT_FOTA, case_23_aiot_download_init_without_portfile),
    ADD_CASE(COMPONENT_FOTA, case_24_aiot_download_send_request_null_task_desc),
    ADD_CASE(COMPONENT_FOTA, case_25_aiot_ota_report_version_with_ota_handle),
    ADD_CASE(COMPONENT_FOTA, case_26_aiot_ota_report_version_with_null_mqtt_handle),
    ADD_CASE(COMPONENT_FOTA, case_27_aiot_ota_setopt_null_handle),
    ADD_CASE(COMPONENT_FOTA, case_28_aiot_download_report_progress_null_handle),
    ADD_CASE(COMPONENT_FOTA, case_29_aiot_download_recv_null_hanndle),
    ADD_CASE(COMPONENT_FOTA, case_30_aiot_download_send_request_null_url),
    ADD_CASE(COMPONENT_FOTA, case_31_aiot_download_small_range_download_test),
    ADD_CASE(COMPONENT_FOTA, case_32_aiot_download_with_mock_data_with_maltformed_md5),
    ADD_CASE(COMPONENT_FOTA, case_33_aiot_ota_report_version_ext),
    ADD_CASE(COMPONENT_FOTA, case_34_aiot_ota_report_version_ext_null_input),
    ADD_CASE(COMPONENT_FOTA, case_35_aiot_download_with_aritifical_data_send_failed),
    ADD_CASE(COMPONENT_FOTA, case_36_aiot_report_version_ext_null_ota_handle_input),
    ADD_CASE(COMPONENT_FOTA, case_37_aiot_report_version_ext_null_mqtt_handle),
    ADD_CASE_NULL
};

