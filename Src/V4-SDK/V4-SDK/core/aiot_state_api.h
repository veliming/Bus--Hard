/**
 * @file aiot_state_api.h
 * @brief SDK Core状态码头文件, 所有Core中的api返回值均在此列出
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 */

#ifndef _AIOT_STATE_API_H_
#define _AIOT_STATE_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief SDK的日志信息输出回调函数原型
 */
typedef int32_t (* aiot_state_logcb_t)(int32_t code, char *message);

/**
 * @brief 设置SDK的日志信息输出使用的回调函数
 *
 * @param handler 日志回调函数
 *
 * @return int32_t 保留
 */
int32_t aiot_state_set_logcb(aiot_state_logcb_t handler);

/**
 * @brief SDK状态码基准值
 *
 */
#define STATE_BASE                                                   (0x0000)

/**
 * @brief API执行成功
 *
 */
#define STATE_SUCCESS                                                (STATE_BASE - 0x0000)

/**
 * @brief SDK状态码(用户输入部分)基准值
 *
 */
#define STATE_USER_INPUT_BASE                                        (-0x0100)

/**
 * @brief 用户输入参数中包含非法的空指针
 *
 */
#define STATE_USER_INPUT_NULL_POINTER                                (STATE_USER_INPUT_BASE - 0x0001)

/**
 * @brief 用户输入参数中包含越界的值
 *
 */
#define STATE_USER_INPUT_OUT_RANGE                                   (STATE_USER_INPUT_BASE - 0x0002)

/**
 * @brief 用户输入的配置项超出配置项取值范围
 *
 */
#define STATE_USER_INPUT_UNKNOWN_OPTION                              (STATE_USER_INPUT_BASE - 0x0003)

/**
 * @brief 用户输入参数中缺少product key
 *
 */
#define STATE_USER_INPUT_MISSING_PRODUCT_KEY                         (STATE_USER_INPUT_BASE - 0x0004)

/**
 * @brief 用户输入参数中缺少device name
 *
 */
#define STATE_USER_INPUT_MISSING_DEVICE_NAME                         (STATE_USER_INPUT_BASE - 0x0005)

/**
 * @brief 用户输入参数中缺少device secret
 *
 */
#define STATE_USER_INPUT_MISSING_DEVICE_SECRET                       (STATE_USER_INPUT_BASE - 0x0006)

/**
 * @brief 用户输入参数中缺少product secret
 *
 */
#define STATE_USER_INPUT_MISSING_PRODUCT_SECRET                      (STATE_USER_INPUT_BASE - 0x0007)

/**
 * @brief 用户输入参数中缺少域名地址或ip地址
 *
 */
#define STATE_USER_INPUT_MISSING_HOST                                (STATE_USER_INPUT_BASE - 0x0008)

/**
 * @brief 用户已调用实例销毁函数销毁实例(如@ref aiot_mqtt_deinit), 其余对该实例操作的API不应该在进行调用
 *
 */
#define STATE_USER_INPUT_EXEC_DISABLED                               (STATE_USER_INPUT_BASE - 0x0009)

/**
 * @brief JSON解析失败
 *
 */
#define STATE_USER_INPUT_JSON_PARSE_FAILED                           (STATE_USER_INPUT_BASE - 0x000A)

/**
 * @brief SDK状态码(系统依赖部分, SDK内部使用)基准值
 *
 */
#define STATE_SYS_DEPEND_BASE                                        (-0x0200)

/**
 * @brief 内存分配失败
 *
 */
#define STATE_SYS_DEPEND_MALLOC_FAILED                               (STATE_SYS_DEPEND_BASE - 0x0001)

/**
 * @brief @ref aiot_sysdep_portfile_t::core_sysdep_network_setopt 的入参非法
 *
 */
#define STATE_SYS_DEPEND_NWK_INVALID_OPTION                          (STATE_SYS_DEPEND_BASE - 0x0002)

/**
 * @brief @ref aiot_sysdep_portfile_t::core_sysdep_network_establish 建立网络失败
 *
 */
