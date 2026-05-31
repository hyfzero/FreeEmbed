# OOP in C 分层硬件抽象框架

本文档把 LED 这类具体嵌入式操作抽象成可复用步骤。目标是让应用层只表达业务动作，不直接认识 GPIO、PWM、I2C、寄存器、HAL 或 FreeRTOS。

## 1. 设计目标

- **硬件无关**：应用层只调用统一接口，例如 `led_on()`、`led_off()`。
- **高内聚**：每种外设能力集中在自己的抽象模块内，LED 相关代码只放在 LED 模块。
- **低耦合**：核心抽象层不包含 MCU、HAL、FreeRTOS 头文件。
- **可扩展**：新增一种硬件实现只增加子类文件，不改应用层。
- **可移植**：换芯片、换板卡或换驱动库时，只调整板级绑定层。
- **参数化构造**：引脚、通道、地址、有效电平等资源由 `init` 函数传入。

## 2. 通用四层架构

| 层级 | 职责 | LED 对应 |
| --- | --- | --- |
| 第 1 层：抽象接口层 | 定义基类对象、操作表和统一分发函数 | `LedBase`、`LedOps`、`led_on()` |
| 第 2 层：子类实现层 | 按具体硬件实现操作表，保存私有资源 | `LedGpio`、未来 `LedPwm`、`LedI2c` |
| 第 3 层：板级绑定层 | 静态实例化对象，传入真实硬件参数，暴露全局句柄 | `board_init.c`、`g_led_alarm` |
| 第 4 层：应用层 | 只使用句柄和抽象 API，不接触硬件细节 | `led_on( g_led_alarm )` |

## 3. 抽象一个嵌入式操作的步骤

1. **识别应用动作**：只保留业务真正需要的动词，例如 LED 的 `on/off/set_brightness`。
2. **区分必选与可选能力**：所有 LED 必须支持 `on/off`，但 GPIO LED 可以不支持 `set_brightness`。
3. **定义基类与操作表**：基类保存公共状态，操作表保存多态函数指针。
4. **实现分发函数**：必选操作使用 `assert` 防御；可选操作允许函数指针为 `NULL` 并跳过。
5. **为每种硬件写子类**：子类第一个成员放基类，后面放自己的硬件资源和回调。
6. **板级绑定硬件资源**：板级层创建静态对象，传入引脚、通道、地址、有效电平等参数。
7. **应用层只拿句柄**：应用层只看到 `LedBase *`，不知道底层是 GPIO、PWM 还是 I2C。

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

## 5. GPIO 子类模式

GPIO LED 的私有字段包括写函数、写函数上下文、引脚号和有效电平：

```c
typedef void ( *LedGpioWriteFn )( void *ctx, uint32_t pin, int level );

typedef struct
{
    LedBase base;
    LedGpioWriteFn write;
    void *ctx;
    uint32_t pin;
    uint8_t active_level;
} LedGpio;
```

这种设计不直接写死 `GPIO_TypeDef *port`。板级层可以把 `ctx` 解释成 STM32 GPIO 端口、寄存器地址、FreeRTOS Demo 的 `ParTest` 包装对象，或任意 BSP 自己的对象。

## 6. 板级绑定示例

板级层是唯一知道硬件资源的地方：

```c
LedBase *g_led_alarm;
static LedGpio alarm_obj;

void board_init( void )
{
    led_gpio_init( &alarm_obj, "alarm", gpio_write, gpio_ctx, 0U, 1U );
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

## 7. 扩展到其他嵌入式操作

按同样模式可以抽象其他外设：

| 操作对象 | 必选动作 | 可选动作 | 子类例子 |
| --- | --- | --- | --- |
| Button | `read` | `enable_irq`、`disable_irq` | `ButtonGpio`、`ButtonMatrix` |
| Buzzer | `on`、`off` | `set_frequency`、`set_volume` | `BuzzerGpio`、`BuzzerPwm` |
| Motor | `start`、`stop` | `set_speed`、`set_direction` | `MotorPwm`、`MotorCan` |
| Sensor | `read` | `calibrate`、`sleep`、`wakeup` | `SensorI2c`、`SensorSpi` |

抽象时优先从应用语言出发，而不是从硬件驱动函数出发。应用层要表达“打开告警灯”，不应该表达“写 GPIOA pin0 为高电平”。

## 8. 与 FreeRTOS 的关系

该模块与 FreeRTOS 兼容但彼此独立：

- LED 核心层不包含 `FreeRTOS.h`、`task.h` 或 `semphr.h`。
- LED 核心层不创建任务、不申请队列、不持有互斥量。
- 如果某个板级写函数需要任务保护，可以在板级层或 BSP 层使用 FreeRTOS 临界区/互斥量。
- 应用任务中可以正常调用 LED API，但并发策略由调用方或底层硬件适配层决定。
