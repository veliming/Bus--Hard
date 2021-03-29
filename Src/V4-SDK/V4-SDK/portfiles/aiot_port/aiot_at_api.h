/**
 * @file aiot_at_api.h
 * @brief AT模块头文件, 提供对接AT模组的能力
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 */

#ifndef _AIOT_AT_API_H_
#define _AIOT_AT_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#define STATE_AT_BASE                                              (-0x1000)
#define STATE_AT_RINGBUF_OVERRUN                                   (STATE_AT_BASE - 0x0001)
#define STATE_AT_LOG_RINGBUF_OVERRUN                               (STATE_AT_BASE - 0x0002)

/**
 * @brief SDK调用 @ref aiot_at_send_connect_handler_t 函数时的第三个参数，用于建立网络连接
 */
typedef struct {
    /**
     * @brief 需要连接的目标域名
     */
    char *domain;
    /**
     * @brief 需要连接的目标端口号
     */
    uint16_t port;
} aiot_at_connect_t;

/**
 * @brief buf结构体
 */
typedef struct {
    /**
     * @brief buf起始地址
     */
    uint8_t *buf;
    /**
     * @brief buf长度
     */
    uint32_t len;
} aiot_at_buf_t;

/**
 * @brief 用户调用 @ref aiot_at_setopt 配置at句柄时的数据类型选项
 *
 * @details
 *
 * 在每个选项的说明中含有数据类型的说明，该数据类型为传入 @ref aiot_at_setopt 函数的第三个参数数据类型
 */
typedef enum {
    /**
     * @brief 用户为at句柄分配的socket字符串标识符
     *
     * @details
     *
     * 数据类型: (char *)
     */
    AIOT_ATOPT_SOCKET_ID,
    /**
     * @brief 用户为at句柄配置的ring buffer长度
     *
     * @details
     *
     * 用户为at句柄配置的ring buffer长度，该缓冲区用于保存从 @ref aiot_at_input 输入的 @ref AIOT_ATRECVOPT_BUF 类型网络数据
     *
     * 数据类型: (uint32_t *) 默认值: (1024) bytes
     */
    AIOT_ATOPT_RING_BUF_LEN,
    AIOT_ATOPT_MAX
} aiot_at_option_t;

/**
 * @brief SDK调用 @ref aiot_at_send 需要发送数据时的数据类型选项
 *
 * @details
 *
 * 在每个选项的说明中含有数据类型的说明，该数据类型为传入 @ref aiot_at_send 函数的第三个参数数据类型
 */
typedef enum {
    /**
     * @brief 当SDK需要建立网络连接时使用
     *
     * @details
     *
     * 使用该选项调用 @ref aiot_at_send 时，将触发用户的 @ref aiot_at_send_connect_handler_t 回调函数
     *
     * 数据类型: (aiot_at_connect_t *)
     */
    AIOT_ATSENDOPT_CONNECT_REQ,
    /**
     * @brief 当SDK需要发送数据时使用
     *
     * @details
     *
     * 使用该选项调用 @ref aiot_at_send 时，将触发用户的 @ref aiot_at_send_buf_handler_t 回调函数
     *
     * 数据类型: (aiot_at_buf_t *)
     */
    AIOT_ATSENDOPT_BUF,
    /**
     * @brief 当SDK需要断开网络连接时使用
     *
     * @details
     *
     * 使用该选项调用 @ref aiot_at_send 时，将触发用户的 @ref aiot_at_send_disconnect_handler_t 回调函数
     *
     * 数据类型: NULL
     */
    AIOT_ATSENDOPT_DISCONNECT_REQ,
    AIOT_ATSENDOPT_MAX
} aiot_at_send_option_t;

/**
 * @brief 用户调用 @ref aiot_at_input 向SDK缓存输入的数据类型选项
 *
 * @details
 *
 * 在每个选项的说明中含有数据类型的说明，该数据类型为传入 @ref aiot_at_input 函数的第三个参数数据类型
 */
