# FreeRTOS V9.0.0 架构分析与开发手册

---

## 第一部分：架构总览

### 1. 项目概述

FreeRTOS V9.0.0 是由 Real Time Engineers Ltd. 于 2016 年发布的一款嵌入式实时操作系统(RTOS)内核。它采用 **GPL v2 + FreeRTOS Exception** 许可证，允许在不必开源专有组件的情况下分发包含 FreeRTOS 的组合产品。

- **版本号**: V9.0.0
- **内核版本字符串**: `tskKERNEL_VERSION_NUMBER "V9.0.0"`
- **许可证**: GPL v2 with FreeRTOS Exception
- **编程语言**: C (遵循 MISRA 编码规范, 1 tab = 4 spaces)
- **代码量**: 344 个源文件, 2311 个目录（含演示项目）

### 2. 顶层目录结构

```
FreeRTOS/
├── Source/              # 内核源代码（核心）
│   ├── include/         # 公共头文件（API 接口）
│   ├── portable/        # 平台移植层
│   │   ├── MemMang/     # 内存管理（5种堆实现）
│   │   ├── Common/      # MPU 包装器（共享）
│   │   ├── GCC/         # GCC 编译器移植（ARM Cortex-M0/M3/M4/M7/R4/R5/A9/A53, AVR, ...）
│   │   ├── IAR/         # IAR 编译器移植
│   │   ├── Keil/        # Keil/ARM 编译器移植
│   │   ├── RVDS/        # ARM RVDS 编译器移植
│   │   ├── MSVC-MingW/  # Windows 模拟器移植
│   │   ├── CCS/         # TI Code Composer Studio 移植
│   │   ├── MPLAB/       # Microchip MPLAB 移植
│   │   ├── Renesas/     # Renesas 编译器移植
│   │   ├── Rowley/      # Rowley CrossWorks 移植
│   │   ├── Tasking/     # Tasking 编译器移植
│   │   └── ...          # 其他编译器/平台（共20个移植层）
│   ├── croutine.c       # 协程实现
│   ├── event_groups.c   # 事件组实现
│   ├── list.c           # 双向链表实现
│   ├── queue.c          # 队列实现（含信号量、互斥量）
│   ├── tasks.c          # 任务管理（调度器核心）
│   └── timers.c         # 软件定时器实现
├── Demo/                # 演示应用程序（覆盖所有官方支持的移植平台）
├── License/             # 许可证文件
└── readme.txt           # 说明文件
```

---

## 第二部分：系统分层架构

FreeRTOS 采用经典的 **三层架构** 设计：

```
┌──────────────────────────────────────────────────────┐
│                  应用层 (Application)                  │
│   用户任务、回调函数、FreeRTOSConfig.h 配置            │
├──────────────────────────────────────────────────────┤
│                   内核层 (Kernel)                      │
│  ┌─────────┐  ┌─────────┐  ┌────────────┐           │
│  │  任务    │  │  队列    │  │ 事件组      │           │
│  │ 管理     │  │ (Queue)  │  │(Event Group)│           │
│  │ (Task)   │  │          │  │             │           │
│  ├─────────┤  ├─────────┤  ├────────────┤           │
│  │  信号量  │  │ 软件定时器│  │  协程       │           │
│  │(Semaphore│  │ (Timer)  │  │(Co-routine)│           │
│  │ /Mutex)  │  │          │  │             │           │
│  └─────────┘  └─────────┘  └────────────┘           │
│  ┌──────────────────────────────────────────┐       │
│  │         链表数据结构 (List)               │       │
│  │      (双向循环链表，按值降序排列)          │       │
│  └──────────────────────────────────────────┘       │
│  ┌──────────────────────────────────────────┐       │
│  │          内存管理 (heap_1~5)              │       │
│  └──────────────────────────────────────────┘       │
├──────────────────────────────────────────────────────┤
│                移植层 (Portable Layer)                │
│  ┌──────────────────────────────────────────┐       │
│  │     CPU 架构特定代码 (port.c, portasm)     │       │
│  │     上下文切换、中断管理、临界区           │       │
│  └──────────────────────────────────────────┘       │
│  ┌──────────────────────────────────────────┐       │
│  │     编译器特定宏 (portmacro.h)            │       │
│  │     数据类型定义、portYIELD等             │       │
│  └──────────────────────────────────────────┘       │
└──────────────────────────────────────────────────────┘
```

---

## 第三部分：核心模块详解

### 3.1 任务管理模块 (tasks.c / task.h)

**这是 FreeRTOS 的心脏**，负责所有任务的创建、调度、同步和状态管理。

#### 3.1.1 任务状态机

一个任务在其生命周期中经历以下状态：

```
         ┌──────────┐
         │  Running  │ ◄── 当前占用 CPU 的任务
         └─────┬─────┘
               │ 被抢占 / 主动让出
         ┌─────▼─────┐
         │   Ready   │ ◄── 就绪但未获得 CPU
         └─────┬─────┘
               │ 调用阻塞 API (vTaskDelay, xQueueReceive...)
         ┌─────▼─────┐
         │  Blocked  │ ◄── 等待事件/超时
         └─────┬─────┘
               │ 事件到达 / 超时到期
         ┌─────▼─────┐
         │   Ready   │
         └───────────┘
               ▲
               │ 外部调用 vTaskSuspend()
         ┌──────┴─────┐
         │  Suspended  │ ◄── 被挂起，不参与调度
         └──────┬─────┘
               │ 外部调用 vTaskResume()
         ┌─────▼─────┐
         │   Ready   │
         └───────────┘
               │ 调用 vTaskDelete()
         ┌─────▼─────┐
         │  Deleted   │ ◄── TCB 待空闲任务回收
         └───────────┘
```

状态枚举定义（`task.h:112-120`）:
```c
typedef enum {
    eRunning = 0,   // 正在运行
    eReady,         // 就绪
    eBlocked,       // 阻塞
    eSuspended,     // 挂起
    eDeleted,       // 已删除 (TCB 尚未释放)
    eInvalid        // 无效状态
} eTaskState;
```

#### 3.1.2 任务控制块 (TCB)

每个任务都有一个 TCB（Task Control Block），包含：
- **栈指针** (pxTopOfStack)
- **任务状态链表项** (xStateListItem) — 用于插入就绪/阻塞/挂起链表
- **事件链表项** (xEventListItem) — 用于插入事件等待链表
- **任务优先级** (uxPriority, uxBasePriority)
- **任务名称** (pcTaskName)
- **栈起始地址** (pxStack)
- **线程本地存储指针** (pvThreadLocalStoragePointers)
- **任务通知值** (ulNotifiedValue) — V9.0 新增特性
- **运行时统计计数器** (ulRunTimeCounter)
- **互斥量持有计数** (uxMutexesHeld)

