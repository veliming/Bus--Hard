/**
 * @file aiot_ota_api.h
 * @brief OTA模块头文件, 提供设备获取固件升级和远程配置的能力
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 */

#ifndef _AIOT_OTA_API_H_
#define _AIOT_OTA_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include <stdio.h>

#include "aiot_sysdep_api.h"
#include "aiot_http_api.h"
#include "aiot_state_api.h"

/**
 * @brief OTA云端消息的事件类型
 *
 */
typedef enum {

    /**
     * @brief 收到的OTA消息为固件升级消息
     *
     */
    AIOT_OTARECV_FOTA,

    /**
     * @brief 收到的OTA消息为远程配置消息
     *
     */
    AIOT_OTARECV_COTA
} aiot_ota_recv_type_t;

/**
 * @brief OTA过程中使用的digest方法类型汇总
 *
 */

typedef enum {

    /**
     * @brief 收到的OTA固件的校验方式为MD5
     *
     */
    AIOT_OTA_DIGEST_MD5,

    /**
     * @brief 收到的OTA固件的校验方式为SHA256
     *
     */
    AIOT_OTA_DIGEST_SHA256,
    AIOT_OTA_DIGEST_MAX
} aiot_ota_digest_type_t;

/**
 * @brief 云端下推的固件任务的具体内容
 *
 */

typedef struct {
    char       *product_key;
    char       *device_name;
    char       *url;
    uint32_t    size_total;
    uint8_t     digest_method;
    char       *expect_digest;

    /**
     * @brief 如果为固件升级消息, 这个version字段不为空. 如果为远程配置消息, 则为空
     *
     */
    char       *version;
    void       *mqtt_handle;
} aiot_download_task_desc_t;

/**
* @brief OTA下行的mqtt消息的描述,包括type(固件升级/远程配置),以及OTA任务的具体描述(包括url等)
*
*/
typedef struct {
    aiot_ota_recv_type_t        type;
    aiot_download_task_desc_t   *task_desc;
} aiot_ota_recv_t;

/**
 * @brief 设备收到OTA的mqtt下行报文时候触发的收包回调函数
 *
 */
typedef void (* aiot_ota_recv_handler_t)(void *handle, const aiot_ota_recv_t *const msg, void *userdata);

/**
* @brief 下载固件过程中收到的分片的报文的类型
*
*/
typedef enum {

    /**
    * @brief 基于HTTP传输的固件分片报文
    *
    */
    AIOT_DLRECV_HTTPBODY
} aiot_download_recv_type_t;

/**
* @brief 下载固件过程中收到的分片的报文的描述,包括类型,以及所存储的buffer的位置和长度
*
*/
typedef struct {
    aiot_download_recv_type_t type;
    struct {
        uint8_t *buffer;
        uint32_t len;
    } data;
} aiot_download_recv_t;

/**
 * @brief 设备收到OTA固件的http(s)下行报文触发的收包回调函数
 *
 */
typedef void (* aiot_download_recv_handler_t)(void *handle, int32_t percent, const aiot_download_recv_t *packet,
        void *userdata);

/**
 * @brief 与云端约定的OTA过程中的错误码, 用户上报给云端时使用
 *
 */

typedef enum {

    /**
     * @brief 与云端约定的设备升级出错的错误描述
     */
    AIOT_OTAERR_UPGRADE_FAILED = -1,

    /**
     * @brief 与云端约定的设备下载过程出错的错误码描述
     */
    AIOT_OTAERR_FETCH_FAILED = -2,

    /**
     * @brief 与云端约定的固件校验和出错的错误码描述
     */
    AIOT_OTAERR_CHECKSUM_MISMATCH = -3,

    /**
     * @brief 与云端约定的烧写固件出错的错误码描述
     */
    AIOT_OTAERR_BURN_FAILED = -4
} aiot_ota_protocol_errcode_t;

