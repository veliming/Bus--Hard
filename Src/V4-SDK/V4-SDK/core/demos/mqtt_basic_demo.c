/*
 * 这个例程适用于`Linux`这类支持pthread的POSIX设备, 它演示了用SDK配置MQTT参数并建立连接, 之后创建2个线程
 *
 * + 一个线程用于保活长连接
 * + 一个线程用于接收消息, 并在有消息到达时进入默认的数据回调, 在连接状态变化时进入事件回调
 *
 * 需要用户关注或修改的部分, 已经用 TODO 在注释中标明
 *
 */
#include <stdio.h>
#include <string.h>

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
void user_send_data_with_delay(char *buffer);

/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1577589489.033][LK-0317] mqtt_basic_demo&a13FN5TplKq
 *
 * 上面这条日志的code就是0317(十六进制), code值的定义见core/aiot_state_api.h
 *
 */

/* 日志回调函数, SDK的日志会从这里输出 */
int32_t demo_state_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
    /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
    case AIOT_MQTTEVT_CONNECT: {
        printf("AIOT_MQTTEVT_CONNECT\n");
        /* TODO: 处理SDK建连成功, 不可以在这里调用耗时较长的阻塞函数 */
    }
    break;

    /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
    case AIOT_MQTTEVT_RECONNECT: {
        printf("AIOT_MQTTEVT_RECONNECT\n");
        /* TODO: 处理SDK重连成功, 不可以在这里调用耗时较长的阻塞函数 */
    }
    break;

    /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
    case AIOT_MQTTEVT_DISCONNECT: {
        char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                      ("heartbeat disconnect");
        printf("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
        /* TODO: 处理SDK被动断连, 不可以在这里调用耗时较长的阻塞函数 */
    }
    break;

    default: {

    }
    }
}

long unsigned int in_cnt=0;
long unsigned int out_cnt=0;

/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
    case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
        printf("heartbeat response\n");
        /* TODO: 处理服务器对心跳的回应, 一般不处理 */
    }
    break;

    case AIOT_MQTTRECV_SUB_ACK: {
        printf("suback, res: -0x%04X, packet id: %d, max qos: %d\n",
               -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
        /* TODO: 处理服务器对订阅请求的回应, 一般不处理 */
    }
    break;

    case AIOT_MQTTRECV_PUB: {
        /* 由于N720目前只用到了一个串口(用于和班子进行at命令通信),因此无法将串口日志重定向到该串口,只能通过打断点后看dbg数组中的内容的方式 */
        {
            in_cnt++;
            if( packet->data.pub.payload != NULL && packet->data.pub.payload_len != 0 ) {
                 printf("payload is %.*s\n", packet->data.pub.payload_len, packet->data.pub.payload);
			}
        }
    }
    break;

    case AIOT_MQTTRECV_PUB_ACK: {
        printf("puback, packet id: %d\n", packet->data.pub_ack.packet_id);
        /* TODO: 处理服务器对QoS1上报消息的回应, 一般不处理 */
    }
    break;

    default: {

    }
    }
}


extern int n720_check_mqttonline();

int do_sub(void *mqtt_handle) {
    int res = 0;
    /* MQTT 订阅topic功能示例, 请根据自己的业务需求进行使用 */
    {
        char *sub_topic = "/sys/a1eICwwUmCt/load_1_dev/thing/service/property/set";
        aiot_mqtt_buff_t sub_topic_buff = {
            .buffer = (uint8_t *)sub_topic,
            .len = (uint32_t)strlen(sub_topic)
        };
        res = aiot_mqtt_sub(mqtt_handle, &sub_topic_buff, NULL, 1, NULL);
        return res;
    }
}

int do_unsub(void *mqtt_handle) {
    int res = 0;
    /* MQTT 订阅topic功能示例, 请根据自己的业务需求进行使用 */
    {
        char *sub_topic = "/sys/a1eICwwUmCt/load_1_dev/thing/service/property/set";
        aiot_mqtt_buff_t sub_topic_buff = {
            .buffer = (uint8_t *)sub_topic,
            .len = (uint32_t)strlen(sub_topic)
        };
        res = aiot_mqtt_unsub(mqtt_handle, &sub_topic_buff);
        return res;
    }
}