为遵循严格的数据隐藏策略，对外暴露 `StaticTask_t` 哑元结构体（`FreeRTOS.h:910-952`），大小和对齐与真实 TCB 等价。

#### 3.1.3 调度器核心

FreeRTOS 实现了 **基于优先级的抢占式调度** + **时间片轮转**：

- **就绪链表数组** (`pxReadyTasksLists[]`): 每个优先级一个就绪链表
- **位图查找** (`uxTopReadyPriority`): 快速定位最高优先级的非空就绪链表
- **调度策略**: 始终运行就绪态中最高优先级的任务
- **时间片**: 同优先级任务通过 `configUSE_TIME_SLICING` 控制，默认启用（每个 tick 轮转一次）
- **任务选择优化**: `configUSE_PORT_OPTIMISED_TASK_SELECTION` 可用硬件 CLZ 指令加速最高优先级计算

关键调度器 API：

| API 函数 | 功能 |
|---------|------|
| `vTaskStartScheduler()` | 启动调度器，创建空闲任务，开始 tick 中断 |
| `vTaskSuspendAll()` | 暂停调度器（不关中断），防止上下文切换 |
| `xTaskResumeAll()` | 恢复调度器，返回 pdTRUE 表示需要上下文切换 |
| `taskYIELD()` | 强制上下文切换（等价于 `portYIELD()`） |
| `taskENTER_CRITICAL()` | 进入临界区（关中断） |
| `taskEXIT_CRITICAL()` | 退出临界区（恢复中断） |
| `vTaskSwitchContext()` | 选择最高优先级就绪任务（供移植层调用） |
| `xTaskIncrementTick()` | Tick 中断中调用，递增 tick 计数、检查超时 |

#### 3.1.4 任务创建 API

```c
// 动态内存分配方式
BaseType_t xTaskCreate(
    TaskFunction_t pvTaskCode,       // 任务函数指针
    const char * const pcName,       // 任务名（调试用，最大 configMAX_TASK_NAME_LEN）
    uint16_t usStackDepth,           // 栈深度（单位：字，非字节）
    void *pvParameters,              // 传递给任务的参数
    UBaseType_t uxPriority,          // 优先级（0 最低）
    TaskHandle_t *pvCreatedTask      // 返回的任务句柄
);

// 静态内存分配方式（V9.0 新增）
TaskHandle_t xTaskCreateStatic(
    TaskFunction_t pxTaskCode,
    const char * const pcName,
    const uint32_t ulStackDepth,
    void * const pvParameters,
    UBaseType_t uxPriority,
    StackType_t * const puxStackBuffer,    // 用户提供的栈缓冲区
    StaticTask_t * const pxTaskBuffer      // 用户提供的 TCB 缓冲区
);
```

#### 3.1.5 任务通知 (Task Notifications) — V9.0 新特性

V9.0 引入的轻量级任务间通信机制，**无需中间对象**（队列/信号量），速度更快、RAM 占用更少：

| 通知操作 | API | 说明 |
|---------|-----|------|
| 发送通知 | `xTaskNotify()` / `xTaskNotifyGive()` | 直接向目标任务发送通知 |
| ISR 中发送 | `xTaskNotifyFromISR()` | ISR 安全版本 |
| 等待通知 | `xTaskNotifyWait()` | 阻塞等待通知 |
| 获取通知 | `ulTaskNotifyTake()` | 用作二进制/计数信号量等价 |
| 清除通知状态 | `xTaskNotifyStateClear()` | 清除通知标志 |

通知值操作类型 (`eNotifyAction`):
- `eNoAction` — 仅通知，不修改值
- `eSetBits` — 按位 OR
- `eIncrement` — 递增（用作计数信号量）
- `eSetValueWithOverwrite` — 无条件覆盖
- `eSetValueWithoutOverwrite` — 仅在上次值被读取后才写入

### 3.2 链表模块 (list.c / list.h)

**FreeRTOS 中最基础的数据结构**，所有调度、排队、阻塞管理的基石。

#### 3.2.1 数据结构

```c
// 链表项
struct xLIST_ITEM {
    TickType_t xItemValue;           // 排序值（通常用于降序排列）
    struct xLIST_ITEM * pxNext;      // 下一项
    struct xLIST_ITEM * pxPrevious;  // 前一项
    void * pvOwner;                  // 指向拥有者对象（通常是 TCB）
    void * pvContainer;              // 指向所属链表
};

// 迷你链表项（用于链表尾标记）
struct xMINI_LIST_ITEM {
    TickType_t xItemValue;
    struct xLIST_ITEM * pxNext;
    struct xLIST_ITEM * pxPrevious;
};

// 链表头
typedef struct xLIST {
    UBaseType_t uxNumberOfItems;     // 链表项数量
    ListItem_t * pxIndex;            // 遍历游标
    MiniListItem_t xListEnd;         // 链表尾标记（值为 portMAX_DELAY）
};
```

#### 3.2.2 核心操作

| 函数 | 功能 | 复杂度 |
|------|------|--------|
| `vListInitialise()` | 初始化链表，设置尾标记 |
| `vListInitialiseItem()` | 初始化链表项（pvContainer=NULL） |
| `vListInsert()` | 按 xItemValue 降序插入 | O(n) |
| `vListInsertEnd()` | 插入到 pxIndex 指向位置之前 | O(1) |
| `uxListRemove()` | 从链表中移除项 | O(1) |
| `listGET_OWNER_OF_NEXT_ENTRY()` | 遍历链表并获取下一个拥有者 | O(1) |

#### 3.2.3 关键设计理念

- **双向循环链表**：尾标记 `xListEnd` 的 `pxNext` 指向真正头部，实现循环遍历
- **降序排列**：`xItemValue` 越大越靠近头部（即"最高优先级"优先被取出）
- **双向链接**：`pvOwner`（链表项 → 对象）+ 对象内嵌链表项（对象 → 链表项）形成双向引用
- **关键用途**：
  - **就绪链表** (`pxReadyTasksLists[]`): 按优先级排序，xItemValue = 优先级
  - **延迟链表** (`xDelayedTaskList1/2`): 按唤醒时间排序，xItemValue = 唤醒 tick
  - **事件等待链表**: 按优先级排序（信号量/队列等待）
  - **挂起链表** (`xSuspendedTaskList`)

### 3.3 队列模块 (queue.c / queue.h)

队列是 FreeRTOS 中**最通用的 IPC 机制**，所有信号量和互斥量都基于队列实现。

#### 3.3.1 队列类型

