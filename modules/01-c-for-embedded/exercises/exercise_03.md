# Exercise 03: Ring Buffer (Circular Buffer) from Scratch

## Objective

Implement a ring buffer (circular buffer) suitable for embedded systems. Ring
buffers are the most common data structure in embedded programming, used for
UART receive/transmit queues, sensor data logging, audio buffers, and inter-task
communication.

## Background

A ring buffer uses a fixed-size array with two indices: `head` (where data is
written) and `tail` (where data is read). When an index reaches the end of the
array, it wraps around to the beginning.

```
  Write (head)
     |
     v
[  ] [  ] [D1] [D2] [D3] [  ] [  ] [  ]
            ^
            |
         Read (tail)
```

Key design decision: To distinguish "full" from "empty" (both have `head == tail`),
we sacrifice one slot. The buffer is full when `(head + 1) % size == tail`.

## Task

Implement the following ring buffer API using only static memory (no `malloc`):

```c
#define RING_BUFFER_SIZE 8  /* Must be a power of 2 for optimization */

typedef struct {
    uint8_t buffer[RING_BUFFER_SIZE];
    volatile uint16_t head;  /* Write index (volatile for ISR safety) */
    volatile uint16_t tail;  /* Read index  (volatile for ISR safety) */
} ring_buffer_t;

/* Initialize the ring buffer */
void ring_init(ring_buffer_t *rb);

/* Add a byte to the buffer. Returns 0 on success, -1 if full. */
int ring_put(ring_buffer_t *rb, uint8_t data);

/* Remove a byte from the buffer. Returns 0 on success, -1 if empty. */
int ring_get(ring_buffer_t *rb, uint8_t *data);

/* Check if buffer is empty */
int ring_is_empty(const ring_buffer_t *rb);

/* Check if buffer is full */
int ring_is_full(const ring_buffer_t *rb);

/* Return number of bytes currently in the buffer */
uint16_t ring_count(const ring_buffer_t *rb);

/* Peek at the next byte without removing it. Returns 0 on success, -1 if empty. */
int ring_peek(const ring_buffer_t *rb, uint8_t *data);

/* Reset the buffer to empty */
void ring_flush(ring_buffer_t *rb);
```

## Requirements

1. Use power-of-2 buffer size and masking (`& (SIZE - 1)`) instead of modulo
   for index wrapping. This is faster on CPUs without hardware divide.

2. The `head` and `tail` members are `volatile` because in real embedded code,
   `ring_put` might be called from an ISR while `ring_get` runs in the main
   loop.

3. Write a test program that:
   - Creates a ring buffer
   - Fills it to capacity and verifies it reports full
   - Reads all data back and verifies order is correct (FIFO)
   - Tests wrap-around by writing more data than the buffer size
   - Tests peek without removing
   - Tests flush

## Hints

- Wrap index: `index = (index + 1) & (RING_BUFFER_SIZE - 1)`
- Count: `(head - tail) & (RING_BUFFER_SIZE - 1)`
- Full: `((head + 1) & (RING_BUFFER_SIZE - 1)) == tail`
- Empty: `head == tail`
- Usable capacity is `RING_BUFFER_SIZE - 1` (one slot is sacrificed)

## Expected Output

```
=== Ring Buffer Test ===

Test 1: Fill buffer to capacity
  Put: 10 20 30 40 50 60 70
  Buffer count: 7 (capacity: 7)
  Buffer full: YES

Test 2: Try to put when full
  ring_put returned -1 (full) [PASS]

Test 3: Read all data (FIFO order)
  Got: 10 20 30 40 50 60 70
  Buffer empty: YES [PASS]

Test 4: Wrap-around test
  Writing 10 bytes with reads in between... [PASS]

Test 5: Peek test
  Peek: 42, still in buffer: YES [PASS]

Test 6: Flush test
  Flushed. Empty: YES [PASS]

All tests passed!
```

## Compilation

```bash
gcc -Wall -Wextra -std=c99 -o exercise_03 solution_03.c
```