#define STATE_SYS_DEPEND_NWK_EST_FAILED                              (STATE_SYS_DEPEND_BASE - 0x0003)

/**
 * @brief SDK检测到网络已断开
 *
 */
#define STATE_SYS_DEPEND_NWK_CLOSED                                  (STATE_SYS_DEPEND_BASE - 0x0004)

/**
 * @brief SDK从网络上实际读取的数据比期望读取的少
 *
 */
#define STATE_SYS_DEPEND_NWK_READ_LESSDATA                           (STATE_SYS_DEPEND_BASE - 0x0005)

/**
 * @brief SDK向网络上写入的实际数据比期望写入的少
 *
 */
#define STATE_SYS_DEPEND_NWK_WRITE_LESSDATA                          (STATE_SYS_DEPEND_BASE - 0x0006)

/**
 * @brief
 *
 */
#define STATE_SYS_DEPEND_NWK_READ_OVERTIME                           (STATE_SYS_DEPEND_BASE - 0x0007)

/**
 * @brief SDK状态码(MQTT模块)基准值
 *
 */
#define STATE_MQTT_BASE                                              (-0x0300)

/**
 * @brief MQTT尝试建立连接时从服务端返回的CONNACK报文格式错误
 *
 */
#define STATE_MQTT_CONNACK_FMT_ERROR                                 (STATE_MQTT_BASE - 0x0001)

/**
 * @brief 阿里云物联网平台MQTT服务端与当前客户端使用的MQTT协议版本不兼容
 */
#define STATE_MQTT_CONNACK_RCODE_UNACCEPTABLE_PROTOCOL_VERSION       (STATE_MQTT_BASE - 0x0002)

/**
 * @brief 阿里云物联网平台MQTT服务端暂时不可用, clienid取值不正确
 */
#define STATE_MQTT_CONNACK_RCODE_SERVER_UNAVAILABLE                  (STATE_MQTT_BASE - 0x0003)

/**
 * @brief 连接阿里云物联网平台MQTT服务端的用户名密码不合法
 *
 */
#define STATE_MQTT_CONNACK_RCODE_BAD_USERNAME_PASSWORD               (STATE_MQTT_BASE - 0x0004)

/**
 * @brief 阿里云物联网平台MQTT服务端认证失败, 用户名或者密码错误, 一般为三元组错误
 *
 */
#define STATE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED                      (STATE_MQTT_BASE - 0x0005)

/**
 * @brief 未知的CONNACK报文状态码
 *
 */
#define STATE_MQTT_CONNACK_RCODE_UNKNOWN                             (STATE_MQTT_BASE - 0x0006)

/**
 * @brief 在尝试缓存QoS1消息时, 检测到MQTT packet id发生卷绕
 *
 */
#define STATE_MQTT_PUBLIST_PACKET_ID_ROLL                            (STATE_MQTT_BASE - 0x0007)

/**
 * @brief topic格式不符合MQTT协议
 *
 */
#define STATE_MQTT_TOPIC_INVALID                                     (STATE_MQTT_BASE - 0x0008)
#define STATE_MQTT_LOG_TOPIC                                         (STATE_MQTT_BASE - 0x0009)
#define STATE_MQTT_LOG_HEXDUMP                                       (STATE_MQTT_BASE - 0x000A)
/**
 * @brief MQTT连接建立成功
 *
 */
#define STATE_MQTT_CONNECT_SUCCESS                                   (STATE_MQTT_BASE - 0x000B)

/**
 * @brief 读取到的MQTT数据包不符合MQTT协议
 *
 */
#define STATE_MQTT_MALFORMED_REMAINING_LEN                           (STATE_MQTT_BASE - 0x000C)

/**
 * @brief 读取到的MQTT数据包不符合MQTT协议
 *
 */
#define STATE_MQTT_MALFORMED_REMAINING_BYTES                         (STATE_MQTT_BASE - 0x000D)

/**
 * @brief 不支持的MQTT报文类型
 *
 */
