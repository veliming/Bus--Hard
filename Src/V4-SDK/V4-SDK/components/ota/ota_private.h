#ifndef _OTA_H_
#define _OTA_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define OTA_VERSION_TOPIC_PREFIX             "/ota/device/inform"
#define OTA_PROGRESS_TOPIC_PREFIX             "/ota/device/progress"

#define OTA_MODULE_NAME                      "OTA"
#define DOWNLOAD_MODULE_NAME                 "DOWNLOAD"

#define OTA_FOTA_TOPIC                       "/ota/device/upgrade/+/+"
#define OTA_FOTA_TOPIC_PREFIX                "/ota/device/upgrade"
#define OTA_COTA_PUSH_TOPIC                  "/sys/+/+/thing/config/push"
#define OTA_COTA_PUSH_TOPIC_POSTFIX          "/thing/config/push"
#define OTA_COTA_GET_REPLY_TOPIC             "/sys/+/+/thing/config/get_reply"
#define OTA_COTA_GET_REPLY_TOPIC_POSTFIX     "/thing/config/get_reply"
#define OTA_COTA_TOPIC_PREFIX                "/sys/"

#define OTA_HTTPCLIENT_MAX_URL_LEN           (256)
#define OTA_MAX_DIGIT_NUM_OF_UINT32          (20)

typedef enum {
    DOWNLOAD_STATUS_START,
    DOWNLOAD_STATUS_FETCH,
    DOWNLOAD_STATUS_RENEWAL,
} download_status_t;

#if defined(__cplusplus)
}
#endif

#endif


