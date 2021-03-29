/**
 * @file aiot_ota_api.c
 * @brief OTA模块接口实现文件, 其中包含了OTA的所有用户API
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 */

#include "aiot_ota_api.h"
#include "aiot_http_api.h"
#include "core_mqtt.h"
#include "core_string.h"
#include "core_http.h"
#include "core_sha256.h"
#include "ota_md5.h"
#include "ota_private.h"
#include "core_log.h"
#include "core_global.h"

static int32_t _ota_subscribe(core_mqtt_handle_t *mqtt_handle, void *ota_handle);
static int32_t _ota_free_task_desc(aiot_sysdep_portfile_t *sysdep, void *data);
static int32_t _ota_report_base(void *handle, char *topic_prefix, char *product_key, char *device_name,  char *params);
static void    _ota_mqtt_process(void *handle, const aiot_mqtt_recv_t *const packet, void *userdata);
static int32_t _ota_parse_json(aiot_sysdep_portfile_t *sysdep, void *in, uint32_t in_len, char *key_word, char **out);
static int32_t _ota_parse_url(const char *url, char *host, char *path);
static int32_t _download_update_digest(download_handle_t *download_handle, uint8_t *buffer, uint32_t buffer_len);
static int32_t _download_verify_digest(download_handle_t *download_handle);
static void    _http_recv_handler(void *handle, const aiot_http_recv_t *recv_data, void *user_data);
static void   *_download_deep_copy_task_desc(aiot_sysdep_portfile_t *sysdep, void *data);
static void    _download_deep_copy_base(aiot_sysdep_portfile_t *sysdep, char *in, char **out);

void *aiot_ota_init(void)
{
    ota_handle_t *ota_handle = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return NULL;
    }

    ota_handle = sysdep->core_sysdep_malloc(sizeof(ota_handle_t), OTA_MODULE_NAME);
    if (ota_handle == NULL) {
        return NULL;
    }
    memset(ota_handle, 0, sizeof(ota_handle_t));
    ota_handle->data_mutex = sysdep->core_sysdep_mutex_init();
    ota_handle->sysdep = sysdep;
    core_global_init(sysdep);
    return ota_handle;
}

int32_t aiot_ota_deinit(void **handle)
{
    ota_handle_t *ota_handle = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;

    if (NULL == handle || NULL == *handle) {
        return STATE_OTA_DEINIT_HANDLE_IS_NULL;
    }
    ota_handle = * (ota_handle_t **)handle;
    sysdep = ota_handle->sysdep;
    core_global_deinit(sysdep);
    sysdep->core_sysdep_mutex_deinit(&(ota_handle->data_mutex));
    sysdep->core_sysdep_free(ota_handle);
    *handle = NULL;
    return STATE_SUCCESS;
}

int32_t aiot_ota_setopt(void *handle, aiot_ota_option_t option, void *data)
{
    int32_t res = STATE_SUCCESS;
    ota_handle_t *ota_handle = (ota_handle_t *)handle;
    aiot_sysdep_portfile_t *sysdep = NULL;

    if (NULL == ota_handle) {
        return STATE_OTA_SETOPT_HANDLE_IS_NULL;
    }
    if (NULL == data) {
        return STATE_OTA_SETOPT_DATA_IS_NULL;
    }

    sysdep = ota_handle->sysdep;
    sysdep->core_sysdep_mutex_lock(ota_handle->data_mutex);
    switch (option) {
        case AIOT_OTAOPT_RECV_HANDLER: {
            ota_handle->recv_handler = (aiot_ota_recv_handler_t)data;
        }
        break;
        case AIOT_OTAOPT_USER_DATA: {
            ota_handle->userdata = data;
        }
        break;
        case AIOT_OTAOPT_MQTT_HANDLE: {
            core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)data;
            _ota_subscribe(mqtt_handle, ota_handle);
            ota_handle->mqtt_handle = mqtt_handle;
        }
        break;
        default: {
            res = STATE_USER_INPUT_UNKNOWN_OPTION;
        }
        break;
    }
    sysdep->core_sysdep_mutex_unlock(ota_handle->data_mutex);

    return res;
}

