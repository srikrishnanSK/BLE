/**
 * @file 03_control_flow.c
 * @brief Control flow constructs: if/else, switch-case, for, while, do-while.
 *
 * Demonstrates control flow in the context of embedded systems: command parsing
 * with switch-case, sensor polling with while loops, and menu systems.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o 03_control_flow 03_control_flow.c
 */

#include <stdint.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Simulated hardware for demonstration
 * ----------------------------------------------------------------------- */
static volatile uint16_t sim_adc_value = 0;
static volatile uint8_t  sim_button    = 0;

/* Thresholds for a simulated temperature monitor */
#define TEMP_LOW      100
#define TEMP_NORMAL   500
#define TEMP_HIGH     800
#define TEMP_CRITICAL 950
#define ADC_MAX       1023

/* Command codes (as if received over UART) */
#define CMD_NOP       0x00
#define CMD_LED_ON    0x01
#define CMD_LED_OFF   0x02
#define CMD_READ_ADC  0x03
#define CMD_SET_PWM   0x04
#define CMD_RESET     0xFF

/* =======================================================================
 * Section 1: if / else if / else
 * ======================================================================= */
static void demonstrate_if_else(void)
{
    printf("\n=== if / else if / else ===\n\n");

    /* Simulate checking a temperature sensor via ADC */
    uint16_t test_values[] = {50, 300, 650, 900, 1000};
    int num_values = sizeof(test_values) / sizeof(test_values[0]);

    for (int i = 0; i < num_values; i++) {
        uint16_t adc = test_values[i];
        printf("ADC = %4u: ", adc);

        if (adc >= TEMP_CRITICAL) {
            printf("CRITICAL - Emergency shutdown!\n");
        } else if (adc >= TEMP_HIGH) {
            printf("HIGH - Activate cooling fan\n");
        } else if (adc >= TEMP_NORMAL) {
            printf("NORMAL - System OK\n");
        } else if (adc >= TEMP_LOW) {
            printf("LOW - Heating may be needed\n");
        } else {
            printf("VERY LOW - Check sensor connection\n");
        }
    }

    /* Embedded pattern: Ternary operator for compact flag checks */
    printf("\nTernary operator examples:\n");
    uint8_t gpio_pin = 1;
    printf("  LED is %s\n", gpio_pin ? "ON" : "OFF");

    uint8_t error_code = 0;
    printf("  Status: %s\n", error_code == 0 ? "OK" : "ERROR");
}

/* =======================================================================
 * Section 2: switch-case (Command Parser)
 * ======================================================================= */
static void handle_command(uint8_t cmd)
{
    switch (cmd) {
        case CMD_LED_ON:
            printf("  -> LED turned ON\n");
            break;

        case CMD_LED_OFF:
            printf("  -> LED turned OFF\n");
            break;

        case CMD_READ_ADC:
            printf("  -> ADC value: %u\n", sim_adc_value);
            break;

        case CMD_SET_PWM:
            printf("  -> PWM duty cycle updated\n");
            break;

        case CMD_RESET:
            printf("  -> System RESET requested\n");
            break;

        case CMD_NOP:
            /* No operation — intentional fall-through to default */
            /* fall through */

        default:
            printf("  -> Unknown command: 0x%02X\n", cmd);
            break;
    }
}

static void demonstrate_switch_case(void)
{
    printf("\n=== switch-case (Command Parser) ===\n\n");

    uint8_t commands[] = {CMD_LED_ON, CMD_READ_ADC, CMD_SET_PWM,
                          CMD_LED_OFF, CMD_RESET, 0x42, CMD_NOP};
    int num_cmds = sizeof(commands) / sizeof(commands[0]);

    sim_adc_value = 512;

    for (int i = 0; i < num_cmds; i++) {
        printf("Received command 0x%02X:\n", commands[i]);
        handle_command(commands[i]);
    }

    /* Switch with fall-through (intentional) */
    printf("\nSwitch with intentional fall-through (severity levels):\n");
    uint8_t severity = 2;  /* 0=info, 1=warning, 2=error, 3=fatal */

    printf("Severity %u actions:\n", severity);
    switch (severity) {
        case 3:  /* Fatal */
            printf("  [FATAL] Halt system\n");
            /* fall through */
        case 2:  /* Error */
            printf("  [ERROR] Log to flash\n");
            /* fall through */
        case 1:  /* Warning */
            printf("  [WARN]  Activate warning LED\n");
            /* fall through */
        case 0:  /* Info */
            printf("  [INFO]  Record in buffer\n");
            break;
    }
}

/* =======================================================================
 * Section 3: Menu System (switch-case + while loop)
 * ======================================================================= */
static void demonstrate_menu_system(void)
{
    printf("\n=== Menu System (Simulated UART) ===\n\n");

    /* In a real system, this would read from UART. We simulate it. */
    char menu_choices[] = {'1', '2', '3', '1', 'q'};
    int num_choices = sizeof(menu_choices) / sizeof(menu_choices[0]);
    int choice_idx = 0;

    printf("=== Embedded Device Menu ===\n");
    printf("1. Read sensor\n");
    printf("2. Toggle LED\n");
    printf("3. Show status\n");
    printf("q. Quit\n\n");

    uint8_t running = 1;
    while (running && choice_idx < num_choices) {
        char choice = menu_choices[choice_idx++];
        printf("Selection: %c\n", choice);

        switch (choice) {
            case '1':
                printf("  Sensor value: %u\n", (uint16_t)42);
                break;
            case '2':
                sim_button ^= 1;
                printf("  LED toggled to %s\n", sim_button ? "ON" : "OFF");
                break;
            case '3':
                printf("  System uptime: 12345 ms\n");
                printf("  Temperature: 23.7 C\n");
                break;
            case 'q':
            case 'Q':
                printf("  Shutting down...\n");
                running = 0;
                break;
            default:
                printf("  Invalid option\n");
                break;
        }
    }
}

