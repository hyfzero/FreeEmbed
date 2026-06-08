# FreeRTOS Queue 操作机制深度分析

> 本文档分析 `Source/queue.c` 中队列的核心操作机制，覆盖发送、接收、锁机制、互斥量递归、优先级继承等关键技术细节。

---

## 1. 总体架构

```
                    ┌──────────────────────────────────────┐
                    │            Queue_t (队列)              │
                    │                                      │
                    │  存储区 (环形缓冲区)                    │
                    │  ┌─────────────────────────────────┐  │
                    │  │ pcHead → [slot0][slot1]...[slotN] │  │
                    │  │ pcTail → [末尾标记字节]            │  │
                    │  │ pcWriteTo ────→ 下一个写入位置     │  │
                    │  │ u.pcReadFrom ──→ 最后读取位置      │  │
                    │  └─────────────────────────────────┘  │
                    │                                      │
                    │  xTasksWaitingToSend  (List_t)        │
                    │    等待发送者链表 (队列满时挂载任务)      │
                    │                                      │
                    │  xTasksWaitingToReceive (List_t)       │
                    │    等待接收者链表 (队列空时挂载任务)      │
                    │                                      │
                    │  cTxLock / cRxLock  (锁计数器)         │
                    └──────────────────────────────────────┘
```

队列的所有操作围绕两个 `List_t` 和一个环形缓冲区展开。API 分为两大类：

| 分类 | 操作 | 能否阻塞 |
|---|---|---|
| **任务级 API** | `xQueueSend` / `xQueueReceive` 等 | ✅ 可阻塞等待 |
| **中断级 API** | `xQueueSendFromISR` / `xQueueReceiveFromISR` 等 | ❌ 绝不阻塞 |

---

## 2. 环形缓冲区存储模型

### 2.1 内存布局

队列存储区大小为 `(uxLength * uxItemSize) + 1` 字节。多出的 1 字节是**区分 "满" 和 "空" 的关键**。

```
┌──────────────────────────────────────────────────────────────┐
│  队列存储区 (uxLength * uxItemSize 字节  +  1 字节末尾标记)      │
│                                                              │
│  pcHead ──→  [Item 0]  [Item 1]  [Item 2]  ... [Item N-1] [标记]  ←── pcTail
│               ↑                                             ↑
│               │  pcReadFrom 指针在这里移动                     │ pcTail 指向标记字节之后
│               │  pcWriteTo 指针在这里移动                      │ (即 pcHead + uxLength*uxItemSize)
└──────────────────────────────────────────────────────────────┘
```

### 2.2 写入方向

```c
/* 写到队尾 (queueSEND_TO_BACK) */
pcWriteTo += uxItemSize;       // 指针前进 uxItemSize 字节
if (pcWriteTo >= pcTail)        // 到达尾部，回绕到头部
    pcWriteTo = pcHead;

/* 写到队首 (queueSEND_TO_FRONT / queueOVERWRITE) */
u.pcReadFrom -= uxItemSize;    // 指针后退 uxItemSize 字节
if (u.pcReadFrom < pcHead)     // 到达头部，回绕到尾部
    u.pcReadFrom = pcTail - uxItemSize;
```

### 2.3 读取方向

```c
u.pcReadFrom += uxItemSize;    // 指针前进 uxItemSize 字节
if (u.pcReadFrom >= pcTail)    // 到达尾部，回绕到头部
    u.pcReadFrom = pcHead;
memcpy(pvBuffer, u.pcReadFrom, uxItemSize); // 从新位置拷贝数据
```

### 2.4 满/空判断

| 条件 | 含义 |
|---|---|
| `pcWriteTo == pcReadFrom` | 队列空（读写指针重合） |
| `(pcWriteTo + uxItemSize) 回绕后 == pcReadFrom` | 队列满 |
| `uxMessagesWaiting == uxLength` | 队列满（高层判断） |
| `uxMessagesWaiting == 0` | 队列空（高层判断） |

实际代码中使用 `uxMessagesWaiting` 计数器来判断，避免了复杂的指针比较。