void *aiot_download_init(void)
{
    download_handle_t *download_handle = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;
    void *http_handle = NULL;
    uint32_t default_body_max_len = AIOT_DOWNLOAD_DEFAULT_BUFFER_LEN;
    uint32_t default_timeout_ms = AIOT_DOWNLOAD_DEFAULT_TIMEOUT_MS;

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return NULL;
    }

    download_handle = sysdep->core_sysdep_malloc(sizeof(download_handle_t), DOWNLOAD_MODULE_NAME);
    if (download_handle == NULL) {
        return NULL;
    }
    memset(download_handle, 0, sizeof(download_handle_t));
    download_handle->sysdep = sysdep;
    download_handle->data_mutex = sysdep->core_sysdep_mutex_init();
    download_handle->recv_mutex = sysdep->core_sysdep_mutex_init();

    http_handle = core_http_init();
    core_http_setopt(http_handle, CORE_HTTPOPT_RECV_HANDLER, _http_recv_handler);
    core_http_setopt(http_handle, CORE_HTTPOPT_USERDATA, (void *)download_handle);
    core_http_setopt(http_handle, CORE_HTTPOPT_BODY_BUFFER_MAX_LEN, (void *)&default_body_max_len);
    core_http_setopt(http_handle, CORE_HTTPOPT_RECV_TIMEOUT_MS, (void *)&default_timeout_ms);
    download_handle->http_handle = http_handle;

    return download_handle;
}

int32_t aiot_download_deinit(void **handle)
{
    int32_t res = STATE_SUCCESS;
    if (NULL == handle || NULL == *handle) {
        return STATE_DOWNLOAD_DEINIT_HANDLE_IS_NULL;
    }

    download_handle_t *download_handle = *(download_handle_t **)(handle);
    aiot_sysdep_portfile_t *sysdep = download_handle->sysdep;
    _ota_free_task_desc(sysdep, download_handle->task_desc);
    core_http_deinit(&(download_handle->http_handle));

    if (AIOT_OTA_DIGEST_SHA256 == download_handle->task_desc->digest_method) {
        if (NULL != download_handle->digest_ctx) {
            core_sha256_free(download_handle->digest_ctx);
            sysdep->core_sysdep_free(download_handle->digest_ctx);
        }
    } else if (AIOT_OTA_DIGEST_MD5 == download_handle->task_desc->digest_method) {
        if (NULL != download_handle->digest_ctx) {
            utils_md5_free(download_handle->digest_ctx);
            sysdep->core_sysdep_free(download_handle->digest_ctx);
        }
    }
    if (NULL != download_handle->task_desc) {
        sysdep->core_sysdep_free(download_handle->task_desc);
    }

    sysdep->core_sysdep_mutex_deinit(&(download_handle->data_mutex));
    sysdep->core_sysdep_mutex_deinit(&(download_handle->recv_mutex));
    sysdep->core_sysdep_free(download_handle);
    *handle = NULL;
    return res;
}

int32_t aiot_download_setopt(void *handle, aiot_download_option_t option, void *data)
{
    int32_t res = STATE_SUCCESS;
    download_handle_t *download_handle = (download_handle_t *)handle;

    if (download_handle == NULL) {
        return STATE_DOWNLOAD_SETOPT_HANDLE_IS_NULL;
    }
    if (NULL == data) {
        return STATE_DOWNLOAD_SETOPT_DATA_IS_NULL;
    }

    aiot_sysdep_portfile_t *sysdep = download_handle->sysdep;

    sysdep->core_sysdep_mutex_lock(download_handle->data_mutex);
    switch (option) {
        case AIOT_DLOPT_NETWORK_CRED: {
            core_http_setopt(download_handle->http_handle, CORE_HTTPOPT_NETWORK_CRED, data);
        }
        break;
        case AIOT_DLOPT_NETWORK_PORT: {
            core_http_setopt(download_handle->http_handle, CORE_HTTPOPT_PORT, data);
        }
        break;
        case AIOT_DLOPT_RECV_TIMEOUT_MS: {
            uint32_t *timeout_ms = (uint32_t *)data;
            void *http_handle = download_handle->http_handle;
            core_http_setopt(http_handle, CORE_HTTPOPT_RECV_TIMEOUT_MS, timeout_ms);
        }
        break;
        case AIOT_DLOPT_USER_DATA: {
            download_handle->userdata = data;
        }
        break;
        case AIOT_DLOPT_TASK_DESC: {
            void *new_task_desc = _download_deep_copy_task_desc(sysdep, data);
            if (NULL == new_task_desc) {
                return STATE_DOWNLOAD_SETOPT_COPIED_DATA_IS_NULL;
            }

            download_handle->task_desc = (aiot_download_task_desc_t *)new_task_desc;
            if (AIOT_OTA_DIGEST_SHA256 == download_handle->task_desc->digest_method) {
                core_sha256_context_t *ctx = sysdep->core_sysdep_malloc(sizeof(core_sha256_context_t), OTA_MODULE_NAME);
                core_sha256_init(ctx);
                core_sha256_starts(ctx);
                download_handle->digest_ctx = (void *) ctx;
            } else if (AIOT_OTA_DIGEST_MD5 == download_handle->task_desc->digest_method) {
                utils_md5_context_t *ctx = sysdep->core_sysdep_malloc(sizeof(utils_md5_context_t), OTA_MODULE_NAME);
                utils_md5_init(ctx);
                utils_md5_starts(ctx);
                download_handle->digest_ctx = (void *) ctx;
            }
            download_handle->download_status = DOWNLOAD_STATUS_START;
        }
        break;
        case  AIOT_DLOPT_RANGE_START: {
            download_handle->range_start = *(uint32_t *)data;
        }
        break;
        case  AIOT_DLOPT_RANGE_END: {
            download_handle->range_end = *(uint32_t *)data;
        }
        break;
        case AIOT_DLOPT_RECV_HANDLER: {
            download_handle->recv_handler = (aiot_download_recv_handler_t)data;
        }
        break;
        case AIOT_DLOPT_BODY_BUFFER_MAX_LEN: {
            core_http_setopt(download_handle->http_handle, CORE_HTTPOPT_BODY_BUFFER_MAX_LEN, data);
        }
        break;
        default: {
            res = STATE_USER_INPUT_OUT_RANGE;
        }
        break;
    }
    sysdep->core_sysdep_mutex_unlock(download_handle->data_mutex);
    return res;
}

