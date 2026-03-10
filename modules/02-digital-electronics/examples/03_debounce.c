/**
 * Module 02 - Example 03: Software Debounce Algorithms
 *
 * Demonstrates four different software debouncing strategies, each with
 * different trade-offs in latency, RAM usage, and CPU load.  All are
 * implemented in portable C that can run on any MCU or be simulated
 * on a desktop.
 *
 * Algorithms:
 *   1. Simple delay         -- blocking, easiest to understand
 *   2. Timer-based          -- non-blocking, most common in production
 *   3. Shift register       -- elegant, fixed-time, deterministic
 *   4. Integrating counter  -- hysteresis-based, noise-resilient
 *
 * Build:  gcc -Wall -Wextra -std=c99 -o debounce 03_debounce.c
 * Run:    ./debounce
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Simulated hardware layer
 *
 * In a real embedded system these would read a GPIO pin and a hardware
 * timer.  Here we feed them from arrays so we can demonstrate the
 * algorithms deterministically.
 * -----------------------------------------------------------------------*/

static const uint8_t sim_signal[] = {
    /* Clean LOW */
    0, 0, 0, 0, 0,
    /* Press event with bounce (bounces for ~5 samples) */
    1, 0, 1, 1, 0, 1, 1, 1,
    /* Stable HIGH */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* Release event with bounce */
    0, 1, 0, 0, 1, 0, 0, 0,
    /* Stable LOW */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* Another press, less bounce */
    1, 0, 1, 1, 1,
    /* Stable HIGH */
    1, 1, 1, 1, 1, 1, 1, 1,
    /* Clean release */
    0, 0, 0, 0, 0
};

#define SIM_LEN  (sizeof(sim_signal) / sizeof(sim_signal[0]))

static uint32_t sim_tick = 0;

/** Simulated "read GPIO pin" */
uint8_t hw_read_pin(void)
{
    if (sim_tick < SIM_LEN)
        return sim_signal[sim_tick];
    return 0;
}

/** Simulated millisecond tick (each call to step = 1ms) */
uint32_t hw_get_tick_ms(void)
{
    return sim_tick;
}

/* -----------------------------------------------------------------------
 * Algorithm 1 : Simple Delay Debounce (blocking)
 *
 * Read the pin, wait N ms, read again.  If both agree, accept the value.
 *
 * Pros:  Dead simple.
 * Cons:  Blocking!  CPU does nothing during the delay.  Unacceptable in
 *        most real-time systems unless used inside an ISR context where
 *        a short spin-wait is tolerable.
 * -----------------------------------------------------------------------*/

typedef struct {
    uint8_t  state;
    uint32_t delay_ms;
} debounce_delay_t;

void debounce_delay_init(debounce_delay_t *d, uint32_t delay_ms)
{
    d->state    = 0;
    d->delay_ms = delay_ms;
}

/**
 * Simulate blocking debounce.  In a real system the delay loop would
 * spin on a hardware timer.  Here we skip ahead by `delay_ms` samples.
 *
 * Returns the debounced value and advances the sim_tick.
 */
uint8_t debounce_delay_read(debounce_delay_t *d)
{
    uint8_t first = hw_read_pin();

    /* Simulate blocking delay by skipping samples */
    uint32_t target = sim_tick + d->delay_ms;
    if (target < SIM_LEN) {
        sim_tick = target;
    }

    uint8_t second = hw_read_pin();

    if (first == second) {
        d->state = second;
    }
    /* If they differ, keep previous stable state */

    return d->state;
}

/* -----------------------------------------------------------------------
 * Algorithm 2 : Timer-Based Debounce (non-blocking)
 *
 * Track the last time the raw input changed.  Only accept a new value
 * after the input has been stable for `debounce_ms` milliseconds.
 *
 * Pros:  Non-blocking, works well in a super-loop or polled architecture.
 * Cons:  Requires a free-running millisecond counter (SysTick, etc.).
 * -----------------------------------------------------------------------*/

typedef struct {
    uint8_t  stable_state;     /* Last accepted (debounced) value       */
    uint8_t  last_raw;         /* Previous raw reading                  */
    uint32_t last_change_time; /* Tick when raw last changed            */
    uint32_t debounce_ms;      /* Required stable duration              */
} debounce_timer_t;

void debounce_timer_init(debounce_timer_t *d, uint32_t debounce_ms)
{
    d->stable_state     = 0;
    d->last_raw         = 0;
    d->last_change_time = 0;
    d->debounce_ms      = debounce_ms;
}