---

## 3. 队列锁机制 (`cTxLock` / `cRxLock`)

### 3.1 设计动机

在任务试图阻塞等待队列时，需要**挂起调度器**（`vTaskSuspendAll()`），然后把自己挂到等待链表。但如果这个过程中发生了 ISR 向队列发送/接收数据，ISR 不能直接操作等待链表（因为调度器已挂起），于是引入了**锁计数器**。

### 3.2 锁状态定义

```c
#define queueUNLOCKED        ((int8_t) -1)   // 队列未锁定
#define queueLOCKED_UNMODIFIED ((int8_t) 0)  // 队列被锁定，锁期间无人操作
```

### 3.3 加锁 (`prvLockQueue`)

```c
#define prvLockQueue(pxQueue)
    taskENTER_CRITICAL();
    {
        if ((pxQueue)->cRxLock == queueUNLOCKED)
            (pxQueue)->cRxLock = queueLOCKED_UNMODIFIED;   // -1 → 0
        if ((pxQueue)->cTxLock == queueUNLOCKED)
            (pxQueue)->cTxLock = queueLOCKED_UNMODIFIED;   // -1 → 0
    }
    taskEXIT_CRITICAL();
```

### 3.4 解锁 (`prvUnlockQueue`)

```c
static void prvUnlockQueue(Queue_t * const pxQueue)
{
    // ====== 处理 cTxLock（发送端锁计数器） ======
    int8_t cTxLock = pxQueue->cTxLock;   // 当前值可能是: -1, 0, 1, 2, ...
    while (cTxLock > queueLOCKED_UNMODIFIED)   // cTxLock > 0
    {
        // 说明锁期间有 ISR 向队列发送了 cTxLock 个数据
        // 逐个检查 xTasksWaitingToReceive 链表，唤醒等待者
        if (xTasksWaitingToReceive 非空)
            xTaskRemoveFromEventList(...);   // 移除一个等待者
        else
            break;  // 没人等了，停止
        --cTxLock;
    }
    pxQueue->cTxLock = queueUNLOCKED;   // 重置为 -1

    // ====== 处理 cRxLock（接收端锁计数器） ======
    int8_t cRxLock = pxQueue->cRxLock;
    while (cRxLock > queueLOCKED_UNMODIFIED)   // cRxLock > 0
    {
        // 说明锁期间有 ISR 从队列取走了 cRxLock 个数据
        // 逐个检查 xTasksWaitingToSend 链表，唤醒等待者
        if (xTasksWaitingToSend 非空)
            xTaskRemoveFromEventList(...);   // 移除一个等待者
        else
            break;
        --cRxLock;
    }
    pxQueue->cRxLock = queueUNLOCKED;
}
```

### 3.5 ISR 端的锁感知

在 `xQueueGenericSendFromISR` 中：

```c
if (cTxLock == queueUNLOCKED)   // -1: 队列未锁定
{
    // ISR 可以直接操作等待链表
    xTaskRemoveFromEventList(&pxQueue->xTasksWaitingToReceive);
}
else                            // >= 0: 队列被锁定
{
    // 只记录计数，等解锁后批量处理
    pxQueue->cTxLock = (int8_t)(cTxLock + 1);  // 0 → 1，1 → 2，...
}
```

**这是 FreeRTOS 中断安全设计的精妙之处**——ISR 可以在队列锁定期间安全地发送数据，而不会破坏任务级代码对等待链表的操作。

### 3.6 锁状态机

```
                      prvLockQueue
          queueUNLOCKED ────────────→ queueLOCKED_UNMODIFIED
              (-1)                          (0)
                 ↑                              │
                 │        ISR 写数据             │ cTxLock++ (每个 ISR 写入)
                 │    cTxLock: 1→2→3...          │
                 │                              ↓
                 │                    queueLOCKED_MODIFIED
                 │                         (N > 0)
                 │                              │
                 └──────────────────────────────┘
                      prvUnlockQueue
                 (逐个唤醒等待者并重置为 -1)
```

---

## 4. `xQueueGenericSend` — 任务级发送