int32_t aiot_ota_report_version(void *handle, char *version)
{
    int32_t res = STATE_SUCCESS;
    ota_handle_t *ota_handle = NULL;
    aiot_sysdep_portfile_t *sysdep;
    char *payload_string;
    char *product_key;
    char *device_name;
    ota_handle = (ota_handle_t *)handle;

    if (NULL == ota_handle) {
        return STATE_OTA_REPORT_HANDLE_IS_NULL;
    }

    if (NULL == version) {
        return STATE_OTA_REPORT_VERSION_IS_NULL;
    }

    core_mqtt_handle_t *mqtt_handle = ota_handle->mqtt_handle;
    if (NULL == mqtt_handle) {
        return STATE_OTA_REPORT_MQTT_HANDLE_IS_NULL;
    }
    product_key = mqtt_handle->product_key;
    device_name = mqtt_handle->device_name;

    sysdep = ota_handle->sysdep;
    {
        char *src[] = {"{\"version\":\"", version, "\"}"};
        uint8_t topic_len = sizeof(src) / sizeof(char *);
        core_sprintf(sysdep, &payload_string, "%s%s%s", src, topic_len, OTA_MODULE_NAME);
    }
    res = _ota_report_base(mqtt_handle, OTA_VERSION_TOPIC_PREFIX, product_key, device_name, payload_string);
    sysdep->core_sysdep_free(payload_string);
    return res;
}

int32_t aiot_ota_report_version_ext(void *handle, char *product_key, char *device_name, char *version)
{
    int32_t res = STATE_SUCCESS;
    ota_handle_t *ota_handle = NULL;
    aiot_sysdep_portfile_t *sysdep;
    char *payload_string;
    ota_handle = (ota_handle_t *)handle;

    if (NULL == ota_handle) {
        return STATE_OTA_REPORT_EXT_HANELD_IS_NULL;
    }

    if (NULL == version) {
        return STATE_OTA_REPORT_EXT_VERSION_NULL;
    }

    if (NULL == product_key) {
        return STATE_OTA_REPORT_EXT_PRODUCT_KEY_IS_NULL;
    }
    if (NULL == device_name) {
        return STATE_OTA_REPORT_EXT_DEVICE_NAME_IS_NULL;
    }

    core_mqtt_handle_t *mqtt_handle = ota_handle->mqtt_handle;
    if (NULL == mqtt_handle) {
        return STATE_OTA_REPORT_EXT_MQTT_HANDLE_IS_NULL;
    }

    sysdep = ota_handle->sysdep;
    {
        char *src[] = {"{\"version\":\"", version, "\"}"};
        uint8_t topic_len = sizeof(src) / sizeof(char *);
        core_sprintf(sysdep, &payload_string, "%s%s%s", src, topic_len, OTA_MODULE_NAME);
    }
    res = _ota_report_base(mqtt_handle, OTA_VERSION_TOPIC_PREFIX, product_key, device_name, payload_string);
    sysdep->core_sysdep_free(payload_string);
    return res;
}

