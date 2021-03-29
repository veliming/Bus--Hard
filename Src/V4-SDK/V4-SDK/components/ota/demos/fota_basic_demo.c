#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_ota_api.h"

char *product_key       = "a1Zd7n5yTt8";
char *device_name       = "aiot_sha256_t3";
char *device_secret     = "KB8KmjRyqMe14J3DFClsGTygg7d6aUwO";

extern const char *ali_ca_crt;
void *g_ota_handle = NULL;
void *g_dl_handle = NULL;
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

    if (percent < 0) {
        /* digest校验错误 */
        printf("exception happend, percent is %d\r\n", percent);
        return;
    }

    /* 烧写到固件 */
    /*
     * TODO: 下载一段固件成功, 这个时候, 用户应该将
     *       起始地址为 packet->data.buffer, 长度为 packet->data.len 的内存, 保存到flash上
     *       如果烧写失败, 需要调用aiot_download_report_progress(handle, AIOT_OTAERR_BURN_FAILED)
     *       将错误信息上报给云平台.可以在把错误信息记录到一个全局变量,下次再进入这个函数时读取一下,
     *       用以决定是否继续烧写等策略
     *
     */

    if (percent == 100) {
        /* 上报版本号 */
        /*
         * TODO: 这个时候, 一般用户就应该完成所有的固件烧录, 保存当前工作, 重启设备, 切换到新的固件上启动了
         *       并且, 新的固件必须要以
         *
         *       aiot_ota_report_version(g_ota_handle, new_version);
         *
         *       这样的操作, 将升级后的新版本号(比如1.0.0升到1.1.0, 则new_version的值是"1.1.0")上报给云平台
         *       云平台收到了新的版本号上报后, 才会判定升级成功, 否则会认为本次升级失败了
         *
         */
    }

    printf("current percent is %d\r\n", percent);
}

void user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            uint32_t res = 0;
            uint16_t port = 443;
            uint32_t max_buffer_len = 2048;
            aiot_sysdep_network_cred_t cred;
            void *dl_handle = aiot_download_init();

            if (NULL == dl_handle || NULL == ota_msg->task_desc) {
                return;
            }

            printf("OTA target firmware version: %s, size: %u Bytes\r\n", ota_msg->task_desc->version,
                   ota_msg->task_desc->size_total);

            memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
            cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_RSA;
            cred.max_tls_fragment = 16384;
            cred.x509_server_cert = ali_ca_crt;
            cred.x509_server_cert_len = strlen(ali_ca_crt);
            /* 设置下载方式, 是否为TLS下载 */
            res = aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_CRED, (void *)(&cred));
            if (res != STATE_SUCCESS) {
                goto exit;
            }
            /* 设置下载方式配套的端口号 */
            res = aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
            if (res != STATE_SUCCESS) {
                goto exit;
            }
            res = aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
            if (res != STATE_SUCCESS) {
                goto exit;
            }
            /* 设置用户处理http下行报文的回调函数 */
            res = aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(user_download_recv_handler));
            if (res != STATE_SUCCESS) {
                goto exit;
            }
            /* 设置单次下载最大的buffer长度,长度满了后会知会用户 */
            res = aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len));
            if (res != STATE_SUCCESS) {
                goto exit;
            }

            /* 发送http的GET请求给http服务器 */
            res = aiot_download_send_request(dl_handle);
            if (res != STATE_SUCCESS) {
                goto exit;
            }
            g_dl_handle = dl_handle;
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
    char *cur_version = NULL;
    void *ota_handle = NULL;
    uint32_t timeout_ms = 0;

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
    g_ota_handle = ota_handle;

    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_connect failed: -0x%04X\r\n", -res);
        goto exit;
    }

    /*   TODO: 非常重要!!!
     *
     *   cur_version 要根据用户实际情况, 改成从设备的配置区获取, 要反映真实的版本号, 而不能像示例这样写为固定值
     *
     *   1. 如果设备从未上报过版本号, 在控制台网页将无法部署升级任务
     *   2. 如果设备升级完成后, 上报的不是新的版本号, 在控制台网页将会显示升级失败
     *
     */

    cur_version = "1.0.0";
    /* 上报当前设备的版本号 */
    res = aiot_ota_report_version(ota_handle, cur_version);
    if (res < STATE_SUCCESS) {
        printf("report version failed, code is -0x%04X\r\n", -res);
    }

    while (1) {
        aiot_mqtt_process(mqtt_handle);
        res = aiot_mqtt_recv(mqtt_handle);

        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        if (NULL != g_dl_handle) {
            timeout_ms = 100;
            int32_t ret = aiot_download_recv(g_dl_handle);

            if (STATE_DOWNLOAD_FINISHED == ret) {
                aiot_download_deinit(&g_dl_handle);
                timeout_ms = 5000;
            }
            aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);
        }
    }

    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_disconnect failed: -0x%04X\r\n", -res);
        goto exit;
    }

exit:
    printf("exit: res is %d\r\n", res);
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


