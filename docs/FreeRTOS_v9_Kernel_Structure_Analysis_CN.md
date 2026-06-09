# FreeRTOS v9.0.0 内核结构与调度、阻塞机制分析

*从 List、Task、Queue、Heap 到临界操作、任务调度与信号量事件阻塞*

> **分析范围**
>
> 以当前仓库 FreeRTOS v9.0.0 的 Source/list.c、tasks.c、queue.c、portable/MemMang/heap_*.c 为主，端口相关行为以 GCC/ARM_CM4F 为实例。其他 CPU 端口的中断屏蔽指令和上下文切换汇编可能不同。

- 版本：1.0
- 日期：2026-06-10
- 工作区：FreeRTOSv9.0.0/FreeRTOS

# 阅读导引

本文不按 API 手册逐个罗列函数，而是围绕三个核心问题展开：内核对象如何挂入链表、任务为什么从运行态进入阻塞态、以及中断和调度器并发时如何保持链表一致性。

| 章节 | 回答的问题 |
| --- | --- |
| 1. 总体结构 | FreeRTOS 内核文件怎样分层，核心对象之间如何关联 |
| 2. List | 内核链表的哨兵、索引、排序和 O(1) 删除如何实现 |
| 3. Task 与调度 | TCB、就绪表、延时表、PendSV 和时间片怎样协作 |
| 4. Queue 与同步对象 | 队列、信号量、互斥量为何共享同一个 Queue_t |
| 5. 临界操作 | 硬件中断屏蔽、调度器挂起、队列锁分别保护什么 |
| 6. 阻塞与唤醒 | 信号量、队列事件的完整阻塞链和 ISR 唤醒链 |
| 7. Heap | 五种 heap 实现的分配、释放、碎片与适用场景 |
| 8. 调试检查表 | 如何从链表和状态变量定位调度、阻塞和内存问题 |

> **先给结论**
>
> FreeRTOS 的核心不是“很多独立对象”，而是“TCB + 多组 List + 一个统一 Queue_t”。任务状态变化，本质上是 TCB 内两个 ListItem_t 在不同链表之间迁移；阻塞等待，本质上是同时登记到状态链表和事件链表；唤醒则是反向移除并加入对应优先级的就绪链表。

# 1. 内核总体结构

## 1.1 文件职责