int32_t aiot_download_report_progress(void *handle, int32_t percent)
{
    int32_t res = STATE_SUCCESS;
    download_handle_t *download_handle = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;
    char out_buffer[4] = {0};
    uint8_t out_len;
    char *payload_string;

    if (NULL == handle) {
        return STATE_DOWNLOAD_REPORT_HANDLE_IS_NULL;
    }

    download_handle = (download_handle_t *)handle;
    sysdep = download_handle->sysdep;

    if (NULL == download_handle->task_desc) {
        return STATE_DOWNLOAD_REPORT_TASK_DESC_IS_NULL;
    }

    core_int2str(percent, out_buffer, &out_len);
    {
        char *src[] = {"{\"step\":\"", out_buffer, "\",\"desc\":\"\"}"};
        uint8_t topic_len = sizeof(src) / sizeof(char *);
        core_sprintf(sysdep, &payload_string, "%s%s%s", src, topic_len, OTA_MODULE_NAME);
    }
    res = _ota_report_base(download_handle->task_desc->mqtt_handle, OTA_PROGRESS_TOPIC_PREFIX,
                           download_handle->task_desc->product_key,
                           download_handle->task_desc->device_name, payload_string);
    sysdep->core_sysdep_free(payload_string);
    return res;
}

int32_t aiot_download_recv(void *handle)
{
    int32_t res = STATE_SUCCESS;
    download_handle_t *download_handle = (download_handle_t *)handle;
    aiot_sysdep_portfile_t *sysdep = NULL;
    void *http_handle = NULL;

    if (NULL == download_handle) {
        return STATE_DOWNLOAD_RECV_HANDLE_IS_NULL;
    }
    http_handle = download_handle->http_handle;
    sysdep = download_handle->sysdep;

    sysdep->core_sysdep_mutex_lock(download_handle->recv_mutex);
    switch (download_handle->download_status) {
        case DOWNLOAD_STATUS_START: {
            aiot_download_setopt(download_handle, AIOT_DLOPT_RANGE_START, (void *) & (download_handle->size_fetched));
            res = aiot_download_send_request(download_handle);
        }
        break;
        case DOWNLOAD_STATUS_FETCH: {
            res = core_http_recv(http_handle);
            if (download_handle->size_fetched == download_handle->task_desc->size_total) {
                res = STATE_DOWNLOAD_FINISHED;
                break;
            }
            /* TODO: range_end碰到后,应该有个不同于STATE_DOWNLOAD_FINISHED的状态码*/
            if (res <= 0) {
                uint8_t res_string_len = 0;
                char res_string[OTA_MAX_DIGIT_NUM_OF_UINT32] = {0};
                core_int2str(res, res_string, &res_string_len);
                core_log1(download_handle->sysdep, STATE_DOWNLOAD_RECV_ERROR, "recv got %s\r\n",
                          &res_string);
                download_handle->download_status = DOWNLOAD_STATUS_START;
                core_log(download_handle->sysdep, STATE_OTA_RENEWAL, "renewal\r\n");
            }
        }
        break;
        default:
            break;
    }
    sysdep->core_sysdep_mutex_unlock(download_handle->recv_mutex);
    return res;
}

int32_t _ota_free_task_desc(aiot_sysdep_portfile_t *sysdep, void *data)
{
    int32_t res = STATE_SUCCESS;
    aiot_download_task_desc_t *task_desc = (aiot_download_task_desc_t *)data;

    if (NULL == task_desc) {
        return res;
    }

    if (NULL != task_desc->product_key) {
        sysdep->core_sysdep_free(task_desc->product_key);
    }
    if (NULL != task_desc->device_name) {
        sysdep->core_sysdep_free(task_desc->device_name);
    }
    if (NULL != task_desc->url) {
        sysdep->core_sysdep_free(task_desc->url);
    }
    if (NULL != task_desc->expect_digest) {
        sysdep->core_sysdep_free(task_desc->expect_digest);
    }
    if (NULL != task_desc->version) {
        sysdep->core_sysdep_free(task_desc->version);
    }
    return res;
}

