# Module 09: RTOS Concepts

## Overview

A **Real-Time Operating System (RTOS)** provides deterministic scheduling,
inter-task communication, and resource management for embedded systems. Unlike
general-purpose operating systems that optimize for throughput, an RTOS
guarantees that critical tasks meet their deadlines. This module covers
FreeRTOS -- the most widely deployed RTOS in microcontroller-based systems.

---

## Table of Contents

1. [Why Use an RTOS?](#1-why-use-an-rtos)
2. [FreeRTOS Architecture](#2-freertos-architecture)
3. [Tasks](#3-tasks)
4. [Scheduling](#4-scheduling)
5. [Queues](#5-queues)
6. [Semaphores](#6-semaphores)
7. [Mutexes](#7-mutexes)
8. [Event Groups](#8-event-groups)
9. [Task Notifications](#9-task-notifications)
10. [Software Timers](#10-software-timers)
11. [Memory Management](#11-memory-management)
12. [Common Pitfalls](#12-common-pitfalls)
13. [Best Practices](#13-best-practices)

---

## 1. Why Use an RTOS?

### Bare-Metal vs. RTOS

In a bare-metal (super-loop) design, the main loop polls every subsystem
sequentially:

```c
while (1) {
    read_sensors();
    process_data();
    update_display();
    handle_communication();
}
```

**Problems with bare-metal:**

| Problem | Description |
|---------|-------------|
| Poor responsiveness | A long-running function blocks everything else |
| Timing fragility | Adding code changes the timing of every other operation |
| Complexity scaling | State machines become unwieldy as features grow |
| Hard to maintain | Tight coupling between unrelated subsystems |

**Benefits of an RTOS:**

| Benefit | Description |
|---------|-------------|
| Deterministic timing | Tasks run based on priority and deadlines |
| Modularity | Each task is an independent unit of execution |
| Scalability | Adding a new task does not affect existing task timing |
| Resource sharing | Built-in primitives (mutexes, semaphores) for safe sharing |
| Power management | Idle task and tickless mode reduce power consumption |

### Hard Real-Time vs. Soft Real-Time

- **Hard real-time**: Missing a deadline is a system failure (e.g., airbag
  deployment, motor control). The RTOS guarantees worst-case response time.
- **Soft real-time**: Missing a deadline degrades quality but is not
  catastrophic (e.g., audio streaming, display updates).
- **Firm real-time**: A late result has no value, but missing it does not cause
  failure (e.g., video frame rendering).

---

## 2. FreeRTOS Architecture

FreeRTOS is a preemptive, priority-based RTOS kernel. Its core components are:

```
+------------------------------------------------------+
|                   Application Tasks                   |
+------------------------------------------------------+
|  Queues | Semaphores | Mutexes | Event Groups | Timers|
+------------------------------------------------------+
|                   FreeRTOS Kernel                      |
|  Scheduler | Context Switch | Tick Interrupt          |
+------------------------------------------------------+
|              Hardware Abstraction (Port)               |
+------------------------------------------------------+
|                  MCU Hardware                          |
+------------------------------------------------------+
```

### Key Configuration: FreeRTOSConfig.h

Every FreeRTOS project requires a `FreeRTOSConfig.h` file that controls kernel
behavior:

```c
#define configUSE_PREEMPTION            1
#define configCPU_CLOCK_HZ              64000000
#define configTICK_RATE_HZ              1000        /* 1 ms tick */
#define configMAX_PRIORITIES            5
#define configMINIMAL_STACK_SIZE        128         /* words */
#define configTOTAL_HEAP_SIZE           (8 * 1024)  /* bytes */
#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_TIMERS                1
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configCHECK_FOR_STACK_OVERFLOW  2
```

---

## 3. Tasks

A **task** is an independent thread of execution with its own stack and
priority. Each task is implemented as a function that never returns.

### Task States

```
                    +------------------+
        +---------+|     Running      |+---------+
        |          +------------------+          |
        |            |            ^               |
  Blocked/          |            |          Preempted
  Suspended    Yield/Block   Scheduled          |
        |            |            |               |
        v            v            |               |
+------------------+   +------------------+      |
|    Blocked       |   |      Ready       |<-----+
|  (waiting for    |   |  (waiting to     |
|   event/timeout) |   |   be scheduled)  |
+------------------+   +------------------+
        |                      ^
        |     Event occurs     |
        +----------------------+

+------------------+
|    Suspended     |  (explicitly suspended via API)
+------------------+
```

- **Ready**: Task is eligible to run but a higher-priority task is running.
- **Running**: Task is currently executing on the CPU.
- **Blocked**: Task is waiting for an event (queue data, semaphore, delay).
- **Suspended**: Task has been explicitly suspended via `vTaskSuspend()`.

### Task Creation

```c
BaseType_t xTaskCreate(
    TaskFunction_t pvTaskCode,      /* Function pointer */
    const char *pcName,             /* Human-readable name */
    configSTACK_DEPTH_TYPE usStackDepth, /* Stack size in words */
    void *pvParameters,             /* Parameter passed to task */
    UBaseType_t uxPriority,         /* Priority (0 = lowest) */
    TaskHandle_t *pxCreatedTask     /* Output: task handle */
);
```

### Task Lifecycle

```c
void vMyTask(void *pvParameters)
{
    /* One-time initialization */
    sensor_init();

    for (;;) {
        /* Periodic work */
        int value = sensor_read();
        process(value);

        /* Block until next period */
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* Tasks should never return. If they do, delete: */
    vTaskDelete(NULL);
}
```

### Stack Sizing

Each task has its own stack. Stack overflow is a common source of crashes.

**Rules of thumb:**

- Start with 256 words (1 KB on 32-bit MCU) for simple tasks.
- Add stack for: deep call chains, large local variables, printf/sprintf,
  floating-point context.
- Use `uxTaskGetStackHighWaterMark()` to check actual usage.
- Set `configCHECK_FOR_STACK_OVERFLOW` to 2 during development.

---

## 4. Scheduling

### Priority-Based Preemptive Scheduling

FreeRTOS uses **fixed-priority preemptive scheduling**:

1. The highest-priority ready task always runs.
2. If a higher-priority task becomes ready, it immediately preempts the current
   task.
3. Equal-priority tasks share CPU time via **round-robin** (time slicing).

```
Priority 3: ████░░░░░░████░░░░░░████   (Highest - sensor reading)
Priority 2: ░░░░████░░░░░░████░░░░░░   (Processing)
Priority 1: ░░░░░░░░██░░░░░░░░██░░░░   (Display)
Priority 0: ░░░░░░░░░░░░░░░░░░░░░░██   (Idle task)
             ──────── Time ──────────>
```

### The Tick Interrupt

The **tick interrupt** fires at `configTICK_RATE_HZ` (typically 1000 Hz = 1 ms
period). On each tick, the scheduler:

1. Increments the tick count.
2. Checks if any blocked tasks should be unblocked.
3. Performs time-slicing for equal-priority tasks.

### Context Switching

A context switch saves the current task's CPU registers and stack pointer, then
restores the next task's context. On Cortex-M, this uses the PendSV exception.

**Cost**: Typically 5-20 microseconds on a Cortex-M4 at 64 MHz.

### Priority Inversion

**Priority inversion** occurs when a high-priority task is blocked by a
low-priority task that holds a shared resource, while a medium-priority task
runs instead.

```
Timeline:
  High:   ──Run──Block(mutex)──────────────────Run──
  Medium: ──────────────────Run──Run──Run──────────
  Low:    ────────────Run(holds mutex)──────Release─

  Problem: Medium preempts Low, preventing Low from releasing
           the mutex, so High is blocked indefinitely.
```

**Solution: Priority Inheritance**

FreeRTOS mutexes implement **priority inheritance**: when a high-priority task
blocks on a mutex held by a low-priority task, the low-priority task's
priority is temporarily raised to match the high-priority task. This prevents
medium-priority tasks from preempting.

---

## 5. Queues

A **queue** is a FIFO buffer for passing data between tasks (or between ISRs
and tasks). Queues are the primary mechanism for inter-task communication.

### Queue Operations

```c
/* Create a queue */
QueueHandle_t xQueueCreate(UBaseType_t uxLength, UBaseType_t uxItemSize);

/* Send data to queue (copy semantics) */
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue,
                      TickType_t xTicksToWait);

/* Receive data from queue */
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer,
                         TickType_t xTicksToWait);

/* ISR-safe variants */
BaseType_t xQueueSendFromISR(QueueHandle_t xQueue, const void *pvItemToQueue,
                              BaseType_t *pxHigherPriorityTaskWoken);
```

### Queue Behavior

- **Full queue**: Sending task blocks (up to timeout) until space is available.
- **Empty queue**: Receiving task blocks (up to timeout) until data is available.
- **Copy semantics**: Data is copied into and out of the queue -- the queue
  owns a copy of the data.
- **Thread-safe**: All queue operations are interrupt-safe when using the
  `FromISR` variants.

### Queue Use Cases

| Pattern | Description |
|---------|-------------|
| Producer-Consumer | One task produces data, another consumes it |
| Command Queue | Tasks send commands/requests to a handler task |
| Data Pipeline | Chain of tasks, each processing and forwarding data |
| Event Reporting | ISRs post events to a task for deferred processing |

---

## 6. Semaphores

Semaphores are signaling mechanisms. They come in two flavors:

### Binary Semaphore

Acts as a flag (0 or 1). Used for **synchronization**, especially ISR-to-task
notification.

```c
SemaphoreHandle_t xSemaphoreCreateBinary(void);

/* ISR gives (signals) the semaphore */
xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);

/* Task takes (waits for) the semaphore */
xSemaphoreTake(xSemaphore, portMAX_DELAY);
```

### Counting Semaphore

Tracks multiple events or manages a pool of resources.

```c
SemaphoreHandle_t xSemaphoreCreateCounting(
    UBaseType_t uxMaxCount,     /* Maximum count value */
    UBaseType_t uxInitialCount  /* Initial count value */
);
```

**Use cases:**

- Counting events that arrive faster than they are processed.
- Managing a pool of N identical resources (e.g., DMA channels, buffer slots).

### Binary Semaphore vs. Mutex

| Feature | Binary Semaphore | Mutex |
|---------|-----------------|-------|
| Purpose | Synchronization / signaling | Mutual exclusion |
| Ownership | No owner | Owned by the task that took it |
| Priority inheritance | No | Yes |
| Can be given from ISR | Yes | No |
| Recursive taking | No | Yes (if recursive mutex) |

---

## 7. Mutexes

A **mutex** (mutual exclusion) protects shared resources so that only one task
can access the resource at a time.

### Mutex Operations

```c
SemaphoreHandle_t xSemaphoreCreateMutex(void);

/* Acquire the mutex */
xSemaphoreTake(xMutex, portMAX_DELAY);

/* Access shared resource */
shared_resource_write(data);

/* Release the mutex */
xSemaphoreGive(xMutex);
```

### Critical Section Guidelines

1. **Keep critical sections short.** Only protect the minimum code needed.
2. **Never block inside a critical section.** Do not call `vTaskDelay()` or
   blocking queue operations while holding a mutex.
3. **Always release the mutex.** Every `Take` must have a matching `Give`.
4. **Avoid nested mutexes.** If you must use multiple mutexes, always acquire
   them in the same order to prevent deadlock.

### Deadlock

**Deadlock** occurs when two or more tasks are each waiting for a resource held
by the other:

```
Task A: Takes Mutex1 ... waits for Mutex2
Task B: Takes Mutex2 ... waits for Mutex1
         --> Neither can proceed.
```

**Prevention strategies:**

- Always acquire mutexes in the same global order.
- Use timeouts on `xSemaphoreTake()` and handle failure.
- Minimize the number of mutexes in the system.
- Use a single mutex to protect a group of related resources.

---

## 8. Event Groups

**Event groups** allow a task to wait for a combination of events (bits). Each
event group contains a configurable number of bits (typically 8 or 24).

```c
EventGroupHandle_t xEventGroupCreate(void);

/* Set bits (signal events) */
xEventGroupSetBits(xEventGroup, SENSOR_READY_BIT | DATA_VALID_BIT);

/* Wait for specific combination of bits */
EventBits_t uxBits = xEventGroupWaitBits(
    xEventGroup,
    SENSOR_READY_BIT | DATA_VALID_BIT,  /* Bits to wait for */
    pdTRUE,                              /* Clear bits on exit */
    pdTRUE,                              /* Wait for ALL bits (AND) */
    portMAX_DELAY                        /* Timeout */
);
```

**Use cases:**

- Synchronizing multiple tasks (rendezvous point).
- Waiting for multiple conditions to be true before proceeding.
- Broadcasting an event to multiple waiting tasks.

---

## 9. Task Notifications

**Task notifications** are a lightweight alternative to semaphores, event
groups, and queues. Each task has a built-in 32-bit notification value.

```c
/* Send notification (acts like a lightweight semaphore give) */
xTaskNotifyGive(xTaskHandle);

/* Wait for notification (acts like a lightweight semaphore take) */
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

/* Send notification with value */
xTaskNotify(xTaskHandle, ulValue, eAction);

/* Receive notification with value */
xTaskNotifyWait(ulBitsToClearOnEntry, ulBitsToClearOnExit,
                &ulNotificationValue, xTicksToWait);
```

**Advantages:**

- 45% faster than semaphores (no kernel object to manage).
- No RAM allocation required (built into the task control block).

**Limitations:**

- Only one task can receive (no broadcast).
- Cannot be used to send data from task to ISR (only ISR to task or task to
  task).

---

## 10. Software Timers

Software timers execute a callback function at a configured interval without
requiring a dedicated task.

```c
TimerHandle_t xTimerCreate(
    const char *pcTimerName,
    TickType_t xTimerPeriod,
    UBaseType_t uxAutoReload,       /* pdTRUE = periodic, pdFALSE = one-shot */
    void *pvTimerID,
    TimerCallbackFunction_t pxCallbackFunction
);

xTimerStart(xTimer, 0);
xTimerStop(xTimer, 0);
xTimerChangePeriod(xTimer, pdMS_TO_TICKS(500), 0);
```

**Important notes:**

- Timer callbacks run in the context of the **timer service task** (daemon
  task), not in an ISR.
- Callbacks must not block (no `vTaskDelay`, no blocking queue operations).
- The timer service task priority is set by `configTIMER_TASK_PRIORITY`.
- All timer commands go through a command queue of length
  `configTIMER_QUEUE_LENGTH`.

---

## 11. Memory Management

FreeRTOS provides five heap implementations:

| Scheme | Allocate | Free | Coalescence | Best For |
|--------|----------|------|-------------|----------|
| heap_1 | Yes | No | N/A | Static systems, never free |
| heap_2 | Yes | Yes | No | Fixed-size allocations |
| heap_3 | Yes | Yes | Yes | Wraps standard malloc/free |
| heap_4 | Yes | Yes | Yes | General purpose (recommended) |
| heap_5 | Yes | Yes | Yes | Non-contiguous memory regions |

### Stack vs. Heap

- **Task stacks** are allocated from the FreeRTOS heap at task creation time.
- **Kernel objects** (queues, semaphores, timers) are allocated from the
  FreeRTOS heap.
- `configTOTAL_HEAP_SIZE` determines the total heap available.

### Detecting Memory Issues

```c
/* Check remaining heap */
size_t xPortGetFreeHeapSize(void);
size_t xPortGetMinimumEverFreeHeapSize(void);

/* Check task stack usage */
UBaseType_t uxTaskGetStackHighWaterMark(xTaskHandle);
```

### Static Allocation

FreeRTOS supports static allocation (no heap required) when
`configSUPPORT_STATIC_ALLOCATION` is set to 1:

```c
static StaticTask_t xTaskBuffer;
static StackType_t xStack[256];

xTaskCreateStatic(vMyTask, "Task", 256, NULL, 2, xStack, &xTaskBuffer);
```

---

## 12. Common Pitfalls

### Stack Overflow

**Symptom**: Random crashes, corrupted data, hard faults.
**Cause**: Task stack too small for its call depth and local variables.
**Fix**: Enable `configCHECK_FOR_STACK_OVERFLOW`, check high water marks,
increase stack sizes.

### Priority Inversion

**Symptom**: High-priority task unresponsive, system appears to hang.
**Cause**: Low-priority task holds a mutex needed by a high-priority task, while
a medium-priority task runs.
**Fix**: Use mutexes (not binary semaphores) for mutual exclusion -- mutexes
provide priority inheritance.

### Deadlock

**Symptom**: Two or more tasks permanently blocked.
**Cause**: Circular dependency on mutexes.
**Fix**: Always acquire mutexes in a consistent global order. Use timeouts.

### Starvation

**Symptom**: Low-priority task never runs.
**Cause**: Higher-priority tasks never block.
**Fix**: Ensure all tasks block at some point. Use `vTaskDelay()` or wait on
a queue/semaphore.

### Incorrect ISR API Usage

**Symptom**: Crashes or corruption when using RTOS primitives in ISRs.
**Cause**: Using task-level API (e.g., `xQueueSend`) instead of ISR-safe API
(e.g., `xQueueSendFromISR`) inside an interrupt handler.
**Fix**: Always use `FromISR` variants in interrupt handlers.

---

## 13. Best Practices

1. **Assign priorities carefully.** The highest priority should go to the most
   time-critical task. Most tasks should run at the same (low) priority.

2. **Use the minimum stack size that works.** Check high water marks under
   worst-case conditions and add a 25% safety margin.

3. **Prefer queues over global variables.** Queues provide thread-safe,
   blocking, copy-based communication.

4. **Keep critical sections short.** Minimize time spent holding a mutex.

5. **Use task notifications when possible.** They are faster and use less RAM
   than semaphores.

6. **Design tasks to block.** A task that never blocks starves lower-priority
   tasks and wastes power.

7. **Use software timers for periodic housekeeping.** Avoid creating a task
   just to do something every N seconds.

8. **Enable stack overflow checking in development.** Disable in production
   for performance.

9. **Monitor heap usage.** Track `xPortGetMinimumEverFreeHeapSize()` to ensure
   the system does not run out of memory.

10. **Document task priorities and resource ownership.** Maintain a table of
    all tasks, their priorities, stack sizes, and which mutexes they use.

---

## Examples

| File | Description |
|------|-------------|
| `examples/01_task_creation.c` | Basic FreeRTOS task creation and deletion |
| `examples/02_led_tasks.c` | Multiple LED blink tasks at different rates |
| `examples/03_producer_consumer.c` | Producer-consumer pattern with queue |
| `examples/04_mutex.c` | Mutex-protected shared resource |
| `examples/05_binary_semaphore.c` | Binary semaphore for ISR-to-task sync |
| `examples/06_software_timer.c` | Periodic and one-shot software timers |
| `examples/07_task_notification.c` | Lightweight ISR-to-task notification |
| `examples/08_idle_hook.c` | Idle hook for power saving and monitoring |

## Exercises

| Exercise | Topic |
|----------|-------|
| 01 | Multi-LED controller with independent timing |
| 02 | Priority inversion demonstration and fix |
| 03 | Bounded buffer with producer-consumer |
| 04 | Event-driven state machine |
| 05 | Watchdog task monitor |
| 06 | Real-time data logger |
| 07 | Stack usage analyzer |

## Project

**Multi-Sensor RTOS Dashboard** -- See `project/README.md` for details.

---

## Further Reading

- [FreeRTOS Official Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- *Mastering the FreeRTOS Real Time Kernel* by Richard Barry
- ARM Cortex-M Context Switching Application Notes
- MISRA C guidelines for RTOS-based systems