/**
 * @brief 调用 @ref aiot_ota_setopt 接口时, option参数的可用值
 *
 */

typedef enum {

    /**
     * @brief 设置处理OTA消息的用户回调函数
     *
     * @details
     *
     * 在该回调中, 用户可能收到两种消息: 固件升级消息或者远程配置消息.
     *
     * 无论哪种消息, 都包含了url, version, digest method, sign等内容.
     *
     * 用户需要在该回调中决定升级策略, 包括是否升级和何时升级等. 如果需要升级, 则需要调用aiot_download_init初始化一个download实例句柄
     */
    AIOT_OTAOPT_RECV_HANDLER,

    /**
     * @brief 设置MQTT的handle
     *
     * @details
     *
     * 设置MQTT的handle, 以便OTA过程中使用MQTT的通道能力
     */
    AIOT_OTAOPT_MQTT_HANDLE,

    /**
     * @brief 设置用户数据
     *
     * @details
     *
     * 设置用户数据, 该数据传递往回调函数中进行处理
     */
    AIOT_OTAOPT_USER_DATA,
    AIOT_OTAOPT_MAX
} aiot_ota_option_t;

/**
 * @brief 调用 @ref aiot_download_setopt 接口时, option参数的可用值
 *
 */

typedef enum {

    /**
     * @brief 设置http下载固件的加密方式
     *
     * @details
     *
     * 用户可以通过http直接下载, 也可以通过https下载, 默认是http. 如果走了https下载, 则需要设置该选项
     */
    AIOT_DLOPT_NETWORK_CRED,

    /**
     * @brief 设置http下载固件的端口号
     */
    AIOT_DLOPT_NETWORK_PORT,

    /**
     * @brief 设置aiot_download_recv函数的接收超时时间
     *
     * @details
     *
     * 如果在该超时时间内还没有收到数据包, 则aiot_download_recv函数返回. 避免因为网络堵塞导致该API一直堵塞的情况
     **/
    AIOT_DLOPT_RECV_TIMEOUT_MS,

    /**
     * @brief 设置download handle收包的回调函数
     *
     */
    AIOT_DLOPT_RECV_HANDLER,

    /**
     * @brief 设置download handle收到http报文时供回调函数使用的用户数据
     *
     */
    AIOT_DLOPT_USER_DATA,

    /**
     * @brief 设置download实例句柄所包含下载任务的具体内容
     *
     * @details
     *
     * 在download实例句柄中开辟内存, 将OTA消息中携带的url, version, digest method, sign等信息复制过来, 并且初始化计算digest的句柄
     *
     **/
    AIOT_DLOPT_TASK_DESC,

    /**
     * @brief 设置download实例句柄分段下载的起始地址
     *
     * @details
     *
     * HTTP 范围请求(range requests)特性中, 表示从第start个Byte开始下载
     *
     **/
    AIOT_DLOPT_RANGE_START,

    /**
     * @brief 设置download实例句柄分段下载的结束地址
     *
     * @details
     *
     * HTTP 范围请求(range requests)特性中, 表示下载到第end个Byte后结束
     *
     **/
    AIOT_DLOPT_RANGE_END,

    /**
     * @brief 设置http分段下载过程中每次使用buffer的最大长度
     *
     * @details
     *
     * 设置该值后, aiot_download_recv收包的时候, 这个buffer收满, 或者到底文件的末尾就会返回
     *
     **/
    AIOT_DLOPT_BODY_BUFFER_MAX_LEN,
    AIOT_DLOPT_MAX
} aiot_download_option_t;


/**
 * @brief 初始化ota实例并设置默认参数
 *
 * @return void*
 * @retval 非NULL ota实例句柄
 * @retval NULL 初始化失败, 一般是内存分配失败导致
 *
 */
void   *aiot_ota_init();

