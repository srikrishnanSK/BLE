/**
 * @file 01_task_creation.c
 * @brief Basic FreeRTOS Task Creation and Deletion
 *
 * Demonstrates:
 * - Creating tasks with xTaskCreate()
 * - Passing parameters to tasks
 * - Task deletion with vTaskDelete()
 * - Using vTaskDelay() for periodic execution
 * - Checking stack high water mark
 *
 * Hardware: Any FreeRTOS-capable MCU (e.g., nRF52840, STM32, ESP32)
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Configuration
 * -------------------------------------------------------------------------- */
#define TASK1_STACK_SIZE    256   /* Stack size in words (1 KB on 32-bit) */
#define TASK2_STACK_SIZE    256
#define TASK3_STACK_SIZE    128

#define TASK1_PRIORITY      2
#define TASK2_PRIORITY      1
#define TASK3_PRIORITY      1

/* --------------------------------------------------------------------------
 * Task Handles
 * -------------------------------------------------------------------------- */
static TaskHandle_t xTask1Handle = NULL;
static TaskHandle_t xTask2Handle = NULL;
static TaskHandle_t xTask3Handle = NULL;

/* --------------------------------------------------------------------------
 * Task Parameters
 * -------------------------------------------------------------------------- */
typedef struct {
    const char *name;
    uint32_t    delay_ms;
    uint32_t    iterations;
} task_params_t;

static const task_params_t task1_params = {
    .name       = "Sensor",
    .delay_ms   = 500,
    .iterations = 0    /* 0 = infinite */
};

static const task_params_t task2_params = {
    .name       = "Logger",
    .delay_ms   = 1000,
    .iterations = 10   /* Will delete itself after 10 iterations */
};

/* --------------------------------------------------------------------------
 * Task 1: Periodic Task (runs forever)
 * -------------------------------------------------------------------------- */
static void vTask1_Periodic(void *pvParameters)
{
    const task_params_t *params = (const task_params_t *)pvParameters;
    uint32_t counter = 0;

    printf("[%s] Task started (priority %lu)\n",
           params->name,
           (unsigned long)uxTaskPriorityGet(NULL));

    for (;;) {
        counter++;
        printf("[%s] Iteration %lu\n", params->name, (unsigned long)counter);

        /* Report stack usage every 10 iterations */
        if (counter % 10 == 0) {
            UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
            printf("[%s] Stack high water mark: %lu words remaining\n",
                   params->name, (unsigned long)high_water);
        }

        vTaskDelay(pdMS_TO_TICKS(params->delay_ms));
    }
}

/* --------------------------------------------------------------------------
 * Task 2: Self-Deleting Task (runs N iterations then exits)
 * -------------------------------------------------------------------------- */
static void vTask2_SelfDeleting(void *pvParameters)
{
    const task_params_t *params = (const task_params_t *)pvParameters;
    uint32_t counter = 0;

    printf("[%s] Task started, will run %lu iterations\n",
           params->name, (unsigned long)params->iterations);

    for (counter = 1; counter <= params->iterations; counter++) {
        printf("[%s] Iteration %lu / %lu\n",
               params->name,
               (unsigned long)counter,
               (unsigned long)params->iterations);

        vTaskDelay(pdMS_TO_TICKS(params->delay_ms));
    }

    printf("[%s] Task complete, deleting self\n", params->name);

    /*
     * Pass NULL to delete the calling task.
     * The idle task will free the memory allocated for this task's
     * stack and TCB.
     */
    xTask2Handle = NULL;
    vTaskDelete(NULL);

    /* Execution never reaches here */
}

/* --------------------------------------------------------------------------
 * Task 3: Monitor Task (watches other tasks)
 * -------------------------------------------------------------------------- */
static void vTask3_Monitor(void *pvParameters)
{
    (void)pvParameters;

    printf("[Monitor] Task started\n");

    for (;;) {
        printf("\n--- Task Status Report ---\n");

        /* Check if Task 1 is still running */
        if (xTask1Handle != NULL) {
            eTaskState state = eTaskGetState(xTask1Handle);
            printf("  Task1 (Sensor): state=%d, stack HWM=%lu\n",
                   (int)state,
                   (unsigned long)uxTaskGetStackHighWaterMark(xTask1Handle));
        }

        /* Check if Task 2 is still running */
        if (xTask2Handle != NULL) {
            eTaskState state = eTaskGetState(xTask2Handle);
            printf("  Task2 (Logger): state=%d, stack HWM=%lu\n",
                   (int)state,
                   (unsigned long)uxTaskGetStackHighWaterMark(xTask2Handle));
        } else {
            printf("  Task2 (Logger): DELETED\n");
        }

        /* Report free heap */
        printf("  Free heap: %lu bytes\n",
               (unsigned long)xPortGetFreeHeapSize());
        printf("  Min free heap ever: %lu bytes\n",
               (unsigned long)xPortGetMinimumEverFreeHeapSize());
        printf("--------------------------\n\n");

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* --------------------------------------------------------------------------
 * Stack Overflow Hook (called if configCHECK_FOR_STACK_OVERFLOW >= 1)
 * -------------------------------------------------------------------------- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("!!! STACK OVERFLOW in task: %s !!!\n", pcTaskName);

    /* Halt in development */
    for (;;) {
        /* Spin -- in production, trigger a reset */
    }
}

/* --------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------- */
int main(void)
{
    BaseType_t result;

    printf("=== FreeRTOS Task Creation Demo ===\n\n");

    /* Create Task 1: Periodic sensor reading */
    result = xTaskCreate(
        vTask1_Periodic,
        "Sensor",
        TASK1_STACK_SIZE,
        (void *)&task1_params,
        TASK1_PRIORITY,
        &xTask1Handle
    );
    configASSERT(result == pdPASS);

    /* Create Task 2: Self-deleting logger */
    result = xTaskCreate(
        vTask2_SelfDeleting,
        "Logger",
        TASK2_STACK_SIZE,
        (void *)&task2_params,
        TASK2_PRIORITY,
        &xTask2Handle
    );
    configASSERT(result == pdPASS);

    /* Create Task 3: Monitor */
    result = xTaskCreate(
        vTask3_Monitor,
        "Monitor",
        TASK3_STACK_SIZE,
        NULL,
        TASK3_PRIORITY,
        &xTask3Handle
    );
    configASSERT(result == pdPASS);

    printf("All tasks created. Starting scheduler...\n\n");

    /* Start the scheduler -- this function never returns */
    vTaskStartScheduler();

    /*
     * If we reach here, there was insufficient heap memory to create
     * the idle task or timer task.
     */
    printf("ERROR: Insufficient heap for scheduler\n");

    for (;;) {
        /* Should never reach here */
    }

    return 0;
}