| 文件 | 核心职责 | 关键数据结构/函数 |
| --- | --- | --- |
| Source/list.c | 通用双向链表容器 | List_t、ListItem_t、vListInsert、uxListRemove |
| Source/tasks.c | 任务生命周期、状态链表、调度与 tick | TCB_t、pxReadyTasksLists、xTaskIncrementTick |
| Source/queue.c | 队列、信号量、互斥量、队列集 | Queue_t、xQueueGenericSend/Receive |
| Source/timers.c | 软件定时器服务任务与命令队列 | Timer_t、timer command queue |
| Source/event_groups.c | 多位事件条件等待 | EventGroup_t、无序事件列表接口 |
| portable/* | 上下文切换、中断屏蔽、tick 和栈初始化 | PendSV、SysTick、portENTER_CRITICAL |
| portable/MemMang | 动态内存策略 | heap_1 至 heap_5、pvPortMalloc/vPortFree |

## 1.2 运行时对象关系

```text
应用 API
  |
  +-- Task API --------> TCB_t
  |                       +-- xStateListItem ----> 就绪/延时/挂起链表
  |                       +-- xEventListItem ----> 队列/信号量事件等待链表
  |
  +-- Queue/Semaphore --> Queue_t
  |                       +-- xTasksWaitingToSend
  |                       +-- xTasksWaitingToReceive
  |
  +-- pvPortMalloc -----> heap_1..heap_5（编译时只选一种）
  |
  +-- Port layer -------> 中断屏蔽、PendSV、SysTick、上下文保存/恢复
```

FreeRTOS 的平台无关层决定“何时需要切换任务”，移植层决定“怎样保存现场并切换栈”。因此阅读调度器时，要把 tasks.c 中的决策和 portable 目录中的机制分开。

# 2. List：内核状态机的基础容器

## 2.1 三个结构体

| 结构 | 关键成员 | 作用 |
| --- | --- | --- |
| ListItem_t | xItemValue | 排序键：延时链表中是唤醒 tick；事件链表中是反向编码的任务优先级 |
| ListItem_t | pxNext / pxPrevious | 双向链接，支持从任意节点 O(1) 删除 |
| ListItem_t | pvOwner | 通常指向拥有该节点的 TCB |
| ListItem_t | pvContainer | 反向指向当前所属 List_t；未入链时为 NULL |
| MiniListItem_t | xItemValue / next / previous | 只作为尾部哨兵，不需要 owner/container |
| List_t | uxNumberOfItems | 有效节点计数 |
| List_t | pxIndex | 轮转游标，支撑同优先级 round-robin |
| List_t | xListEnd | 值为 portMAX_DELAY 的哨兵节点 |

*源码定位：Source/include/list.h: ListItem_t、MiniListItem_t、List_t；Source/list.c: vListInitialise()*

## 2.2 初始化与哨兵

vListInitialise() 将 pxIndex 指向 xListEnd，并让 xListEnd 的前后指针都指向自身。空链表因此仍是一个闭合环，插入和删除无需为头节点、尾节点分别写特殊分支。xListEnd.xItemValue 被设置为 portMAX_DELAY，使普通排序节点自然插到它之前。

```text
空链表：
        +----------------------+
        |                      v
  pxIndex --> xListEnd <-------+
               ^     |
               +-----+

加入 A、B 后：
  xListEnd <-> A <-> B <-> xListEnd
```

## 2.3 三种核心操作

| 操作 | 复杂度 | 语义 |
| --- | --- | --- |
| vListInsertEnd | O(1) | 插到 pxIndex 前；同优先级就绪任务以此形成轮转序列 |
| vListInsert | O(n) | 按 xItemValue 数值升序查找插入位置；相同值的新节点放在已有相同值节点之后 |
| uxListRemove | O(1) | 利用 pvContainer 找到容器并直接摘链；若删到 pxIndex，则把索引回退 |

> **容易误读的排序方向**
>
> vListInsert() 的实际链接顺序是 xItemValue 数值升序。事件等待链表使用 configMAX_PRIORITIES - uxPriority 作为排序值，所以任务优先级越高，数值越小，越靠近链表头。因此“事件语义上按优先级从高到低”与“底层数值从小到大”并不矛盾。

*源码定位：Source/list.c: vListInsertEnd()、vListInsert()、uxListRemove()*

## 2.4 List 在内核中的四类用法

| 链表类型 | 节点 | 排序/插入方式 | 含义 |
| --- | --- | --- | --- |
| 就绪链表 | TCB.xStateListItem | vListInsertEnd，按优先级分多个 List_t | 可运行任务；pxIndex 实现时间片轮转 |
| 延时链表 | TCB.xStateListItem | vListInsert，键为绝对唤醒 tick | 尚未到期的阻塞任务 |
| 挂起链表 | TCB.xStateListItem | 通常插尾 | 无限期等待或显式挂起 |
| 事件等待链表 | TCB.xEventListItem | vListInsert，键为反向优先级 | 等待队列、信号量、互斥量等事件 |
| Pending Ready | TCB.xEventListItem | 插尾 | 调度器挂起期间被 ISR/事件唤醒的任务 |

同一个任务通常同时拥有两个链表身份：xStateListItem 表示“现在处于什么调度状态”，xEventListItem 表示“正在等待哪个事件”。这正是理解阻塞与唤醒的关键。

### 2.4.1 对应到源码中的具体链表

| 逻辑名称 | 源码中的实际对象 | 对象数量与归属 |
| --- | --- | --- |
| 就绪链表 | `pxReadyTasksLists[configMAX_PRIORITIES]` | 调度器全局数组；每个优先级对应一个 `List_t` |
| 当前延时链表 | `pxDelayedTaskList` | 调度器全局指针，指向 `xDelayedTaskList1` 或 `xDelayedTaskList2` |
| 溢出延时链表 | `pxOverflowDelayedTaskList` | 调度器全局指针，指向另一张延时表 |
| 挂起链表 | `xSuspendedTaskList` | 调度器全局唯一链表 |
| 等待发送链表 | `Queue_t.xTasksWaitingToSend` | 每个队列对象各有一张 |
| 等待接收链表 | `Queue_t.xTasksWaitingToReceive` | 每个队列、信号量或互斥量对象各有一张 |
| Pending Ready | `xPendingReadyList` | 调度器全局唯一链表 |

#### 就绪链表：`pxReadyTasksLists[]`

它不是一张链表，而是一个链表数组：

```c
static List_t pxReadyTasksLists[ configMAX_PRIORITIES ];
```

数组下标就是任务优先级。例如 `pxReadyTasksLists[3]` 保存所有优先级为 3 的就绪任务。任务的 `xStateListItem` 挂在其中。调度器先找到最高的非空数组项，再通过该链表的 `pxIndex` 在同优先级任务间轮转。

当前正在运行的任务也仍然位于对应就绪链表中，只是由 `pxCurrentTCB` 指向它，并不存在单独的 Running 链表。

#### 延时链表：`xDelayedTaskList1/2`

源码实际创建两张物理链表：

```c
static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;
static List_t * volatile pxDelayedTaskList;
static List_t * volatile pxOverflowDelayedTaskList;
```

- `pxDelayedTaskList`：当前 tick 计数周期内会到期的任务。
- `pxOverflowDelayedTaskList`：唤醒 tick 已跨过计数回绕点的任务。
- `xDelayedTaskList1` 和 `xDelayedTaskList2`：真正存储节点的两张表。
- tick 计数回绕时，两个指针交换，不需要搬迁全部任务节点。

任务以 `xStateListItem.xItemValue = 绝对唤醒 tick` 排序，链表头始终是最近需要唤醒的任务。

#### 挂起链表：`xSuspendedTaskList`

这是一张调度器全局链表，任务的 `xStateListItem` 挂在其中。它容纳两种任务：

1. 通过 `vTaskSuspend()` 显式挂起的任务。
2. 在启用 `INCLUDE_vTaskSuspend` 时，以 `portMAX_DELAY` 无限期等待事件的任务。

第二种任务虽然位于 Suspended List，但从 API 语义看仍是在等待信号量或队列事件；其 `xEventListItem` 同时还挂在对应对象的事件等待链表中。

#### 事件等待链表：对象内部的等待表

事件等待链表不是调度器中的一张全局表。每个 `Queue_t` 对象内部都有两张：

```c
List_t xTasksWaitingToSend;
List_t xTasksWaitingToReceive;
```

- `xTasksWaitingToSend`：队列已满时，等待发送空间的任务。
- `xTasksWaitingToReceive`：队列为空时，等待接收数据的任务。
- 对二值信号量、计数信号量和互斥量，`Take` 本质上走接收路径，所以等待任务主要位于 `xTasksWaitingToReceive`。
- 对象释放一个 token 或收到数据时，从相应事件链表头取出最高优先级等待任务。

事件表挂的是任务的 `xEventListItem`，不是 `xStateListItem`。同一任务阻塞时通常同时存在于两张表：

```text
TCB.xEventListItem --> 某个 Queue_t.xTasksWaitingToReceive
TCB.xStateListItem --> pxDelayedTaskList 或 xSuspendedTaskList
```

前者回答“任务在等待什么”，后者回答“任务当前处于什么调度状态，以及何时超时”。

#### Pending Ready：`xPendingReadyList`

当 `uxSchedulerSuspended > 0` 时，内核不能立即重排普通就绪链表。如果 ISR 或其他事件唤醒了一个任务，内核会：

1. 从原事件等待链表移除该任务的 `xEventListItem`。
2. 把同一个 `xEventListItem` 插入 `xPendingReadyList`。
3. 暂时保留其 `xStateListItem` 在延时链表或挂起链表中。
4. 等 `xTaskResumeAll()` 恢复调度器后，再移除状态节点并加入 `pxReadyTasksLists[priority]`。

因此 `xPendingReadyList` 是调度器挂起期间的“延迟就绪中转站”，并不是正常任务状态链表。

*源码定位：Source/tasks.c: pxReadyTasksLists、xDelayedTaskList1/2、xSuspendedTaskList、xPendingReadyList；Source/queue.c: Queue_t.xTasksWaitingToSend、Queue_t.xTasksWaitingToReceive*

# 3. Task：TCB、状态链表与任务调度

## 3.1 TCB 的关键字段

| 字段 | 作用 | 设计含义 |
| --- | --- | --- |
| pxTopOfStack | 当前任务保存后的栈顶 | 必须位于 TCB 前部，供端口汇编快速访问 |
| xStateListItem | 挂入就绪、延时、溢出延时或挂起链表 | 一个任务任一时刻只能处于一个调度状态链表 |
| xEventListItem | 挂入对象事件等待链表或 Pending Ready | 事件关系与状态关系分离 |
| uxPriority | 当前有效优先级 | 调度和事件排序依据 |
| uxBasePriority | 互斥量继承前的基础优先级 | 用于优先级恢复 |
| uxMutexesHeld | 持有互斥量数量 | 只有数量归零时才恢复基础优先级 |
| uxCriticalNesting | 部分端口按任务保存临界嵌套深度 | 任务切换后仍保持各自的嵌套状态 |

*源码定位：Source/tasks.c: TCB_t 定义及任务初始化逻辑*

## 3.2 任务状态不是枚举，而是链表归属

```text
创建/解除阻塞
                    |
                    v
  Ready[p] ----调度----> Running
     ^                    |
     |                    | vTaskDelay / queue wait / semaphore take
     |                    v
     +--------------- Delayed / Suspended
                         |
                         +-- 同时：xEventListItem -> 某对象等待链表

说明：Running 任务仍保留在对应 Ready[p] 中，pxCurrentTCB 指向当前运行者。
```

FreeRTOS 没有为“运行态”单独建立链表。当前任务仍在对应优先级的就绪链表中，pxCurrentTCB 只是指出其中哪个 TCB 正在运行。任务阻塞时才从就绪链表摘除，并把 xStateListItem 移到延时或挂起链表。

## 3.3 就绪链表与最高优先级选择

内核为每个优先级维护一个就绪链表 pxReadyTasksLists[priority]。加入就绪态时使用 vListInsertEnd()，选择任务时先找到最高的非空优先级，再通过 listGET_OWNER_OF_NEXT_ENTRY() 推进 pxIndex，从而在同优先级任务之间轮转。

| 配置/路径 | 最高优先级查找 | 同优先级处理 |
| --- | --- | --- |
| configUSE_PORT_OPTIMISED_TASK_SELECTION = 0 | 从 uxTopReadyPriority 向下扫描 | pxIndex 每次向后移动一个节点 |
| = 1 | 端口位图/CLZ 等硬件方法 | 仍由同一 List 宏完成轮转 |
| configUSE_TIME_SLICING = 1 | tick 到来时检查同级链表长度 | 同优先级超过一个任务时请求切换 |
| configUSE_PREEMPTION = 1 | 高优先级任务就绪可触发切换 | 是否立即切换仍由上下文和端口宏决定 |

*源码定位：Source/tasks.c: taskSELECT_HIGHEST_PRIORITY_TASK、prvAddTaskToReadyList、vTaskSwitchContext*

## 3.4 Tick 路径

1. SysTick 中断进入端口层，调用 xTaskIncrementTick()。

1. 若调度器未挂起，xTickCount 加一；发生计数回绕时交换普通延时表与溢出延时表。

1. 只检查延时链表头部，因为链表已按唤醒 tick 排序；连续取出所有已到期任务。

1. 删除任务的 xStateListItem；若任务还挂在某事件链表，也删除 xEventListItem。

1. 把任务加入其当前优先级的就绪链表。

1. 若新就绪任务优先级不低于当前任务，或同级时间片到期，则返回需要切换的标志。

1. 端口层挂起 PendSV；PendSV 保存当前任务现场，调用 vTaskSwitchContext()，恢复下一任务现场。

> **延时链表为什么有两张**
>
> 绝对唤醒 tick 可能跨越 TickType_t 回绕点。当前周期内到期的任务放入 xDelayedTaskList，回绕后才到期的任务放入 xOverflowDelayedTaskList；tick 回绕时交换两张表即可，无需重算全部节点。

*源码定位：Source/tasks.c: xTaskIncrementTick()、prvAddCurrentTaskToDelayedList()；portable/GCC/ARM_CM4F/port.c: SysTick/PendSV*

## 3.5 调度触发源

| 触发源 | 典型条件 | 动作 |
| --- | --- | --- |
| Tick | 高优先级延时任务到期；同优先级时间片到期 | xTaskIncrementTick 返回 pdTRUE，挂起 PendSV |
| 任务 API | give/send 后唤醒更高优先级任务 | queueYIELD_IF_USING_PREEMPTION 或 taskYIELD |
| FromISR API | 唤醒任务优先级高于被中断任务 | 置 pxHigherPriorityTaskWoken，由 ISR 尾部 portYIELD_FROM_ISR |
| 显式让出 | taskYIELD() | 当前优先级链表 pxIndex 前移 |
| 恢复调度器 | xTaskResumeAll 处理 Pending Ready 或积压 tick | 必要时产生一次切换 |

# 4. Queue：队列、信号量和互斥量的统一内核对象

## 4.1 Queue_t 布局

| 字段 | 普通队列含义 | 信号量/互斥量含义 |
| --- | --- | --- |
| pcHead / pcTail | 存储区起点/尾后地址 | 零长度数据项时被复用为类型标记或持有者 |
| pcWriteTo | 下一写入位置 | 通常无实际数据复制 |
| u.pcReadFrom | 上一次读取位置 | 互斥量时 union 复用递归计数 |
| uxMessagesWaiting | 当前消息数 | 当前 token 数；互斥量通常为 0/1 |
| uxLength / uxItemSize | 队列容量/单项大小 | 信号量 uxItemSize = 0 |
| xTasksWaitingToSend | 等待空位的任务 | 等待 give 的发送侧场景通常较少 |
| xTasksWaitingToReceive | 等待数据的任务 | 等待 take 的任务 |
| cRxLock / cTxLock | 调度器挂起期间累计延迟唤醒次数 | 同样用于同步对象 |

*源码定位：Source/queue.c: Queue_t、prvInitialiseNewQueue()、prvInitialiseMutex()*

## 4.2 为什么统一实现

信号量本质上只需要“计数 + 等待者”。普通队列在此基础上再增加数据存储区，因此 FreeRTOS 让二值信号量、计数信号量、互斥量和递归互斥量复用 Queue_t 及其事件链表。semphr.h 中的 API 最终映射到 queue.c 的发送或接收路径。

| 对象 | Queue_t 参数/特征 | Take | Give |
| --- | --- | --- | --- |
| 普通队列 | uxLength=N，uxItemSize>0 | xQueueReceive | xQueueSend |
| 二值信号量 | uxLength=1，uxItemSize=0 | xQueueGenericReceive | xQueueGenericSend |
| 计数信号量 | uxLength=max，uxItemSize=0 | 计数减一 | 计数加一 |
| 互斥量 | 长度 1、零大小、记录 holder | 获取并记录持有者 | 释放并执行优先级去继承 |
| 递归互斥量 | 互斥量 + 递归计数 | 同一任务可重复获取 | 计数归零才真正释放 |

> **互斥量与二值信号量不能互换**
>
> 互斥量具有所有权和优先级继承，只能由持有任务释放，不能在 ISR 中使用；二值/计数信号量没有所有权，适合任务间或 ISR 到任务的事件通知。

## 4.3 发送与接收的快路径

xQueueGenericSend() 和 xQueueGenericReceive() 都先进入一个很短的任务临界区：检查队列状态、复制数据、更新 uxMessagesWaiting，并尝试唤醒对侧等待任务。只要操作能够立即完成，就不会挂起调度器，也不会进入事件阻塞路径。

```text
发送成功：
  taskENTER_CRITICAL
    检查未满 -> 复制数据 -> messages++
    若接收等待链表非空：xTaskRemoveFromEventList()
  taskEXIT_CRITICAL
  必要时 yield

接收成功：
  taskENTER_CRITICAL
    检查非空 -> 复制数据 -> messages--
    若发送等待链表非空：xTaskRemoveFromEventList()
  taskEXIT_CRITICAL
  必要时 yield
```

## 4.4 阻塞前为何要“挂起调度器 + 锁队列”

当队列满/空且调用者允许等待时，函数退出短临界区，挂起调度器并锁定队列，然后重新检查超时和队列状态。原因是阻塞过程要修改多张任务链表，时间比简单复制数据长，不应全程屏蔽中断；与此同时 ISR 仍可能对队列收发。

队列锁不阻止 ISR 修改 uxMessagesWaiting 或数据区，而是阻止 ISR 直接修改事件等待链表。ISR 只增加 cTxLock/cRxLock；任务侧解锁时，prvUnlockQueue() 再集中完成对应次数的唤醒。这是一种延迟链表操作机制。

*源码定位：Source/queue.c: prvLockQueue、prvUnlockQueue、xQueueGenericSend、xQueueGenericReceive*

# 5. 临界操作：严格说 3 类，按内核并发层次看 5 类

“临界操作有几种”取决于统计口径。若只统计硬件中断屏蔽接口，可归纳为 3 类；若把 FreeRTOS 用来保持调度链表一致性的机制一起统计，则应按下面 5 类理解。

| 类别 | 典型接口 | 是否关中断 | 是否允许 ISR 运行 | 主要保护对象 |
| --- | --- | --- | --- | --- |
| 1. 任务临界区 | taskENTER_CRITICAL / EXIT | 是，通常可嵌套 | 仅高于屏蔽阈值的 ISR | 短小共享状态和内核链表操作 |
| 2. ISR 保存/恢复屏蔽 | portSET_INTERRUPT_MASK_FROM_ISR / CLEAR | 是，保存原状态 | 仅高于屏蔽阈值的 ISR | FromISR API 的原子更新 |
| 3. 原始禁用/启用 | taskDISABLE_INTERRUPTS / ENABLE | 是，不提供通用嵌套语义 | 取决于端口 | 端口启动、断言或极底层路径 |
| 4. 调度器挂起 | vTaskSuspendAll / xTaskResumeAll | 否 | 是 | 防止任务切换；延迟就绪和 tick 处理 |
| 5. 队列逻辑锁 | cRxLock / cTxLock + prvUnlockQueue | 否，内部只用短临界区 | 是，ISR 可改数据 | 延迟事件等待链表的唤醒操作 |

> **ARM Cortex-M4F 端口的具体含义**
>
> 本仓库 GCC/ARM_CM4F 端口使用 BASEPRI 屏蔽可调用 FreeRTOS API 的中断优先级范围，更高紧迫度的中断仍可能运行，但它们不得调用 FreeRTOS API。其他端口可能使用全局关中断，不能把 BASEPRI 结论机械套用到所有 CPU。

## 5.1 任务临界区

- 可嵌套：只有最外层退出时才恢复允许的中断。

- 部分端口把 uxCriticalNesting 保存在 TCB 中，因此任务在临界区内被设计上禁止切换，但嵌套状态仍属于任务上下文。

- 临界区必须短；不可在其中等待信号量、调用可能阻塞的 API 或执行长时间外设轮询。

- taskENTER_CRITICAL() 不能直接在 ISR 中替代 FromISR 版本。

## 5.2 FromISR 屏蔽保存/恢复

ISR 可能在进入 FreeRTOS API 前已经处于某种屏蔽状态，因此必须保存旧 mask 并在退出时精确恢复。这也是 FromISR API 通常把 mask 保存到局部变量，而不是简单成对 enable/disable 的原因。

## 5.3 调度器挂起不是互斥锁

vTaskSuspendAll() 只增加 uxSchedulerSuspended，不关闭中断。期间 ISR 可以发生，也可以通过 FromISR API 使任务逻辑上就绪；但任务不会立即加入普通就绪表，而是进入 xPendingReadyList，tick 也通过 uxPendedTicks 记账。xTaskResumeAll() 在短临界区内回放这些变化，然后决定是否切换。

> **错误用法**
>
> 不能用 vTaskSuspendAll() 保护会被 ISR 直接读写的普通共享变量；ISR 仍在运行。它适合保护只会被任务并发访问的长操作，或配合内核的 Pending Ready/队列锁协议使用。

*源码定位：Source/tasks.c: vTaskSuspendAll()、xTaskResumeAll()；Source/portable/GCC/ARM_CM4F/portmacro.h*

## 5.4 五类机制的选择规则

| 场景 | 应使用 | 不应使用 |
| --- | --- | --- |
| 任务与 ISR 共享一个短变量/寄存器影子 | 任务临界区 + ISR mask 版本 | 只挂起调度器 |
| 任务间保护较长的业务数据结构 | 互斥量 | 长时间 taskENTER_CRITICAL |
| 内核需要跨多个链表操作且 ISR 仍需响应 | 调度器挂起 + Pending Ready 协议 | 全程关中断 |
| 队列阻塞过程中允许 ISR 继续收发 | 调度器挂起 + cRxLock/cTxLock | 应用自行修改 Queue_t |
| ISR 通知任务 | xSemaphoreGiveFromISR 等 + portYIELD_FROM_ISR | 普通 xSemaphoreGive |

# 6. 信号量与队列事件的阻塞、唤醒

## 6.1 阻塞的双重登记

任务等待队列或信号量时，vTaskPlaceOnEventList() 完成两件事：

1. 把当前任务的 xEventListItem 插入对象的事件等待链表，按反向编码优先级排序。

1. 把当前任务的 xStateListItem 从就绪链表移除，并按等待时间放入延时链表；若允许永久阻塞且等待时间为 portMAX_DELAY，则放入挂起链表。

```text
阻塞后的同一 TCB：

  TCB.xEventListItem ----> semaphore/queue.xTasksWaitingToReceive
           （等什么）

  TCB.xStateListItem ----> xDelayedTaskList 或 xSuspendedTaskList
           （等多久/当前调度状态）
```

*源码定位：Source/tasks.c: vTaskPlaceOnEventList()、prvAddCurrentTaskToDelayedList()*

## 6.2 二值/计数信号量 Take：无 token 时

1. xSemaphoreTake() 宏进入 xQueueGenericReceive()。

1. 短临界区检查 uxMessagesWaiting；若大于 0，直接减一并返回成功。

1. 若为 0 且 xTicksToWait=0，立即返回失败。

1. 若允许等待，初始化超时状态，挂起调度器并锁队列。

1. 再次检查队列和超时，避免在退出临界区到加锁之间丢失事件。

1. 仍无 token 时，把当前任务放入 xTasksWaitingToReceive 和延时/挂起链表。

1. 解锁队列、恢复调度器；当前任务已不在就绪表，因此发生上下文切换。

## 6.3 ISR Give：唤醒等待任务

1. ISR 调用 xSemaphoreGiveFromISR()，进入 xQueueGiveFromISR()。

1. 保存并提高中断屏蔽级别，检查计数未达到上限，然后 uxMessagesWaiting 加一。

1. 若队列未锁，直接从 xTasksWaitingToReceive 取出最高优先级等待任务。

1. xTaskRemoveFromEventList() 删除其事件节点和状态节点，并加入对应优先级就绪表；若调度器挂起，则先放入 xPendingReadyList。

1. 如果被唤醒任务优先级高于当前被中断任务，设置 *pxHigherPriorityTaskWoken=pdTRUE。

1. 恢复中断 mask；ISR 退出前调用 portYIELD_FROM_ISR()，在合适时机触发 PendSV。

```text
ISR                      Kernel lists                    Scheduler
 | give token                 |                               |
 |--------------------------->| messages++                    |
 |                            | remove highest waiter         |
 |                            | waiter -> Ready[priority]      |
 |<---------------------------| higherPriorityTaskWoken=TRUE  |
 | portYIELD_FROM_ISR ----------------------------------------> PendSV
 |                                                            switch
```

> **不会丢失事件的关键**
>
> 阻塞路径在挂起调度器并锁队列后会重新检查队列状态；ISR 在队列锁定期间仍可增加 token，但只累计锁计数。解锁时内核依据累计次数唤醒等待者。因此“检查为空”和“真正入等待链表”之间的竞态由队列锁协议闭合。

## 6.4 超时唤醒与事件唤醒的竞争

同一个任务既在事件链表，也在延时链表。若事件先到，xTaskRemoveFromEventList() 会删除状态节点并就绪；若超时先到，xTaskIncrementTick() 会删除状态节点，同时检查并删除事件节点。由于关键摘链操作处于相应临界协议中，最终只有一条路径获得该任务，另一条路径看到节点已不在原容器。

## 6.5 队列满时发送任务阻塞

xQueueSend() 在队列已满时使用 xTasksWaitingToSend。接收方成功取走一个元素后，会从该链表唤醒最高优先级发送者。其链表迁移与信号量 Take/Give 完全对称，只是事件方向相反。

| 条件 | 等待链表 | 解除阻塞事件 |
| --- | --- | --- |
| 接收空队列/Take 空信号量 | xTasksWaitingToReceive | 发送数据或 Give token |
| 发送满队列 | xTasksWaitingToSend | 接收数据腾出空间 |
| 等待超时 | 同时在延时链表 | tick 到达唤醒时间 |
| 调度器挂起期间事件到达 | xPendingReadyList | xTaskResumeAll 回放到就绪表 |

## 6.6 互斥量与优先级继承

高优先级任务等待已被低优先级任务持有的互斥量时，内核调用 vTaskPriorityInherit() 提升持有者的当前优先级，必要时还要把持有者从旧优先级就绪链表移动到新优先级链表。释放互斥量时执行优先级去继承。

```text
L(优先级1) 持有 Mutex
M(优先级2) 就绪
H(优先级3) 尝试 Take Mutex -> 阻塞

若无继承：M 可长期抢占 L，H 间接被 M 阻塞
有继承：L 临时提升到 3 -> 尽快运行并释放 Mutex -> H 获得 Mutex
```

> **v9.0.0 的简化继承模型**
>
> TCB 记录 uxMutexesHeld。任务持有多个互斥量时，通常要等持有数量降到 0 才恢复 uxBasePriority。这不是完整的逐互斥量依赖图算法，设计嵌套锁时应避免长临界链和复杂锁顺序。

*源码定位：Source/tasks.c: vTaskPriorityInherit()、xTaskPriorityDisinherit()；Source/queue.c: mutex receive/give path*

# 7. Heap：五种内存管理实现

FreeRTOS 的 pvPortMalloc()/vPortFree() 由 portable/MemMang 中某一个 heap_x.c 提供。工程必须只编译一个实现；它们是互斥替代方案，不是可同时注册的多个堆。

| 实现 | 分配策略 | 释放 | 合并空闲块 | 主要优点 | 主要风险/限制 |
| --- | --- | --- | --- | --- | --- |
| heap_1 | 线性递增指针 | 不支持 | 无 | 最小、确定、不会外部碎片 | 删除对象不归还内存 |
| heap_2 | 按块大小组织，近似 best-fit | 支持 | 不合并 | 结构简单，可回收 | 长期反复分配易碎片化 |
| heap_3 | 封装 libc malloc/free | 支持 | 由 libc 决定 | 使用运行库能力 | 确定性和线程安全依赖 C 库/链接器 |
| heap_4 | 地址有序空闲链表，first-fit | 支持 | 相邻块合并 | 通用、可观测剩余量 | 仍可能碎片化，分配时间与空闲链相关 |
| heap_5 | heap_4 + 多个不连续区域 | 支持 | 区域内相邻合并 | 利用分散 RAM 区 | 必须先定义区域且按地址顺序配置 |

*源码定位：Source/portable/MemMang/heap_1.c 至 heap_5.c*

## 7.1 各实现内部结构

- heap_1：只有当前偏移量；分配时对齐并向后推进，没有空闲链表。

- heap_2：空闲链表按块大小排序，找到能容纳请求的块并可拆分；释放后重新按大小插入，但不检查物理相邻。

- heap_3：在 vTaskSuspendAll()/xTaskResumeAll() 包围下调用标准 malloc/free。

- heap_4：空闲链表按内存地址排序，释放插入时检查前后块地址，能合并连续空闲块。

- heap_5：先通过 vPortDefineHeapRegions() 把多个地址区间串成与 heap_4 相同的地址有序空闲结构。

## 7.2 Heap 使用的是调度器挂起，而不是长期关中断

这些实现通常在分配/释放期间调用 vTaskSuspendAll()/xTaskResumeAll()，以避免另一个任务同时修改堆结构，但不因此屏蔽 ISR。这也说明动态分配 API 面向任务上下文，不应从 ISR 调用。

## 7.3 选择建议

| 系统特征 | 建议 |
| --- | --- |
| 所有对象启动时创建，运行后不删除 | heap_1；结构最简单，容量可静态预算 |
| 对象大小固定且生命周期可控 | heap_2 可用，但必须评估不合并造成的碎片 |
| 希望使用平台 libc，且已验证线程安全与确定性 | heap_3 |
| 一般嵌入式动态创建/删除 | 优先考虑 heap_4 |
| RAM 分布在多个不连续地址区 | heap_5，并在创建任何内核对象前定义区域 |
| 高完整性/强确定性系统 | 优先静态创建 API；动态堆只用于初始化阶段 |

# 8. 三条完整调用链

## 8.1 任务等待信号量，ISR 释放

```text
Task H:
  xSemaphoreTake
    -> xQueueGenericReceive
    -> queue empty
    -> vTaskSuspendAll + prvLockQueue
    -> vTaskPlaceOnEventList(xTasksWaitingToReceive)
       -> event item 加入信号量等待表
       -> state item 加入 delayed/suspended
    -> prvUnlockQueue + xTaskResumeAll
    -> switch out

ISR:
  xSemaphoreGiveFromISR
    -> xQueueGiveFromISR
    -> messages++
    -> xTaskRemoveFromEventList
       -> H 移出事件表和 delayed/suspended
       -> H 加入 Ready[H.priority]
    -> higherPriorityTaskWoken = pdTRUE
  portYIELD_FROM_ISR
    -> PendSV
    -> vTaskSwitchContext
    -> H 运行
```

## 8.2 发送者等待满队列，接收者腾出空间

```text
Sender:
  xQueueSend -> full
  -> 加入 xTasksWaitingToSend + delayed list
  -> 阻塞

Receiver:
  xQueueReceive -> copy out -> messages--
  -> xTaskRemoveFromEventList(xTasksWaitingToSend)
  -> Sender 加入 Ready
  -> 若 Sender 优先级更高，触发切换

Sender 恢复后：
  重新进入发送循环并再次检查队列，而不是假设空间永久属于自己。
```

## 8.3 Tick 使等待任务超时

```text
SysTick
  -> xTaskIncrementTick
  -> 当前 tick >= delayed list head.xItemValue
  -> uxListRemove(xStateListItem)
  -> 若 xEventListItem 仍有 container：uxListRemove(xEventListItem)
  -> prvAddTaskToReadyList
  -> 必要时 PendSV

任务恢复到 xQueueGenericReceive/Send：
  -> xTaskCheckForTimeOut
  -> 再检查对象状态
  -> 返回 errQUEUE_EMPTY / errQUEUE_FULL 或相应失败值
```

# 9. 关键不变量与调试检查表

## 9.1 链表不变量

- ListItem_t.pvContainer 为 NULL 表示未挂入任何链表；非 NULL 时必须指向实际容器。

- xStateListItem 在就绪、延时、溢出延时、挂起链表中最多属于一个。

- xEventListItem 在对象等待链表与 Pending Ready 中最多属于一个。

- 延时链表头必须是最近到期任务；事件链表头必须对应最高任务优先级。

- List_t.uxNumberOfItems 必须与闭环中的有效节点数一致。

## 9.2 调度问题排查

| 现象 | 优先检查 |
| --- | --- |
| 高优先级任务未运行 | 是否真的进入 Ready；pxHigherPriorityTaskWoken 是否传入并在 ISR 尾部 yield |
| 任务永久阻塞 | 对象等待链表、xDelayedTaskList/xSuspendedTaskList、portMAX_DELAY 配置 |
| 同优先级任务不轮转 | configUSE_TIME_SLICING、tick 是否运行、是否长期关中断 |
| xTaskResumeAll 后状态异常 | uxSchedulerSuspended 嵌套是否配对，Pending Ready 是否积压 |
| 队列偶发丢唤醒 | 是否混用普通 API/FromISR API，是否绕过 Queue_t 直接改字段 |
| 优先级反转 | 是否使用二值信号量代替互斥量，持锁时间和嵌套顺序是否合理 |

## 9.3 内存问题排查

- 确认工程只链接一个 heap_x.c。

- 开启 malloc failed hook，并检查 xPortGetFreeHeapSize() 与 xPortGetMinimumEverFreeHeapSize()。

- heap_2 出现总空闲量足够但大块分配失败时，重点怀疑外部碎片。

- heap_4/5 检查越界写是否破坏 BlockLink_t 和块头分配标志。

- heap_5 确认 vPortDefineHeapRegions() 早于任何创建任务、队列、信号量的调用。

# 10. 核心结论

1. List 是 FreeRTOS 状态管理的基础。TCB 的两个链表节点分别表示调度状态和事件等待关系。

1. 调度器按最高优先级选择任务，同优先级通过就绪链表 pxIndex 轮转；tick 同时负责延时到期和时间片。

1. Queue_t 是统一同步对象。信号量是零大小数据项的队列，互斥量在此基础上增加所有权和优先级继承。

1. 阻塞是“双重登记”：事件节点进入对象等待表，状态节点进入延时或挂起表；事件或超时任一路径负责清理另一节点。

1. 硬件临界区、调度器挂起、队列逻辑锁解决的是不同并发问题，不能互相替代。

1. heap_1..heap_5 是编译时五选一策略；一般动态场景以 heap_4 为基线，多内存区域使用 heap_5。

# 附录 A：源码索引

| 主题 | 文件与符号 |
| --- | --- |
| 链表结构 | Source/include/list.h: ListItem_t、MiniListItem_t、List_t |
| 链表算法 | Source/list.c: vListInitialise、vListInsertEnd、vListInsert、uxListRemove |
| TCB 与就绪表 | Source/tasks.c: TCB_t、pxReadyTasksLists、prvAddTaskToReadyList |
| tick 与切换决策 | Source/tasks.c: xTaskIncrementTick、vTaskSwitchContext |
| 事件阻塞/唤醒 | Source/tasks.c: vTaskPlaceOnEventList、xTaskRemoveFromEventList |
| 延时链表 | Source/tasks.c: prvAddCurrentTaskToDelayedList |
| 调度器挂起 | Source/tasks.c: vTaskSuspendAll、xTaskResumeAll |
| 优先级继承 | Source/tasks.c: vTaskPriorityInherit、xTaskPriorityDisinherit |
| 队列对象 | Source/queue.c: Queue_t、xQueueGenericSend、xQueueGenericReceive |
| ISR 队列路径 | Source/queue.c: xQueueGenericSendFromISR、xQueueGiveFromISR |
| 队列锁 | Source/queue.c: prvLockQueue、prvUnlockQueue |
| 信号量宏 | Source/include/semphr.h |
| 内存管理 | Source/portable/MemMang/heap_1.c ... heap_5.c |
| Cortex-M4F 临界宏 | Source/portable/GCC/ARM_CM4F/portmacro.h |
| Cortex-M4F 上下文切换 | Source/portable/GCC/ARM_CM4F/port.c: PendSV、SysTick |

注：源码行号会随本地修改发生变化，因此本文以文件名和函数/类型符号作为稳定定位信息。