int32_t core_uint2str(uint32_t input, char *output, uint8_t *output_len);
int mqtt_demo()
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL;
    char       *url = "iot-as-mqtt.cn-shanghai.aliyuncs.com"; /* 阿里云平台上海站点的域名后缀 */
    char        host[100] = {0}; /* 用这个数组拼接设备连接的云平台站点全地址, 规则是 ${productKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com */
    uint16_t    port = 443;      /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */

    /* TODO: 替换为自己设备的三元组 */
    char *product_key       = "a1eICwwUmCt";
    char *device_name       = "load_1_dev";
    char *device_secret     = "oo2RSkYBcUAPSzMkbSva0GVP37nNJGFJ";

    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(demo_state_logcb);


    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        printf("aiot_mqtt_init failed\n");
        return -1;
    }

    /* TODO: 如果以下代码不被注释, 则例程会用TCP而不是TLS连接云平台 */
    /*
    {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }
    */

    snprintf(host, 100, "%s.%s", product_key, url);
    /* 配置MQTT服务器地址 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)host);
    /* 配置MQTT服务器端口 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /* 配置设备productKey */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    /* 配置设备deviceName */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /* 配置设备deviceSecret */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /* 配置MQTT默认消息接收回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    /* 配置MQTT事件回调函数 */
    // aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /* first disconn */
    res = aiot_mqtt_disconnect(mqtt_handle);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(mqtt_handle);

    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_connect failed: -0x%04X\n", -res);
        return -1;
    }


    while(n720_check_mqttonline() != 0) {
        aiot_mqtt_disconnect(mqtt_handle);
        aiot_mqtt_connect(mqtt_handle);
    }

    do_sub(mqtt_handle);

   /* 主循环进入休眠 */
    while (1) {
        osDelay(1200);
        aiot_mqtt_recv(mqtt_handle);
        aiot_mqtt_process(mqtt_handle);

        extern int g_rx_error, g_tx_error, g_rx_other_error;
        if(g_tx_error || g_rx_error || g_rx_other_error) {
            printf("happedn rx error, %d, rx other %d, tx %d\n", g_rx_error, g_rx_other_error, g_tx_error);
        }

        uint32_t remain = xPortGetFreeHeapSize();
        char out_mem[64] = {0};
        uint32_t str_len = 0;
        core_uint2str(remain, out_mem, &str_len);

#if 1
        if(out_cnt < in_cnt)
        {
#if 1
            char *pub_topic = "/sys/a1eICwwUmCt/load_1_dev/thing/event/property/post";
            //char *pub_payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";
            char *pub_payload = out_mem;
            aiot_mqtt_buff_t pub_topic_buff = {
                .buffer = (uint8_t *)pub_topic,
                .len = (uint32_t)strlen(pub_topic)
            };
            aiot_mqtt_buff_t pub_payload_buff = {
                .buffer = (uint8_t *)pub_payload,
                .len = (uint32_t)strlen(pub_payload)
            };
            int res = aiot_mqtt_pub(mqtt_handle, &pub_topic_buff, &pub_payload_buff, 0);

#endif
            out_cnt++;
        } else {
#if 1
            char *pub_topic = "/a1eICwwUmCt/load_1_dev/user/update";
            char *pub_payload = out_mem; //"hello:1" ;
            //char *pub_payload = "keepalive:1" ;
            aiot_mqtt_buff_t pub_topic_buff = {
                .buffer = (uint8_t *)pub_topic,
                .len = (uint32_t)strlen(pub_topic)
            };
            aiot_mqtt_buff_t pub_payload_buff = {
                .buffer = (uint8_t *)pub_payload,
                .len = (uint32_t)strlen(pub_payload)
            };
            int res = aiot_mqtt_pub(mqtt_handle, &pub_topic_buff, &pub_payload_buff, 0);
#endif
        }
#endif

    }

    /* 断开MQTT连接, 一般不会运行到这里 */
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_disconnect failed: -0x%04X\n", -res);
        return -1;
    }

    /* 销毁MQTT实例, 一般不会运行到这里 */
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_deinit failed: -0x%04X\n", -res);
        return -1;
    }

    return 0;
}