| 类型 | 枚举值 | 说明 |
|------|--------|------|
| 基础队列 | `queueQUEUE_TYPE_BASE` (0) | 通用 FIFO 队列 |
| 队列集 | `queueQUEUE_TYPE_SET` (0) | 多队列选择机制 |
| 互斥量 | `queueQUEUE_TYPE_MUTEX` (1) | 带优先级继承的互斥锁 |
| 计数信号量 | `queueQUEUE_TYPE_COUNTING_SEMAPHORE` (2) | 计数信号量 |
| 二进制信号量 | `queueQUEUE_TYPE_BINARY_SEMAPHORE` (3) | 二进制信号量 |
| 递归互斥量 | `queueQUEUE_TYPE_RECURSIVE_MUTEX` (4) | 可递归获取的互斥锁 |

#### 3.3.2 核心数据结构

队列采用 **数据拷贝** 而非引用传递方式：
- 队列结构体（Queue_t）包含存储区域的指针
- 数据存储区 = `uxQueueLength × uxItemSize` 字节
- 通过环形缓冲区方式管理读写位置

#### 3.3.3 核心 API

**发送操作** (任务级):
```c
xQueueSend()              // 发送到尾部（等同于 xQueueSendToBack）
xQueueSendToFront()       // 发送到头部（高优先级消息）
xQueueSendToBack()        // 发送到尾部（标准 FIFO）
xQueueOverwrite()         // 覆盖写入（仅用于长度为1的队列）
xQueueGenericSend()       // 通用发送（底层函数）
```

**发送操作** (ISR 级):
```c
xQueueSendFromISR()             // ISR 中发送
xQueueSendToFrontFromISR()      // ISR 中发送到头部
xQueueSendToBackFromISR()       // ISR 中发送到尾部
xQueueOverwriteFromISR()        // ISR 中覆盖写入
xQueueGenericSendFromISR()      // ISR 中通用发送（底层函数）
```

**接收操作**:
```c
xQueueReceive()                  // 接收并移除
xQueuePeek()                     // 查看但不移除
xQueueReceiveFromISR()           // ISR 中接收
xQueuePeekFromISR()              // ISR 中查看
xQueueGenericReceive()           // 通用接收（底层函数）
```

**查询操作**:
```c
uxQueueMessagesWaiting()         // 队列中当前消息数
uxQueueSpacesAvailable()         // 队列中可用空间
xQueueIsQueueEmptyFromISR()      // ISR 中判断队列是否为空
xQueueIsQueueFullFromISR()       // ISR 中判断队列是否已满
uxQueueMessagesWaitingFromISR()  // ISR 中获取消息数
```

#### 3.3.4 阻塞与超时机制

当队列满（发送）或空（接收）时：
1. 任务被放入队列的等待链表（按优先级排序）
2. 同时被放入延迟链表（设置超时唤醒时间）
3. 任务进入 Blocked 状态
4. 当条件满足或超时到期时，任务被移回就绪链表

#### 3.3.5 信号量与互斥量的实现

**关键设计**：信号量完全基于队列实现（`semphr.h` 中的宏展开即队列调用）：

```c
// 二进制信号量创建 = 创建长度为1、项大小为0的队列
xSemaphoreCreateBinary() → xQueueGenericCreate(1, 0, queueQUEUE_TYPE_BINARY_SEMAPHORE)

// 互斥量创建 = 创建特殊类型的队列
xSemaphoreCreateMutex() → xQueueCreateMutex(queueQUEUE_TYPE_MUTEX)

// Take = 从队列接收（不关心数据）
xSemaphoreTake() → xQueueGenericReceive(xSemaphore, NULL, xBlockTime, pdFALSE)

// Give = 向队列发送（无数据）
xSemaphoreGive() → xQueueGenericSend(xSemaphore, NULL, semGIVE_BLOCK_TIME, queueSEND_TO_BACK)
```

**互斥量的优先级继承**：当高优先级任务等待低优先级任务持有的互斥量时，低优先级任务会临时提升到高优先级，防止优先级反转。

### 3.4 软件定时器模块 (timers.c / timers.h)

#### 3.4.1 架构设计

软件定时器通过 **定时器守护任务 (Timer Daemon Task)** 实现：

```
┌──────────────┐     Timer Command Queue     ┌──────────────┐
│  应用任务     │ ──────────────────────────► │  定时器守护    │
│  (调用 API)   │   xTimerStart/Stop/Reset/   │  任务         │
│              │   ChangePeriod/Delete       │  (处理定时器)  │
└──────────────┘                             └──────┬───────┘
                                                    │
                                            按过期时间排序的
                                            定时器链表
                                                    │
                                            到期时调用回调函数
```

#### 3.4.2 定时器命令

所有定时器 API 都向定时器命令队列发送消息（`xTimerGenericCommand`），由守护任务处理：

| 命令 | 任务级 | ISR 级 | 说明 |
|------|--------|--------|------|
| START | `tmrCOMMAND_START` | `tmrCOMMAND_START_FROM_ISR` | 启动定时器 |
| STOP | `tmrCOMMAND_STOP` | `tmrCOMMAND_STOP_FROM_ISR` | 停止定时器 |
| RESET | `tmrCOMMAND_RESET` | `tmrCOMMAND_RESET_FROM_ISR` | 重置定时器 |
| CHANGE_PERIOD | `tmrCOMMAND_CHANGE_PERIOD` | `tmrCOMMAND_CHANGE_PERIOD_FROM_ISR` | 修改周期 |
| DELETE | `tmrCOMMAND_DELETE` | — | 删除定时器 |

定时器类型：
- **单次定时器** (One-shot): `uxAutoReload = pdFALSE`，到期后进入休眠态
- **自动重载定时器** (Auto-reload): `uxAutoReload = pdTRUE`，周期性触发

#### 3.4.3 Pend Function Call 机制

`xTimerPendFunctionCallFromISR()` 提供 ISR 延迟处理机制：将函数调用推迟到守护任务中执行，避免 ISR 执行过长。

### 3.5 事件组模块 (event_groups.c / event_groups.h)

#### 3.5.1 功能概述

事件组允许任务等待一个或多个事件标志位的组合：
- **位宽**：由 `configUSE_16_BIT_TICKS` 决定（16位模式=8位可用，32位模式=24位可用）
- **等待模式**：等待任意位 (OR) 或等待所有位 (AND)
- **自动清除**：等待条件满足时可自动清除指定位（防止竞态条件）
- **同步点(Rendezvous)**：通过 `xEventGroupSync()` 实现多任务同步

#### 3.5.2 核心 API

```c
xEventGroupCreate()              // 创建事件组
xEventGroupSetBits()             // 设置位（自动唤醒等待任务）
xEventGroupClearBits()           // 清除位
xEventGroupWaitBits()            // 阻塞等待位
xEventGroupSync()                // 原子设置+等待（多任务同步）
xEventGroupGetBits()             // 读取当前位值
xEventGroupDelete()              // 删除事件组

// ISR 版本（通过定时器命令队列代理执行）
xEventGroupSetBitsFromISR()
xEventGroupClearBitsFromISR()
xEventGroupGetBitsFromISR()
```

