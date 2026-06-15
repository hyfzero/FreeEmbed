# 串行存储器与总线抽象设计

## 1. 目标

本模块为烧录器、数据记录器和配置存储等应用提供统一的串行存储访问接口，同时隔离以下变化：

- Application 的业务流程；
- 存储器的读、编程、擦除语义；
- SPI、I2C 等通信协议；
- MCU、HAL、RTOS 和板级资源。

模块采用同步阻塞接口，不使用动态内存，不直接依赖厂商 HAL 或 FreeRTOS。

## 2. 分层

```text
Application
    |
    v
Storage API
    |
    +-- JEDEC SPI NOR
    `-- 24Cxx I2C EEPROM
             |
             v
       SPI / I2C Transport
             |
             v
       Board / HAL Adapter
```

各层职责如下：

- Application：编排擦除、编程、校验、进度和错误恢复。
- Storage：定义统一操作、容量、页和擦除能力。
- Device Driver：实现芯片指令、分页、地址转换和忙状态轮询。
- Transport：表达完整总线事务，不包含存储器语义。
- Board：绑定外设、GPIO、片选、锁、HAL 和时间源。

## 3. Storage 接口契约

`Storage` 通过 `StorageOps` 提供以下操作：

```c
StorageStatus storage_init( Storage *storage );
StorageStatus storage_read( Storage *storage,
                            uint64_t address,
                            uint8_t *data,
                            size_t length );
StorageStatus storage_program( Storage *storage,
                               uint64_t address,
                               const uint8_t *data,
                               size_t length );
StorageStatus storage_erase( Storage *storage,
                             uint64_t address,
                             uint64_t length );
StorageStatus storage_get_info( const Storage *storage,
                                StorageInfo *info );
```

地址和容量统一使用 64 位。具体驱动必须根据器件能力检查范围和地址格式。

`program` 表示器件原生编程操作，不隐含擦除：

- SPI NOR 在编程前由 Application 显式调用 `storage_erase()`。
- EEPROM 可以直接调用 `storage_program()`。
- EEPROM 不支持擦除，调用 `storage_erase()` 返回 `STORAGE_UNSUPPORTED`。

这样可以避免自动擦除整个扇区时破坏调用方未覆盖的数据。

### 3.1 `StorageInfo` 字段语义

```c
typedef struct
{
    uint64_t capacity_bytes;
    uint32_t page_size_bytes;
    uint32_t erase_size_bytes;
    uint32_t capabilities;
    uint8_t erased_value;
} StorageInfo;
```

`StorageInfo` 描述的是器件已经初始化并识别成功后的运行时几何信息和能力。Application 可以据此生成合法的操作计划，而不需要判断具体驱动类型。

| 字段 | 单位 | 含义 | 是否表示能力 |
| --- | --- | --- | --- |
| `capacity_bytes` | 字节 | 从地址 `0` 开始可访问的总容量 | 否 |
| `page_size_bytes` | 字节 | 单次原生编程不能跨越的页边界 | 否 |
| `erase_size_bytes` | 字节 | 公共接口允许的最小擦除对齐和长度粒度 | 否 |
| `capabilities` | 位集合 | 器件支持读、编程、擦除等哪些操作 | 是 |
| `erased_value` | 字节值 | 擦除完成后存储单元通常呈现的值 | 否 |

`capacity_bytes` 和 `capabilities` 不可相互替代：

- `capacity_bytes` 回答“这个器件能存多少字节”。
- `capabilities` 回答“这个器件能够执行哪些操作”。
- 两颗容量相同的器件可以有完全不同的能力，例如 2 MiB SPI NOR 支持硬件擦除，而 2 MiB EEPROM 可能没有独立擦除操作。
- 两颗能力相同的器件也可以有不同容量，例如 W25Q16 和 W25Q128 都支持读、编程和擦除，但容量不同。

地址范围统一为半开区间：

```text
[0, capacity_bytes)
```

因此长度为 `length` 的操作必须满足：

```text
address < capacity_bytes
length <= capacity_bytes - address
```

检查时应使用减法形式，不能直接依赖 `address + length <= capacity_bytes`，否则加法可能发生整数溢出。零长度操作由公共 Storage 层按接口实现约定处理。

`page_size_bytes` 不是 Application 每次必须写入的长度，也不是地址必须对齐的要求。它表示驱动必须遵守的硬件分页边界。例如页大小为 256 字节，从地址 `0x01F0` 写入 32 字节时，驱动需要拆成：

```text
0x01F0..0x01FF  16 bytes
0x0200..0x020F  16 bytes
```

`erase_size_bytes` 表示公共擦除接口的最小粒度：

- SPI NOR 通常为 4096 字节。
- 不支持独立擦除的 EEPROM 设置为 `0`。
- 如果 NOR 同时支持 4 KiB、32 KiB 和 64 KiB 擦除，字段仍填写最小的 4 KiB；驱动内部可以选择更大的擦除命令优化事务数量。

`erased_value` 是器件擦除后的典型读取值。SPI NOR 通常为 `0xFF`。它用于空白检查、镜像填充和烧录规划，但不表示当前区域已经被擦除，也不能代替实际读取校验。

### 3.2 能力位

`capabilities` 是位掩码，可以同时设置多个标志：

```c
#define STORAGE_CAP_READ           ( 1UL << 0 )
#define STORAGE_CAP_PROGRAM        ( 1UL << 1 )
#define STORAGE_CAP_ERASE          ( 1UL << 2 )
#define STORAGE_CAP_REQUIRES_ERASE ( 1UL << 3 )
```

每个标志的契约如下：

| 标志 | 含义 |
| --- | --- |
| `STORAGE_CAP_READ` | 支持通过 `storage_read()` 读取数据 |
| `STORAGE_CAP_PROGRAM` | 支持通过 `storage_program()` 改写数据 |
| `STORAGE_CAP_ERASE` | 支持独立的 `storage_erase()` 操作 |
| `STORAGE_CAP_REQUIRES_ERASE` | 编程前通常必须先把目标区域恢复到擦除态 |

`STORAGE_CAP_REQUIRES_ERASE` 是编程约束，不等于 `STORAGE_CAP_ERASE`。当前 SPI NOR 两者同时存在，但接口保留独立标志，是为了准确表达未来设备：

- 某些设备可以编程，但擦除由外部控制器或更高层服务完成。
- 某些设备支持擦除命令，但特定写入模式不一定要求每次预擦除。

典型 SPI NOR：

```c
info.capabilities = STORAGE_CAP_READ
                  | STORAGE_CAP_PROGRAM
                  | STORAGE_CAP_ERASE
                  | STORAGE_CAP_REQUIRES_ERASE;
