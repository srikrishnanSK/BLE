/**
 * @file 02_queue_messaging.c
 * @brief Inter-Task Communication with Message Queues (Simulated)
 *
 * Demonstrates:
 * - Thread-safe FIFO queue for inter-task communication
 * - Blocking send/receive with timeouts
 * - Multiple producers sending to a single consumer
 * - Copy semantics (data is copied into/out of the queue)
 *
 * Simulates FreeRTOS xQueueCreate / xQueueSend / xQueueReceive
 * using POSIX threads, mutexes, and condition variables.
 *
 * Build: gcc -Wall -Wextra -o 02_queue_messaging 02_queue_messaging.c -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

/* --------------------------------------------------------------------------
 * Simulated RTOS Queue
 * -------------------------------------------------------------------------- */

#define QUEUE_MAX_ITEM_SIZE   128
#define QUEUE_MAX_LENGTH       16

typedef struct {
    uint8_t  buffer[QUEUE_MAX_LENGTH][QUEUE_MAX_ITEM_SIZE];
    size_t   item_size;
    size_t   max_length;
    size_t   count;
    size_t   head;           /* Next read position  */
    size_t   tail;           /* Next write position  */

    pthread_mutex_t lock;
    pthread_cond_t  not_full;
    pthread_cond_t  not_empty;
} sim_queue_t;

/**
 * @brief Create a simulated queue.
 *
 * @param q           Pointer to queue structure.
 * @param max_length  Maximum number of items the queue can hold.
 * @param item_size   Size of each item in bytes.
 * @return 0 on success, -1 on failure.
 */
static int queue_create(sim_queue_t *q, size_t max_length, size_t item_size)
{
    if (!q || max_length > QUEUE_MAX_LENGTH || item_size > QUEUE_MAX_ITEM_SIZE) {
        return -1;
    }

    memset(q, 0, sizeof(*q));
    q->item_size  = item_size;
    q->max_length = max_length;
    q->count      = 0;
    q->head       = 0;
    q->tail       = 0;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);

    return 0;
}

/**
 * @brief Destroy a simulated queue.
 */
static void queue_destroy(sim_queue_t *q)
{
    if (!q) return;
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
}

/**
 * @brief Send an item to the queue (blocks if full, with timeout).
 *
 * Mimics xQueueSend() with copy semantics.
 *
 * @param q             Pointer to queue.
 * @param item          Pointer to data to copy into the queue.
 * @param timeout_ms    Maximum time to wait (0 = no wait).
 * @return 0 on success, -1 on timeout or error.
 */
static int queue_send(sim_queue_t *q, const void *item, uint32_t timeout_ms)
{
    struct timespec ts;

    pthread_mutex_lock(&q->lock);

    /* Wait while queue is full */
    while (q->count >= q->max_length) {
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&q->lock);
            return -1;  /* Would block */
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec  += 1;
            ts.tv_nsec -= 1000000000L;
        }
        int rc = pthread_cond_timedwait(&q->not_full, &q->lock, &ts);
        if (rc == ETIMEDOUT) {
            pthread_mutex_unlock(&q->lock);
            return -1;
        }
    }

    /* Copy item into queue (copy semantics, like FreeRTOS) */
    memcpy(q->buffer[q->tail], item, q->item_size);
    q->tail = (q->tail + 1) % q->max_length;
    q->count++;

    /* Signal any waiting receiver */
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);

    return 0;
}

/**
 * @brief Receive an item from the queue (blocks if empty, with timeout).
 *
 * Mimics xQueueReceive() with copy semantics.
 *
 * @param q             Pointer to queue.
 * @param item          Pointer to buffer to copy data into.
 * @param timeout_ms    Maximum time to wait (0 = no wait).
 * @return 0 on success, -1 on timeout or error.
 */
static int queue_receive(sim_queue_t *q, void *item, uint32_t timeout_ms)
{
    struct timespec ts;

    pthread_mutex_lock(&q->lock);

    /* Wait while queue is empty */
    while (q->count == 0) {
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&q->lock);
            return -1;  /* Would block */
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec  += 1;
            ts.tv_nsec -= 1000000000L;
        }
        int rc = pthread_cond_timedwait(&q->not_empty, &q->lock, &ts);
        if (rc == ETIMEDOUT) {
            pthread_mutex_unlock(&q->lock);
            return -1;
        }
    }

    /* Copy item out of queue */
    memcpy(item, q->buffer[q->head], q->item_size);
    q->head = (q->head + 1) % q->max_length;
    q->count--;

    /* Signal any waiting sender */
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);

    return 0;
}

/* --------------------------------------------------------------------------
 * Application: Sensor Data Pipeline
 *
 * Two producer tasks generate sensor readings and send them through a queue
 * to a single consumer task that processes and displays the data.
 * -------------------------------------------------------------------------- */