uint8_t debounce_timer_update(debounce_timer_t *d, uint8_t raw, uint32_t now)
{
    if (raw != d->last_raw) {
        /* Input changed -- restart the timer */
        d->last_change_time = now;
        d->last_raw = raw;
    }

    /* Has the input been stable long enough? */
    if ((now - d->last_change_time) >= d->debounce_ms) {
        d->stable_state = d->last_raw;
    }

    return d->stable_state;
}

/* -----------------------------------------------------------------------
 * Algorithm 3 : Shift Register Debounce
 *
 * Shift each new sample into an N-bit register.  The output is HIGH only
 * when all N bits are 1, and LOW only when all N bits are 0.  Otherwise
 * the previous state is held.
 *
 * Pros:  Very elegant, constant time, no timer needed -- just call at
 *        a fixed rate.  Memory-efficient (1 byte for 8 samples).
 * Cons:  Latency = N * sample_period.  Must be called at a regular rate.
 *
 * This is the approach used in Jack Ganssle's famous debouncing article
 * and is popular in production firmware.
 * -----------------------------------------------------------------------*/

typedef struct {
    uint8_t  shift_reg;    /* Rolling window of samples        */
    uint8_t  state;        /* Current debounced output         */
    uint8_t  match_high;   /* Pattern for "confirmed HIGH"     */
    uint8_t  match_low;    /* Pattern for "confirmed LOW"      */
} debounce_shift_t;

/**
 * @param required_samples  Number of consecutive identical samples
 *                          required to change state (1-8).
 */
void debounce_shift_init(debounce_shift_t *d, uint8_t required_samples)
{
    d->shift_reg  = 0x00;
    d->state      = 0;

    /* Build the match masks.  For 4 samples: match_high = 0x0F */
    if (required_samples > 8) required_samples = 8;
    d->match_high = (uint8_t)((1u << required_samples) - 1);
    d->match_low  = 0x00;
}

uint8_t debounce_shift_update(debounce_shift_t *d, uint8_t raw)
{
    d->shift_reg = (uint8_t)((d->shift_reg << 1) | (raw & 1));

    /* Mask to the relevant window width */
    uint8_t masked = d->shift_reg & d->match_high;

    if (masked == d->match_high) {
        d->state = 1;
    } else if (masked == d->match_low) {
        d->state = 0;
    }
    /* Otherwise: still bouncing, hold previous state */

    return d->state;
}

/* -----------------------------------------------------------------------
 * Algorithm 4 : Integrating (Counter) Debounce with Hysteresis
 *
 * Maintain a counter.  Increment on HIGH reading, decrement on LOW.
 * Clamp to [0, max].  Output goes HIGH when counter reaches max, LOW
 * when it reaches 0.  The gap between 0 and max provides hysteresis.
 *
 * Pros:  Excellent noise rejection, configurable hysteresis.
 * Cons:  Slightly more RAM and CPU.  Latency = max * sample_period.
 * -----------------------------------------------------------------------*/

typedef struct {
    uint8_t  counter;
    uint8_t  max_count;
    uint8_t  state;
} debounce_integrating_t;

void debounce_integrating_init(debounce_integrating_t *d, uint8_t max_count)
{
    d->counter   = 0;
    d->max_count = max_count;
    d->state     = 0;
}

uint8_t debounce_integrating_update(debounce_integrating_t *d, uint8_t raw)
{
    if (raw && d->counter < d->max_count) {
        d->counter++;
    } else if (!raw && d->counter > 0) {
        d->counter--;
    }

    if (d->counter >= d->max_count) {
        d->state = 1;
    } else if (d->counter == 0) {
        d->state = 0;
    }
    /* Between 0 and max: hold previous state (hysteresis) */

    return d->state;
}

/* -----------------------------------------------------------------------
 * Section: Edge detection on debounced signal
 *
 * Once you have a clean debounced signal, you often need to detect
 * transitions (press / release events), not just levels.
 * -----------------------------------------------------------------------*/

typedef struct {
    uint8_t prev;
} edge_detect_t;

void edge_detect_init(edge_detect_t *e)
{
    e->prev = 0;
}

/**
 * Returns:
 *   0  = no edge
 *   1  = rising edge  (0 -> 1, button pressed)
 *  -1  = falling edge (1 -> 0, button released)
 */
int8_t edge_detect_update(edge_detect_t *e, uint8_t current)
{
    int8_t result = 0;

    if (current && !e->prev) {
        result = 1;   /* rising */
    } else if (!current && e->prev) {
        result = -1;  /* falling */
    }

    e->prev = current;
    return result;
}