int32_t _ota_prase_topic(aiot_sysdep_portfile_t *sysdep, char *topic, uint8_t topic_len, uint8_t *ota_type,
                         char **product_key,
                         char **device_name)
{
    char *_product_key, *_device_name, *temp;
    uint8_t _product_key_len, _device_name_len, temp_len;
    if (memcmp(topic, OTA_FOTA_TOPIC_PREFIX, strlen(OTA_FOTA_TOPIC_PREFIX)) == 0) {
        _product_key = topic + strlen(OTA_FOTA_TOPIC_PREFIX) + 1;
        temp = strstr((const char *)(_product_key), "/");
        _product_key_len = temp - _product_key;
        _device_name = temp + 1;
        _device_name_len = topic_len - (_device_name - topic);
        *ota_type = AIOT_OTARECV_FOTA;
    } else {
        _product_key = topic + strlen(OTA_COTA_TOPIC_PREFIX);
        temp = strstr((const char *)(_product_key), "/");
        _product_key_len = temp - _product_key;
        _device_name = temp + 1;
        temp = strstr((const char *)(_device_name), "/");
        _device_name_len = temp - _device_name;
        *ota_type = AIOT_OTARECV_COTA;
    }

    temp_len = _product_key_len + 1;
    temp = sysdep->core_sysdep_malloc(temp_len, OTA_MODULE_NAME);
    if (NULL == temp) {
        goto error_exit;
    }
    memset(temp, 0, temp_len);
    memcpy(temp, _product_key, _product_key_len);
    *product_key = temp;

    temp_len = _device_name_len + 1;
    temp = sysdep->core_sysdep_malloc(temp_len, OTA_MODULE_NAME);
    if (NULL == temp) {
        goto error_exit;
    }
    memset(temp, 0, temp_len);
    memcpy(temp, _device_name, _device_name_len);
    *device_name = temp;
    return STATE_SUCCESS;
error_exit:
    return STATE_SYS_DEPEND_MALLOC_FAILED;
}

void _ota_mqtt_process(void *handle, const aiot_mqtt_recv_t *const packet, void *userdata)
{
    int32_t res = STATE_SUCCESS;
    ota_handle_t *ota_handle = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;
    char *product_key = NULL;
    char *device_name = NULL;
    char *version = NULL;
    char *size_string = NULL;
    uint32_t size;
    char *url = NULL;
    char *digest_method_string = NULL;
    char *expect_digest = NULL;
    uint8_t ota_type;
    char *data = NULL;
    char *key = NULL;
    uint32_t data_len = 0;
    aiot_download_task_desc_t task_desc = {0};

    if (AIOT_MQTTRECV_PUB != packet->type) {
        return;
    }

    ota_handle = (ota_handle_t *)userdata;
    if (NULL == userdata) {
        return;
    }
    sysdep = ota_handle->sysdep;
    res = _ota_prase_topic(sysdep, packet->data.pub.topic, packet->data.pub.topic_len, &ota_type, &product_key,
                           &device_name);
    if (res != STATE_SUCCESS) {
        goto exit;
    }
    task_desc.product_key = product_key;
    task_desc.device_name = device_name;
    task_desc.mqtt_handle = ota_handle->mqtt_handle;
    key = "data";
    res = core_json_value((const char *)(packet->data.pub.payload), packet->data.pub.payload_len,
                          key, strlen(key), &data, &data_len);
    if (res != STATE_SUCCESS) {
        goto exit;
    }

    if (AIOT_OTARECV_FOTA == ota_type) {
        key = "size";
        res = _ota_parse_json(sysdep, data, data_len, key, &size_string);
        if (res != STATE_SUCCESS) {
            goto exit;
        }
        key = "version";
        res = _ota_parse_json(sysdep, data, data_len, key, &version);
        if (res != STATE_SUCCESS) {
            goto exit;
        }
        task_desc.version = version;
    } else {
        key = "configSize";
        res = _ota_parse_json(sysdep, data, data_len, key, &size_string);
        if (res != STATE_SUCCESS) {
            goto exit;
        }
    }
    core_str2uint(size_string, strlen(size_string), &size);
    task_desc.size_total = size;
    key = "url";
    res = _ota_parse_json(sysdep, data, data_len, key, &url);
    if (res != STATE_SUCCESS) {
        goto exit;
    }
    task_desc.url = url;

    key = "signMethod";
    res = _ota_parse_json(sysdep, data, data_len, key,
                          &digest_method_string);
    if (res != STATE_SUCCESS) {
        goto exit;
    }

    if (strcmp(digest_method_string, "SHA256") == 0 || strcmp(digest_method_string, "Sha256") == 0) {
        task_desc.digest_method = AIOT_OTA_DIGEST_SHA256;
    } else if (strcmp(digest_method_string, "Md5") == 0) {
        task_desc.digest_method = AIOT_OTA_DIGEST_MD5;
    } else {
        res = STATE_OTA_UNKNOWN_DIGEST_METHOD;
        goto exit;
    }

    key = "sign";
    res = _ota_parse_json(sysdep, data, data_len, key, &expect_digest);
    if (res != STATE_SUCCESS) {
        goto exit;
    }
    task_desc.expect_digest = expect_digest;

    aiot_ota_recv_t msg = {
        .type = ota_type,
        .task_desc = &task_desc
    };

    if (ota_handle->recv_handler) {
        ota_handle->recv_handler(ota_handle, &msg, NULL);
    }

exit:
    if (NULL != size_string) {
        sysdep->core_sysdep_free(size_string);
    }
    if (NULL != digest_method_string) {
        sysdep->core_sysdep_free(digest_method_string);
    }
    if (res != STATE_SUCCESS) {
        core_log(sysdep, res, key);
    }
    _ota_free_task_desc(sysdep, (void *)(&task_desc));
}