### 3.6 协程模块 (croutine.c / croutine.h)

协程（Co-routine）是 FreeRTOS 的轻量级并发机制：
- **共享栈**：所有协程共用同一栈空间，RAM 开销极小
- **非抢占**：协程间通过 `crYIELD()` 主动让出 CPU
- **优先级调度**：类似任务的优先级机制
- **与任务的区别**：
  - 任务有独立栈，协程共享栈
  - 任务可抢占，协程合作式调度
  - 协程不能阻塞（会阻塞所有协程）
- **默认禁用**：`configUSE_CO_ROUTINES` 默认为 0

### 3.7 内存管理模块 (portable/MemMang/) — Heap 与 RTOS 深度集成

#### 3.7.1 Heap 在 RTOS 中的角色

FreeRTOS 内核在运行时会频繁调用 `pvPortMalloc()` / `vPortFree()`，Heap 是**所有动态创建操作的内存支柱**：

```
 ┌─────────────────────────────────────────────────┐
 │                  应用层                          │
 │  xTaskCreate()  xQueueCreate()  xTimerCreate()  │
 │  xEventGroupCreate()  xSemaphoreCreateMutex()   │
 └────────┬────────────────────────────────────────┘
          │ 所有 Create API 最终调用
          ▼
 ┌─────────────────────────────────────────────────┐
 │              pvPortMalloc()                      │
 │         (heap_1 / heap_2 / heap_3 /              │
 │          heap_4 / heap_5 之一)                   │
 └────────┬────────────────────────────────────────┘
          │ 分配
          ▼
 ┌─────────────────────────────────────────────────┐
 │   TCB  │  任务栈  │  Queue结构  │  队列数据区   │
 │        │          │  Timer结构  │  EventGroup   │
 └─────────────────────────────────────────────────┘
```

**一次 `xTaskCreate()` 需要两次 `pvPortMalloc()`**：

```c
// tasks.c 内部逻辑（简化）:
pxNewTCB  = (TCB_t *)      pvPortMalloc( sizeof(TCB_t) );         // ① TCB
pxStack   = (StackType_t *) pvPortMalloc( stackDepthInBytes );     // ② 任务栈
```

**一次 `xQueueCreate()` 需要一到两次**：

```c
pxNewQueue = (Queue_t *) pvPortMalloc( sizeof(Queue_t) );                     // ① 队列结构
if( uxItemSize != 0 )
    pxNewQueue->pcHead = (uint8_t *) pvPortMalloc( length * itemSize );       // ② 数据存储区
```

**信号量**（二进制/互斥量/计数）的 `uxItemSize = 0`，所以只需一次分配（队列结构），没有数据存储区。

**软件定时器**（`xTimerCreate()`）和**事件组**（`xEventGroupCreate()`）也各自需要一次 `pvPortMalloc()`。

#### 3.7.2 五种 Heap 实现全景

| 实现 | 文件 | 核心策略 | 可释放 | 碎片合并 | 元数据/块 | 分配速度 | 适用场景 |
|------|------|---------|:---:|:---:|:---:|:---:|---------|
| **heap_1** | heap_1.c | 单调递增指针 | ❌ | N/A | **0** | **O(1)** | 启动时全部创建、永不删除 |
| **heap_2** | heap_2.c | 链表 + 释放 | ✅ | ❌ | 块头 | O(n) | 固定大小对象池（已被 heap_4 取代） |
| **heap_3** | heap_3.c | 包装标准 malloc/free | ✅ | 取决于 libc | malloc 头 | 取决于 libc | 需对接第三方 malloc（tlsf 等） |
| **heap_4** | heap_4.c | First-fit + 合并 | ✅ | ✅ 双向 | 8字节 | O(n) | **通用首选**，适合动态创建/删除 |
| **heap_5** | heap_5.c | 同 heap_4 + 多段 | ✅ | ✅ 双向 | 8字节 | O(n) | 含外部 SRAM/SDRAM 的系统 |

**V9.0 新增**：静态内存分配（`configSUPPORT_STATIC_ALLOCATION = 1`），可完全绕过 `pvPortMalloc()`。

#### 3.7.3 heap_1 深度解析 — 仅分配不释放

**数据结构 — 极简到只有两个变量**：

```c
static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];   // 堆数组
static size_t xNextFreeByte = 0;                   // 空闲区起始偏移（单调递增）
```

**内存模型**：

```
ucHeap[]
 ┌────┬──────────┬──────────┬────────────────────┐
 │对齐│  Task1   │  Task2   │      空闲          │
 │损耗│ (已分配)  │ (已分配)  │                    │
 └────┴──────────┴──────────┴────────────────────┘
  ↑                                   ↑
  pucAlignedHeap                   xNextFreeByte (单调递增，永不后退)
```

**`pvPortMalloc()` 核心 — 仅 3 行代码**：

```c
pvReturn = pucAlignedHeap + xNextFreeByte;   // 返回当前空闲位置
xNextFreeByte += xWantedSize;                // 推进指针（永不后退）
```

**`vPortFree()`** — 空操作 + 防御断言：

```c
void vPortFree( void *pv ) {
    configASSERT( pv == NULL );   // 任何非 NULL 的 free 都会触发断言
}
```

**线程安全**：使用 `vTaskSuspendAll()` / `xTaskResumeAll()` 而非关中断 — 因为 ISR 不应该调 `malloc`，只需防止任务间竞争。

**对齐技巧**（首次调用延迟初始化）：

```c
// 找出堆中第一个对齐地址（跳过前端不对齐字节）
pucAlignedHeap = (uint8_t *)(
    ((portPOINTER_SIZE_TYPE)&ucHeap[ portBYTE_ALIGNMENT ])
    & (~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK))
);
```

#### 3.7.4 heap_4 深度解析 — First-fit + 碎片合并

**数据结构**：

```c
typedef struct A_BLOCK_LINK {
    struct A_BLOCK_LINK *pxNextFreeBlock;   // 下一空闲块（4字节）
    size_t xBlockSize;                       // 块大小，最高位 = 分配标志（4字节）
} BlockLink_t;                               // 共 8 字节（32位平台）
```

**关键技巧 — 最高位复用**：

```
xBlockSize 字段（32位）:
 bit31                          bit0
  ┌───┬────────────────────────────┐
  │ A │        实际块大小           │
  └───┴────────────────────────────┘
    ↑
  xBlockAllocatedBit = 0x80000000
  A=1 → 已分配    A=0 → 空闲
```

无额外字段，0 内存浪费。

**初始状态**（`prvHeapInit`，仅首次 malloc 触发）：