typedef enum {
    /**
     * @brief 当从模组收到建立网络连接的应答时的数据
     *
     * @details
     *
     * 0 - 建立网络连接失败
     * 1 - 建立网络连接成功
     *
     * 数据类型: (uint8_t *)
     */
    AIOT_ATRECVOPT_CONNECT_RESP,
    /**
     * @brief 当从模组收到上一条发送数据的应答时
     *
     * @details
     *
     * 数据类型: (uint8_t *)
     */
    AIOT_ATRECVOPT_SEND_RESP,
    /**
     * @brief 当从模组收到网络数据时使用
     *
     * @details
     *
     * 数据类型: (aiot_at_buf_t *)
     */
    AIOT_ATRECVOPT_BUF,
    /**
     * @brief 当从模组收到断开网络连接的应答时的数据
     *
     * @details
     *
     * 0 - 断开网络连接失败
     * 1 - 断开网络连接成功
     *
     * 数据类型
     */
    AIOT_ATRECVOPT_DISCONNECT_RESP,
    AIOT_ATRECVOPT_MAX
} aiot_at_recv_option_t;

/**
 * @brief SDK需要建立网络连接时调用此回调函数
 *
 * @details
 *
 * 用户在实现此函数时，需要为 handle 绑定一个socket的字符串标识符
 *
 * 该绑定过程可通过 @ref aiot_at_setopt 的 @ref AIOT_ATOPT_SOCKET_ID 配置项完成
 *
 * 绑定socket字符串标识符后，按照模组的AT指令规范组装数据包并使用MCU的uart发送至模组
 *
 * @param[out] handle 由portfile初始化的at句柄
 * @param[out] connect 建联参数，包括域名和端口号，更多信息请参考 @ref aiot_at_connect_t
 *
 * @return int32_t
 *
 * @retval <0 发送失败
 * @retval >=0 发送成功
 */
typedef int32_t (*aiot_at_send_connect_handler_t)(void *handle, aiot_at_connect_t *connect);

/**
 * @brief SDK需要发送数据时调用此回调函数
 *
 * @details
 *
 * SDK需要发送数据时，会调用此函数。此时，用户应该按照模组的AT指令规范组装数据包并使用MCU的uart发送至模组
 *
 * 若此时无法一次发送完成，那么在返回值中返回成功发送的字节数即可
 *
 * @param[out] socket_id 在 @ref aiot_at_send_connect_handler_t 被调用时用户分配的socket字符串标识符
 * @param[out] buf 需要发送的数据，更多信息请参考 @ref aiot_at_buf_t
 *
 * @return int32_t
 *
 * @retval <0 发送失败
 * @retval >=0 发送成功的字节数
 */
typedef int32_t (*aiot_at_send_buf_handler_t)(char *socket_id, aiot_at_buf_t *buf);

/**
 * @brief SDK需要断开网络连接时调用此函数
 *
 * @details
 *
 * SDK需要断开网络时，会调用此函数。此时，用户应该按照模组的AT指令规范组装数据包并使用MCU的uart发送至模组
 *
 * @param[out] socket_id 在 @ref aiot_at_send_connect_handler_t 被调用时用户分配的socket字符串标识符
 *
 * @return int32_t
 *
 * @retval <0 发送失败
 * @retval >=0 发送成功
 */
typedef int32_t (*aiot_at_send_disconnect_handler_t)(char *socket_id);

/**
 * @brief SDK需要发送指令或数据至模组时，调用的回调函数
 *
 * @details
 *
 * SDK在建立socket连接，发送网络数据，断开socket连接时会调用的回调函数
 *
 */
typedef struct {
    /**
     * @brief SDK需要建立网络连接时调用此回调函数
     */
    aiot_at_send_connect_handler_t connect_handler;
    /**
     * @brief SDK需要发送网络数据时调用此回调函数
     */
    aiot_at_send_buf_handler_t send_handler;
    /**
     * @brief SDK需要断开网络连接时调用此函数
     */
    aiot_at_send_disconnect_handler_t disconnect_handler;
} aiot_at_send_handler_t;

/**
 * @brief 设置SDK需要的发送回调函数
 *
 * @param handler 回调函数集合
 *
 * @return int32_t
 *
 * @retval <STATE_SUCCESS 设置失败
 * @retval >=STATE_SUCCESS 设置成功
 */
int32_t aiot_at_set_send_handler(aiot_at_send_handler_t *handler);

/**
 * @brief 初始化at句柄，此函数应在portfile中调用
 *
 * @details
 *
 * 此函数应在portfile的 @ref core_sysdep_network_init 函数中调用，初始化at句柄
 *
 * @return void*
 *
 * @retval 非NULL 初始化成功
 * @retval NULL 初始化失败
 */