#define STATE_MQTT_PACKET_TYPE_UNKNOWN                               (STATE_MQTT_BASE - 0x000E)

/**
 * @brief MQTT topic订阅失败
 *
 */
#define STATE_MQTT_SUBACK_RCODE_FAILURE                              (STATE_MQTT_BASE - 0x000F)

/**
 * @brief 不支持的MQTT SUBACK状态码
 *
 */
#define STATE_MQTT_SUBACK_RCODE_UNKNOWN                              (STATE_MQTT_BASE - 0x0010)

/**
 * @brief MQTT topic匹配失败
 *
 */
#define STATE_MQTT_TOPIC_COMPARE_FIALED                              (STATE_MQTT_BASE - 0x0011)

/**
 * @brief 执行@ref aiot_mqtt_deinit 时, 等待其他API执行结束的超过设定的超时时间, MQTT实例销毁失败
 *
 */
#define STATE_MQTT_DEINIT_TIMEOUT                                    (STATE_MQTT_BASE - 0x0012)
#define STATE_MQTT_LOG_CONNECT                                       (STATE_MQTT_BASE - 0x0013)
#define STATE_MQTT_LOG_RECONNECTING                                  (STATE_MQTT_BASE - 0x0014)
#define STATE_MQTT_LOG_CONNECT_TIMEOUT                               (STATE_MQTT_BASE - 0x0015)
#define STATE_MQTT_LOG_DISCONNECT                                    (STATE_MQTT_BASE - 0x0016)
#define STATE_MQTT_LOG_USERNAME                                      (STATE_MQTT_BASE - 0x0017)
#define STATE_MQTT_LOG_PASSWORD                                      (STATE_MQTT_BASE - 0x0018)
#define STATE_MQTT_LOG_CLIENTID                                      (STATE_MQTT_BASE - 0x0019)
#define STATE_MQTT_LOG_TLS_PSK                                       (STATE_MQTT_BASE - 0x001A)

#define STATE_HTTP_BASE                                              (-0x0400)
#define STATE_HTTP_STATUS_LINE_INVALID                               (STATE_HTTP_BASE - 0x0001)
#define STATE_HTTP_READ_BODY_FINISHED                                (STATE_HTTP_BASE - 0x0002)
#define STATE_HTTP_DEINIT_TIMEOUT                                    (STATE_HTTP_BASE - 0x0003)
#define STATE_HTTP_AUTH_CODE_FAILED                                  (STATE_HTTP_BASE - 0x0004)
#define STATE_HTTP_AUTH_NOT_FINISHED                                 (STATE_HTTP_BASE - 0x0005)
#define STATE_HTTP_AUTH_TOKEN_FAILED                                 (STATE_HTTP_BASE - 0x0006)
#define STATE_HTTP_NEED_AUTH                                         (STATE_HTTP_BASE - 0x0007)
#define STATE_HTTP_RECV_NOT_FINISHED                                 (STATE_HTTP_BASE - 0x0008)
#define STATE_HTTP_HEADER_BUFFER_TOO_SHORT                           (STATE_HTTP_BASE - 0x0009)
#define STATE_HTTP_HEADER_INVALID                                    (STATE_HTTP_BASE - 0x000A)
#define STATE_HTTP_LOG_SEND_HEADER                                   (STATE_HTTP_BASE - 0x000B)
#define STATE_HTTP_LOG_SEND_CONTENT                                  (STATE_HTTP_BASE - 0x000C)
#define STATE_HTTP_LOG_RECV_HEADER                                   (STATE_HTTP_BASE - 0x000D)
#define STATE_HTTP_LOG_RECV_CONTENT                                  (STATE_HTTP_BASE - 0x000E)
#define STATE_HTTP_LOG_DISCONNECT                                    (STATE_HTTP_BASE - 0x000F)
#define STATE_HTTP_LOG_AUTH                                          (STATE_HTTP_BASE - 0x0010)

