# FreeEmbed HALite 嵌入式操作抽象框架

本文档把 LED 这类具体嵌入式操作抽象成可复用步骤。目标是让应用层只表达业务动作，不直接认识 GPIO、PWM、I2C、寄存器、芯片 HAL 或 FreeRTOS。

## 1. 设计目标

- 硬件无关：应用层只调用统一接口，例如 `led_on()`、`led_off()`。
- 高内聚：LED 相关接口和实现集中在 LED 模块内。
- 低耦合：核心抽象层不包含 MCU、HAL 或 FreeRTOS 头文件。
- 可扩展：新增一种硬件实现时只增加子类文件，不改应用层。
- 板级可读：硬件资源用配置表描述，初始化调用保持短小。
- 参数稳定：未来增加字段时优先扩展配置结构，不反复修改初始化函数签名。

## 2. 通用四层架构

| 层级 | 职责 | LED 对应 |
| --- | --- | --- |
| 第 1 层：抽象接口层 | 定义基类对象、操作表和统一分发函数 | `LedBase`、`LedOps`、`led_on()` |
| 第 2 层：子类实现层 | 按具体硬件实现操作表，保存私有资源 | `LedGpio`，未来可扩展 `LedPwm`、`LedI2c` |
| 第 3 层：板级绑定层 | 静态实例化对象，绑定真实硬件资源，暴露全局句柄 | `board_init.c`、`g_led_alarm` |
| 第 4 层：应用层 | 只使用句柄和抽象 API，不接触硬件细节 | `led_on( g_led_alarm )` |

## 3. 抽象一个嵌入式操作的步骤

1. 识别应用动作：只保留业务真正需要的动词，例如 LED 的 `on/off/set_brightness`。
2. 区分必选和可选能力：所有 LED 必须支持 `on/off`，GPIO LED 可以不支持 `set_brightness`。
3. 定义基类和操作表：基类保存公共状态，操作表保存多态函数指针。
4. 实现分发函数：必选操作使用 `assert` 防御，可选操作允许函数指针为 `NULL` 并静默跳过。
5. 为每种硬件写子类：第一个成员放基类，后续成员放自己的硬件资源或描述符。
6. 在板级层绑定资源：板级层创建静态对象和配置表，传入引脚、通道、地址、有效电平等真实参数。
7. 应用层只拿句柄：应用层只看到 `LedBase *`，不关心底层是 GPIO、PWM 还是 I2C。

## 4. LED 抽象接口

`LedOps` 是 LED 的操作表：

```c
typedef struct LedBase LedBase;

typedef struct
{
    void ( *on )( LedBase *me );
    void ( *off )( LedBase *me );
    void ( *set_brightness )( LedBase *me, int val );
} LedOps;
```

`LedBase` 是所有 LED 子类共享的基类：

```c
struct LedBase
{
    const char *name;
    int state;
    const LedOps *ops;
};
```

统一分发规则：

- `led_on()`：要求 `me`、`me->ops`、`me->ops->on` 都有效，调用成功后 `state = 1`。
- `led_off()`：要求 `me`、`me->ops`、`me->ops->off` 都有效，调用成功后 `state = 0`。
- `led_set_brightness()`：仅当 `set_brightness` 存在时调用；不支持调光时静默跳过。

## 5. GPIO 子类的新接口

GPIO LED 不再使用这种长参数形式：

```c
led_gpio_init( &obj, "alarm", write, ctx, pin, active_level );
```

改为描述符和配置结构：

```c
typedef void ( *LedGpioWriteFn )( void *ctx, uint32_t pin, int level );

typedef struct
{
    LedGpioWriteFn write;
    void *ctx;
    uint32_t pin;
    uint8_t active_level;
} LedGpioLine;

typedef struct
{
    const char *name;
    LedGpioLine line;
} LedGpioConfig;

void led_gpio_init( LedGpio *me, const LedGpioConfig *config );
```

板级层可以用宏把配置写成硬件表：

```c
static const LedGpioConfig alarm_led_config =
    LED_GPIO_CONFIG( "alarm", gpio_write, &alarm_port, 0U, LED_GPIO_ACTIVE_HIGH );
```

再初始化对象：

```c
led_gpio_init( &alarm_obj, &alarm_led_config );
g_led_alarm = led_gpio_as_base( &alarm_obj );
```

这种形式的优点：

- 参数有名字和层次，`name` 与 GPIO line 信息不会混在一串裸参数里。
- 多个 LED 的资源可以写成表，board 层更像硬件清单。
- 初始化函数签名稳定，后续增加 flags、drive mode、debounce 等字段时优先扩展配置结构。
- 核心 LED 模块仍然只认识 `write + ctx + pin + active_level`，不依赖具体 MCU。

## 6. 板级绑定示例

板级层是唯一知道硬件资源的地方：

```c
LedBase *g_led_alarm;

static LedGpio alarm_obj;
static ExampleGpioPort alarm_port = { "ALARM_PORT", 0U, 0 };

static const LedGpioConfig alarm_led_config =
    LED_GPIO_CONFIG( "alarm", example_gpio_write, &alarm_port, 0U, LED_GPIO_ACTIVE_HIGH );

void board_init( void )
{
    led_gpio_init( &alarm_obj, &alarm_led_config );
    g_led_alarm = led_gpio_as_base( &alarm_obj );
}
```

应用层只使用全局句柄：

```c
board_init();
led_on( g_led_alarm );
led_set_brightness( g_led_alarm, 50 );
```

如果 `g_led_alarm` 底层是 GPIO，调光接口会自动跳过；如果后续换成 PWM 子类，应用层代码不需要变化。

## 7. 与开源项目风格的关系

这个接口借鉴了成熟项目的描述符思路，但保持 FreeEmbed 自己的轻量实现：

- Zephyr 常用 `gpio_dt_spec` 把 GPIO device、pin、flags 聚合成一个描述符，调用点不需要到处传散落参数。
- Linux GPIO consumer API 也强调通过 GPIO descriptor 操作 GPIO，并让 active-low 这类语义属于 descriptor。
- Mbed OS 的 `DigitalOut` 也把 pin 和初始值收进构造阶段，用户后续只操作对象。

FreeEmbed 不直接依赖这些项目的类型；只采用“先描述硬件线，再初始化对象”的接口形态。

## 8. 扩展到其他嵌入式操作

按同样模式可以抽象其他外设：

| 操作对象 | 必选动作 | 可选动作 | 子类例子 |
| --- | --- | --- | --- |
| Button | `read` | `enable_irq`、`disable_irq` | `ButtonGpio`、`ButtonMatrix` |
| Buzzer | `on`、`off` | `set_frequency`、`set_volume` | `BuzzerGpio`、`BuzzerPwm` |
| Motor | `start`、`stop` | `set_speed`、`set_direction` | `MotorPwm`、`MotorCan` |
| Sensor | `read` | `calibrate`、`sleep`、`wakeup` | `SensorI2c`、`SensorSpi` |

抽象时优先从应用语言出发，而不是从硬件驱动函数出发。应用层要表达“打开告警灯”，不应该表达“写 GPIOA pin0 为高电平”。

## 9. 与 FreeRTOS 的关系

该模块与 FreeRTOS 兼容但彼此独立：

- LED 核心层不包含 `FreeRTOS.h`、`task.h` 或 `semphr.h`。
- LED 核心层不创建任务、不申请队列、不持有互斥量。
- 如果某个板级写函数需要任务保护，可以在板级层或 BSP 层使用 FreeRTOS 临界区或互斥量。
- 应用任务中可以正常调用 LED API，但并发策略由调用方或底层硬件适配层决定。