```

典型 I2C EEPROM：

```c
info.capabilities = STORAGE_CAP_READ
                  | STORAGE_CAP_PROGRAM;
```

Application 在建立烧录流程前应先读取能力，而不是根据设备名称猜测：

```c
StorageInfo info;

if ( storage_get_info( storage, &info ) != STORAGE_OK )
{
    return APP_STORAGE_ERROR;
}

if ( ( info.capabilities & STORAGE_CAP_PROGRAM ) == 0U )
{
    return APP_STORAGE_READ_ONLY;
}

if ( ( info.capabilities & STORAGE_CAP_REQUIRES_ERASE ) != 0U )
{
    /* 先按 erase_size_bytes 规划并执行擦除。 */
}
```

### 3.3 `program`、`write` 和 `erase` 的术语

公共接口使用 `program` 而不是 `write`，用于强调它是存储芯片的原生编程行为：

- 不承诺像 RAM 一样可以任意覆盖旧值。
- 不自动执行扇区读改写。
- 不自动擦除。
- 不自动校验。
- 不提供掉电原子性。

自动擦除、镜像比较、空白检查、写后校验、失败重试和断点恢复属于烧录器 Application 或更高层 Storage Service，不应隐藏在器件驱动中。

### 3.4 初始化和错误契约

具体对象必须先构造，再调用 `storage_init()`。初始化成功前调用读、编程或擦除返回 `STORAGE_NOT_INITIALIZED`。

公共错误含义：

| 错误 | 典型原因 |
| --- | --- |
| `STORAGE_INVALID_ARGUMENT` | 空对象、空缓冲区或非法长度参数 |
| `STORAGE_NOT_INITIALIZED` | 尚未成功初始化 |
| `STORAGE_OUT_OF_RANGE` | 地址范围超过 `capacity_bytes` |
| `STORAGE_UNALIGNED` | 擦除地址或长度不符合最小擦除粒度 |
| `STORAGE_UNSUPPORTED` | 设备不支持请求的操作 |
| `STORAGE_IO_ERROR` | 总线或设备通信失败 |
| `STORAGE_TIMEOUT` | 编程、擦除或写周期在时限内未完成 |
| `STORAGE_NOT_FOUND` | 未探测到器件或 JEDEC ID 未匹配 |
| `STORAGE_BUSY` | 资源或设备暂时忙 |
| `STORAGE_BAD_CONFIG` | 几何参数、地址宽度或操作表配置无效 |

## 4. Transport 事务模型

### 4.1 SPI

一次 `spi_transfer()` 调用表示一个完整事务。Board 适配器必须按以下顺序执行：

1. 获取共享总线锁；
2. 根据 `SpiDevice` 配置模式和频率；
3. 拉低对应片选；
4. 连续处理所有 `SpiSegment`；
5. 释放片选；
6. 释放总线锁。

分段仅用于表达命令、地址、Dummy 和数据缓冲区边界，不允许在分段之间切换片选。

### 4.2 I2C

一个 `I2cMessage` 内的多个 `I2cSegment` 必须连续发送，不产生停止或重复起始。

同一次 `i2c_transfer()` 中相邻的 `I2cMessage` 之间使用重复起始，最后一个消息完成后产生停止条件。典型 EEPROM 随机读由以下两个消息组成：

```text
START + address(W) + word address
RESTART + address(R) + data + STOP
```

`i2c_probe()` 用于 EEPROM 写周期 ACK 轮询。Board 适配器应将地址 NACK 转换为 `HW_STATUS_NOT_FOUND` 或 `HW_STATUS_BUSY`。

## 5. 时间、错误与并发

器件配置通过 `HwTime` 注入：

```c
typedef struct
{
    uint32_t ( *get_time_ms )( void *ctx );
    void ( *delay_ms )( void *ctx, uint32_t delay_ms );
    void *ctx;
} HwTime;
```

驱动使用无符号时间差判断超时，允许毫秒计数器回绕。

Board 层必须把 HAL 返回值转换为 `HwStatus`，不得把厂商状态枚举传播到通用驱动。应用重试、降级和告警属于 Application 职责。

共享总线的互斥由 Board Transport 实现负责。器件驱动自身不依赖 mutex，也不保证从 ISR 调用安全。所有公开存储操作仅允许在任务或主循环上下文调用。

## 6. SPI NOR

### 6.1 配置和型号参数分离

SPI NOR 对象由两类信息组成：

- `SpiNorConfig` 描述板上实例：SPI 设备、片选标识、总线频率、时间源和传输超时。
- `SpiNorProfile` 描述芯片型号：JEDEC ID、容量、页大小、地址宽度、操作码、忙标志和擦除类型。

这样设计是为了隔离两个独立变化源。同一种 W25Q 芯片可以出现在不同板卡和不同片选上；同一个 SPI Transport 也可以连接不同容量、不同指令集的 NOR。新增板卡不需要复制器件算法，新增兼容芯片通常只需要增加参数表。

### 6.2 初始化和型号识别

初始化流程如下：

```text
校验对象和 Transport
    |
    v