/* =======================================================================
 * Section 4: for loops
 * ======================================================================= */
static void demonstrate_for_loops(void)
{
    printf("\n=== for Loops ===\n\n");

    /* Basic counting */
    printf("Counting up (0 to 7):\n  ");
    for (uint8_t i = 0; i < 8; i++) {
        printf("%u ", i);
    }
    printf("\n");

    /* Counting down (common for timeout loops) */
    printf("\nCountdown (timeout):\n  ");
    for (int8_t i = 5; i >= 0; i--) {
        printf("%d ", i);
    }
    printf("TIMEOUT!\n");

    /* Iterate over array */
    printf("\nIterating sensor readings:\n");
    uint16_t readings[] = {100, 250, 300, 275, 310, 290};
    int count = sizeof(readings) / sizeof(readings[0]);
    uint32_t sum = 0;

    for (int i = 0; i < count; i++) {
        sum += readings[i];
        printf("  readings[%d] = %u\n", i, readings[i]);
    }
    printf("  Average: %u\n", (uint16_t)(sum / count));

    /* Nested loops: initialize a 2D buffer */
    printf("\nInitializing 3x4 buffer:\n");
    uint8_t buffer[3][4];
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 4; col++) {
            buffer[row][col] = (uint8_t)(row * 4 + col);
        }
    }
    for (int row = 0; row < 3; row++) {
        printf("  Row %d: ", row);
        for (int col = 0; col < 4; col++) {
            printf("%3u ", buffer[row][col]);
        }
        printf("\n");
    }

    /* Stepping by 2 (skip alternate pins) */
    printf("\nStepping by 2 (even pins):\n  ");
    for (uint8_t pin = 0; pin < 16; pin += 2) {
        printf("Pin%u ", pin);
    }
    printf("\n");
}

/* =======================================================================
 * Section 5: while and do-while loops
 * ======================================================================= */
static void demonstrate_while_loops(void)
{
    printf("\n=== while and do-while Loops ===\n\n");

    /* while loop: polling for a condition */
    printf("Polling simulation (wait for ADC ready):\n");
    uint8_t status = 0;
    uint8_t attempts = 0;

    while (!(status & 0x01)) {
        attempts++;
        printf("  Attempt %u: status = 0x%02X (not ready)\n", attempts, status);
        if (attempts >= 3) {
            status = 0x01;  /* Simulate becoming ready */
        }
    }
    printf("  Ready after %u attempts!\n", attempts);

    /* do-while: execute at least once */
    printf("\ndo-while: Read until valid data:\n");
    uint8_t data;
    uint8_t read_count = 0;
    uint8_t sim_data[] = {0xFF, 0xFF, 0x42};  /* First two are invalid */

    do {
        data = sim_data[read_count];
        printf("  Read: 0x%02X%s\n", data, (data == 0xFF) ? " (invalid)" : " (valid!)");
        read_count++;
    } while (data == 0xFF && read_count < sizeof(sim_data));

    /* Infinite loop pattern (embedded main loop) */
    printf("\nEmbedded super-loop pattern (3 iterations shown):\n");
    uint32_t tick = 0;
    while (1) {
        printf("  Tick %u: process sensors, update outputs\n", tick);
        tick++;
        if (tick >= 3) {
            printf("  (Exiting demo loop)\n");
            break;
        }
    }
}

/* =======================================================================
 * Section 6: break and continue
 * ======================================================================= */
static void demonstrate_break_continue(void)
{
    printf("\n=== break and continue ===\n\n");

    /* break: exit loop early when target found */
    printf("Search for value 42 in array:\n");
    uint8_t data[] = {10, 20, 30, 42, 50, 60};
    int found_idx = -1;

    for (int i = 0; i < 6; i++) {
        printf("  Checking index %d: value = %u", i, data[i]);
        if (data[i] == 42) {
            found_idx = i;
            printf(" -> FOUND!\n");
            break;
        }
        printf("\n");
    }
    printf("  Result: %s at index %d\n",
           found_idx >= 0 ? "Found" : "Not found", found_idx);

    /* continue: skip invalid readings */
    printf("\nFilter invalid sensor readings (skip 0xFFFF):\n");
    uint16_t readings[] = {100, 0xFFFF, 250, 0xFFFF, 0xFFFF, 300, 275};
    int valid_count = 0;
    uint32_t sum = 0;

    for (int i = 0; i < 7; i++) {
        if (readings[i] == 0xFFFF) {
            printf("  [%d] = 0xFFFF (skipped)\n", i);
            continue;  /* Skip to next iteration */
        }
        printf("  [%d] = %u (valid)\n", i, readings[i]);
        sum += readings[i];
        valid_count++;
    }
    if (valid_count > 0) {
        printf("  Average of valid readings: %u\n", (uint16_t)(sum / valid_count));
    }
}

/* ======================================================================= */
int main(void)
{
    printf("=== Module 01, Example 03: Control Flow ===\n");

    demonstrate_if_else();
    demonstrate_switch_case();
    demonstrate_menu_system();
    demonstrate_for_loops();
    demonstrate_while_loops();
    demonstrate_break_continue();

    printf("\n=== End of Example ===\n");
    return 0;
}