### 4.1 函数签名

```c
BaseType_t xQueueGenericSend(
    QueueHandle_t xQueue,
    const void * const pvItemToQueue,
    TickType_t xTicksToWait,       // 超时等待时间
    const BaseType_t xCopyPosition  // queueSEND_TO_BACK / queueSEND_TO_FRONT / queueOVERWRITE
);
```

### 4.2 完整流程图

```
进入函数
  │
  ▼
┌─ for(;;) ────────────────────────────────────────────────────┐
│                                                               │
│  ▼ 进入临界区                                                  │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │  队列有空位? (uxMessagesWaiting < uxLength)               │  │
│  │    或 xCopyPosition == queueOVERWRITE?                   │  │
│  │                                                         │  │
│  │  YES ──────────────────────────────────────────────→    │  │
│  │  │ ① prvCopyDataToQueue()  把数据拷入环形缓冲区          │  │
│  │  │ ② 检查 xTasksWaitingToReceive 是否有等待者             │  │
│  │  │    有 → xTaskRemoveFromEventList() 唤醒等待者          │  │
│  │  │    唤醒的是更高优先级? → queueYIELD_IF_USING_PREEMPTION │  │
│  │  │ ③ 退出临界区，return pdPASS                           │  │
│  │                                                         │  │
│  │  NO (队列满) ──────────────────────────────────────→    │  │
│  │  │ xTicksToWait == 0?                                   │  │
│  │  │   是 → 不等待，退出临界区，return errQUEUE_FULL          │  │
│  │  │   否 → 首次? 初始化超时结构 (xTimeOut)                  │  │
│  │  │          继续循环                                      │  │
│  └─────────────────────────────────────────────────────────┘  │
│  ▼ 退出临界区                                                  │
│                                                               │
│  ▼ vTaskSuspendAll()  挂起调度器                               │
│  ▼ prvLockQueue()  锁定队列 (cTxLock/cRxLock: -1 → 0)         │
│                                                               │
│  ▼ xTaskCheckForTimeOut()  检查超时                            │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │  未超时?                                                  │  │
│  │  │ 队列仍然满?                                            │  │
│  │  │   是 → vTaskPlaceOnEventList(&xTasksWaitingToSend)    │  │
│  │  │         把自己挂到等待发送者链表                        │  │
│  │  │         prvUnlockQueue()  解锁队列                     │  │
│  │  │         xTaskResumeAll()  恢复调度                      │  │
│  │  │         如果没发生上下文切换 → portYIELD_WITHIN_API    │  │
│  │  │         (循环重新开始，队列有空位时被唤醒)               │  │
│  │  │                                                       │  │
│  │  │   否 (队列有空位了) → prvUnlockQueue(), 循环重试        │  │
│  │                                                         │  │
│  │  已超时 → prvUnlockQueue(), return errQUEUE_FULL         │  │
│  └─────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

### 4.3 关键设计点

1. **"先查再睡" 的模式**：进入临界区先检查条件是否满足，不满足才阻塞。这是防止**丢失唤醒**（lost wakeup）的标准模式。

2. **阻塞路径的顺序**：`vTaskSuspendAll()` → `prvLockQueue()` → 检查条件 → `vTaskPlaceOnEventList()` → `prvUnlockQueue()` → `xTaskResumeAll()`。这个顺序确保了在锁期间 ISR 只能通过锁计数器间接操作。

3. **双向唤醒**：发送成功后检查 `xTasksWaitingToReceive`，如果有等待接收者就唤醒它。这是生产者-消费者同步的核心。

---

## 5. `xQueueGenericSendFromISR` — 中断级发送

### 5.1 与任务级的核心区别

| 特性 | 任务级 | ISR 级 |
|---|---|---|
| 阻塞等待 | ✅ 支持 | ❌ 不支持 |
| 唤醒任务 | 直接操作就绪链表 | 通过 `pxHigherPriorityTaskWoken` 标记 |
| 锁处理 | 加锁/解锁配对 | 感知锁状态，递增计数器 |

### 5.2 关键代码逻辑

```c
uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();  // 关中断嵌套
{
    if (队列有空位)
    {
        const int8_t cTxLock = pxQueue->cTxLock;  // 读取锁状态

        prvCopyDataToQueue(...);   // 拷贝数据

        if (cTxLock == queueUNLOCKED)   // 队列未锁定 → 可以直接操作事件链表
        {
            if (xTasksWaitingToReceive 非空)
            {
                // 唤醒最高优先级的等待接收者
                if (xTaskRemoveFromEventList(...))
                    *pxHigherPriorityTaskWoken = pdTRUE;  // 标记需要上下文切换
            }
        }
        else   // 队列被锁定 → 只记录，等解锁时批量处理
        {
            pxQueue->cTxLock = (int8_t)(cTxLock + 1);
        }
        xReturn = pdPASS;
    }
    else
    {
        xReturn = errQUEUE_FULL;   // 队列满，立即返回失败
    }
}
portCLEAR_INTERRUPT_MASK_FROM_ISR(...);
```

### 5.3 `xQueueGiveFromISR` — 信号量专用

与 `xQueueGenericSendFromISR` 不同：
- **不拷贝数据**（`uxItemSize == 0`），直接 `uxMessagesWaiting++`
- **不做优先级继承**（信号量没有持有者概念）
- **断言**互斥量不能从 ISR 给出（`configASSERT(pxQueue->pxMutexHolder == NULL)`）

---

## 6. `xQueueGenericReceive` — 任务级接收

### 6.1 函数签名

```c
BaseType_t xQueueGenericReceive(
    QueueHandle_t xQueue,
    void * const pvBuffer,
    TickType_t xTicksToWait,
    const BaseType_t xJustPeeking   // pdTRUE: 只看不取; pdFALSE: 取出
);
```

### 6.2 Peek vs Take

```
                      ┌─ xJustPeeking == pdFALSE (Take) ──────────────────────────
                      │
                      │  pcReadFrom += uxItemSize  (指针先前进，表示"已读")
                      │  memcpy(pvBuffer, pcReadFrom, uxItemSize)
                      │  uxMessagesWaiting--
                      │  互斥量: 记录持有者 (pxMutexHolder = 当前任务)
                      │  检查 xTasksWaitingToSend，有空位则唤醒等待发送者
                      │