```
 xStart(静态)       pxFirstFreeBlock(堆内)      pxEnd(堆尾)
 ┌──────────┐    ┌───────────────────────┐    ┌──────────┐
 │pxNext ───┼───►│ pxNext ───────────────┼───►│ pxNext=N │
 │xBlock=0  │    │ xBlock = 整个可用空间  │    │ xBlock=0 │
 └──────────┘    └───────────────────────┘    └──────────┘
  .bss 段             堆内                        堆末尾（占用 8 字节）

xStart   — 哨兵节点，在静态数据区，不占堆空间
pxEnd    — 指针在静态数据区，指向的 BlockLink_t 嵌入堆末尾
空闲块    — 各结点嵌在堆内，每个空闲块前 8 字节即是 BlockLink_t
```

**分配流程**：

```
① 首次调用 → prvHeapInit()
② xWantedSize + 块头(8字节) + 对齐 padding → 实际需求
③ 快速失败: xWantedSize > xFreeBytesRemaining? → NULL
④ First-fit 遍历: xStart → 块A → 块B → 块C → pxEnd
                   找到第一个 xBlockSize ≥ 需求量 的块
                   走到 pxEnd 还没找到? → NULL
⑤ 从链表摘除: pxPrev->pxNextFreeBlock = pxBlock->pxNextFreeBlock
⑥ 切分: 剩余 > heapMINIMUM_BLOCK_SIZE(16字节)?
     ├─ 是 → 前半段分配，后半段插回空闲链表
     └─ 否 → 整块分配（防止"幽灵碎片"）
⑦ pxBlock->xBlockSize |= 0x80000000  (标记已分配)
⑧ pvReturn = 块起始 + 8  (跳过块头，返回用户数据区指针)
```

**用户拿到的指针跳过了块头**：

```
 ┌──────────┬──────────────────────────────┐
 │BlockLink_t│        用户数据区             │
 │  (8字节)  │                              │
 └──────────┴──────────────────────────────┘
            ↑
        pvReturn (返回给用户)
```

**释放 + 合并**（`vPortFree` → `prvInsertBlockIntoFreeList`）：

释放时按地址升序插入空闲链表，并**检查前后两个方向**是否物理相邻：

```
① 与前块相邻?
   pxIterator + xBlockSize == pxBlockToInsert ?
   是 → 向前合并（前块大小 += 释放块大小）

② 与后块相邻?
   pxBlockToInsert + xBlockSize == pxIterator->pxNextFreeBlock ?
   是 → 向后合并（释放块吞并后块）

③ 与前后都相邻? → 三块合一

④ 前后都不相邻? → 作为独立空闲块插入链表
```

```
释放场景示意:

【场景A】前后都不相邻 → 纯插入
   [已分配] [  空闲  ] [已分配]
            ↑ 插入，不合并

【场景B】与前块相邻 → 向前合并
   [  空闲  ] [  释放  ] [已分配]
        └──合并──┘
   结果: [    大空闲    ] [已分配]

【场景C】与前后都相邻 → 三块合一
   [  空闲  ] [  释放  ] [  空闲  ]
    └────────合并──────────┘
   结果: [        超大空闲块        ]
```

#### 3.7.5 Heap 线程安全机制

| heap | 保护方式 | 为什么这样选 |
|------|---------|------------|
| heap_1 | `vTaskSuspendAll()` / `xTaskResumeAll()` | 操作极短，ISR 不应调 malloc |
| heap_2 | `vTaskSuspendAll()` / `xTaskResumeAll()` | 同上 |
| heap_3 | 包装标准库的锁机制 | 取决于底层 libc |
| heap_4 | `vTaskSuspendAll()` / `xTaskResumeAll()` | 释放时需合并，但操作仍可控 |
| heap_5 | `vTaskSuspendAll()` / `xTaskResumeAll()` | 同 heap_4 |

**为什么不用 `taskENTER_CRITICAL()`（关中断）？**

- `malloc/free` 不是和 ISR 竞争数据，而是和其他任务竞争
- 关中断会影响所有 ISR（包括 Tick），调度器锁只阻止任务切换
- 但要注意：heap_4 的释放需要遍历合并，如果碎片严重可能耗时较长，可以用 `configMAX_SLEEP_TIME` 或 `heap_5` 多段策略缓解

#### 3.7.6 Heap 与 RTOS 对象生命周期

```
vTaskStartScheduler()
      │
      ├── 创建 Idle Task
      │       └── pvPortMalloc(TCB) + pvPortMalloc(栈)
      │
      ├── 创建 Timer Daemon Task (如果 configUSE_TIMERS=1)
      │       └── pvPortMalloc(TCB) + pvPortMalloc(栈)
      │
      └── 调度器启动，tick 中断开始

运行时:
  xTaskCreate()    → 2x pvPortMalloc (TCB + 栈)
  vTaskDelete()    → TCB 和栈加入"待删除链表"，Idle Task 调用 vPortFree()
  xQueueCreate()   → 1~2x pvPortMalloc (结构 + 数据区)
  vQueueDelete()   → 1~2x vPortFree
  xTimerCreate()   → 1x pvPortMalloc
  xTimerDelete()   → 1x vPortFree
  xEventGroupCreate() → 1x pvPortMalloc
  vEventGroupDelete() → 1x vPortFree
```

**TCB 和栈的延迟释放**：`vTaskDelete()` 不立即释放 TCB/栈内存，而是把 TCB 放入 `xTasksWaitingTermination` 链表，由 Idle Task 在空闲时调用 `vPortFree()`。这也意味着**如果 Idle Task 被饿死，内存泄漏会不断累积**。

#### 3.7.7 配置选型指南

**通用配置**：

```c
// FreeRTOSConfig.h — 使用 heap_4 的标准配置
#define configSUPPORT_DYNAMIC_ALLOCATION   1
#define configSUPPORT_STATIC_ALLOCATION    0    // 允许静态分配作为备选
#define configTOTAL_HEAP_SIZE             ((size_t) 20480)  // 20KB
#define configAPPLICATION_ALLOCATED_HEAP   0    // 让 FreeRTOS 定义 ucHeap
#define configUSE_MALLOC_FAILED_HOOK       1    // 分配失败时回调
```

**安全关键配置**：

```c
#define configSUPPORT_DYNAMIC_ALLOCATION   0    // 禁用动态分配
#define configSUPPORT_STATIC_ALLOCATION    1    // 仅使用静态分配
// 所有 Create API 使用 Static 版本:
// xTaskCreateStatic(), xQueueCreateStatic(), xTimerCreateStatic(), ...
```

**选型速查**：

| 需求 | 推荐 |
|------|------|
| 启动后对象数量不变，安全关键 | heap_1 或全静态分配 |
| 通用嵌入式项目 | **heap_4** |
| 含外部 RAM（SRAM/SDRAM） | heap_5 |
| 使用自定义分配器（tlsf 等） | heap_3 |
| 仅创建删除固定大小对象 | heap_2（不推荐，已被 heap_4 取代）

---

## 第四部分：移植层 (Portable Layer)