/**
 * @brief 释放ota实例句柄的资源
 *
 * @param[in] handle 指向ota实例句柄的指针
 *
 * @return int32_t
 * @retval STATE_OTA_DEINIT_HANDLE_IS_NULL handle或者handle所指向的地址为空
 * @retval STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_ota_deinit(void **handle);

/**
 * @brief 上报设备(非网关中所接入的子设备)的版本号
 *
 * @param[in] handle 指向ota实例句柄的指针
 * @param[in] version 待上报的版本号
 *
 * @return int32_t
 * @retval STATE_OTA_REPORT_HANDLE_IS_NULL handle为空
 * @retval STATE_OTA_REPORT_VERSION_IS_NULL 版本号为空
 * @retval STATE_OTA_REPORT_MQTT_HANDLE_IS_NULL handle中的mqtt句柄为空
 * @retval STATE_OTA_REPORT_FAILED 上报失败
 * @retval STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_ota_report_version(void *handle, char *version);

/**
 * @brief 用于网关中子设备上报版本号
 *
 * @param[in] handle download句柄
 * @param[in] product_key 产品的product_key
 * @param[in] device_name 设备的名称
 * @param[in] version 版本号
 *
 * @return int32_t
 * @retval STATE_SUCCESS 上报成功
 * @retval STATE_OTA_REPORT_EXT_HANELD_IS_NULL handle为空
 * @retval STATE_OTA_REPORT_EXT_VERSION_NULL 版本号为空
 * @retval STATE_OTA_REPORT_EXT_PRODUCT_KEY_IS_NULL 子设备的product_key输入为空
 * @retval STATE_OTA_REPORT_EXT_DEVICE_NAME_IS_NULL 子设备的device_name输入为空
 * @retval STATE_OTA_REPORT_EXT_MQTT_HANDLE_IS_NULL 上报版本号所需的mqtt_handle为空
 *
 */
int32_t aiot_ota_report_version_ext(void *handle, char *product_key, char *device_name, char *version);

/**
 * @brief 设置ota句柄参数
 *
 * @param[in] handle ota句柄
 * @param[in] option 配置选项, 更多信息请参考@ref aiot_ota_option_t
 * @param[in] data   配置选项数据, 更多信息请参考@ref aiot_ota_option_t
 *
 * @return int32_t
 * @retval STATE_OTA_SETOPT_HANDLE_IS_NULL ota_handle为空
 * @retval STATE_OTA_SETOPT_DATA_IS_NULL data字段为空
 * @retval STATE_USER_INPUT_UNKNOWN_OPTION option不支持
 * @retval STATE_SUCCESS 参数设置成功
 *
 */
int32_t aiot_ota_setopt(void *handle, aiot_ota_option_t option, void *data);

/**
 * @brief 初始化download实例并设置默认参数
 *
 * @return void*
 * @retval 非NULL download实例句柄
 * @retval NULL 初始化失败, 一般是内存分配失败导致
 *
 */
void   *aiot_download_init();

/**
 * @brief 释放download实例句柄的资源
 *
 * @param[in] handle 指向download实例句柄的指针
 *
 * @return int32_t
 * @retval STATE_DOWNLOAD_DEINIT_HANDLE_IS_NULL handle或者handle指向的内容为空
 * @retval STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_download_deinit(void **handle);

/**
 * @brief 通过download实例句柄下载一段buffer
 *
 * @param[in] handle 指向download实例句柄的指针
 *
 * @return int32_t
 * @retval STATE_DOWNLOAD_RECV_HANDLE_IS_NULL 收包时候的handle为空
 * @retval STATE_DOWNLOAD_FINISHED 整个固件包下载完成
 *
 */
int32_t aiot_download_recv(void *handle); /* 返回条件: 网络出错 | 校验出错 | 读到EOF | buf填满 */