进入 ─────────────────┤
                      │
                      ├─ xJustPeeking == pdTRUE (Peek) ───────────────────────────
                      │
                      │  先记录 pcOriginalReadPosition = pcReadFrom
                      │  pcReadFrom += uxItemSize    (前进)
                      │  memcpy(pvBuffer, pcReadFrom, uxItemSize)
                      │  pcReadFrom = pcOriginalReadPosition  (恢复!)
                      │  uxMessagesWaiting 不变
                      │  检查 xTasksWaitingToReceive (如果有同优先级等待者就 yield)
                      │
                      └────────────────────────────────────────────────────────────
```

**Peek 的微妙之处**：恢复读指针后如果有同优先级的等待接收者，当前任务需要 yield，让同优先级任务也能有机会取数据。

### 6.3 互斥量接收时的优先级继承

```c
if (xJustPeeking == pdFALSE)   // 真正取走数据
{
    if (pxQueue->uxQueueType == queueQUEUE_IS_MUTEX)
    {
        // 记录当前任务为互斥量持有者
        pxQueue->pxMutexHolder = pvTaskIncrementMutexHeldCount();
    }

    // 检查等待发送者链表
    if (xTasksWaitingToSend 非空)
        xTaskRemoveFromEventList(...);   // 唤醒一个等待发送者
}
else   // Peek: 不改变所有权
{
    // 数据留在队列中，如果有其他等待接收者就 yield
    if (xTasksWaitingToReceive 非空)
        xTaskRemoveFromEventList(...);
}
```

### 6.4 阻塞接收时的优先级继承

```c
// 在阻塞等待之前...
if (pxQueue->uxQueueType == queueQUEUE_IS_MUTEX)
{
    taskENTER_CRITICAL();
    {
        // 关键! 提升当前互斥量持有者的优先级
        vTaskPriorityInherit(pxQueue->pxMutexHolder);
    }
    taskEXIT_CRITICAL();
}
// 然后把自己挂到等待接收链表
vTaskPlaceOnEventList(&pxQueue->xTasksWaitingToReceive, xTicksToWait);
```

**这是优先级继承的核心时机**：当高优先级任务等待互斥量时，提升当前持有者的优先级，防止优先级反转。

---

## 7. `xQueueReceiveFromISR` — 中断级接收

结构与 `xQueueGenericSendFromISR` 对称：
- 不阻塞，队列空时立即返回 `pdFAIL`
- 锁感知：`cRxLock == queueUNLOCKED` 时直接操作等待链表，否则递增 `cRxLock`
- 通过 `pxHigherPriorityTaskWoken` 传递唤醒标记

---

## 8. `prvCopyDataToQueue` — 数据拷贝核心

```c
static BaseType_t prvCopyDataToQueue(
    Queue_t * const pxQueue,
    const void *pvItemToQueue,
    const BaseType_t xPosition   // queueSEND_TO_BACK / queueSEND_TO_FRONT / queueOVERWRITE
);
```

### 8.1 三条路径

```
                    ┌─ uxItemSize == 0 (信号量/互斥量) ──────────────────
                    │  不拷数据
                    │  互斥量: xTaskPriorityDisinherit() 解除优先级继承
                    │          pxMutexHolder = NULL
                    │  返回: 是否发生了优先级反继承
                    │
  prvCopyDataToQueue ┤
                    ├─ xPosition == queueSEND_TO_BACK (入队尾) ─────────
                    │  memcpy(pcWriteTo, pvItemToQueue, uxItemSize)
                    │  pcWriteTo += uxItemSize
                    │  if (pcWriteTo >= pcTail) pcWriteTo = pcHead
                    │
                    ├─ xPosition == queueSEND_TO_FRONT (入队首) ─────────
                    │  memcpy(u.pcReadFrom, pvItemToQueue, uxItemSize)
                    │  u.pcReadFrom -= uxItemSize
                    │  if (u.pcReadFrom < pcHead) u.pcReadFrom = pcTail - uxItemSize
                    │
                    └─ xPosition == queueOVERWRITE (覆盖队首) ──────────
                       与 queueSEND_TO_FRONT 相同，但:
                       if (uxMessagesWaiting > 0) uxMessagesWaiting--;
                       (因为覆盖了已有数据，计数不变，之后统一++)
