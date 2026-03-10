/**
 * @file solution_03.c
 * @brief Solution for Exercise 03: Ring Buffer (Circular Buffer) from Scratch
 *
 * Implements a power-of-2 sized ring buffer using index masking for fast
 * wrap-around. Head and tail are volatile for ISR safety.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o solution_03 solution_03.c
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Ring Buffer Configuration
 *
 * RING_BUFFER_SIZE must be a power of 2 so we can use bitwise AND for
 * index wrapping instead of the expensive modulo operator.
 *
 * Usable capacity = RING_BUFFER_SIZE - 1 (one slot is sacrificed to
 * distinguish full from empty).
 * ----------------------------------------------------------------------- */
#define RING_BUFFER_SIZE  8

/* Compile-time check: size must be a power of 2 */
#if (RING_BUFFER_SIZE & (RING_BUFFER_SIZE - 1)) != 0
#error "RING_BUFFER_SIZE must be a power of 2"
#endif

#define RING_MASK  (RING_BUFFER_SIZE - 1)

typedef struct {
    uint8_t  buffer[RING_BUFFER_SIZE];
    volatile uint16_t head;   /* Next write position */
    volatile uint16_t tail;   /* Next read position  */
} ring_buffer_t;

/* -----------------------------------------------------------------------
 * ring_init - Reset the ring buffer to empty state
 *
 * Sets head and tail to the same value (indicating empty) and zeroes
 * the data buffer for cleanliness.
 * ----------------------------------------------------------------------- */
void ring_init(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    memset(rb->buffer, 0, RING_BUFFER_SIZE);
}

/* -----------------------------------------------------------------------
 * ring_is_empty - Check if the buffer has no data
 *
 * Empty when head == tail (both point to the same slot and there is
 * nothing between them to read).
 * ----------------------------------------------------------------------- */
int ring_is_empty(const ring_buffer_t *rb)
{
    return rb->head == rb->tail;
}

/* -----------------------------------------------------------------------
 * ring_is_full - Check if the buffer is at capacity
 *
 * Full when advancing head by 1 would make it equal to tail. We
 * sacrifice one slot to distinguish full from empty.
 * ----------------------------------------------------------------------- */
int ring_is_full(const ring_buffer_t *rb)
{
    return ((rb->head + 1) & RING_MASK) == rb->tail;
}

/* -----------------------------------------------------------------------
 * ring_count - Number of bytes currently stored
 *
 * Uses unsigned subtraction with masking. This works correctly even
 * when head has wrapped around past tail.
 * ----------------------------------------------------------------------- */
uint16_t ring_count(const ring_buffer_t *rb)
{
    return (rb->head - rb->tail) & RING_MASK;
}

/* -----------------------------------------------------------------------
 * ring_put - Add a byte to the buffer
 *
 * Writes to buffer[head], then advances head with wrap-around.
 *
 * @return 0 on success, -1 if buffer is full
 * ----------------------------------------------------------------------- */
int ring_put(ring_buffer_t *rb, uint8_t data)
{
    if (ring_is_full(rb)) {
        return -1;  /* Buffer full, reject */
    }

    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) & RING_MASK;

    return 0;
}

/* -----------------------------------------------------------------------
 * ring_get - Remove and return a byte from the buffer
 *
 * Reads from buffer[tail], then advances tail with wrap-around.
 *
 * @return 0 on success, -1 if buffer is empty
 * ----------------------------------------------------------------------- */
int ring_get(ring_buffer_t *rb, uint8_t *data)
{
    if (ring_is_empty(rb)) {
        return -1;  /* Buffer empty, nothing to read */
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) & RING_MASK;

    return 0;
}

/* -----------------------------------------------------------------------
 * ring_peek - Look at the next byte without removing it
 *
 * Same as ring_get but does not advance the tail pointer.
 *
 * @return 0 on success, -1 if buffer is empty
 * ----------------------------------------------------------------------- */
int ring_peek(const ring_buffer_t *rb, uint8_t *data)
{
    if (ring_is_empty(rb)) {
        return -1;
    }

    *data = rb->buffer[rb->tail];
    return 0;
}

/* -----------------------------------------------------------------------
 * ring_flush - Discard all data in the buffer
 *
 * Simply resets tail to match head. The data remains in memory but
 * is logically removed.
 * ----------------------------------------------------------------------- */
void ring_flush(ring_buffer_t *rb)
{
    rb->tail = rb->head;
}