static int32_t _ota_parse_url(const char *url, char *host, char *path)
{
    char *host_ptr = (char *) strstr(url, "://");
    uint32_t host_len = 0;
    uint32_t path_len;
    char *path_ptr;
    char *fragment_ptr;

    if (host_ptr == NULL) {
        return 0;
    }
    host_ptr += 3;

    path_ptr = strchr(host_ptr, '/');
    if (NULL == path_ptr) {
        return 0;
    }

    if (host_len == 0) {
        host_len = path_ptr - host_ptr;
    }

    memcpy(host, host_ptr, host_len);
    host[host_len] = '\0';
    fragment_ptr = strchr(host_ptr, '#');
    if (fragment_ptr != NULL) {
        path_len = fragment_ptr - path_ptr;
    } else {
        path_len = strlen(path_ptr);
    }

    memcpy(path, path_ptr, path_len);
    path[path_len] = '\0';
    return 0;
}

int32_t aiot_download_send_request(void *handle)
{
    int32_t res = STATE_SUCCESS;
    char host[OTA_HTTPCLIENT_MAX_URL_LEN] = { 0 };
    char path[OTA_HTTPCLIENT_MAX_URL_LEN] = { 0 };
    char *header_string = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;
    download_handle_t *download_handle = (download_handle_t *)handle;
    if (NULL == download_handle) {
        return STATE_DOWNLOAD_REQUEST_HANDLE_IS_NULL;
    }
    if (NULL == download_handle->task_desc) {
        return STATE_DOWNLOAD_REQUEST_TASK_DESC_IS_NULL;
    }
    if (NULL == download_handle->task_desc->url) {
        return STATE_DOWNLOAD_REQUEST_URL_IS_NULL;
    }
    sysdep = download_handle->sysdep;

    {
        uint32_t range_start = download_handle->range_start;
        uint32_t range_end = download_handle->range_end;
        uint8_t range_start_string_len = 0;
        uint8_t range_end_string_len = 0;
        char range_start_string[OTA_MAX_DIGIT_NUM_OF_UINT32] = {0};
        char range_end_string[OTA_MAX_DIGIT_NUM_OF_UINT32] = {0};
        core_int2str(range_start, range_start_string, &range_start_string_len);

        if (0 != range_end) {
            if (download_handle->size_fetched >= range_end) {
                return STATE_DOWNLOAD_GOT_RANGE_END;
            }
            core_int2str(range_end, range_end_string, &range_end_string_len);
        }

        {
            char *prefix = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nRange: bytes=";
            char *src[] = { prefix, range_start_string, "-", range_end_string};
            uint32_t len = sizeof(src) / sizeof(char *);
            res = core_sprintf(sysdep, &header_string, "%s%s%s%s\r\n", src, len, OTA_MODULE_NAME);
            if (res != STATE_SUCCESS) {
                return res;
            }
        }
    }

    _ota_parse_url(download_handle->task_desc->url, host, path);
    core_http_setopt(download_handle->http_handle, CORE_HTTPOPT_HOST, host);
    core_http_request_t request = {
        .method = "GET",
        .path = path,
        .header = header_string,
        .content = NULL,
        .content_len = 0
    };
    res = core_http_connect(download_handle->http_handle);
    if (res != STATE_SUCCESS) {
        return res;
    }
    res = core_http_send(download_handle->http_handle, &request);
    sysdep->core_sysdep_free(header_string);
    /* core_http_send 返回的是发送的body的长度; 错误返回负数 */
    if (res < STATE_SUCCESS) {
        download_handle->download_status = DOWNLOAD_STATUS_START;
        res = STATE_DOWNLOAD_SEND_REQUEST_FAILED;
    } else {
        download_handle->download_status = DOWNLOAD_STATUS_FETCH;
        aiot_download_report_progress(download_handle, download_handle->percent);
        res = STATE_SUCCESS;
    }
    return res;
}

