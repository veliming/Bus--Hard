# 简介

阿里云的[物联网平台](https://www.aliyun.com/product/iot)提供安全可靠的设备连接通信能力, 帮助用户将海量设备数据采集上云, 设备可通过集成 `Link Kit SDK` 具备连接物联网平台服务器的能力

---
这个SDK以C99编写, 开源发布, 支持多种操作系统和硬件平台, 阅读本文开始使用

# 了解SDK架构

<img src="https://linkkit-export.oss-cn-shanghai.aliyuncs.com/sdk_new_arch.png" width="800">

# 获取SDK内容及用户编程导引

本代码仓库对应上图中的 `Core Components` 基座部分, 点击以下链接阅读编程导引开始使用

+ [MQTT连云能力](http://gitlab.alibaba-inc.com/Apsaras64/pub/wikis/Iterations/V400/UserGuide/MQTT_Connect)
+ [HTTP连云能力](http://gitlab.alibaba-inc.com/Apsaras64/pub/wikis/Iterations/V400/UserGuide/HTTP_Connect)
+ [RRPC](http://gitlab.alibaba-inc.com/Apsaras64/pub/wikis/Iterations/V400/UserGuide/RRPC_Process)

---
除此以外, 本仓库所形成的基座上, 也可以由用户选择动态安插, 装备上其它高阶的能力, 点击以下链接阅读编程导引开始使用

+ [*Bootstrap*]() *(待开发)*
+ [*动态注册*]() *(待开发)*
+ [OTA](http://gitlab.alibaba-inc.com/Apsaras64/pub/wikis/Iterations/V400/UserGuide/OTA_Upgrade)
+ [*物模型管理*]() *(待开发)*
+ [*子设备管理*]() *(待开发)*
+ [*CoAP连云能力*]() *(待开发)*
+ [*设备热点配网*]() *(待开发)*
+ [*蓝牙辅助配网*]() *(待开发)*

---
在面向应用的编程导引外, 以上所有组件也提供面向代码的详细手册, [SDK线上手册地址](http://gaic.alicdn.com/ztms/linkkit/html/index.html)

# 装备高阶能力

以上高阶能力, 任意两两之间无依赖关系, 仅依赖基座 `Core`, 用户装备任一种能力的方式都是一致的

+ 下载组件压缩包, 每一种高阶能力对应一个组件代码压缩包
+ 解压组件压缩包, 放置到 `components/xxx` 目录

> 目前已就绪的高阶组件有

| **高阶组件**    | **下载地址**
|-----------------|-----------------------------------------------------------------------------------------
| `OTA`           | http://gitlab.alibaba-inc.com/united-sdk/linkkit-ota/repository/archive.zip?ref=master

# 编译方式

目前SDK仅支持在`Mac OSX`或`Linux`主机的开发环境下, 以命令行上编译, 编译命令如下

| **编译命令**    | **说明**
|-----------------|-----------------------------------------------------
| `make`          | 编译SDK静态库, 编好的库在`output/libaiot_sdk.a`
| `make prog`     | 编译SDK例程, 编好的例程在`output/xxxx_demo`

# 其它

+ 我们也提供[全集下载](http://gitlab.alibaba-inc.com/united-sdk/V4-SDK/repository/archive.zip?ref=master), 其中包含了全部现有组件的最新版本, 不在乎SDK尺寸的用户可一次性全部打包下载
+ 为兼容`4.0.0`之前的旧版本接口, 特别设立了 `components/compat-legacy-api` 组件, 装备后可获取老版本的API. **(待开发)**

# 设计原则

不论是对于MQTT连云这样的`Core`基础组件, 还是OTA这样的`components/xxx`高阶组件, 一致使用以下的设计原则

+ 用户须知的API函数接口和数据结构, 在`xxx/aiot_xxx_api.h`头文件中列出
+ 这些API函数的实现源码, 在`xxx/aiot_xxx_api.c`中
+ 组件能力的使用范例, 在`xxx/demos/xxx_{basic,posix}_demo.c`中
+ 组件的API函数原型, 遵循统一的设计模式

    - `aiot_xxx_init()`: 初始化1个会话实例, 并设置默认参数
    - `aiot_xxx_deinit()`: 销毁1个会话实例, 回收其占用资源
    - `aiot_xxx_setopt()`: 配置1个会话实例的运行参数, 可用参数及用法在编程手册中有指出
    - `aiot_xxx_send_yyy()`: 向云平台发起某种会话请求
    - `aiot_xxx_recv()`: 从云平台接收请求的应答

+ 组件能力运行时, 对外表达的途径, 遵循统一的设计模式

    - API的返回值: 是1个`16bit`的非正数整型, 0表成功, 其余表达运行状态
        * `retval = aiot_xxx_yyy()`方式获取返回值
        * 所有返回值唯一对应内部运行分支, 含义详见`core/aiot_state_api.h`或者`components/xxx/aiot_xxx_api.h`
        * 所有组件的返回值的值域互不重叠, 共同分别分布在`0x0000 - 0xFFFF`
    - 数据回调函数: 是1个用户实现并传入SDK的回调函数, SDK从外界读取到值得关注的数据报文是, SDK会调用它, 用户可在此处理数据流
        * `aiot_xxx_setopt(handle, AIOT_XXXOPT_RECV_HANDLER, user_recv_cb)`设置数据回调
    - 事件回调函数: 是1个用户实现并传入SDK的回调函数, SDK内部运行状态发生值得关注的变化时, SDK会调用它, 用户可在此处理控制流
        * `aiot_xxx_setopt(handle, AIOT_XXXOPT_EVENT_HANDLER, user_event_cb)`设置事件回调
    - 运行日志回调: 是1个用户实现并传入SDK的回调函数, SDK运行时, 详细的运行日志字符串会传入这个回调
        * `aiot_state_set_logcb(user_log_cb)`设置日志回调
        * 若不想看到SDK运行日志, 回调为空即可
        * 日志格式统一为: `[timestamp][LK-XXXX] message`, `XXXX`以数字形式描述日志字符串, 它和返回值共享一个状态码分布空间, 在`0x0000 - 0xFFFF`