发送 Read JEDEC ID (0x9F)
    |
    v
读取 manufacturer/type/capacity 三字节
    |
    v
在配置表或默认 W25Q 表中匹配
    |
    v
校验容量、页、地址宽度和擦除参数
    |
    v
需要时发送 Enter 4-Byte Address Mode
    |
    v
发布 StorageInfo 并标记 initialized
```

首版内置 W25Q16、W25Q32、W25Q64、W25Q128、W25Q256 和 W25Q512 参数。

首版使用 JEDEC ID 参数表而不解析 SFDP，原因是：

- 参数表代码路径短，适合资源受限 MCU。
- 支持范围明确，测试结果确定。
- 不需要在首版实现复杂的 SFDP 表版本和厂商差异处理。
- 后续可以在不改变 Storage API 的前提下增加 SFDP 探测。

代价是未收录的芯片即使指令兼容也不会自动工作，必须显式增加 `SpiNorProfile`。

### 6.3 SPI 原子事务

一次 `spi_transfer()` 表示一次完整片选事务。以普通读取为例：

```text
CS = 0
TX: READ opcode
TX: address bytes
RX: payload
CS = 1
```

命令/地址和数据使用不同 `SpiSegment`，但 Board 不得在段间释放 CS。分段模型避免为了拼接命令和大块数据而申请临时缓冲区，也让 DMA 后端可以直接使用调用方缓冲区。

写使能和页编程是两个独立事务：

```text
Transaction 1: CS low -> WREN -> CS high
Transaction 2: CS low -> PROGRAM + address + data -> CS high
Transaction 3..N: RDSR busy polling
```

不能把 WREN 和 PROGRAM 合并在同一个 CS 周期，因为芯片只会在前一个命令结束、CS 释放后锁存写使能状态。

### 6.4 分页编程

SPI NOR 的 Page Program 不能跨越物理页边界。驱动按以下公式计算每个分片：

```text
page_offset = address % page_size_bytes
page_room   = page_size_bytes - page_offset
chunk       = min(remaining, page_room)
```

每个分片执行：

```text
WREN
PROGRAM + address + chunk data
轮询 WIP 直到清零或超时
```

驱动负责正确拆页，但不负责判断旧数据能否直接编程。对于常见 NOR，位只能通过编程从 `1` 变为 `0`，从 `0` 恢复为 `1` 必须擦除。调用方必须根据能力位安排擦除和校验。

### 6.5 擦除规划

默认 W25Q 参数提供 4 KiB、32 KiB 和 64 KiB 擦除类型。`StorageInfo.erase_size_bytes` 发布最小的 4 KiB 粒度，因此公共擦除请求必须满足：

```text
address % erase_size_bytes == 0
length  % erase_size_bytes == 0
```

请求合法后，驱动在当前位置选择“地址对齐、尺寸不超过剩余长度”的最大擦除类型。例如从 64 KiB 对齐地址擦除 68 KiB：

```text
64 KiB Block Erase
4 KiB Sector Erase
```

这种贪心选择减少命令数量，同时保持调用方请求的精确范围。驱动不会为了使用更大的擦除命令而扩大范围，因为那会破坏相邻数据。

每个擦除命令拥有自己的最大超时。执行顺序为：

```text
WREN
ERASE opcode + address
轮询 WIP
完成或返回 STORAGE_TIMEOUT
```

### 6.6 忙状态和时间

编程和擦除完成时间受电压、温度、磨损和操作类型影响，因此不能只使用固定延时。驱动通过 Read Status Register 读取 WIP 位：

```text
读取状态
    |
    +-- WIP = 0 -> 完成
    |
    `-- WIP = 1 -> delay 1 ms -> 再次读取
```