/**
 * @brief 设置download句柄参数
 *
 * @param[in] handle download句柄
 * @param[in] option 配置选项, 更多信息请参考@ref aiot_download_option_t
 * @param[in] data   配置选项数据, 更多信息请参考@ref aiot_download_option_t
 *
 * @return int32_t
 * @retval STATE_DOWNLOAD_SETOPT_HANDLE_IS_NULL download_handle为空
 * @retval STATE_DOWNLOAD_SETOPT_DATA_IS_NULL data字段为空
 * @retval STATE_DOWNLOAD_SETOPT_COPIED_DATA_IS_NULL 拷贝task_desc失败
 * @retval STATE_SUCCESS 参数设置成功
 *
 */
int32_t aiot_download_setopt(void *handle, aiot_download_option_t option, void *data);

/**
 * @brief 上报下载完成度的百分比或者错误码
 *
 * @param[in] handle download句柄
 * @param[in] percent 当前所下载内容完成度的百分比或者错误码
 *
 * @return int32_t
 * @retval STATE_DOWNLOAD_REPORT_HANDLE_IS_NULL 上报时handle为空
 * @retval STATE_DOWNLOAD_REPORT_TASK_DESC_IS_NULL 上报时task_desc为空, 无法找到相应的product_key, device_name
 * @retval STATE_SUCCESS 参数设置成功
 *
 */
int32_t aiot_download_report_progress(void *handle, int32_t percent);

/**
 * @brief 向云端发送GET固件报文请求
 *
 * @param[in] handle download句柄, 包含了固件的url等信息
 *
 * @return int32_t
 * @retval STATE_DOWNLOAD_REQUEST_HANDLE_IS_NULL 发送GET请求的时候handle为空
 * @retval STATE_DOWNLOAD_REQUEST_TASK_DESC_IS_NULL 发送GET请求的时候task_desc为空
 * @retval STATE_DOWNLOAD_REQUEST_URL_IS_NULL 发送GET请求的时候task_descb不为空, 但是其中的url为空
 * @retval STATE_DOWNLOAD_SEND_REQUEST_FAILED 发送GET请求的时候http底层发包逻辑报错

 * @retval STATE_SUCCESS 请求发送成功
 */
int32_t aiot_download_send_request(void *handle);

/**
 * @brief ota过程中处理mqtt消息的句柄, 该句柄主要用于通过mqtt协议从云端收取固件升级消息, 包括固件的url等
 *
 */
typedef struct {                                      /* 处理云端消息的OTA句柄 */
    void                        *userdata;            /* 组件调用recv_handler的入参之一 */
    aiot_ota_recv_handler_t     recv_handler;         /* 网络报文发生变化时, 通知用户的回调 */
    aiot_sysdep_portfile_t      *sysdep;

    /*---- 以上都是用户在API可配 ----*/
    /*---- 以下都是OTA内部使用, 用户无感知 ----*/

    void            *mqtt_handle;
    void            *data_mutex;
} ota_handle_t;

/**
 * @brief http下载句柄, 该句柄主要用于通过http协议从指定的url下载固件
 *
 */
typedef struct {                                        /* 处理本地下载的句柄 */
    void                               *userdata;       /* 组件调用event_handler 时的入参之一 */
    aiot_download_recv_handler_t       recv_handler;    /* 组件内部运行状态变更时, 通知用户的回调 */
    aiot_download_task_desc_t          *task_desc;      /* 某次下载活动的目标描述信息, 如URL等 */
    aiot_sysdep_portfile_t             *sysdep;
    uint32_t                           range_start;
    uint32_t                           range_end;

    /*---- 以上都是用户在API可配 ----*/
    /*---- 以下都是downloader内部使用, 用户无感知 ----*/

    uint8_t         download_status;
    void            *http_handle;
    uint32_t        size_fetched;
    int32_t         percent;
    void            *digest_ctx;
    void            *data_mutex;
    void            *recv_mutex;
} download_handle_t;


/**
 * @brief 下载固件过程中默认的buffer的长度
 *
 */
#define AIOT_DOWNLOAD_DEFAULT_BUFFER_LEN              (2 * 1024)

