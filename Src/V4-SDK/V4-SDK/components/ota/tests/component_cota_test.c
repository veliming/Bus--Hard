#include <unistd.h>
#include <pthread.h>
#include "cu_test.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "core_list.h"
#include "core_mqtt.h"
#include "aiot_ota_api.h"
#define RUN_DOWNLOAD_DEMO

static int32_t aiot_mqtt_test_logcb(int32_t code, char *message)
{
    if (STATE_HTTP_LOG_RECV_CONTENT != code) {
        printf("%s", message);
    }
    return 0;
}

CASE(COMPONENT_COTA, case_01_aiot_ota_init_without_portfile)
{
    void *ota_handle = NULL;

    aiot_sysdep_set_portfile(NULL);
    ota_handle = aiot_ota_init();
    ASSERT_NULL(ota_handle);
}

CASE(COMPONENT_COTA, case_02_aiot_ota_init_with_portfile)
{
    void *ota_handle = NULL;

    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    ota_handle = aiot_ota_init();
    ASSERT_NOT_NULL(ota_handle);

    aiot_ota_deinit(&ota_handle);
}

DATA(COMPONENT_COTA)
{
    void *ota_handle;
    void *mqtt_handle;
    char *host;
    uint16_t port;
    char *product_key;
    char *device_name;
    char *device_secret;
};

SETUP(COMPONENT_COTA)
{
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    aiot_state_set_logcb(aiot_mqtt_test_logcb);

    data->ota_handle = aiot_ota_init();
    data->mqtt_handle = aiot_mqtt_init();

    data->product_key = "a1dhWKuKqX5";
    data->device_name = "tmmv2990";
    data->device_secret = "EmEcYq0GXTDRgGWPN3hz8gyTWtfh7wq2";
    data->host = "a1dhWKuKqX5.iot-as-mqtt.cn-shanghai.aliyuncs.com";
    data->port = 443;
}

TEARDOWN(COMPONENT_COTA)
{
    aiot_ota_deinit(&data->ota_handle);
}

CASEs(COMPONENT_COTA, case_03_aiot_ota_setopt_mqtt_handle)
{
    int32_t res = STATE_SUCCESS;

    res = aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE, (void *)data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

    ASSERT_EQ(((ota_handle_t *)data->ota_handle)->mqtt_handle, data->mqtt_handle);
}

#ifdef RUN_DOWNLOAD_DEMO
static int finished = 0;
static void *demo_ota_download_loop(void *dl_handle)
{
    while (0 == finished) {
        int32_t res = aiot_download_recv(dl_handle);
        /* report new version */
        if (res == STATE_DOWNLOAD_FINISHED) {
            goto exit;
        }
    }
exit:

    aiot_download_deinit(&dl_handle);
    return NULL;
}

static void user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_COTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));

            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            demo_ota_download_loop(dl_handle);
            break;
        }
        case AIOT_OTARECV_FOTA:
            /* TBD */
            break;
        default:
            break;
    }
}

CASEs(COMPONENT_COTA, case_04_aiot_fetch_cota_file)
{
    int32_t res;
    int32_t iter = 0;
    finished = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    user_ota_recv_handler);           /* 设置ota_handle的recv_handler */
    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

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

        aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 0);
    }

    finished = 0;
    while (iter < 5) {
        aiot_mqtt_process(data->mqtt_handle);
        res = aiot_mqtt_recv(data->mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        iter++;
    }
    finished = 1;
}

static void user_ota_recv_handler_verify_checksum_mismatch(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_COTA: {
            download_handle_t *dl_handle = aiot_download_init();
            aiot_sysdep_portfile_t *sysdep = NULL;

            sysdep = aiot_sysdep_get_portfile();
            if (sysdep == NULL) {
                return;
            }
            uint16_t port = 80;
            aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));

            aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            memset(dl_handle->task_desc->expect_digest, 0, 5);
            demo_ota_download_loop(dl_handle);
            break;
        }
        case AIOT_OTARECV_FOTA:
            /* TBD */
            break;
        default:
            break;
    }
}


CASEs(COMPONENT_COTA, case_05_aiot_ota_checksum_mismatch)
{
    int32_t res;
    int iter = 0;
    finished = 0;
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_HOST, (void *)data->host);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&data->port);

    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)data->product_key);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)data->device_name);
    aiot_mqtt_setopt(data->mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)data->device_secret);

    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_MQTT_HANDLE,
                    data->mqtt_handle);                      /* 设置ota_handle的mqtt_handle */
    aiot_ota_setopt(data->ota_handle, AIOT_OTAOPT_RECV_HANDLER,
                    user_ota_recv_handler_verify_checksum_mismatch);
    res = aiot_mqtt_connect(data->mqtt_handle);
    ASSERT_EQ(res, STATE_SUCCESS);

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

        aiot_mqtt_pub(data->mqtt_handle, &topic_buff, &payload_buff, 0);
    }

    finished = 0;
    while (iter < 5) {
        aiot_mqtt_process(data->mqtt_handle);
        res = aiot_mqtt_recv(data->mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        iter++;
    }
    finished = 1;
}
#endif


CASEs(COMPONENT_COTA, case_06_aiot_ota_deinit_with_NULL)
{
    void *indata = NULL;
    int32_t res;
    res = aiot_ota_deinit((void *)&indata);
    ASSERT_EQ(res, STATE_OTA_DEINIT_HANDLE_IS_NULL);
}


CASEs(COMPONENT_COTA, case_07_aiot_download_deinit_with_NULL)
{
    void *indata = NULL;
    int32_t res;
    res = aiot_download_deinit((void *)&indata);
    ASSERT_EQ(res, STATE_DOWNLOAD_DEINIT_HANDLE_IS_NULL);
}


SUITE(COMPONENT_COTA) = {
    ADD_CASE(COMPONENT_COTA, case_01_aiot_ota_init_without_portfile),
    ADD_CASE(COMPONENT_COTA, case_02_aiot_ota_init_with_portfile),
    ADD_CASE(COMPONENT_COTA, case_03_aiot_ota_setopt_mqtt_handle),
#ifdef RUN_DOWNLOAD_DEMO
    ADD_CASE(COMPONENT_COTA, case_04_aiot_fetch_cota_file),
    ADD_CASE(COMPONENT_COTA, case_05_aiot_ota_checksum_mismatch),
#endif
    ADD_CASE(COMPONENT_COTA, case_06_aiot_ota_deinit_with_NULL),
    ADD_CASE(COMPONENT_COTA, case_07_aiot_download_deinit_with_NULL),
    ADD_CASE_NULL
};

