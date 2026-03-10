/**
 * Module 02 - Example 04: Signal Edge Detector
 *
 * A comprehensive edge detection system that identifies rising edges,
 * falling edges, and measures pulse widths from a simulated digital
 * signal.  This is the foundation for building a logic analyzer,
 * decoding serial protocols, and measuring PWM duty cycles.
 *
 * Concepts covered:
 *   - Rising / falling edge detection
 *   - Pulse width measurement
 *   - Frequency estimation from edge timing
 *   - Glitch filtering (minimum pulse width)
 *   - ASCII timing diagram output
 *
 * Build:  gcc -Wall -Wextra -std=c99 -o edge_detector 04_edge_detector.c
 * Run:    ./edge_detector
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Edge detector core
 * -----------------------------------------------------------------------*/

typedef enum {
    EDGE_NONE    = 0,
    EDGE_RISING  = 1,
    EDGE_FALLING = 2
} edge_type_t;

typedef struct {
    uint8_t    prev_state;       /* Previous sample value              */
    uint32_t   last_edge_time;   /* Timestamp of last detected edge    */
    edge_type_t last_edge_type;  /* Type of last detected edge         */
    uint32_t   min_pulse_width;  /* Glitch filter: minimum pulse (ticks) */
    bool       glitch_filter_en; /* Enable glitch filtering?           */
} edge_detector_t;

void edge_detector_init(edge_detector_t *ed, uint32_t min_pulse_width)
{
    ed->prev_state       = 0;
    ed->last_edge_time   = 0;
    ed->last_edge_type   = EDGE_NONE;
    ed->min_pulse_width  = min_pulse_width;
    ed->glitch_filter_en = (min_pulse_width > 0);
}

/**
 * Process one sample.  Returns the edge type detected (or EDGE_NONE).
 *
 * @param ed      Edge detector state
 * @param sample  Current digital sample (0 or 1)
 * @param tick    Current time in ticks
 * @return        Type of edge detected at this sample
 */
edge_type_t edge_detector_update(edge_detector_t *ed,
                                  uint8_t sample, uint32_t tick)
{
    edge_type_t detected = EDGE_NONE;

    if (sample != ed->prev_state) {
        /* An edge occurred */
        uint32_t pulse_width = tick - ed->last_edge_time;

        /* Apply glitch filter */
        if (ed->glitch_filter_en && pulse_width < ed->min_pulse_width) {
            /* Pulse too short -- treat as glitch, ignore */
            /* Don't update prev_state: pretend the edge didn't happen */
            return EDGE_NONE;
        }

        if (sample && !ed->prev_state) {
            detected = EDGE_RISING;
        } else if (!sample && ed->prev_state) {
            detected = EDGE_FALLING;
        }

        ed->last_edge_time = tick;
        ed->last_edge_type = detected;
        ed->prev_state = sample;
    }

    return detected;
}

/* -----------------------------------------------------------------------
 * Pulse width measurement
 * -----------------------------------------------------------------------*/

typedef struct {
    uint32_t high_start;      /* Tick when pulse went high        */
    uint32_t low_start;       /* Tick when pulse went low         */
    uint32_t last_high_width; /* Duration of last HIGH pulse      */
    uint32_t last_low_width;  /* Duration of last LOW pulse       */
    uint32_t last_period;     /* Duration of last full cycle      */
    bool     measuring_high;  /* Currently in HIGH state?         */
    bool     valid;           /* At least one complete cycle done */
} pulse_meter_t;

void pulse_meter_init(pulse_meter_t *pm)
{
    memset(pm, 0, sizeof(*pm));
}

/**
 * Feed an edge event into the pulse meter to compute widths.
 */
void pulse_meter_update(pulse_meter_t *pm, edge_type_t edge, uint32_t tick)
{
    if (edge == EDGE_RISING) {
        if (pm->measuring_high) {
            /* Shouldn't happen (two rising edges in a row) */
            return;
        }
        pm->last_low_width = tick - pm->low_start;
        pm->last_period    = tick - pm->high_start;
        pm->high_start     = tick;
        pm->measuring_high = true;

        if (pm->low_start > 0) {
            pm->valid = true;
        }
    } else if (edge == EDGE_FALLING) {
        if (!pm->measuring_high && pm->high_start > 0) {
            return;
        }
        pm->last_high_width = tick - pm->high_start;
        pm->low_start       = tick;
        pm->measuring_high  = false;
    }
}

/**
 * Compute duty cycle as a percentage (0-100).
 * Requires at least one complete cycle.
 */
float pulse_meter_duty_cycle(const pulse_meter_t *pm)
{
    if (!pm->valid || pm->last_period == 0) return 0.0f;
    return (float)pm->last_high_width / (float)pm->last_period * 100.0f;
}

/* -----------------------------------------------------------------------
 * ASCII timing diagram renderer
 * -----------------------------------------------------------------------*/