移植层实现了 RTOS 与硬件/编译器之间的抽象，是 FreeRTOS 跨平台能力的核心。

### 4.1 移植层接口

每个移植必须提供：

| 文件 | 内容 |
|------|------|
| `portmacro.h` | 数据类型定义（TickType_t, BaseType_t 等），临界区宏，上下文切换宏 |
| `port.c` | 栈初始化，启动第一个任务，Tick 中断 ISR，空闲任务钩子 |
| `portasm.s/asm` | 汇编实现的上下文保存/恢复（取决于架构） |

### 4.2 关键移植宏

| 宏 | 功能 |
|----|------|
| `portENTER_CRITICAL()` | 进入临界区（关中断） |
| `portEXIT_CRITICAL()` | 退出临界区（恢复中断） |
| `portYIELD()` | 请求上下文切换 |
| `portYIELD_FROM_ISR()` | 在 ISR 中请求上下文切换 |
| `portEND_SWITCHING_ISR()` | ISR 结束时执行上下文切换 |
| `portDISABLE_INTERRUPTS()` | 禁用所有可屏蔽中断 |
| `portENABLE_INTERRUPTS()` | 启用所有可屏蔽中断 |
| `portTICK_PERIOD_MS` | Tick 周期（毫秒） |
| `portMAX_DELAY` | 最大阻塞时间（无限等待） |

### 4.3 支持的编译器/平台（20种）

| 编译器 | 主要目标平台 |
|--------|------------|
| **GCC** | ARM Cortex-M0/M3/M4/M7/R4/R5, ARM7/9, ARM CA9/CA53(64位), AVR32, ColdFire, HCS12, H8S, IA32, MSP430, RISC-V, TriCore, etc. |
| **IAR** | ARM Cortex-M3/M4/M7, ARM7/9, AVR32, MSP430, RX, STM8, etc. |
| **Keil** | ARM7/9, ARM Cortex-M3/M4, etc. |
| **RVDS** | ARM Cortex-M3/M4, ARM7/9, etc. |
| **MSVC-MingW** | Windows (模拟器/调试) |
| **CCS** | TI MSP430, ARM Cortex-M4, ARM Cortex-R4 |
| **MPLAB** | PIC24/dsPIC, PIC32 |
| **Renesas** | RX, SH2A, R8C, RL78, H8S |
| **Others** | BCC(16位DOS), CodeWarrior(ColdFire/HCS12), Rowley, SDCC, Tasking, etc. |

---

## 第五部分：配置系统 (FreeRTOSConfig.h)

每个应用程序都必须提供 `FreeRTOSConfig.h`，以下是关键配置项的分类：

### 5.1 必须定义的配置

| 配置项 | 说明 |
|--------|------|
| `configMINIMAL_STACK_SIZE` | 空闲任务栈大小（单位：字） |
| `configMAX_PRIORITIES` | 最大优先级数 |
| `configUSE_PREEMPTION` | 1=抢占式, 0=合作式 |
| `configUSE_IDLE_HOOK` | 1=启用空闲任务钩子 |
| `configUSE_TICK_HOOK` | 1=启用 Tick 钩子 |
| `configUSE_16_BIT_TICKS` | 1=16位 Tick, 0=32位 Tick |

### 5.2 可选功能配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `configUSE_MUTEXES` | 0 | 启用互斥量 |
| `configUSE_RECURSIVE_MUTEXES` | 0 | 启用递归互斥量 |
| `configUSE_COUNTING_SEMAPHORES` | 0 | 启用计数信号量 |
| `configUSE_TIMERS` | 0 | 启用软件定时器 |
| `configUSE_QUEUE_SETS` | 0 | 启用队列集 |
| `configUSE_TASK_NOTIFICATIONS` | 1 | 启用任务通知（V9.0 新） |
| `configUSE_TRACE_FACILITY` | 0 | 启用跟踪/调试工具 |
| `configUSE_TICKLESS_IDLE` | 0 | 启用无 Tick 空闲模式 |
| `configUSE_TIME_SLICING` | 1 | 启用同优先级时间片轮转 |
| `configUSE_NEWLIB_REENTRANT` | 0 | newlib 重入支持 |
| `configUSE_PORT_OPTIMISED_TASK_SELECTION` | 0 | 硬件加速最高优先级查找 |
| `configSUPPORT_STATIC_ALLOCATION` | 0 | 启用静态内存分配 |
| `configSUPPORT_DYNAMIC_ALLOCATION` | 1 | 启用动态内存分配 |
| `configGENERATE_RUN_TIME_STATS` | 0 | 启用运行时统计 |
| `configCHECK_FOR_STACK_OVERFLOW` | 0 | 栈溢出检查（0/1/2） |
| `configMAX_TASK_NAME_LEN` | 16 | 任务名最大长度 |
| `configASSERT` | 空 | 断言宏定义 |

### 5.3 API 裁剪配置

以 `INCLUDE_` 前缀的宏用于条件编译裁剪 API 函数（节省代码空间）：

| 配置项 | 默认值 | 控制的 API |
|--------|--------|-----------|
| `INCLUDE_vTaskPrioritySet` | 0 | vTaskPrioritySet() |
| `INCLUDE_uxTaskPriorityGet` | 0 | uxTaskPriorityGet() |
| `INCLUDE_vTaskDelete` | 0 | vTaskDelete() |
| `INCLUDE_vTaskSuspend` | 0 | vTaskSuspend(), vTaskResume() |
| `INCLUDE_vTaskDelayUntil` | 0 | vTaskDelayUntil() |
| `INCLUDE_vTaskDelay` | 0 | vTaskDelay() |
| `INCLUDE_xTaskGetIdleTaskHandle` | 0 | xTaskGetIdleTaskHandle() |
| `INCLUDE_xTaskAbortDelay` | 0 | xTaskAbortDelay() |
| `INCLUDE_xTaskGetSchedulerState` | 0 | xTaskGetSchedulerState() |
| `INCLUDE_uxTaskGetStackHighWaterMark` | 0 | uxTaskGetStackHighWaterMark() |
| `INCLUDE_eTaskGetState` | 0 | eTaskGetState() |

---

## 第六部分：开发手册

### 6.1 快速开始

#### 6.1.1 从 Demo 开始

```
推荐步骤：
1. 在 Demo/ 目录中找到与你硬件平台最接近的演示项目
2. 编译并运行演示项目，确保工具链和硬件工作正常
3. 删除演示任务代码，替换为你的应用代码
4. 根据需求调整 FreeRTOSConfig.h
```

#### 6.1.2 最小化项目配置

一个最小的 FreeRTOS 项目需要：

**必需的内核源文件**：
- `Source/tasks.c` — 任务管理
- `Source/queue.c` — 队列和信号量
- `Source/list.c` — 链表数据结构
- `Source/portable/MemMang/heap_X.c` — 选择一个堆实现
- `Source/portable/[编译器]/[平台]/port.c` — 移植层
- 如果使用硬件浮点或需要汇编上下文切换，还需要 `portasm.s`

