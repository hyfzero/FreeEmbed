# 嵌入式模块分层设计指南

## 1. 目的

本文定义一种可复用的嵌入式模块组织方式，用于 GPIO 外设、传感器、执行器、通信设备、存储器等模块。

核心目标是隔离三类变化：

- 应用需求变化：业务流程、状态机、任务调度。
- 板级资源变化：引脚、外设实例、总线、地址、中断号、有效电平。
- 驱动实现变化：GPIO、PWM、I2C、SPI、CAN 等不同后端。

该模式不依赖某一种具体设备，也不限定 MCU、HAL 或 RTOS。

## 2. 分层模型

```text
Application
    |
    v
Board / BSP
    |
    v
Reusable Driver
    |
    v
MCU HAL / LL / RTOS / Hardware
```

### 2.1 Application 层

Application 层负责业务逻辑，只表达“要做什么”：

- 调用板级初始化入口。
- 获取或使用板级公开的抽象对象。
- 调用通用驱动公开的业务接口。
- 编排任务、状态机、超时和错误恢复。

Application 层不得包含：

- GPIO 端口和引脚编号。
- I2C/SPI/UART 外设实例或设备地址。
- 有效电平、片选逻辑和 HAL 回调。
- 具体驱动实现类型的实例和配置结构。

### 2.2 Board / BSP 层

Board 层负责把抽象驱动绑定到当前电路板，只表达“这块板如何提供该设备”：

- 保存具体驱动对象。
- 定义静态硬件配置。
- 适配 MCU HAL/LL API。
- 绑定端口、引脚、总线、地址、中断和极性。
- 初始化驱动对象。
- 向 Application 层公开抽象句柄或板级服务。

Board 层是唯一允许同时了解“通用驱动接口”和“具体硬件资源”的层。

### 2.3 Reusable Driver 层

通用驱动负责设备语义和可复用实现：

- 定义稳定的公开接口。
- 定义对象、操作表、配置描述符和状态。
- 实现参数检查、状态维护和后端分发。
- 通过函数指针与 `void *context` 接收平台操作。

通用驱动不得：

- 包含某一 MCU 的 HAL 头文件。
- 直接使用具体端口、引脚或外设实例。
- 引用项目的 `main.h`、`board.h`。
- 创建应用任务或编码具体业务流程。

## 3. 依赖规则

依赖只能自上而下：

```text
Application -> Board -> Driver
Board       -> MCU HAL
Driver      -> C standard library or project-wide basic utilities
```

禁止以下反向依赖：

```text
Driver -X-> Board
Driver -X-> Application
Board  -X-> Application
```

一个简单判断标准是：把通用驱动目录复制到另一块 MCU 或主机测试工程后，不修改驱动源码也应能够编译。

## 4. 推荐目录

```text
Project/
|-- App/
|   |-- include/
|   `-- src/
|-- Board/
|   |-- include/
|   |   `-- board.h
|   `-- src/
|       `-- board.c
|-- Modules/
|   `-- device/
|       |-- include/
|       |   |-- device.h
|       |   `-- device_backend.h
|       `-- src/
|           |-- device.c
|           `-- device_backend.c
`-- Platform/
    `-- vendor HAL or platform support
```

如果模块需要复制到独立示例工程，应复制到工程自己的子目录并加入构建系统，不应通过相对路径直接引用其他工程中的源文件。这样可以保证示例独立、版本明确且可复现。

## 5. 通用接口设计

### 5.1 从业务动作开始

先定义 Application 真正需要的动作，再考虑硬件实现。例如：

- 输入设备：`read`、`enable`、`disable`。
- 输出设备：`start`、`stop`、`set_value`。
- 传感器：`sample`、`calibrate`、`sleep`。
- 存储设备：`read`、`write`、`erase`、`sync`。

公开 API 不应直接暴露 `HAL_GPIO_WritePin`、寄存器地址或总线事务细节。

### 5.2 使用操作表隔离实现

需要支持多种后端时，可使用基类对象和操作表：

```c
typedef struct Device Device;

typedef struct
{
    int ( *start )( Device *self );
    int ( *stop )( Device *self );
    int ( *set_value )( Device *self, int value );
} DeviceOps;

struct Device
{
    const char *name;
    const DeviceOps *ops;
    int state;
};
```

公开分发函数只依赖 `Device`：

```c
int device_start( Device *self );
int device_stop( Device *self );
int device_set_value( Device *self, int value );
```

具体后端通过组合基类实现多态：

```c
typedef struct
{
    Device base;
    void *context;
    unsigned int channel;
} DeviceBackend;
```

基类应放在具体对象的第一个成员，或者使用统一的 `container_of` 工具从基类恢复具体对象。

### 5.3 使用配置结构描述资源

初始化参数较多时使用配置结构，不使用不断增长的长参数列表：

