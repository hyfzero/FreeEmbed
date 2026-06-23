# EZPro100_v20 IAR 到 Keil MDK5 转换说明

## 1. 工程范围

本次只转换 APP 工程：

- IAR 工程：`Project\EZPro10_v910\EWARM\EZPro100_v20.ewp`
- Keil 工程：`Project\EZPro10_v910\MDK-ARM\EZPro100_v20.uvprojx`

未转换 `Project\IAP` 工程。原 `EWARM` 目录、IAR `.ewp/.eww/.ewd/.icf` 文件，以及原有 `src`、`inc`、`Libraries`、`Utilities` 源文件均未删除、未移动、未覆盖。

## 2. 转换依据

IAR 工程目标名为 `STM3210E-EVAL`。工程属性中存在过时的 `STM32F103VB` 字段，但该字段与实际 APP 构建不一致，不能作为 Keil 转换依据。实际依据如下：

- IAR 宏定义包含 `STM32F10X_HD`。
- IAR 实际链接 High Density 启动对象。
- IAR linker 文件 `EWARM\stm32f10x_flash.icf` 将 APP Flash 起始地址设为 `0x08004000`。
- APP 代码中 `main.c` 调用 `NVIC_SetVectorTable(NVIC_VectTab_FLASH,0x4000)`。
- `system_stm32f10x.c` 在 `SINOMCU_IAP` 宏下将 `VECT_TAB_OFFSET` 设为 `0x4000`。
- IAR RAM 区间为 `0x20000000-0x20017FFF`。

因此 Keil 工程按 STM32F10x High Density APP 工程转换，设备选择 `STM32F103ZG`，同时使用自定义 scatter 文件强制保持真实 APP 地址布局。

## 3. IAR 与 Keil 配置对应

| 项目 | IAR | Keil |
| --- | --- | --- |
| 工程文件 | `EWARM\EZPro100_v20.ewp` | `MDK-ARM\EZPro100_v20.uvprojx` |
| 工具链 | IAR EWARM | Keil MDK5 ARMCC5 |
| 输出 ELF | `EZPro100_v20.out` | `Objects\EZPro100_v20.axf` |
| HEX 输出 | IAR 输出配置 | `Objects\EZPro100_v20.hex` |
| SREC 输出 | `EZPro100_APP_v20.srec` | After Build 调用 `fromelf --m32` 生成 |
| Linker | `EWARM\stm32f10x_flash.icf` | `MDK-ARM\EZPro100_v20.sct` |
| 启动文件 | IAR High Density startup | Keil ARMASM `startup_stm32f10x_hd.s` 副本 |
| Flash 起始 | `0x08004000` | `0x08004000` |
| Flash 结束 | `0x080FFFFF` | 长度 `0x000FC000` |
| RAM 起始 | `0x20000000` | `0x20000000` |
| RAM 结束 | `0x20017FFF` | 长度 `0x00018000` |
| Stack | `0x400` | `0x400` |
| Heap | `0x800` | `0x800` |

Keil 宏定义：

```text
USE_STDPERIPH_DRIVER,STM32F10X_HD,USE_STM3210E_EVAL,SINOMCU_IAP
```

Keil include path：

```text
.
..\inc
..\..\..\Libraries\CMSIS\CM3\CoreSupport
..\..\..\Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x
..\..\..\Libraries\STM32_USB-FS-Device_Driver\inc
..\..\..\Libraries\STM32F10x_StdPeriph_Driver\inc
..\..\..\Utilities\STM32_EVAL
..\..\..\Utilities\STM32_EVAL\Common
..\..\..\Utilities\STM32_EVAL\STM3210E_EVAL
```

Keil ARMCC5 配置启用了 GNU extensions，用于兼容现有 `Config.h` 中的匿名 union 写法。

## 4. 新增 Keil 文件

`MDK-ARM` 目录下新增：