**可选的源文件**：
- `Source/timers.c` — 软件定时器（`configUSE_TIMERS=1`）
- `Source/event_groups.c` — 事件组
- `Source/croutine.c` — 协程（极少使用）

**必需的头文件路径**：
- `Source/include` — 内核公共头文件
- `Source/portable/[编译器]/[平台]` — 移植层头文件
- 项目目录 — FreeRTOSConfig.h

#### 6.1.3 Hello World 示例

```c
#include "FreeRTOS.h"
#include "task.h"

// LED 闪烁任务
void vLEDTask(void *pvParameters) {
    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;  // 500ms

    for (;;) {
        // 切换 LED
        GPIO_Toggle(LED_PIN);

        // 阻塞 500ms
        vTaskDelay(xDelay);
    }
}

int main(void) {
    // 硬件初始化
    Hardware_Init();

    // 创建 LED 闪烁任务
    xTaskCreate(
        vLEDTask,           // 任务函数
        "LED",              // 任务名
        configMINIMAL_STACK_SIZE,  // 栈大小
        NULL,               // 参数
        tskIDLE_PRIORITY + 1,     // 优先级
        NULL                // 不需要句柄
    );

    // 启动调度器 (永不返回)
    vTaskStartScheduler();

    // 不应到达此处
    for (;;);
    return 0;
}
```

### 6.2 核心 API 使用指南

#### 6.2.1 任务管理

**创建任务**:
```c
TaskHandle_t xHandle;

// 动态分配
xTaskCreate(vTaskFunction, "TaskName", STACK_SIZE, NULL, 1, &xHandle);

// 静态分配
StaticTask_t xTaskBuffer;
StackType_t xStack[STACK_SIZE];
xHandle = xTaskCreateStatic(vTaskFunction, "TaskName", STACK_SIZE,
                             NULL, 1, xStack, &xTaskBuffer);
```

**任务延迟**:
```c
// 相对延迟（从调用时开始计时）
vTaskDelay(pdMS_TO_TICKS(100));  // 延迟 100ms

// 绝对延迟（精确周期执行）
TickType_t xLastWakeTime = xTaskGetTickCount();
for (;;) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));  // 每 10ms 执行一次
    // 执行周期性任务...
}
```

**任务控制**:
```c
vTaskSuspend(xHandle);                     // 挂起
vTaskResume(xHandle);                      // 恢复
vTaskPrioritySet(xHandle, 3);              // 修改优先级
eTaskState state = eTaskGetState(xHandle); // 查询状态
vTaskDelete(xHandle);                      // 删除
```

**获取信息**:
```c
UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);  // 栈高水位（调用者自身）
char *pcName = pcTaskGetName(NULL);                                // 任务名
TickType_t xTicks = xTaskGetTickCount();                           // 系统 Tick 计数
```

#### 6.2.2 队列使用

```c
// 创建队列（可容纳10个 uint32_t 值）
QueueHandle_t xQueue = xQueueCreate(10, sizeof(uint32_t));

// 发送（任务级）
uint32_t ulValue = 42;
xQueueSend(xQueue, &ulValue, pdMS_TO_TICKS(100));  // 阻塞最多100ms

// 接收（任务级）
uint32_t ulReceived;
if (xQueueReceive(xQueue, &ulReceived, portMAX_DELAY) == pdTRUE) {
    // 成功接收到数据
}

// ISR 中发送
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xQueueSendFromISR(xQueue, &ulValue, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);  // 如有必要，切换上下文
```

#### 6.2.3 信号量与互斥量

**二进制信号量**（任务同步）:
```c
SemaphoreHandle_t xSemaphore = xSemaphoreCreateBinary();

// 中断中释放（表示事件发生）
xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);

// 任务中获取（等待事件）
xSemaphoreTake(xSemaphore, portMAX_DELAY);
```

**互斥量**（资源保护 — 带优先级继承）:
```c
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();

// 获取互斥量
if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    // 访问共享资源...
    // ...

    // 释放互斥量
    xSemaphoreGive(xMutex);
}
```

**计数信号量**（资源池管理）:
```c
SemaphoreHandle_t xCountingSem = xSemaphoreCreateCounting(5, 5);  // 最多5个资源

xSemaphoreTake(xCountingSem, portMAX_DELAY);  // 获取一个资源
// 使用资源...
xSemaphoreGive(xCountingSem);                  // 归还资源
```

**√ 轻量替代方案 — 任务通知**（V9.0 推荐）:
```c
// 发送通知（替代 Give）
xTaskNotifyGive(xTaskHandle);

// 等待通知（替代 Take）
uint32_t ulNotifiedValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
```

#### 6.2.4 软件定时器

```c
// 定时器回调函数
void vTimerCallback(TimerHandle_t xTimer) {
    // 定时器到期处理...
}

// 创建定时器（100ms 周期，自动重载）
TimerHandle_t xTimer = xTimerCreate(
    "MyTimer",
    pdMS_TO_TICKS(100),  // 周期
    pdTRUE,              // 自动重载
    (void *)0,           // 定时器 ID
    vTimerCallback       // 回调函数
);

// 启动定时器
xTimerStart(xTimer, 0);

// 在 ISR 中启动/重置定时器
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xTimerStartFromISR(xTimer, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

#### 6.2.5 事件组

```c
#define BIT_TEMP_READY   (1 << 0)
#define BIT_PRESS_READY  (1 << 1)

EventGroupHandle_t xEventGroup = xEventGroupCreate();

// 任务1：设置温度数据就绪事件
xEventGroupSetBits(xEventGroup, BIT_TEMP_READY);

// 任务2：等待任意一个事件就绪
EventBits_t uxBits = xEventGroupWaitBits(
    xEventGroup,
    BIT_TEMP_READY | BIT_PRESS_READY,  // 等待的位
    pdTRUE,   // 等待后清除
    pdFALSE,  // 等待任意一位（OR）
    portMAX_DELAY
);

// 多任务同步（Rendezvous）
#define ALL_SYNC_BITS (TASK0_BIT | TASK1_BIT | TASK2_BIT)
xEventGroupSync(xEventGroup, TASK0_BIT, ALL_SYNC_BITS, portMAX_DELAY);
```

### 6.3 中断服务程序 (ISR) 最佳实践

1. **使用 FromISR 版本的 API**：`xQueueSendFromISR()`, `xSemaphoreGiveFromISR()`, `xTaskNotifyFromISR()` 等
2. **正确处理上下文切换请求**：
```c
void vISR_Handler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // ISR 处理...
    xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);

    // ISR 退出前检查是否需要上下文切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```