/* ======================================================================= */
int main(void)
{
    ring_buffer_t rb;
    uint8_t val;
    int ret;

    printf("=== Ring Buffer Test ===\n");
    printf("Buffer size: %d, Usable capacity: %d\n\n",
           RING_BUFFER_SIZE, RING_BUFFER_SIZE - 1);

    ring_init(&rb);

    /* -----------------------------------------------------------
     * Test 1: Fill buffer to capacity
     * ----------------------------------------------------------- */
    printf("Test 1: Fill buffer to capacity\n");
    uint8_t test_data[] = {10, 20, 30, 40, 50, 60, 70};
    printf("  Put: ");
    for (int i = 0; i < 7; i++) {
        ret = ring_put(&rb, test_data[i]);
        if (ret == 0) {
            printf("%d ", test_data[i]);
        } else {
            printf("(FAIL at %d) ", test_data[i]);
        }
    }
    printf("\n");
    printf("  Buffer count: %d (capacity: %d)\n", ring_count(&rb), RING_BUFFER_SIZE - 1);
    printf("  Buffer full: %s\n\n", ring_is_full(&rb) ? "YES" : "NO");

    /* -----------------------------------------------------------
     * Test 2: Try to put when full
     * ----------------------------------------------------------- */
    printf("Test 2: Try to put when full\n");
    ret = ring_put(&rb, 0xFF);
    printf("  ring_put returned %d (full) %s\n\n",
           ret, (ret == -1) ? "[PASS]" : "[FAIL]");

    /* -----------------------------------------------------------
     * Test 3: Read all data back (verify FIFO order)
     * ----------------------------------------------------------- */
    printf("Test 3: Read all data (FIFO order)\n");
    printf("  Got: ");
    int fifo_pass = 1;
    for (int i = 0; i < 7; i++) {
        ret = ring_get(&rb, &val);
        if (ret == 0) {
            printf("%d ", val);
            if (val != test_data[i]) {
                fifo_pass = 0;
            }
        }
    }
    printf("\n");
    printf("  FIFO order correct: %s\n", fifo_pass ? "[PASS]" : "[FAIL]");
    printf("  Buffer empty: %s %s\n\n",
           ring_is_empty(&rb) ? "YES" : "NO",
           ring_is_empty(&rb) ? "[PASS]" : "[FAIL]");

    /* -----------------------------------------------------------
     * Test 4: Wrap-around test
     *
     * Write and read interleaved so the indices wrap around
     * the end of the physical array.
     * ----------------------------------------------------------- */
    printf("Test 4: Wrap-around test\n");
    ring_init(&rb);
    int wrap_pass = 1;

    for (uint8_t i = 0; i < 20; i++) {
        /* Put a value */
        ret = ring_put(&rb, i);
        if (ret != 0) {
            printf("  Put failed at i=%d (count=%d)\n", i, ring_count(&rb));
            wrap_pass = 0;
            break;
        }

        /* Read it back every other iteration to cause wrapping */
        if (i % 2 == 1) {
            uint8_t v1, v2;
            ring_get(&rb, &v1);
            ring_get(&rb, &v2);
            if (v1 != i - 1 || v2 != i) {
                printf("  Mismatch at i=%d: got %d,%d expected %d,%d\n",
                       i, v1, v2, i - 1, i);
                wrap_pass = 0;
            }
        }
    }

    /* Drain remaining */
    while (!ring_is_empty(&rb)) {
        ring_get(&rb, &val);
    }

    printf("  Writing 20 values with reads in between... %s\n\n",
           wrap_pass ? "[PASS]" : "[FAIL]");

    /* -----------------------------------------------------------
     * Test 5: Peek test
     * ----------------------------------------------------------- */
    printf("Test 5: Peek test\n");
    ring_init(&rb);
    ring_put(&rb, 42);
    ring_put(&rb, 99);

    ring_peek(&rb, &val);
    int peek_pass = (val == 42) && (ring_count(&rb) == 2);
    printf("  Peek: %d, count still %d: %s\n\n",
           val, ring_count(&rb), peek_pass ? "[PASS]" : "[FAIL]");

    /* -----------------------------------------------------------
     * Test 6: Flush test
     * ----------------------------------------------------------- */
    printf("Test 6: Flush test\n");
    ring_flush(&rb);
    printf("  Flushed. Empty: %s %s\n\n",
           ring_is_empty(&rb) ? "YES" : "NO",
           ring_is_empty(&rb) ? "[PASS]" : "[FAIL]");

    /* -----------------------------------------------------------
     * Summary
     * ----------------------------------------------------------- */
    int all_pass = fifo_pass && wrap_pass && peek_pass && ring_is_empty(&rb);
    printf("%s\n", all_pass ? "All tests passed!" : "Some tests FAILED!");

    printf("\n=== End of Exercise 03 ===\n");
    return 0;
}