/**
 * @brief 下载固件过程中默认的超时时间
 *
 */
#define AIOT_DOWNLOAD_DEFAULT_TIMEOUT_MS              (5 * 1000)


/**
 * @brief OTA模块的状态码基准值
 *
 */
#define STATE_OTA_BASE                                  (-0x0900)

/**
 * @brief 下载完成, 校验和匹配成功
 *
 */
#define STATE_OTA_DIGEST_MATCH                          (STATE_OTA_BASE - 0x0001)

/**
 * @brief OTA模块上报版本号失败
 *
 */
#define STATE_OTA_REPORT_FAILED                         (STATE_OTA_BASE - 0x0002)

/**
 * @brief OTA模块收取http报文包时出现错误
 *
 */
#define STATE_DOWNLOAD_RECV_ERROR                       (STATE_OTA_BASE - 0x0003)

/**
 * @brief OTA模块收包时出现校验和错误, 即md5或者sha256计算结果跟云端下行的不匹配
 *
 * @details
 *
 * 固件的md5或者sha256计算结果跟云端下行的参考值不匹配所致的错误
 *
 */
#define STATE_OTA_DIGEST_MISMATCH                       (STATE_OTA_BASE - 0x0004)

/**
 * @brief OTA模块解析mqtt下行json报文出错
 *
 * @details
 *
 * md5或者sha256计算结果跟云端下行的不匹配
 *
 */
#define STATE_OTA_PARSE_JSON_ERROR                      (STATE_OTA_BASE - 0x0005)

/**
 * @brief OTA模块发送上行http请求报文失败
 *
 * @details
 *
 * OTA模块往存储固件的服务器发送GET请求失败
 *
 */
#define STATE_DOWNLOAD_SEND_REQUEST_FAILED              (STATE_OTA_BASE - 0x0006)

/**
 * @brief 按照range下载的时候已经下载到了range_end字段指定的地方
 *
 * @details
 *
 * 按照range下载的时候已经下载到了range_end字段指定的地方. 如果用户此时还是继续尝试去下载, SDK返回返回错误码提示用户
 *
 */
#define STATE_DOWNLOAD_GOT_RANGE_END                    (STATE_OTA_BASE - 0x0007)

/**
 * @brief 下载中断, 断点续传特性会重新发起连接, 打印出有关日志
 *
 */
#define STATE_OTA_RENEWAL                               (STATE_OTA_BASE - 0x0008)

/**
 * @brief ota_handle反初始化的时候handle为空
 *
 */
#define STATE_OTA_DEINIT_HANDLE_IS_NULL                 (STATE_OTA_BASE - 0x0009)

/**
 * @brief ota_handle设置选项的时候handle为空
 *
 */
#define STATE_OTA_SETOPT_HANDLE_IS_NULL                 (STATE_OTA_BASE - 0x000A)

/**
 * @brief ota_handle设置选项的时候数据指针为空
 *
 */
#define  STATE_OTA_SETOPT_DATA_IS_NULL                  (STATE_OTA_BASE - 0x000B)

/**
 * @brief download_handle反初始化的时候该handle为空
 *
 */
#define  STATE_DOWNLOAD_DEINIT_HANDLE_IS_NULL           (STATE_OTA_BASE - 0x000C)

/**
 * @brief download_handle设置选项的时候handle为空
 *
 */
#define  STATE_DOWNLOAD_SETOPT_HANDLE_IS_NULL           (STATE_OTA_BASE - 0x000D)

/**
 * @brief download_handle设置选项的时候data为空
 *
 */
#define  STATE_DOWNLOAD_SETOPT_DATA_IS_NULL             (STATE_OTA_BASE - 0x000E)

/**
 * @brief download_handle设置选项的时候无法从ota handle那里拷贝数据过来
 *
 */
#define  STATE_DOWNLOAD_SETOPT_COPIED_DATA_IS_NULL      (STATE_OTA_BASE - 0x000F)