typedef struct {
    uint8_t  sensor_id;      /* Which sensor produced this reading    */
    uint16_t value;           /* Raw sensor value                      */
    uint32_t timestamp_ms;    /* Simulated timestamp                   */
} sensor_msg_t;

/* Shared queue for sensor messages */
static sim_queue_t g_sensor_queue;

/* Running flag */
static volatile bool g_running = true;

/**
 * @brief Get a monotonic timestamp in milliseconds (simulated tick count).
 */
static uint32_t get_tick_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* --------------------------------------------------------------------------
 * Producer Task: Simulates periodic sensor readings
 * -------------------------------------------------------------------------- */

typedef struct {
    uint8_t     sensor_id;
    uint32_t    period_ms;      /* How often to produce a reading */
    uint32_t    num_readings;   /* How many readings to produce  */
} producer_params_t;

static void *producer_task(void *arg)
{
    const producer_params_t *params = (const producer_params_t *)arg;
    sensor_msg_t msg;

    printf("[Producer %u] Started (period=%u ms, readings=%u)\n",
           params->sensor_id, params->period_ms, params->num_readings);

    for (uint32_t i = 0; i < params->num_readings && g_running; i++) {
        /* Simulate sensor reading */
        msg.sensor_id    = params->sensor_id;
        msg.value        = (uint16_t)(rand() % 4096);   /* 12-bit ADC */
        msg.timestamp_ms = get_tick_ms();

        /* Send to queue with 500 ms timeout */
        int rc = queue_send(&g_sensor_queue, &msg, 500);
        if (rc == 0) {
            printf("[Producer %u] Sent: value=%u, ts=%u\n",
                   params->sensor_id, msg.value, msg.timestamp_ms);
        } else {
            printf("[Producer %u] Queue full -- message dropped!\n",
                   params->sensor_id);
        }

        /* Simulate periodic delay */
        usleep(params->period_ms * 1000);
    }

    printf("[Producer %u] Finished\n", params->sensor_id);
    return NULL;
}

/* --------------------------------------------------------------------------
 * Consumer Task: Receives and processes sensor messages
 * -------------------------------------------------------------------------- */

static void *consumer_task(void *arg)
{
    (void)arg;
    sensor_msg_t msg;
    uint32_t total_received = 0;
    uint32_t sum_by_sensor[2] = {0, 0};
    uint32_t count_by_sensor[2] = {0, 0};

    printf("[Consumer] Started -- waiting for sensor data\n");

    while (g_running) {
        /* Block waiting for a message (1 second timeout) */
        int rc = queue_receive(&g_sensor_queue, &msg, 1000);
        if (rc == 0) {
            total_received++;

            /* Track per-sensor statistics */
            if (msg.sensor_id < 2) {
                sum_by_sensor[msg.sensor_id] += msg.value;
                count_by_sensor[msg.sensor_id]++;
            }

            printf("[Consumer] Received: sensor=%u, value=%u, ts=%u "
                   "(total=%u)\n",
                   msg.sensor_id, msg.value, msg.timestamp_ms,
                   total_received);
        }
        /* Timeout just lets us check g_running */
    }

    /* Print summary */
    printf("\n[Consumer] === Summary ===\n");
    printf("[Consumer] Total messages received: %u\n", total_received);
    for (int i = 0; i < 2; i++) {
        if (count_by_sensor[i] > 0) {
            printf("[Consumer] Sensor %d: %u readings, avg=%u\n",
                   i, count_by_sensor[i],
                   sum_by_sensor[i] / count_by_sensor[i]);
        }
    }

    return NULL;
}

/* --------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------- */

int main(void)
{
    printf("=== Queue Messaging Demo (Simulated RTOS) ===\n\n");

    /* Seed random number generator */
    srand((unsigned int)time(NULL));

    /* Create the queue: holds up to 8 sensor messages */
    if (queue_create(&g_sensor_queue, 8, sizeof(sensor_msg_t)) != 0) {
        fprintf(stderr, "Failed to create queue\n");
        return 1;
    }

    /* Define producer parameters */
    producer_params_t prod1 = { .sensor_id = 0, .period_ms = 200, .num_readings = 10 };
    producer_params_t prod2 = { .sensor_id = 1, .period_ms = 350, .num_readings = 8  };

    /* Create threads (simulating RTOS tasks) */
    pthread_t producer1_thread, producer2_thread, consumer_thread;

    pthread_create(&consumer_thread,  NULL, consumer_task,  NULL);
    pthread_create(&producer1_thread, NULL, producer_task,  &prod1);
    pthread_create(&producer2_thread, NULL, producer_task,  &prod2);

    /* Wait for producers to finish */
    pthread_join(producer1_thread, NULL);
    pthread_join(producer2_thread, NULL);

    /* Give consumer time to drain the queue */
    usleep(500000);

    /* Signal consumer to stop */
    g_running = false;
    pthread_join(consumer_thread, NULL);

    /* Cleanup */
    queue_destroy(&g_sensor_queue);

    printf("\n=== Demo Complete ===\n");
    return 0;
}
