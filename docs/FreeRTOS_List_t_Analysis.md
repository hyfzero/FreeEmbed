# FreeRTOS `List_t` 深度分析

> 本文档分析 FreeRTOS v9.0.0 调度器核心数据结构 `List_t`，包括其结构定义、成员字段、实现算法、内存设计以及在调度器和 Queue_t 中的应用。

---

## 1. 概述

`List_t` 是 FreeRTOS 调度器最底层的数据结构，本质是一个**双向循环链表**（严格说是一个**有哨兵节点的双向循环链表/环形链表—— circular doubly-linked list with sentinel node**）。它定义在 [`Source/include/list.h:205-212`](../Source/include/list.h#L205-L212)，实现在 [`Source/list.c`](../Source/list.c)。

它的设计目标：
- **极致的高效**：删除操作 O(1)（不需要遍历）
- **按值降序排列**：高优先级的任务（数值大的）排在链表的头部
- **RAM 极致节省**：哨兵节点用 `MiniListItem_t` 而非 `ListItem_t`，节省 `pvOwner` 和 `pvContainer` 两个指针

---

## 2. 核心结构体定义

### 2.1 `ListItem_t` — 链表节点（[list.h:181-191](../Source/include/list.h#L181-L191)）

```c
struct xLIST_ITEM
{
    listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE   /* 可选完整性校验值 */
    configLIST_VOLATILE TickType_t xItemValue;   /* 排序用的值（降序排列） */
    struct xLIST_ITEM * configLIST_VOLATILE pxNext;     /* 后继指针 */
    struct xLIST_ITEM * configLIST_VOLATILE pxPrevious; /* 前驱指针 */
    void * pvOwner;                  /* 拥有此节点的对象（通常是 TCB） */
    void * configLIST_VOLATILE pvContainer;     /* 所在链表的 List_t 指针 */
    listSECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE  /* 可选完整性校验值 */
};
typedef struct xLIST_ITEM ListItem_t;
```

**关键字段说明：**

| 字段 | 说明 |
|---|---|
| `xItemValue` | 排序依据值。通常存储**任务优先级**（就绪链表）或**唤醒时间**（延时链表）。降序排列。 |
| `pxNext` / `pxPrevious` | 标准双向链表指针 |
| `pvOwner` | **双向链接**的关键——指向包含此 `ListItem_t` 的对象（通常是 TCB）。通过链表遍历时可直接拿到 TCB 指针。 |
| `pvContainer` | 指向所属的 `List_t`。实现 O(1) 删除：删除时不需要知道是哪个链表，直接从 `pvContainer` 回溯。 |

### 2.2 `MiniListItem_t` — 迷你节点（[list.h:193-200](../Source/include/list.h#L193-L200)）

```c
struct xMINI_LIST_ITEM
{
    listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE
    configLIST_VOLATILE TickType_t xItemValue;
    struct xLIST_ITEM * configLIST_VOLATILE pxNext;
    struct xLIST_ITEM * configLIST_VOLATILE pxPrevious;
};
typedef struct xMINI_LIST_ITEM MiniListItem_t;
```

`MiniListItem_t` 是 `ListItem_t` 的**精简版**——去掉了 `pvOwner` 和 `pvContainer`。它**仅用于 `List_t.xListEnd`** 哨兵节点，因为哨兵不需要被谁"拥有"，也不需要知道自己在哪个链表里。这一设计**节省了 8 字节（32位）或 16 字节（64位）的 RAM**。

### 2.3 `List_t` — 链表本身（[list.h:205-212](../Source/include/list.h#L205-L212)）

```c
typedef struct xLIST
{
    listFIRST_LIST_INTEGRITY_CHECK_VALUE
    configLIST_VOLATILE UBaseType_t uxNumberOfItems;  /* 链表中的节点数量（不含哨兵） */
    ListItem_t * configLIST_VOLATILE pxIndex;          /* 遍历游标，指向最近一次被 listGET_OWNER_OF_NEXT_ENTRY 返回的节点 */
    MiniListItem_t xListEnd;                           /* 哨兵节点，xItemValue 固定为 portMAX_DELAY */
    listSECOND_LIST_INTEGRITY_CHECK_VALUE
} List_t;
```

**关键字段说明：**

| 字段 | 类型 | 说明 |
|---|---|---|
| `uxNumberOfItems` | `volatile UBaseType_t` | 当前链表中的有效节点数（不含哨兵 `xListEnd`） |
| `pxIndex` | `volatile ListItem_t*` | 链表遍历游标。每次调用 `listGET_OWNER_OF_NEXT_ENTRY` 宏时，`pxIndex` 向前移动一步。这是实现**时间片轮转（Round-Robin）**的核心。 |
| `xListEnd` | `MiniListItem_t` | 哨兵节点。`xItemValue` 初始化为 `portMAX_DELAY`（即 `0xFFFFFFFF`），确保它永远是链表中"值最大"的节点，始终在逻辑末尾。 |

---

## 3. 哨兵节点设计

哨兵 `xListEnd` 是整个链表设计的精髓：

```
                    HEAD (最高优先级/最大值)
                    ┌──────────┐
                    │ Item A   │  xItemValue = 5
                    │ pxNext ──┼──→ ...
                    │ pxPrev ──┼──→ xListEnd (哨兵)
                    └──────────┘
                         ↑
                         │
                    ┌──────────┐
                    │ Item C   │  xItemValue = 2
                    │ pxNext ──┼──→ xListEnd
                    │ pxPrev ──┼──→ Item B
                    └──────────┘
                         ↑
                    ┌──────────┐
                    │ Item B   │  xItemValue = 3
                    │ pxNext ──┼──→ Item C
                    │ pxPrev ──┼──→ Item A
                    └──────────┘
                         ↑
                         │
                    ╔══════════╗
                    ║ xListEnd ║  xItemValue = portMAX_DELAY (永远最大)
                    ║ pxNext ──╫──→ Item A (真正头部)
                    ║ pxPrev ──╫──→ Item C (真正尾部)
                    ╚══════════╝
                    哨兵节点（MiniListItem_t）
```

**关键特性：**

1. **`xListEnd.pxNext`** 总是指向链表的**真正头部**（`xItemValue` 最大的有效节点）
2. **`xListEnd.pxPrevious`** 总是指向链表的**真正尾部**（`xItemValue` 最小的有效节点）
3. **空链表时**：`xListEnd.pxNext == xListEnd.pxPrevious == &xListEnd`（哨兵自环）
4. **`xListEnd.xItemValue = portMAX_DELAY`**：确保它永远在排序链表的末尾（降序排列，最大值在最后）

---

## 4. 排序规则（`vListInsert`）

`vListInsert` 按 `xItemValue` **降序**插入新节点（[list.c:145-209](../Source/list.c#L145-L209)）。

### 核心插入算法

```c
void vListInsert( List_t * const pxList, ListItem_t * const pxNewListItem )
{
    ListItem_t *pxIterator;
    const TickType_t xValueOfInsertion = pxNewListItem->xItemValue;

    if( xValueOfInsertion == portMAX_DELAY )
    {
        // 特殊情况：值 == portMAX_DELAY 时，直接插到哨兵之前
        pxIterator = pxList->xListEnd.pxPrevious;
    }
    else
    {
        // 遍历链表，找到第一个 pxNext->xItemValue <= xValueOfInsertion 的位置
        for( pxIterator = (ListItem_t *)&(pxList->xListEnd);
             pxIterator->pxNext->xItemValue <= xValueOfInsertion;
             pxIterator = pxIterator->pxNext )
        {
            // 继续前进
        }
    }

    // 标准双向链表插入
    pxNewListItem->pxNext = pxIterator->pxNext;
    pxNewListItem->pxNext->pxPrevious = pxNewListItem;
    pxNewListItem->pxPrevious = pxIterator;
    pxIterator->pxNext = pxNewListItem;

    pxNewListItem->pvContainer = (void *)pxList;
    (pxList->uxNumberOfItems)++;
}
```

### 同值处理：实现公平调度

**关键设计**：当插入值与已有节点值相同时，新节点插入到**已有同值节点之后**（更靠近哨兵）。这保证了：

- 就绪链表中，**同优先级任务按照 FIFO 顺序轮转**
- 延时链表中，**同唤醒时间的任务按先后顺序依次唤醒**

### 遍历起始点：为什么从 `xListEnd` 开始？

从哨兵 `xListEnd` 开始遍历，`pxIterator->pxNext` 的初始值就是链表头部（最大值），符合降序遍历的逻辑。因为哨兵的 `xItemValue = portMAX_DELAY` 是最大值，位于末尾。

---

## 5. 核心操作 API

### 5.1 初始化

| 函数 | 位置 | 说明 |
|---|---|---|
| `vListInitialise(pxList)` | [list.c:79-101](../Source/list.c#L79-L101) | 初始化链表。设置 `pxIndex` 指向 `xListEnd`，`xListEnd.xItemValue = portMAX_DELAY`，哨兵自环 |
| `vListInitialiseItem(pxItem)` | [list.c:104-113](../Source/list.c#L104-L113) | 初始化节点。将 `pvContainer = NULL` |

### 5.2 插入

| 函数/宏 | 说明 |
|---|---|
| `vListInsert(pxList, pxItem)` | **按值降序插入**。用于就绪链表、事件等待链表、延时链表 |
| `vListInsertEnd(pxList, pxItem)` | **插到 `pxIndex` 指向的位置之前**。用于随机插入（不关心排序） |

`vListInsertEnd` 插入到 `pxIndex->pxPrevious` 和 `pxIndex` 之间，新节点将是**最后一个**被 `listGET_OWNER_OF_NEXT_ENTRY` 遍历到的节点。

### 5.3 删除 — O(1) 的关键

```c
UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove )
{
    List_t * const pxList = (List_t *)pxItemToRemove->pvContainer;
    // 从双向链表中摘除
    pxItemToRemove->pxNext->pxPrevious = pxItemToRemove->pxPrevious;
    pxItemToRemove->pxPrevious->pxNext = pxItemToRemove->pxNext;
    // 如果 pxIndex 正好指向被删除的节点，前移一位
    if( pxList->pxIndex == pxItemToRemove )
        pxList->pxIndex = pxItemToRemove->pxPrevious;
    pxItemToRemove->pvContainer = NULL;
    (pxList->uxNumberOfItems)--;
    return pxList->uxNumberOfItems;
}
```

**O(1) 的秘诀**：因为每个 `ListItem_t` 通过 `pvContainer` 知道自己属于哪个链表，删除时不需要遍历搜索。这在调度器频繁的"把任务从阻塞链表移到就绪链表"操作中至关重要。

### 5.4 遍历（时间片轮转的核心）

```c
#define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )
{
    List_t * const pxConstList = ( pxList );
    // pxIndex 前进一位
    ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;
    // 如果正好是哨兵，再跳一步（跳过哨兵回到头部）
    if( ( void * )( pxConstList )->pxIndex == ( void * )&( ( pxConstList )->xListEnd ) )
    {
        ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;
    }
    // 返回当前节点拥有者（TCB 指针）
    ( pxTCB ) = ( pxConstList )->pxIndex->pvOwner;
}
```

这就是 FreeRTOS **同优先级时间片轮转调度**的核心机制：每次调用此宏，`pxIndex` 沿链表步进一步，跳过哨兵实现循环。如果一个优先级上有多任务，它们会被轮流选中。

### 5.5 判断与查询宏

| 宏 | 说明 |
|---|---|
| `listLIST_IS_EMPTY(pxList)` | `uxNumberOfItems == 0`？ |
| `listCURRENT_LIST_LENGTH(pxList)` | 返回 `uxNumberOfItems` |
| `listGET_HEAD_ENTRY(pxList)` | 返回 `xListEnd.pxNext`（即链表头部节点） |
| `listGET_ITEM_VALUE_OF_HEAD_ENTRY(pxList)` | 返回头部节点的 `xItemValue` |
| `listGET_OWNER_OF_HEAD_ENTRY(pxList)` | 返回头部节点的 `pvOwner` |
| `listIS_CONTAINED_WITHIN(pxList, pxItem)` | 判断某节点是否在指定链表中 |
| `listLIST_IS_INITIALISED(pxList)` | `xListEnd.xItemValue == portMAX_DELAY`？ |

---

## 6. RAM 优化：`MiniListItem_t`

`ListItem_t` vs `MiniListItem_t` 的内存对比（32位架构）：

| 结构体 | 成员 | 大小 |
|---|---|---|
| `ListItem_t` | `xItemValue(4) + pxNext(4) + pxPrevious(4) + pvOwner(4) + pvContainer(4)` | **20 字节** |
| `MiniListItem_t` | `xItemValue(4) + pxNext(4) + pxPrevious(4)` | **12 字节** |

哨兵用 `MiniListItem_t` 省掉了 `pvOwner` 和 `pvContainer`，因为哨兵永远不需要通过 `pvOwner` 找所属对象、也不需要反向查找链表。每个 `List_t` 能节省 **8 字节**（32位）或 **16 字节**（64位）。

对于一个有数十个链表的中等规模系统（就绪链表 N 条 + 延时链表 2 条 + 每个队列 2 条…），这个差距会累积到数百字节——在资源受限的 MCU 上非常可观。

---

## 7. 在 Queue_t 中的应用

回到 [`Queue_t`](../Source/queue.c#L130-L165) 结构体：

```c
typedef struct QueueDefinition
{
    // ...
    List_t xTasksWaitingToSend;     // 等待发送到队列而被阻塞的任务链表
    List_t xTasksWaitingToReceive;  // 等待从队列接收而被阻塞的任务链表
    // ...
} Queue_t;
```

### 7.1 `xTasksWaitingToSend`

当任务调用 `xQueueSend()` 而队列**已满**时，该任务从就绪链表移除，其 TCB 中的 `xEventListItem` 被插入到此链表。

- **排序方式**：按**任务优先级**降序排列（`xItemValue = uxPriority`）
- **作用**：当队列有空位时，**高优先级等待者优先**获得写入权
- **互斥量场景**：该链表用于实现**优先级继承**——当高优先级任务在此等待时，持有互斥量的低优先级任务会被临时提升优先级

### 7.2 `xTasksWaitingToReceive`

当任务调用 `xQueueReceive()` 而队列**为空**时，该任务的 `xEventListItem` 被插入到此链表。

- **排序方式**：按**任务优先级**降序排列
- **作用**：当有新数据入队时，**高优先级等待者优先**获得数据

### 7.3 任务与链表的双向链接

```
    ┌──────────────────────┐           ┌─────────────────────┐
    │   TCB (任务控制块)    │           │   List_t (链表)      │
    │                      │           │                     │
    │   xStateListItem ────┼──→ 嵌入 →─┤   就绪链表 或        │
    │   xEventListItem ────┼──→ 嵌入 →─┤   xTasksWaitingToXXX │
    │   ...                │           │                     │
    └──────────────────────┘           └─────────────────────┘
              ↑                                  │
              │     pvOwner (指向 TCB)            │
              └──────────────────────────────────┘
              │     pvContainer (指向 List_t)     │
              └──────────────────────────────────┘
```

**双链设计的关键**：
- `pvOwner`：遍历链表时能直接拿到 TCB 指针，不需要 `container_of` 宏偏移计算
- `pvContainer`：删除节点时无需知道是哪个链表，直接从节点自身找到并操作，实现 O(1) 移除

### 7.4 队列操作中的链表生命周期

以一次 `xQueueSend` (队列已满的场景) 为例：

```
1. xQueueSend(队列已满)
   │
2. vListInsert(xTasksWaitingToSend, &(pxCurrentTCB->xEventListItem))
   │  将当前任务的 xEventListItem 插入等待发送链表（按优先级降序）
   │
3. vListRemove(&(pxCurrentTCB->xStateListItem))
   │  从就绪链表移除（任务进入 Blocked 状态）
   │
4. 任务调度 → 切换到其他任务执行
   │
5. ... 队列有空位 ...
   │
6. 从 xTasksWaitingToSend 头部取出最高优先级等待者
   │
7. uxListRemove(xEventListItem)  ← O(1) 操作
   │
8. 任务重新加入就绪链表
```

---

## 8. 完整链表状态示意图

### 空链表
```
 ╔══════════╗
 ║ xListEnd ║  pxNext ────→ self
 ║          ║  pxPrev ────→ self
 ╚══════════╝
 uxNumberOfItems = 0
 pxIndex → xListEnd
```

### 含3个元素的链表（按 xItemValue 降序）

```
 HEAD ←──────────────────────────────────→ TAIL
 ┌─────────┐    ┌─────────┐    ┌─────────┐    ╔══════════╗
 │Task Pri5│→   │Task Pri3│→   │Task Pri2│→   ║ xListEnd ║
 │xValue=5 │    │xValue=3 │    │xValue=2 │    ║xValue=MAX║
 │         │←   │         │←   │         │←   ║          ║
 └─────────┘    └─────────┘    └─────────┘    ╚══════════╝
 uxNumberOfItems = 3
 pxIndex → (取决于上次遍历位置)

 listGET_HEAD_ENTRY() → Task Pri5 (最高优先级/最大值)
 listGET_OWNER_OF_HEAD_ENTRY() → Task Pri5 的 TCB
```

---

## 9. 设计总结

| 设计特点 | 实现方式 | 收益 |
|---|---|---|
| **O(1) 删除** | `ListItem_t.pvContainer` 反向指向所在链表 | 调度器核心操作极致高效 |
| **双向链接** | `ListItem_t.pvOwner` 指向拥有者 TCB | 遍历时可立即获取 TCB，不需要偏移计算 |
| **哨兵节点** | `MiniListItem_t xListEnd`，`xItemValue = portMAX_DELAY` | 消除首尾判断特殊情况，统一插入/删除逻辑 |
| **RAM 优化** | 哨兵用 `MiniListItem_t`（省 8/16 字节） | 每链表节省内存，嵌入系统非常关键 |
| **降序排列** | `vListInsert` 按 `xItemValue` 降序 | 高优先级任务天然在链表头部，取最高优先级 O(1) |
| **同值 FIFO** | 同值插在已有节点之后 | 同优先级公平轮转调度 |
| **时间片遍历** | `pxIndex` 游标 + `listGET_OWNER_OF_NEXT_ENTRY` | 同优先级 Round-Robin 的实现基础 |

---

## 10. 相关文件索引

| 文件 | 内容 |
|---|---|
| [Source/include/list.h](../Source/include/list.h) | `ListItem_t`、`MiniListItem_t`、`List_t` 结构体定义 + 全部操作宏 |
| [Source/list.c](../Source/list.c) | `vListInitialise`、`vListInsert`、`vListInsertEnd`、`uxListRemove` 实现 |
| [Source/queue.c](../Source/queue.c) | `Queue_t` 中包含两个 `List_t` 成员 (`xTasksWaitingToSend` / `xTasksWaitingToReceive`) |
| [Source/tasks.c:293-367](../Source/tasks.c#L293-L367) | TCB 定义，包含 `xStateListItem` 和 `xEventListItem` 两个 `ListItem_t` |
| [Source/include/queue.h](../Source/include/queue.h) | 队列对外 API 声明 |