- `EZPro100_v20.uvprojx`：Keil MDK5 工程文件。
- `EZPro100_v20.uvoptx`：Keil 工程选项文件。
- `EZPro100_v20.sct`：Keil scatter 文件，映射 IAR `.icf` 的 APP 地址。
- `startup_stm32f10x_hd.s`：Keil ARMASM 启动文件副本，stack 为 `0x400`，heap 为 `0x800`。
- `stm32f10x.h`：Keil 兼容包装头。
- `gen_srec.bat`：手动生成 `EZPro100_APP_v20.srec` 的备用脚本。
- `Keil_Conversion.md`：本转换说明。

`stm32f10x.h` 包装头的作用是处理 Keil 设备包行为：选择 `STM32F103ZG` 时，Keil 可能自动注入 XL-density 宏，而原 IAR APP 工程实际按 `STM32F10X_HD` 构建。包装头在包含原始 StdPeriph `stm32f10x.h` 前取消 `STM32F10X_XL`，避免 HD/XL 中断枚举同时展开导致编译冲突。

## 5. 源文件范围

Keil 工程只引用 APP 实际参与链接的源文件：

- `Project\EZPro10_v910\src` 用户代码。
- CMSIS `core_cm3.c`。
- APP 自带 `system_stm32f10x.c`。
- STM32F10x StdPeriph Driver 所需源文件。
- USB FS Device Driver 所需源文件。
- `Utilities\STM32_EVAL\stm32_eval.c`。
- `Utilities\STM32_EVAL\STM3210E_EVAL\stm3210e_eval.c`。

`stm32f10x_sdio.c` 已加入 Keil 工程，因为 `STM32_EVAL` 层引用了 `SDIO_*` 接口，否则 ARMCC 链接阶段会出现未定义符号。

未加入 STM32L1xx 文件、IAP 工程文件、IAR startup 文件和其他未参与 APP 链接的启动变体。

## 6. 构建步骤

1. 使用 Keil uVision5 打开：
   ```text
   Project\EZPro10_v910\MDK-ARM\EZPro100_v20.uvprojx
   ```
2. 确认工具链为 ARM Compiler 5，不使用 ARM Compiler 6。
3. 如果 Keil 提示缺少 `STM32F103ZG`，安装或更新 STM32F1 DFP，不建议改成低容量设备型号替代。
4. 执行 Rebuild。

期望输出：

```text
MDK-ARM\Objects\EZPro100_v20.axf
MDK-ARM\Objects\EZPro100_v20.hex
MDK-ARM\Objects\EZPro100_APP_v20.srec
MDK-ARM\Listings\EZPro100_v20.map
```

After Build 命令配置为：

```text
fromelf --m32 --output .\Objects\EZPro100_APP_v20.srec .\Objects\EZPro100_v20.axf
```

`nStopA1X` 设为 `0`，避免 uVision 命令行模式把已成功执行的用户命令误判为目标失败；实际验收以 Keil 汇总的 `0 Error(s)` 和 `.srec` 文件存在为准。若某台机器的 Keil User 命令环境找不到 `fromelf`，可在 Options for Target -> User 中改成 `fromelf.exe` 绝对路径，或在工程目录运行 `gen_srec.bat` 生成 SREC。

## 7. 本机验证结果

已在本机 Keil MDK5 ARMCC5 下执行命令行 Rebuild：

```text
"Objects\EZPro100_v20.axf" - 0 Error(s), 884 Warning(s).
Program Size: Code=34800 RO-data=1772 RW-data=752 ZI-data=20696
```

已生成：

```text
Objects\EZPro100_v20.axf
Objects\EZPro100_v20.hex
Objects\EZPro100_APP_v20.srec
Listings\EZPro100_v20.map
```

map 检查项：

- `RESET`/向量表位于 `0x08004000`。
- Flash load region 为 `0x08004000` 起始，最大长度 `0x000FC000`。
- RAM execution region 为 `0x20000000` 起始，最大长度 `0x00018000`。

当前 warning 主要来自原工程旧式函数声明、未使用变量、文件末尾缺少换行等历史代码风格问题；这些不是本次 IAR 到 Keil 工程转换新增的链接错误。