/**
 * Print an ASCII timing diagram for a signal buffer.
 *
 * Output format:
 *           _____       ___
 *   SIG: __|     |_____|   |____
 *         0 1 2 3 4 5 6 7 8 9 ...
 */
void print_timing_diagram(const char *label, const uint8_t *signal,
                          size_t length)
{
    /* Top line: high segments */
    printf("  %6s: ", "");
    for (size_t i = 0; i < length; i++) {
        if (signal[i]) {
            printf("__");
        } else {
            printf("  ");
        }
    }
    printf("\n");

    /* Signal line: edges and levels */
    printf("  %6s: ", label);
    for (size_t i = 0; i < length; i++) {
        uint8_t curr = signal[i];
        uint8_t prev = (i > 0) ? signal[i - 1] : 0;

        if (curr && !prev) {
            /* Rising edge */
            printf("|~");
        } else if (!curr && prev) {
            /* Falling edge */
            printf("|_");
        } else if (curr) {
            printf("__");
        } else {
            printf("__");
        }
    }
    printf("\n");

    /* Time ruler */
    printf("  %6s: ", "tick");
    for (size_t i = 0; i < length; i++) {
        if (i % 5 == 0) {
            printf("%-2zu", i);
        } else {
            printf("  ");
        }
    }
    printf("\n");
}

/**
 * Higher-quality timing diagram with distinct high/low levels.
 */
void print_waveform(const char *label, const uint8_t *signal,
                    size_t length)
{
    /* High rail */
    printf("        ");
    for (size_t i = 0; i < length; i++) {
        uint8_t curr = signal[i];
        uint8_t prev = (i > 0) ? signal[i - 1] : 0;

        if (curr && prev)       printf("___");
        else if (curr && !prev) printf(" __");
        else                    printf("   ");
    }
    printf("\n");

    /* Transition line */
    printf("  %4s: ", label);
    for (size_t i = 0; i < length; i++) {
        uint8_t curr = signal[i];
        uint8_t prev = (i > 0) ? signal[i - 1] : 0;

        if (curr && !prev)       printf("|  ");  /* rising edge  */
        else if (!curr && prev)  printf("|  ");  /* falling edge */
        else                     printf("   ");
    }
    printf("\n");

    /* Low rail */
    printf("        ");
    for (size_t i = 0; i < length; i++) {
        uint8_t curr = signal[i];
        uint8_t prev = (i > 0) ? signal[i - 1] : 0;

        if (!curr && !prev)      printf("___");
        else if (!curr && prev)  printf(" __");
        else                     printf("   ");
    }
    printf("\n");
}

/* -----------------------------------------------------------------------
 * Edge event log
 * -----------------------------------------------------------------------*/

#define MAX_EVENTS 64

typedef struct {
    uint32_t    tick;
    edge_type_t type;
} edge_event_t;

typedef struct {
    edge_event_t events[MAX_EVENTS];
    int          count;
} edge_log_t;

void edge_log_init(edge_log_t *log)
{
    log->count = 0;
}

void edge_log_add(edge_log_t *log, uint32_t tick, edge_type_t type)
{
    if (log->count < MAX_EVENTS) {
        log->events[log->count].tick = tick;
        log->events[log->count].type = type;
        log->count++;
    }
}

void edge_log_print(const edge_log_t *log)
{
    printf("  %-6s  %-10s  %s\n", "Tick", "Type", "Annotation");
    printf("  %-6s  %-10s  %s\n", "------", "----------", "----------");
    for (int i = 0; i < log->count; i++) {
        const char *type_str = (log->events[i].type == EDGE_RISING)
                                ? "RISING"
                                : "FALLING";
        /* Compute time since previous edge */
        uint32_t delta = 0;
        if (i > 0) {
            delta = log->events[i].tick - log->events[i - 1].tick;
        }
        printf("  %-6u  %-10s  ", log->events[i].tick, type_str);
        if (i > 0) {
            printf("dt=%u ticks since last edge", delta);
        } else {
            printf("(first edge)");
        }
        printf("\n");
    }
}

/* -----------------------------------------------------------------------
 * Main : Run detector on test signals
 * -----------------------------------------------------------------------*/