```

### 8.2 互斥量释放的特殊处理

当 `uxItemSize == 0` 且队列类型是互斥量时，`prvCopyDataToQueue` 调用 `xTaskPriorityDisinherit()`：

```c
// 互斥量的 "发送" = 释放互斥量
xReturn = xTaskPriorityDisinherit(pxQueue->pxMutexHolder);
pxQueue->pxMutexHolder = NULL;
```

`xReturn` 指示是否发生了优先级降级（disinherit），如果是且没有等待接收者，调用者 `xQueueGenericSend` 会执行 `queueYIELD_IF_USING_PREEMPTION()`——因为释放互斥量后，当前任务的优先级降低了，应该让出 CPU。

---

## 9. 递归互斥量

### 9.1 `xQueueTakeMutexRecursive`

```c
BaseType_t xQueueTakeMutexRecursive(QueueHandle_t xMutex, TickType_t xTicksToWait)
{
    if (pxMutex->pxMutexHolder == 当前任务)
    {
        // 已经持有 → 直接增加递归计数
        pxMutex->u.uxRecursiveCallCount++;
        return pdPASS;
    }
    else
    {
        // 尚未持有 → 走标准接收流程（可能阻塞等待）
        xReturn = xQueueGenericReceive(pxMutex, NULL, xTicksToWait, pdFALSE);
        if (xReturn != pdFAIL)
            pxMutex->u.uxRecursiveCallCount++;   // 首次获取，计数 = 1
    }
}
```

### 9.2 `xQueueGiveMutexRecursive`

```c
BaseType_t xQueueGiveMutexRecursive(QueueHandle_t xMutex)
{
    if (pxMutex->pxMutexHolder == 当前任务)   // 必须持有者才能释放
    {
        pxMutex->u.uxRecursiveCallCount--;
        if (pxMutex->u.uxRecursiveCallCount == 0)   // 完全释放
        {
            xQueueGenericSend(pxMutex, NULL, queueMUTEX_GIVE_BLOCK_TIME, queueSEND_TO_BACK);
            // ↑ 这会调用 xTaskPriorityDisinherit() + 唤醒等待者
        }
        return pdPASS;
    }
    else
    {
        return pdFAIL;  // 不是持有者，不能释放
    }
}
```

**递归互斥量的关键语义**：
- 同一任务可以多次 Take（每次 `uxRecursiveCallCount++`）
- 必须 Give 相同次数才能真释放（`uxRecursiveCallCount` 归零时调 `xQueueGenericSend`）
- 只有持有者才能 Give，否则返回失败

---

## 10. 多对象复用统一模型

`Queue_t` 复用于五种内核对象的完整行为矩阵：

| 操作 | 普通队列 | 二值信号量 | 计数信号量 | 互斥量 | 递归互斥量 |
|---|---|---|---|---|---|
| **Send/Give** | 拷贝数据 | `uxMessagesWaiting++` | `uxMessagesWaiting++` | 释放 + 优先级反继承 | 递归计数-- |
| **Receive/Take** | 取出数据 | `uxMessagesWaiting--` | `uxMessagesWaiting--` | 取走 + 记录持有者 + 优先级继承 | 递归计数++ |
| **uxItemSize** | > 0 | 0 | 0 | 0 | 0 |
| **pcHead** | 指向存储区 | 指向自身 | 指向自身 | `NULL` | `NULL` |
| **u 联合体** | `pcReadFrom` | `pcReadFrom` | `pcReadFrom` | `uxRecursiveCallCount` | `uxRecursiveCallCount` |
| **从 ISR 操作** | ✅ Send/Receive | ✅ Give/Take | ✅ Give/Take | ❌ 不能 Give | ❌ 不能操作 |
| **优先级继承** | ❌ | ❌ | ❌ | ✅ | ✅ |

---

## 11. 同步模型总结

```
                    ┌──────────────────────────────────────────┐
                    │          生产者-消费者同步模型              │
                    │                                          │
                    │   Send 端:                                │
                    │   · 队列满 → 挂到 xTasksWaitingToSend      │
                    │   · 队列不满 → 拷贝数据，检查 Receive 等待者  │
                    │                                          │
                    │   Receive 端:                             │
                    │   · 队列空 → 挂到 xTasksWaitingToReceive   │
                    │   · 队列不空 → 取出数据，检查 Send 等待者    │
                    │                                          │
                    │   ISR 端:                                 │
                    │   · 绝不阻塞，立即返回成功/失败              │
                    │   · 通过 cTxLock/cRxLock 与任务端协调      │
                    │   · 通过 pxHigherPriorityTaskWoken 通知调度 │
                    └──────────────────────────────────────────┘
```

---

## 12. 相关文件索引

| 文件 | 内容 |
|---|---|
| [Source/queue.c](../Source/queue.c) | 队列全部操作实现（~2400行） |
| [Source/include/queue.h](../Source/include/queue.h) | 队列对外 API 声明 |
| [Source/include/list.h](../Source/include/list.h) | `List_t` / `ListItem_t` 定义 |
| [Source/list.c](../Source/list.c) | 链表操作实现 |
| [Source/tasks.c](../Source/tasks.c) | TCB 定义、任务调度、优先级继承实现 |

> 前序文档：[FreeRTOS_List_t_Analysis.md](FreeRTOS_List_t_Analysis.md) — 链表结构深度分析
> 前序文档：[FreeRTOS_Queue_t_Analysis.md](FreeRTOS_Queue_t_Analysis.md) — 队列/互斥量/信号量结构复用模型的详细分析