超时判断使用注入的 `HwTime`，并通过无符号时间差支持 32 位毫秒计数器回绕。轮询是同步阻塞的，所以不能从 ISR 调用。

### 6.7 四字节地址

三字节地址最多表达 16 MiB。W25Q256、W25Q512 等更大器件需要四字节地址。Profile 同时描述地址字节数和可选的进入四字节模式命令，驱动在初始化时完成模式设置，之后所有地址编码统一按 Profile 执行。

当前模型假设器件具有均匀擦除结构。混合扇区、Bank Register 或特殊启动区需要扩展 Profile 的区域模型，但不需要改变 Application 使用的 Storage 接口。

## 7. I2C EEPROM

### 7.1 为什么使用显式配置

24Cxx 系列通常没有统一的运行时型号识别命令。同一个 I2C 地址上可能是不同容量和页大小的器件，错误猜测几何信息可能造成地址回绕和数据覆盖。因此 EEPROM 不自动识别型号，而由 Board 通过 `I2cEepromConfig` 明确提供：

- 总容量和页大小；
- 一个或两个字节的内部字地址；
- 映射到 I2C 从地址低位的块地址位数；
- 单次传输和内部写周期超时；
- 基础 7 位 I2C 地址。

构造阶段会检查配置容量是否能由“字地址位数 + 块地址位数”表达，避免运行后才发现地址不可达。

### 7.2 字地址和块地址

EEPROM 访问同时可能包含两种地址：

```text
I2C slave address: 选择器件或器件内部地址块
word address:      选择该块中的具体字节
```

具有两字节字地址的器件通常直接发送：

```text
device address + A15..A8 + A7..A0
```

24C04、24C08、24C16 等小容量器件会把高位存储地址映射到 I2C 从地址的低位。`block_address_bits` 用来描述这种组织方式：

```text
effective_device_address =
    base_address with low block bits replaced by address block number
```

当读写跨越块边界时，从机地址会改变，所以驱动必须在边界处分段并重新发起事务。这个规则不能由通用 I2C Transport 推断，因为它属于 EEPROM 芯片地址组织，而不是 I2C 协议本身。

### 7.3 随机读取和重复起始

EEPROM 随机读取先以写方向发送内部字地址，再切换到读方向接收数据：

```text
START
slave address + W
word address bytes
REPEATED START
slave address + R
data bytes
STOP
```

驱动使用一个包含两个 `I2cMessage` 的 `i2c_transfer()` 表达该过程。消息之间必须产生重复起始，不能提前 STOP。若 Board 把两条消息实现成两个互不相关的 HAL 调用并在中间产生 STOP，部分器件可能不能正确锁存随机读地址。

读取没有页写限制，但仍不能跨越会导致 I2C 从地址变化的块边界，因此长读取按块分段。

### 7.4 分页编程