```c
typedef int ( *DeviceWriteFn )( void *context,
                                unsigned int channel,
                                int value );

typedef struct
{
    const char *name;
    DeviceWriteFn write;
    void *context;
    unsigned int channel;
    unsigned int flags;
} DeviceBackendConfig;

void device_backend_init( DeviceBackend *self,
                          const DeviceBackendConfig *config );
```

配置结构的优势：

- 每个参数有明确名称。
- 容易静态定义为板级资源表。
- 新增可选字段时可保持初始化函数签名稳定。
- 便于单元测试构造假后端。

## 6. Board 层实现模板

板级头文件只公开 Application 需要的内容：

```c
#ifndef BOARD_H
#define BOARD_H

#include "device.h"

extern Device *g_board_device;

void board_init( void );

#endif
```

板级源文件拥有具体对象、硬件适配和配置：

```c
#include "board.h"

#include "device_backend.h"
#include "vendor_hal.h"

Device *g_board_device;

static DeviceBackend board_device_object;

static int board_device_write( void *context,
                               unsigned int channel,
                               int value )
{
    return vendor_hal_write( context, channel, value );
}

static const DeviceBackendConfig board_device_config =
{
    "board_device",
    board_device_write,
    VENDOR_PERIPHERAL_INSTANCE,
    BOARD_DEVICE_CHANNEL,
    BOARD_DEVICE_FLAGS
};

void board_init( void )
{
    device_backend_init( &board_device_object, &board_device_config );
    g_board_device = device_backend_as_base( &board_device_object );
}
```

具体对象和配置使用 `static`，防止硬件实现泄漏到其他层。对外只暴露抽象句柄。

## 7. Application 层实现模板

Application 只做初始化和业务调用：

```c
#include "board.h"

int app_main( void )
{
    board_init();

    device_start( g_board_device );
    device_set_value( g_board_device, 50 );

    return 0;
}
```

这里不应出现具体后端类型、HAL API、端口、通道或设备地址。

## 8. 初始化职责

推荐初始化顺序：

1. MCU 启动和时钟初始化。
2. 厂商生成的基础外设初始化。
3. `board_init()` 完成板级资源绑定。
4. Application 创建任务、启动状态机或进入主循环。

如果项目规模增大，可拆分为：

```c
void board_early_init( void );
void board_init( void );
void app_init( void );
void app_run( void );
```

中断开启前必须保证中断会访问的板级对象已经初始化。

## 9. 状态、错误和并发

- 通用驱动统一返回项目约定的错误码，不向上泄漏厂商 HAL 状态枚举。
- Board 层负责把 HAL 错误转换为通用错误码。
- 对象状态由拥有该状态的层维护，避免多层重复保存同一状态。
- 是否允许在 ISR 中调用必须在接口文档中明确。
- 互斥、临界区和 DMA 完成同步应靠近真正共享硬件资源的层实现。
- Application 层负责业务重试策略；Driver 和 Board 层负责报告准确失败原因。

## 10. 测试策略

### 10.1 Driver 测试

使用假回调和内存上下文测试，不依赖真实 MCU：

- 初始化参数检查。
- 操作分发是否正确。
- 有效标志和边界值处理。
- 状态更新和错误传播。

### 10.2 Board 测试

在目标板或 HAL 模拟层验证：

- 资源绑定是否正确。
- 端口、通道、地址和极性是否正确。
- 初始化顺序是否满足硬件要求。
- 中断和 DMA 回调是否连接到正确对象。

### 10.3 Application 测试

使用抽象对象或模拟服务验证业务流程，不依赖具体硬件后端。

## 11. 迁移现有代码

迁移一个直接调用 HAL 的功能时，按以下顺序执行：

1. 列出 Application 真正需要的业务动作。
2. 建立不包含 HAL 类型的通用接口。
3. 将硬件读写封装为函数指针和上下文。
4. 将具体对象、资源配置和 HAL 适配函数移到 Board 层。
5. 让 Application 只包含 `board.h` 和通用接口头文件。
6. 将模块源码复制到目标工程自己的模块目录并加入构建配置。
7. 编译、静态检查并在目标板验证。

## 12. 评审检查表

提交新模块前检查：

- [ ] Application 中没有 HAL API 和硬件资源编号。
- [ ] Application 中没有声明具体驱动对象或硬件配置结构。
- [ ] Board 层拥有所有板级实例、配置和 HAL 适配函数。
- [ ] 通用驱动不包含 MCU、板卡和应用专用头文件。
- [ ] 依赖方向符合 `Application -> Board -> Driver`。
- [ ] 公开 API 使用业务语义，而不是寄存器或总线语义。
- [ ] 配置结构能够描述资源，初始化函数签名保持稳定。
- [ ] 示例工程使用本地复制的模块源码，不跨工程引用源文件。
- [ ] 错误码、ISR 可用性和并发策略已有定义。
- [ ] 通用驱动可以通过假后端进行独立测试。

只要其中任一项不满足，就应先确认该依赖是否确实属于当前层，而不是为了调用方便而泄漏实现细节。