void *aiot_at_init(void);

/**
 * @brief 发送数据至模组，此函数应在portfile中调用
 *
 * @details
 *
 * 此函数应在portfile的 @ref core_sysdep_network_establish, @ref core_sysdep_network_send
 *
 * 或 @ref core_sysdep_network_deinit 中调用，分别用于建立网络连接、发送网络数据和断开网络连接
 *
 * @param handle at句柄
 * @param option 期望发送至模组的指令类型，更多信息请参考 @ref aiot_at_send_option_t
 * @param data 期望发送至模组的指令数据，更多信息请参考 @ref aiot_at_send_option_t
 *
 * @return int32_t
 *
 * @retval <STATE_SUCCESS 发送失败
 * @retval >=0 成功发送的字节数
 */
int32_t aiot_at_send(void *handle, aiot_at_send_option_t option, void *data);

/**
 * @brief 从缓存中读取数据，此函数应在portfile中调用
 *
 * @details
 *
 * 此函数应在portfile的 @ref core_sysdep_network_establish, @ref core_sysdep_network_recv
 *
 * 或 @ref core_sysdep_network_deinit 中调用，分别用于接收建立网络连接的应答、接收网络数据和接收断开网络连接的应答
 *
 * @param handle at句柄
 * @param option 期望从缓存中获取的指令类型，更多信息请参考 @ref aiot_at_recv_option_t
 * @param data 期望从缓存中获取的指令数据，更多信息请参考 @ref aiot_at_recv_option_t
 *
 * @return int32_t
 *
 * @retval <STATE_SUCCESS 从缓存中读取数据失败
 * @retval >=0 成功读取的字节数
 */
int32_t aiot_at_recv(void *handle, aiot_at_recv_option_t option, void *data);

/**
 * @brief 销毁at句柄，此函数应在portfile中调用
 *
 * @details
 *
 * 此函数应在portfile的 @ref core_sysdep_network_deinit 中调用，用于销毁at句柄
 *
 * @param handle 指向at句柄的指针
 *
 * @return int32_t
 *
 * @retval <STATE_SUCCESS 销毁句柄失败
 * @retval >=STATE_SUCCESS 销毁句柄成功
 */
int32_t aiot_at_deinit(void **handle);

/**
 * @brief 用于配置at句柄，可在 @ref aiot_at_send_connect_handler_t 被触发时对at句柄的参数进行配置
 *
 * @param handle at句柄
 * @param option 配置at句柄的配置项，更多信息请参考 @ref aiot_at_option_t
 * @param data 配置at句柄的配置数据，更多信息请参考 @ref aiot_at_option_t
 *
 * @return int32_t
 *
 * @retval <STATE_SUCCESS 配置成功
 * @retval >=STATE_SUCCESS 配置失败
 */
int32_t aiot_at_setopt(void *handle, aiot_at_option_t option, void *data);

/**
 * @brief 用于向SDK缓存输入从模组收到的数据，此函数应在接收模组数据并进行AT指令解析后调用
 *
 * @details
 *
 * 当从模组收到数据后，对AT指令进行解析，以下3种数据需要调用此函数输入至SDK缓存中
 *
 * 1. 建立网络连接的应答，通过 @ref AIOT_ATRECVOPT_CONNECT_RESP 选项输入至SDK
 *
 * 2. 从网络上收到的数据，通过 @ref AIOT_ATRECVOPT_BUF 选项输入至SDK
 *
 * 3. 断开网络连接的应答，通过 @ref AIOT_ATRECVOPT_DISCONNECT_RESP 选项输入至SDK
 *
 * @param socket_id 在 @ref aiot_at_send_connect_handler_t 被调用时用户分配的socket字符串标识符
 * @param option 接收的数据类型选项，更多信息请参考 @ref aiot_at_recv_option_t
 * @param data 接收到的数据，更多信息请参考 @ref aiot_at_recv_option_t
 *
 * @return int32_t
 *
 * @retval <STATE_SUCCESS 操作成功
 * @retval >=STATE_SUCCESS 操作失败
 */
int32_t aiot_at_input(char *socket_id, aiot_at_recv_option_t option, void *data);

#if defined(__cplusplus)
}
#endif

#endif