/**
 * @brief 设备(非网关的子设备)上报版本号的时候handle为空
 *
 */
#define  STATE_OTA_REPORT_HANDLE_IS_NULL                (STATE_OTA_BASE - 0x0010)

/**
 * @brief 设备(非网关的子设备)上报版本号的时候版本号为空
 *
 */
#define  STATE_OTA_REPORT_VERSION_IS_NULL               (STATE_OTA_BASE - 0x0011)

/**
 * @brief 设备(非网关的子设备)上报版本号的时候mqtt handle为空
 *
 */
#define  STATE_OTA_REPORT_MQTT_HANDLE_IS_NULL           (STATE_OTA_BASE - 0x0012)

/**
 * @brief 网关的子设备上报版本号的时候handle为空
 *
 */
#define  STATE_OTA_REPORT_EXT_HANELD_IS_NULL            (STATE_OTA_BASE - 0x0013)

/**
 * @brief 网关的子设备上报版本号的时候version为空
 *
 */
#define  STATE_OTA_REPORT_EXT_VERSION_NULL              (STATE_OTA_BASE - 0x0014)

/**
 * @brief 网关的子设备上报版本号的时候product key为空
 *
 */
#define  STATE_OTA_REPORT_EXT_PRODUCT_KEY_IS_NULL       (STATE_OTA_BASE - 0x0015)

/**
 * @brief 非网关的子设备上报版本号的时候deivce name为空
 *
 */
#define  STATE_OTA_REPORT_EXT_DEVICE_NAME_IS_NULL       (STATE_OTA_BASE - 0x0016)

/**
 * @brief 非网关的子设备上报版本号的时候, ota_handle中的mqtt handle为空
 *
 */
#define  STATE_OTA_REPORT_EXT_MQTT_HANDLE_IS_NULL       (STATE_OTA_BASE - 0x0017)

/**
 * @brief 上报下载进展百分比或者错误码的时候, handle为空
 *
 */
#define  STATE_DOWNLOAD_REPORT_HANDLE_IS_NULL           (STATE_OTA_BASE - 0x0018)

/**
 * @brief 上报下载进展百分比或者错误码的时候, task_desc为空
 *
 */
#define  STATE_DOWNLOAD_REPORT_TASK_DESC_IS_NULL        (STATE_OTA_BASE - 0x0019)

/**
 * @brief 使用download handle去收包的时候, handle为空
 *
 */
#define  STATE_DOWNLOAD_RECV_HANDLE_IS_NULL             (STATE_OTA_BASE - 0x001A)

/**
 * @brief 使用download handle往云端发送报文的时候, handle为空
 *
 */
#define  STATE_DOWNLOAD_REQUEST_HANDLE_IS_NULL          (STATE_OTA_BASE - 0x001B)

/**
 * @brief 使用download handle往云端发送报文的时候, task_desc字段为空
 *
 */
#define  STATE_DOWNLOAD_REQUEST_TASK_DESC_IS_NULL       (STATE_OTA_BASE - 0x001C)

/**
 * @brief 使用download handle往云端发送报文的时候, task_desc字段里面的url为空
 *
 */
#define  STATE_DOWNLOAD_REQUEST_URL_IS_NULL             (STATE_OTA_BASE - 0x001D)

/**
 * @brief 解析OTA下行报文时, 其中的digest方法并非md5或者sha256, 为其他未支持的方法
 *
 */
#define STATE_OTA_UNKNOWN_DIGEST_METHOD                 (STATE_OTA_BASE - 0x001E)

/**
 * @brief 整个固件(而不是单独一次的下载片段)收取完成, 已收取的累计字节数与固件大小字节数一致
 *
 */
#define STATE_DOWNLOAD_FINISHED                         (STATE_OTA_BASE - 0x001F)

#if defined(__cplusplus)
}
#endif

#endif