EEPROM 页写只能在当前页内进行。如果从页尾继续发送数据，常见器件会回绕到该页开头并覆盖先前内容，而不是自动进入下一页。

每个写分片同时受两个边界限制：

```text
page_room  = page_size - address % page_size
block_room = block_size - address % block_size
chunk      = min(remaining, page_room, block_room)
```

单个页写事务包含两个连续 Segment：

```text
START + slave address(W)
Segment 1: word address
Segment 2: payload
STOP
```

Segment 之间不能产生重新寻址、重复起始或停止。使用分段而不是拼接缓冲区，可以避免为“地址 + 一页数据”额外申请临时内存。

### 7.5 ACK 轮询

EEPROM 在接收 STOP 后开始内部非易失写入。在写周期内，它通常会对自己的地址返回 NACK。驱动在每个页写后调用 `i2c_probe()`：

```text
probe effective address
    |
    +-- ACK -> 写周期完成
    |
    +-- NACK/BUSY -> delay 1 ms -> 重试
    |
    `-- 其他总线错误 -> 立即返回
```

ACK 轮询优于固定等待最大写周期：

- 芯片提前完成时可以立即继续。
- 不需要依赖某个型号的典型延时。
- 器件永久无响应时可以通过 `write_cycle_timeout_ms` 退出。
- 可以在主机测试中确定性模拟忙周期和超时。

Board 必须区分“地址 NACK，器件仍忙”和“总线控制器故障”。前者转换为 `HW_STATUS_NOT_FOUND` 或 `HW_STATUS_BUSY` 供驱动重试，后者应立即传播为 I/O 错误。

### 7.6 为什么 EEPROM 不实现擦除

EEPROM 的字节/页编程在器件内部完成擦写，不要求 Application 先发独立擦除命令。因此它发布：

```text
READ | PROGRAM
```

并设置：

```text
erase_size_bytes = 0
erase operation  = unsupported
```

用 `storage_program()` 写入一片 `0xFF` 不能等同于硬件擦除：

- 它仍然消耗 EEPROM 写入寿命。
- 它是普通数据写入，不是独立擦除能力。
- 它不应让 Application 误以为存在 NOR 式擦除语义。

如果业务需要“逻辑清空”，应由上层明确执行填充值写入，并单独命名为 clear、format 或 reset，而不是改变底层 `storage_erase()` 的契约。

## 8. Board 绑定示例

Board 层拥有总线、具体器件对象、时间回调和硬件参数：

```c
static SpiBus board_spi_bus =
{
    &board_spi_ops,
    &hspi1
};

static SpiNor board_flash;

static const SpiNorConfig board_flash_config =
{
    .name = "programmer_flash",
    .device =
    {
        .bus = &board_spi_bus,
        .chip_select = BOARD_FLASH_CS,
        .max_frequency_hz = 10000000U,
        .mode = 0U
    },
    .time =
    {
        .get_time_ms = board_get_time_ms,
        .delay_ms = board_delay_ms,
        .ctx = 0
    },
    .profiles = 0,
    .profile_count = 0U,
    .transfer_timeout_ms = 100U
};
```

Board 初始化时构造并探测器件，再向 Application 暴露 `Storage *`。Application 不应看到 `SpiNor`、HAL 句柄、片选编号或芯片参数表。

## 9. 扩展规则

新增存储器时：

1. 先判断现有 `Storage` 语义是否足够，不把芯片寄存器操作加入公共接口。
2. 新驱动组合 `Storage` 基类并实现 `StorageOps`。
3. 使用现有 Transport；只有协议事务无法表达时才扩展 Transport。
4. 把芯片型号差异放入配置或参数表。
5. 使用假 Transport 测试命令、分页、边界、错误和超时。
6. 在 Board 层绑定 HAL 和硬件资源。

新增总线后端时：

1. 保持 Transport 公共头文件不依赖 HAL。
2. 将完整事务作为加锁单位。
3. 明确片选、重复起始、停止和超时行为。
4. 将平台错误统一转换为 `HwStatus`。

## 10. 测试

主机测试位于 `tests/storage`，通过假 SPI、I2C 和时间源验证：

- JEDEC ID 匹配和几何信息；
- SPI 多段事务、分页编程和擦除选择；
- 轮询超时及计数器回绕；
- EEPROM 页拆分、块地址切换和重复起始；
- ACK 轮询、越界、未初始化和不支持操作。

标准构建方式：

```powershell
cmake -S tests/storage -B build/storage-tests
cmake --build build/storage-tests
ctest --test-dir build/storage-tests --output-on-failure
```
