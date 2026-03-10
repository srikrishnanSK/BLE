# Exercise 04: Software Debouncer with Configurable Delay

## Objective

Build a production-quality software debouncer that supports:
- Multiple independent channels (buttons)
- Configurable debounce time per channel
- Rising and falling edge callbacks
- Both polling and interrupt-driven usage patterns

## Background

Every embedded product with buttons needs debouncing.  A well-designed
debounce module is reusable across projects.  This exercise asks you to
build one with a clean API, then test it with simulated bouncy input.

## Requirements

### Part A -- Debouncer Data Structure

```c
#define DEBOUNCE_MAX_CHANNELS 8

typedef void (*edge_callback_t)(uint8_t channel, bool rising);

typedef struct {
    uint8_t   stable_state;       /* Current debounced state       */
    uint8_t   last_raw;           /* Last raw reading              */
    uint32_t  last_change_tick;   /* Tick when raw last changed    */
    uint32_t  debounce_ticks;     /* Required stable duration      */
    edge_callback_t callback;     /* Called on debounced edge      */
} debounce_channel_t;

typedef struct {
    debounce_channel_t channels[DEBOUNCE_MAX_CHANNELS];
    uint8_t            num_channels;
} debouncer_t;
```

### Part B -- API Functions

```c
/* Initialize the debouncer with a given number of channels */
void debouncer_init(debouncer_t *db, uint8_t num_channels);

/* Configure a channel's debounce time and callback */
void debouncer_configure(debouncer_t *db, uint8_t channel,
                         uint32_t debounce_ticks,
                         edge_callback_t callback);

/* Process one sample for all channels.
 * raw_states: bitmask where bit N = raw state of channel N.
 * tick: current system tick.
 * Returns: bitmask of debounced states. */
uint8_t debouncer_update(debouncer_t *db, uint8_t raw_states,
                         uint32_t tick);

/* Read the debounced state of a single channel */
bool debouncer_read(const debouncer_t *db, uint8_t channel);

/* Check if a rising edge occurred since last query (auto-clearing) */
bool debouncer_rose(debouncer_t *db, uint8_t channel);

/* Check if a falling edge occurred since last query (auto-clearing) */
bool debouncer_fell(debouncer_t *db, uint8_t channel);
```

### Part C -- Simulation Test Harness

Create a test that feeds simulated button signals through the debouncer:

1. **Clean press/release** -- verify single edge detected
2. **Bouncy press** -- verify bounces are filtered
3. **Very short press** (shorter than debounce time) -- verify it is ignored
4. **Two buttons simultaneously** -- verify independent operation
5. **Rapid toggling** (faster than debounce period) -- verify stable output

Print the raw input and debounced output side-by-side for visual inspection.

### Part D -- Statistics (bonus)

Track per-channel statistics:
```c
typedef struct {
    uint32_t press_count;       /* Number of rising edges   */
    uint32_t release_count;     /* Number of falling edges  */
    uint32_t bounce_count;      /* Number of filtered bounces */
    uint32_t longest_press_ms;  /* Longest HIGH duration    */
} debounce_stats_t;
```

## Hints

1. Use the timer-based algorithm from Example 03 as a starting point.
2. The `debouncer_rose()` / `debouncer_fell()` functions need a flag that
   gets set on edge detection and cleared when the function is called.
3. To test multiple channels, pack raw states into a bitmask:
   `raw |= (pin_state << channel_num)`.

## Deliverable

A single C file `solution_04.c`.