3. **中断优先级**：确保 Tick 中断和调用 FreeRTOS API 的中断的优先级配置正确（受 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 限制）
4. **ISR 中使用任务通知的轻量同步**：比信号量更快的替代方案

### 6.4 调试与故障排查

#### 6.4.1 栈溢出检测

```c
// FreeRTOSConfig.h
#define configCHECK_FOR_STACK_OVERFLOW  2  // 0=关闭, 1=基本检测, 2=增强检测
```

在 `vApplicationStackOverflowHook()` 中处理溢出事件。

#### 6.4.2 运行时统计

```c
// FreeRTOSConfig.h
#define configGENERATE_RUN_TIME_STATS  1
#define configUSE_TRACE_FACILITY       1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1

// 使用
char cBuffer[512];
vTaskList(cBuffer);           // 打印任务状态列表
vTaskGetRunTimeStats(cBuffer); // 打印运行时统计
```

#### 6.4.3 内存分配失败钩子

```c
#define configUSE_MALLOC_FAILED_HOOK  1

void vApplicationMallocFailedHook(void) {
    // 内存分配失败处理（如：进入安全模式）
    for (;;);
}
```

#### 6.4.4 断言

```c
#define configASSERT(x)  if((x)==0) { taskDISABLE_INTERRUPTS(); for(;;); }
```

#### 6.4.5 常见问题排查

| 问题 | 可能原因 | 解决方向 |
|------|---------|---------|
| 调度器不启动 | 未创建任务 | 在 vTaskStartScheduler() 前创建至少1个任务 |
| 任务不运行 | 优先级过低 | 检查任务优先级是否高于空闲任务 |
| Hard Fault | 栈溢出 | 增大栈大小，启用栈溢出检测 |
| 内存分配失败 | 堆太小 | 增大 configTOTAL_HEAP_SIZE，或使用静态分配 |
| 中断响应延迟 | 临界区过长 | 缩小临界区范围 |

### 6.5 内存管理策略选择

| 场景 | 推荐方案 |
|------|---------|
| 任务/对象数量固定 | heap_1 + 静态分配 (configSUPPORT_STATIC_ALLOCATION=1) |
| 动态创建和删除 | heap_4（最常用） |
| 多段 RAM（内部+外部） | heap_5 |
| 使用第三方库需要 malloc | heap_3 |
| 固定大小的对象池 | heap_2 |

### 6.6 性能优化建议

1. **使用任务通知替代信号量**：V9.0 的 Task Notifications 比信号量快 ~45%、节省 RAM
2. **启用硬件加速任务选择**：`configUSE_PORT_OPTIMISED_TASK_SELECTION = 1`（ARM Cortex-M 上利用 CLZ 指令）
3. **合理设置 Tick 频率**：默认 1000Hz（1ms），过低导致延迟大，过高导致中断开销大
4. **使用直接任务通知替代事件组**：对于简单的单事件同步
5. **使用 FromISR 延迟处理**：`xTimerPendFunctionCallFromISR()` 将重处理从 ISR 移到任务
6. **无 Tick 空闲模式**：`configUSE_TICKLESS_IDLE = 1` 显著降低功耗

---

## 第七部分：头文件依赖关系

```
FreeRTOS.h ◄── FreeRTOSConfig.h (应用程序提供)
    ├── stddef.h, stdint.h
    ├── projdefs.h (pdPASS/pdFAIL/pdTRUE/pdFALSE 等基础定义)
    ├── portable.h
    │       └── portmacro.h (移植层数据类型和宏定义)
    │
    ├── list.h ◄── FreeRTOS.h (链表数据结构)
    │
    ├── task.h ◄── FreeRTOS.h + list.h (任务管理 API)
    │
    ├── queue.h ◄── FreeRTOS.h (队列 API)
    │
    ├── semphr.h ◄── FreeRTOS.h + queue.h (信号量/互斥量 API)
    │
    ├── timers.h ◄── FreeRTOS.h + task.h (软件定时器 API)
    │
    ├── event_groups.h ◄── FreeRTOS.h + timers.h (事件组 API)
    │
    ├── croutine.h ◄── FreeRTOS.h + queue.h (协程 API)
    │
    ├── mpu_wrappers.h / mpu_prototypes.h (MPU 保护相关)
    └── deprecated_definitions.h (废弃 API 定义)
```

**应用程序包含头文件的正确顺序**：
```c
#include "FreeRTOS.h"    // 必须第一个包含
#include "task.h"        // 任务管理
#include "queue.h"       // 队列
#include "semphr.h"      // 信号量/互斥量
#include "timers.h"      // 软件定时器
#include "event_groups.h"// 事件组
```

---

## 第八部分：版本特性 (V9.0.0)

V9.0.0 相比 V8.x 的主要新特性：

| 特性 | 说明 |
|------|------|
| **任务通知** (Task Notifications) | 无需中间对象的直接任务间通信，更快更省 RAM |
| **静态内存分配** | `xTaskCreateStatic()`, `xQueueCreateStatic()` 等，完全避免动态内存 |
| **删除 xTaskCheckForTimeOut** | 内部重构 |
| **xTaskAbortDelay()** | 可中止其他任务的阻塞状态 |
| **任务通知替代方案** | 可用于替代二进制/计数信号量、事件组 |
| **移除 Alternative API** | `configUSE_ALTERNATIVE_API` 已废弃 |

---

## 第九部分：文件清单速查

### 内核核心文件
| 文件 | 行数估计 | 说明 |
|------|---------|------|
| `Source/tasks.c` | ~3500+ | 任务管理与调度器 |
| `Source/queue.c` | ~2800+ | 队列、信号量、互斥量 |
| `Source/list.c` | ~200 | 链表实现 |
| `Source/timers.c` | ~1200 | 软件定时器 |
| `Source/event_groups.c` | ~800 | 事件组 |
| `Source/croutine.c` | ~500 | 协程 |

### 头文件
| 文件 | 说明 |
|------|------|
| `Source/include/FreeRTOS.h` | 主头文件，配置检查，数据类型 |
| `Source/include/task.h` | 任务管理 API 声明 |
| `Source/include/queue.h` | 队列 API 声明 |
| `Source/include/semphr.h` | 信号量/互斥量 API（宏定义） |
| `Source/include/timers.h` | 软件定时器 API 声明 |
| `Source/include/event_groups.h` | 事件组 API 声明 |
| `Source/include/list.h` | 链表数据结构和 API |
| `Source/include/croutine.h` | 协程 API 声明 |
| `Source/include/projdefs.h` | 项目定义（pdPASS/pdFAIL 等） |
| `Source/include/portable.h` | 移植层抽象 |
| `Source/include/StackMacros.h` | 栈操作宏 |
| `Source/include/MPU_wrappers.h` | MPU 包装器 |

---

*文档生成日期: 2026-05-31*
*基于: FreeRTOS V9.0.0 源码分析*