int main(void)
{
    printf("==========================================\n");
    printf("  Signal Edge Detector & Pulse Analyzer\n");
    printf("==========================================\n\n");

    /* --- Test signal 1: simple square wave --- */
    printf("--- Test 1: Square Wave (period=10, duty=50%%) ---\n\n");

    uint8_t square_wave[60];
    for (int i = 0; i < 60; i++) {
        square_wave[i] = (i % 10) < 5 ? 1 : 0;
    }

    print_waveform("SQ", square_wave, 40);
    printf("\n");

    edge_detector_t det;
    edge_detector_init(&det, 0);
    pulse_meter_t pm;
    pulse_meter_init(&pm);
    edge_log_t log;
    edge_log_init(&log);

    for (int i = 0; i < 60; i++) {
        edge_type_t e = edge_detector_update(&det, square_wave[i], (uint32_t)i);
        if (e != EDGE_NONE) {
            pulse_meter_update(&pm, e, (uint32_t)i);
            edge_log_add(&log, (uint32_t)i, e);
        }
    }

    printf("Edge Event Log:\n");
    edge_log_print(&log);
    printf("\nPulse measurements:\n");
    printf("  High width:  %u ticks\n", pm.last_high_width);
    printf("  Low width:   %u ticks\n", pm.last_low_width);
    printf("  Period:      %u ticks\n", pm.last_period);
    printf("  Duty cycle:  %.1f%%\n", pulse_meter_duty_cycle(&pm));

    /* --- Test signal 2: PWM with 75% duty --- */
    printf("\n--- Test 2: PWM Signal (period=8, duty=75%%) ---\n\n");

    uint8_t pwm_signal[48];
    for (int i = 0; i < 48; i++) {
        pwm_signal[i] = (i % 8) < 6 ? 1 : 0;  /* 6/8 = 75% */
    }

    print_waveform("PWM", pwm_signal, 40);
    printf("\n");

    edge_detector_init(&det, 0);
    pulse_meter_init(&pm);
    edge_log_init(&log);

    for (int i = 0; i < 48; i++) {
        edge_type_t e = edge_detector_update(&det, pwm_signal[i], (uint32_t)i);
        if (e != EDGE_NONE) {
            pulse_meter_update(&pm, e, (uint32_t)i);
            edge_log_add(&log, (uint32_t)i, e);
        }
    }

    printf("Edge Event Log:\n");
    edge_log_print(&log);
    printf("\nPulse measurements:\n");
    printf("  High width:  %u ticks\n", pm.last_high_width);
    printf("  Low width:   %u ticks\n", pm.last_low_width);
    printf("  Period:      %u ticks\n", pm.last_period);
    printf("  Duty cycle:  %.1f%%\n", pulse_meter_duty_cycle(&pm));

    /* --- Test signal 3: noisy signal with glitch filter --- */
    printf("\n--- Test 3: Noisy Signal + Glitch Filter ---\n\n");

    uint8_t noisy[] = {
        0,0,0,0,0, 1,0,1, 1,1,1,1,1,1,1,1,  /* glitch during rise */
        0,1,0, 0,0,0,0,0,0,0,0,               /* glitch during fall */
        1,1,1,1,1, 0,0,0,0,0                   /* clean transitions  */
    };
    size_t noisy_len = sizeof(noisy);

    printf("Without glitch filter:\n");
    print_waveform("RAW", noisy, noisy_len);
    printf("\n");

    edge_detector_init(&det, 0);  /* No filter */
    edge_log_init(&log);
    for (size_t i = 0; i < noisy_len; i++) {
        edge_type_t e = edge_detector_update(&det, noisy[i], (uint32_t)i);
        if (e != EDGE_NONE) {
            edge_log_add(&log, (uint32_t)i, e);
        }
    }
    printf("Unfiltered edges: %d detected\n", log.count);
    edge_log_print(&log);

    printf("\nWith glitch filter (min_pulse = 3 ticks):\n");
    edge_detector_init(&det, 3);  /* Filter pulses < 3 ticks */
    edge_log_init(&log);
    for (size_t i = 0; i < noisy_len; i++) {
        edge_type_t e = edge_detector_update(&det, noisy[i], (uint32_t)i);
        if (e != EDGE_NONE) {
            edge_log_add(&log, (uint32_t)i, e);
        }
    }
    printf("Filtered edges: %d detected\n", log.count);
    edge_log_print(&log);

    /* --- Test signal 4: Frequency measurement --- */
    printf("\n--- Test 4: Frequency Measurement ---\n\n");
    printf("  If sample rate = 1 MHz (1 tick = 1 us):\n");

    struct {
        const char *name;
        uint32_t    period_ticks;
    } freq_tests[] = {
        {"1 kHz",    1000},
        {"10 kHz",   100},
        {"100 kHz",  10},
        {"500 kHz",  2},
    };

    for (int t = 0; t < 4; t++) {
        uint32_t period = freq_tests[t].period_ticks;
        uint32_t half   = period / 2;

        edge_detector_init(&det, 0);
        pulse_meter_init(&pm);

        /* Generate 10 cycles */
        for (uint32_t i = 0; i < period * 10; i++) {
            uint8_t sample = (i % period) < half ? 1 : 0;
            edge_type_t e = edge_detector_update(&det, sample, i);
            if (e != EDGE_NONE) {
                pulse_meter_update(&pm, e, i);
            }
        }

        float freq_hz = 1000000.0f / (float)pm.last_period;
        printf("  %-8s  period=%4u ticks  measured_freq=%.0f Hz  duty=%.1f%%\n",
               freq_tests[t].name, pm.last_period, freq_hz,
               pulse_meter_duty_cycle(&pm));
    }

    printf("\n");
    return 0;
}