/* -----------------------------------------------------------------------
 * Main : Simulate all four algorithms on the same input signal
 * -----------------------------------------------------------------------*/

int main(void)
{
    printf("=========================================\n");
    printf("  Software Debounce Algorithm Comparison\n");
    printf("=========================================\n\n");

    /* Print the raw signal */
    printf("Raw input signal (%zu samples, 1 sample = 1ms):\n", SIM_LEN);
    printf("  ");
    for (size_t i = 0; i < SIM_LEN; i++) {
        printf("%d", sim_signal[i]);
    }
    printf("\n\n");

    /* --- Algorithm 2: Timer-based --- */
    printf("--- Algorithm: Timer-Based (20ms debounce) ---\n");
    debounce_timer_t tmr;
    debounce_timer_init(&tmr, 5);  /* 5ms for this short sim */

    printf("  Raw: ");
    for (size_t i = 0; i < SIM_LEN; i++) printf("%d", sim_signal[i]);
    printf("\n  Out: ");
    for (size_t i = 0; i < SIM_LEN; i++) {
        printf("%d", debounce_timer_update(&tmr, sim_signal[i], (uint32_t)i));
    }
    printf("\n\n");

    /* --- Algorithm 3: Shift register --- */
    printf("--- Algorithm: Shift Register (4 consecutive samples) ---\n");
    debounce_shift_t shr;
    debounce_shift_init(&shr, 4);

    printf("  Raw: ");
    for (size_t i = 0; i < SIM_LEN; i++) printf("%d", sim_signal[i]);
    printf("\n  Out: ");
    for (size_t i = 0; i < SIM_LEN; i++) {
        printf("%d", debounce_shift_update(&shr, sim_signal[i]));
    }
    printf("\n\n");

    /* --- Algorithm 4: Integrating counter --- */
    printf("--- Algorithm: Integrating Counter (max=5) ---\n");
    debounce_integrating_t intg;
    debounce_integrating_init(&intg, 5);

    printf("  Raw: ");
    for (size_t i = 0; i < SIM_LEN; i++) printf("%d", sim_signal[i]);
    printf("\n  Out: ");
    for (size_t i = 0; i < SIM_LEN; i++) {
        printf("%d", debounce_integrating_update(&intg, sim_signal[i]));
    }
    printf("\n  Cnt: ");
    /* Re-run to show counter values */
    debounce_integrating_init(&intg, 5);
    for (size_t i = 0; i < SIM_LEN; i++) {
        debounce_integrating_update(&intg, sim_signal[i]);
        printf("%d", intg.counter);
    }
    printf("\n\n");

    /* --- Edge detection on debounced signal --- */
    printf("--- Edge Detection on Shift-Register Output ---\n");
    debounce_shift_init(&shr, 4);
    edge_detect_t edge;
    edge_detect_init(&edge);

    printf("  Raw: ");
    for (size_t i = 0; i < SIM_LEN; i++) printf("%d", sim_signal[i]);
    printf("\n  Deb: ");
    uint8_t debounced[SIM_LEN];
    for (size_t i = 0; i < SIM_LEN; i++) {
        debounced[i] = debounce_shift_update(&shr, sim_signal[i]);
        printf("%d", debounced[i]);
    }
    printf("\n  Edg: ");
    for (size_t i = 0; i < SIM_LEN; i++) {
        int8_t e = edge_detect_update(&edge, debounced[i]);
        if (e == 1)       printf("^");  /* rising  */
        else if (e == -1) printf("v");  /* falling */
        else              printf(".");
    }
    printf("\n\n");

    /* --- Comparison summary --- */
    printf("=== Algorithm Comparison ===\n\n");
    printf("  %-22s %-12s %-10s %-10s %s\n",
           "Algorithm", "Blocking?", "RAM (1ch)", "Latency", "Best For");
    printf("  %-22s %-12s %-10s %-10s %s\n",
           "----------------------", "----------", "--------",
           "--------", "----------");
    printf("  %-22s %-12s %-10s %-10s %s\n",
           "Simple Delay", "YES", "2 bytes", "Fixed", "Prototyping");
    printf("  %-22s %-12s %-10s %-10s %s\n",
           "Timer-Based", "No", "6 bytes", "Variable", "General use");
    printf("  %-22s %-12s %-10s %-10s %s\n",
           "Shift Register", "No", "2 bytes", "Fixed", "Periodic polling");
    printf("  %-22s %-12s %-10s %-10s %s\n",
           "Integrating Counter", "No", "3 bytes", "Variable",
           "Noisy environments");
    printf("\n");

    return 0;
}