static int32_t _download_update_digest(download_handle_t *download_handle, uint8_t *buffer, uint32_t buffer_len)
{
    int32_t res = STATE_SUCCESS;
    if (AIOT_OTA_DIGEST_SHA256 == download_handle->task_desc->digest_method) {
        core_sha256_update(download_handle->digest_ctx, buffer, buffer_len);
    } else if (AIOT_OTA_DIGEST_MD5 == download_handle->task_desc->digest_method) {
        res = utils_md5_update(download_handle->digest_ctx, buffer, buffer_len);
    }
    return res;
}

static int32_t _download_verify_digest(download_handle_t *download_handle)
{
    uint8_t output[32] = {0};
    uint8_t expected_digest[32] = {0};

    if (AIOT_OTA_DIGEST_SHA256 == download_handle->task_desc->digest_method) {
        core_str2hex(download_handle->task_desc->expect_digest, 64, expected_digest);
        core_sha256_finish(download_handle->digest_ctx, output);
        if (memcmp(output, expected_digest, 32) == 0) {
            return STATE_SUCCESS;
        }
    } else if (AIOT_OTA_DIGEST_MD5 == download_handle->task_desc->digest_method) {
        core_str2hex(download_handle->task_desc->expect_digest, 32, expected_digest);
        utils_md5_finish(download_handle->digest_ctx, output);
        if (memcmp(output, expected_digest, 16) == 0) {
            return STATE_SUCCESS;
        }
    }
    return STATE_OTA_DIGEST_MISMATCH;
}

void _http_recv_handler(void *handle, const aiot_http_recv_t *packet, void *userdata)
{
    download_handle_t *download_handle = (download_handle_t *)userdata;
    switch (packet->type) {
        case AIOT_HTTPRECV_STATUS_CODE : {
            /* 获取HTTP响应的状态码 */
        }
        break;
        case AIOT_HTTPRECV_HEADER: {
            /* 获取HTTP首部，每次返回一对键值对 */
            /* TODO:解析出content-length */
            /* TODO:与mqtt下行报文的content-len进行校验,如果不一致则报错 */
        }
        break;
        case AIOT_HTTPRECV_BODY: {
            int32_t percent = 0;
            aiot_download_recv_t recv_data = {
                .type = AIOT_DLRECV_HTTPBODY,
                .data = {
                    .buffer = packet->data.body.buffer,
                    .len = packet->data.body.len
                }
            };

            download_handle->size_fetched += packet->data.body.len;
            percent = (100 * download_handle->size_fetched) / download_handle->task_desc->size_total;
            _download_update_digest(download_handle, packet->data.body.buffer, packet->data.body.len);

            if (download_handle->size_fetched == download_handle->task_desc->size_total) {
                int32_t ret = _download_verify_digest(download_handle);
                if (ret != STATE_SUCCESS) {
                    percent = AIOT_OTAERR_CHECKSUM_MISMATCH;
                    core_log(download_handle->sysdep, ret, "digest mismatch\r\n");
                    aiot_download_report_progress(download_handle, AIOT_OTAERR_CHECKSUM_MISMATCH);
                } else {
                    core_log(download_handle->sysdep, STATE_OTA_DIGEST_MATCH, "digest matched\r\n");
                }
            }
            download_handle->percent = percent;
            if (NULL != download_handle->recv_handler) {
                download_handle->recv_handler(download_handle, percent, &recv_data, download_handle->userdata);
            }
        }
        break;
        default:
            break;
    }
}

static void _download_deep_copy_base(aiot_sysdep_portfile_t *sysdep, char *in, char **out)
{
    uint8_t len = 0;
    void *temp = NULL;
    if (NULL == in) {
        return;
    }
    len = strlen(in) + 1;
    temp = (aiot_download_task_desc_t *)sysdep->core_sysdep_malloc(len,
            DOWNLOAD_MODULE_NAME);
    if (NULL == temp) {
        return;
    }
    memset(temp, 0, len);
    memcpy(temp, in, strlen(in));
    *out = temp;
}

static void *_download_deep_copy_task_desc(aiot_sysdep_portfile_t *sysdep, void *data)
{
    aiot_download_task_desc_t *src_task_desc = (aiot_download_task_desc_t *)data;
    aiot_download_task_desc_t *dst_task_desc = NULL;

    dst_task_desc = (aiot_download_task_desc_t *)sysdep->core_sysdep_malloc(sizeof(aiot_download_task_desc_t),
                    DOWNLOAD_MODULE_NAME);
    if (NULL == dst_task_desc) {
        return NULL;
    }
    memset(dst_task_desc, 0, sizeof(aiot_download_task_desc_t));

    dst_task_desc->size_total = src_task_desc->size_total;
    dst_task_desc->digest_method = src_task_desc->digest_method;
    dst_task_desc->mqtt_handle = src_task_desc->mqtt_handle;
    _download_deep_copy_base(sysdep, src_task_desc->product_key, &(dst_task_desc->product_key));
    _download_deep_copy_base(sysdep, src_task_desc->device_name, &(dst_task_desc->device_name));
    _download_deep_copy_base(sysdep, src_task_desc->url, &(dst_task_desc->url));
    _download_deep_copy_base(sysdep, src_task_desc->version, &(dst_task_desc->version));
    _download_deep_copy_base(sysdep, src_task_desc->expect_digest, &(dst_task_desc->expect_digest));
    return dst_task_desc;
}