#define STATE_PORT_BASE                                              (-0x0F00)
#define STATE_PORT_INPUT_NULL_POINTER                                (STATE_PORT_BASE - 0x0001)
#define STATE_PORT_INPUT_OUT_RANGE                                   (STATE_PORT_BASE - 0x0002)
#define STATE_PORT_MALLOC_FAILED                                     (STATE_PORT_BASE - 0x0003)
#define STATE_PORT_MISSING_HOST                                      (STATE_PORT_BASE - 0x0004)
#define STATE_PORT_TCP_CLIENT_NOT_IMPLEMENT                          (STATE_PORT_BASE - 0x0005)
#define STATE_PORT_TCP_SERVER_NOT_IMPLEMENT                          (STATE_PORT_BASE - 0x0006)
#define STATE_PORT_UDP_CLIENT_NOT_IMPLEMENT                          (STATE_PORT_BASE - 0x0007)
#define STATE_PORT_UDP_SERVER_NOT_IMPLEMENT                          (STATE_PORT_BASE - 0x0008)
#define STATE_PORT_NETWORK_UNKNOWN_OPTION                            (STATE_PORT_BASE - 0x0009)
#define STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE                       (STATE_PORT_BASE - 0x000A)
#define STATE_PORT_NETWORK_DNS_FAILED                                (STATE_PORT_BASE - 0x000B)
#define STATE_PORT_NETWORK_CONNECT_TIMEOUT                           (STATE_PORT_BASE - 0x000C)
#define STATE_PORT_NETWORK_CONNECT_FAILED                            (STATE_PORT_BASE - 0x000D)
#define STATE_PORT_NETWORK_SELECT_FAILED                             (STATE_PORT_BASE - 0x000E)
#define STATE_PORT_NETWORK_SEND_FAILED                               (STATE_PORT_BASE - 0x000F)
#define STATE_PORT_NETWORK_RECV_FAILED                               (STATE_PORT_BASE - 0x0010)
#define STATE_PORT_NETWORK_SEND_CONNECTION_CLOSED                    (STATE_PORT_BASE - 0x0011)
#define STATE_PORT_NETWORK_RECV_CONNECTION_CLOSED                    (STATE_PORT_BASE - 0x0012)
#define STATE_PORT_TLS_INVALID_CRED_OPTION                           (STATE_PORT_BASE - 0x0013)
#define STATE_PORT_TLS_INVALID_MAX_FRAGMENT                          (STATE_PORT_BASE - 0x0014)
#define STATE_PORT_TLS_INVALID_SERVER_CERT                           (STATE_PORT_BASE - 0x0015)
#define STATE_PORT_TLS_INVALID_CLIENT_CERT                           (STATE_PORT_BASE - 0x0016)
#define STATE_PORT_TLS_INVALID_CLIENT_KEY                            (STATE_PORT_BASE - 0x0017)
#define STATE_PORT_TLS_DNS_FAILED                                    (STATE_PORT_BASE - 0x0018)
#define STATE_PORT_TLS_SOCKET_CREATE_FAILED                          (STATE_PORT_BASE - 0x0019)
#define STATE_PORT_TLS_SOCKET_CONNECT_FAILED                         (STATE_PORT_BASE - 0x001A)
#define STATE_PORT_TLS_INVALID_RECORD                                (STATE_PORT_BASE - 0x001B)
#define STATE_PORT_TLS_RECV_FAILED                                   (STATE_PORT_BASE - 0x001C)
#define STATE_PORT_TLS_SEND_FAILED                                   (STATE_PORT_BASE - 0x001D)
#define STATE_PORT_TLS_RECV_CONNECTION_CLOSED                        (STATE_PORT_BASE - 0x001E)
#define STATE_PORT_TLS_SEND_CONNECTION_CLOSED                        (STATE_PORT_BASE - 0x001F)
#define STATE_PORT_TLS_CONFIG_PSK_FAILED                             (STATE_PORT_BASE - 0x0020)
#define STATE_PORT_TLS_INVALID_HANDSHAKE                             (STATE_PORT_BASE - 0x0021)

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _AIOT_STATE_API_H_ */