static int32_t _ota_report_base(void *handle, char *topic_prefix, char *product_key, char *device_name,  char *params)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;
    char *topic_string = NULL;
    char *payload_string = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;

    if (NULL == product_key || NULL == device_name || NULL == mqtt_handle) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    sysdep = mqtt_handle->sysdep;
    {
        char *src[] = { topic_prefix, product_key, device_name };
        uint8_t topic_len = sizeof(src) / sizeof(char *);
        res = core_sprintf(sysdep, &topic_string, "%s/%s/%s", src, topic_len, OTA_MODULE_NAME);
        if (res != STATE_SUCCESS) {
            goto exit;
        }
    }

    {
        int32_t alink_id;
        uint8_t alink_id_string_len;
        char alink_id_string[OTA_MAX_DIGIT_NUM_OF_UINT32] = {0};
        res = core_global_alink_id_next(sysdep, &alink_id);
        if (res != STATE_SUCCESS) {
            goto exit;
        }
        core_int2str(alink_id, alink_id_string, &alink_id_string_len);

        {
            char *src[] = {
                "{\"id\":", alink_id_string, ", \"params\":", params, "}"
            };
            uint8_t topic_len = sizeof(src) / sizeof(char *);

            res = core_sprintf(sysdep, &payload_string, "%s%s%s%s%s", src, topic_len, OTA_MODULE_NAME);
            if (res != STATE_SUCCESS) {
                goto exit;
            }
        }
    }

    aiot_mqtt_buff_t topic_buff = {
        .buffer = (uint8_t *)topic_string,
        .len = (uint32_t)strlen(topic_string)
    };

    aiot_mqtt_buff_t payload_buff = {
        .buffer = (uint8_t *)payload_string,
        .len = (uint32_t)strlen(payload_string)
    };

    res = aiot_mqtt_pub(mqtt_handle, &topic_buff, &payload_buff, 0);

exit:
    if (NULL != topic_string) {
        sysdep->core_sysdep_free(topic_string);
    }
    if (NULL != payload_string) {
        sysdep->core_sysdep_free(payload_string);
    }

    if (res != STATE_SUCCESS) {
        core_log1(sysdep, STATE_OTA_REPORT_FAILED, "%d\r\n", (void *)(&res));
    }
    return res;
}

static int32_t _ota_subscribe(core_mqtt_handle_t *mqtt_handle, void *ota_handle)
{
    int32_t res = STATE_SUCCESS;
    char *topic[3] = { OTA_FOTA_TOPIC, OTA_COTA_PUSH_TOPIC, OTA_COTA_GET_REPLY_TOPIC };
    uint8_t len = sizeof(topic) / sizeof(char *);
    uint8_t count = 0;

    for (count = 0; count < len; count++) {
        char *topic_string = topic[count];
        aiot_mqtt_buff_t topic = {(uint8_t *)topic_string, (uint32_t)strlen(topic_string)};
        aiot_mqtt_topic_map_t topic_map = {topic, _ota_mqtt_process, (void *)ota_handle};
        res = aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_APPEND_TOPIC_MAP, (void *)&topic_map);
    }
    return res;
}

static int32_t _ota_parse_json(aiot_sysdep_portfile_t *sysdep, void *input, uint32_t input_len, char *key_word,
                               char **out)
{
    int32_t res = STATE_SUCCESS;
    char *value = NULL, *buffer = NULL;
    uint32_t value_len = 0, buffer_len = 0;

    res = core_json_value((const char *)input, input_len, key_word, strlen(key_word), &value, &value_len);
    if (res != STATE_SUCCESS) {
        goto error_exit;
    }

    buffer_len = value_len + 1;
    buffer = sysdep->core_sysdep_malloc(buffer_len, OTA_MODULE_NAME);
    if (NULL == buffer) {
        goto error_exit;
    }
    memset(buffer, 0, buffer_len);
    memcpy(buffer, value, value_len);
    *out = buffer;
    return res;

error_exit:
    return STATE_OTA_PARSE_JSON_ERROR;
}

