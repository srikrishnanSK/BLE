/**
 * Module 03 - Example 02: GPIO Input -- Reading a Button
 *
 * Demonstrates reading a push-button connected to a GPIO input pin,
 * with pull-up/pull-down configuration and basic software debouncing.
 * All registers are simulated so the program compiles on a desktop.
 *
 * Concepts covered:
 *   - Input mode (MODER = 00)
 *   - PUPDR register (internal pull-up / pull-down)
 *   - IDR register (reading pin state)
 *   - Active-low vs active-high button wiring
 *   - Simple debounce by repeated sampling
 *
 * Build:  gcc -Wall -Wextra -std=c99 -o gpio_input 02_gpio_input.c
 * Run:    ./gpio_input
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Section 1 : Simulated Hardware Registers
 * ----------------------------------------------------------------------- */

#define GPIO_MODER_IDX    0
#define GPIO_OTYPER_IDX   1
#define GPIO_OSPEEDR_IDX  2
#define GPIO_PUPDR_IDX    3
#define GPIO_IDR_IDX      4
#define GPIO_ODR_IDX      5
#define GPIO_BSRR_IDX    6
#define GPIO_REG_COUNT   10

static volatile uint32_t GPIOA_REGS[GPIO_REG_COUNT];
static volatile uint32_t GPIOC_REGS[GPIO_REG_COUNT];
static volatile uint32_t RCC_AHB1ENR;

typedef volatile uint32_t *GPIO_Port;

#define SIM_GPIOA  ((GPIO_Port)GPIOA_REGS)
#define SIM_GPIOC  ((GPIO_Port)GPIOC_REGS)

/* Mode values */
#define GPIO_MODE_INPUT   0x00U
#define GPIO_MODE_OUTPUT  0x01U

/* Pull-up/pull-down values */
#define GPIO_PUPD_NONE     0x00U
#define GPIO_PUPD_PULLUP   0x01U
#define GPIO_PUPD_PULLDOWN 0x02U

/* RCC enable bits */
#define RCC_AHB1ENR_GPIOAEN  (1U << 0)
#define RCC_AHB1ENR_GPIOCEN  (1U << 2)

/* -----------------------------------------------------------------------
 * Section 2 : GPIO Configuration Functions
 * ----------------------------------------------------------------------- */

static void gpio_set_mode(GPIO_Port port, uint32_t pin, uint32_t mode)
{
    uint32_t pos = pin * 2;
    port[GPIO_MODER_IDX] &= ~(0x3U << pos);
    port[GPIO_MODER_IDX] |=  (mode  << pos);
}

/**
 * Configure pull-up / pull-down resistor for a pin.
 *
 * PUPDR uses 2 bits per pin:
 *   00 = No pull    (floating -- external resistor needed)
 *   01 = Pull-up    (~40 kOhm to VDD on STM32F4)
 *   10 = Pull-down  (~40 kOhm to GND)
 *   11 = Reserved
 */
static void gpio_set_pull(GPIO_Port port, uint32_t pin, uint32_t pull)
{
    uint32_t pos = pin * 2;
    port[GPIO_PUPDR_IDX] &= ~(0x3U << pos);
    port[GPIO_PUPDR_IDX] |=  (pull  << pos);
}

/**
 * Read a single input pin from IDR.
 *
 * IDR (Input Data Register) is read-only on real hardware.
 * Each bit reflects the current electrical state of the corresponding pin.
 */
static bool gpio_read_pin(GPIO_Port port, uint32_t pin)
{
    return (port[GPIO_IDR_IDX] & (1U << pin)) != 0;
}

/**
 * Read the full 16-bit IDR value.
 */
static uint16_t gpio_read_port(GPIO_Port port)
{
    return (uint16_t)(port[GPIO_IDR_IDX] & 0xFFFFU);
}

/* -----------------------------------------------------------------------
 * Section 3 : Simulated Button Press Injection
 *
 * Since we have no real hardware, we inject button state directly into
 * the IDR register.  On real hardware the pin voltage would determine
 * the IDR bit automatically.
 * ----------------------------------------------------------------------- */

static void sim_set_button(GPIO_Port port, uint32_t pin, bool pressed,
                           bool active_low)
{
    /*
     * Active-low button: pressed => pin LOW (0), released => pin HIGH (1)
     * Active-high button: pressed => pin HIGH (1), released => pin LOW (0)
     */
    bool level = active_low ? !pressed : pressed;

    if (level) {
        port[GPIO_IDR_IDX] |=  (1U << pin);
    } else {
        port[GPIO_IDR_IDX] &= ~(1U << pin);
    }
}

/* -----------------------------------------------------------------------
 * Section 4 : Simple Debounce
 *
 * Mechanical switches bounce for 5-20 ms when pressed or released.
 * A simple approach: read the pin multiple times with short delays
 * and only accept the value if all readings agree.
 * ----------------------------------------------------------------------- */

#define DEBOUNCE_SAMPLES  5

/**
 * Debounced read: returns the stable pin state.
 *
 * In simulation we always get the same value, but this demonstrates
 * the algorithm used on real hardware.
 */
static bool gpio_read_debounced(GPIO_Port port, uint32_t pin)
{
    uint32_t count = 0;

    for (int i = 0; i < DEBOUNCE_SAMPLES; i++) {
        if (gpio_read_pin(port, pin)) {
            count++;
        }
        /* On real hardware: delay ~1 ms here */
    }

    /* Majority vote */
    return count > (DEBOUNCE_SAMPLES / 2);
}

/* -----------------------------------------------------------------------
 * Section 5 : LED control helpers (output on PA5)
 * ----------------------------------------------------------------------- */

#define LED_PIN    5
#define BUTTON_PIN 13   /* PC13 = user button on Nucleo boards */

static void gpio_set_pin(GPIO_Port port, uint32_t pin)
{
    port[GPIO_BSRR_IDX] = (1U << pin);
    port[GPIO_ODR_IDX] |= (1U << pin);
}

static void gpio_clear_pin(GPIO_Port port, uint32_t pin)
{
    port[GPIO_BSRR_IDX] = (1U << (pin + 16));
    port[GPIO_ODR_IDX] &= ~(1U << pin);
}

/* -----------------------------------------------------------------------
 * Section 6 : Main
 * ----------------------------------------------------------------------- */

int main(void)
{
    printf("=== GPIO Input: Button Reading (Simulated STM32F4) ===\n\n");

    /* Enable clocks for GPIOA (LED) and GPIOC (button) */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    printf("[RCC] GPIOA and GPIOC clocks enabled\n");

    /* Configure PA5 as output (LED) */
    gpio_set_mode(SIM_GPIOA, LED_PIN, GPIO_MODE_OUTPUT);
    printf("[GPIO] PA%u: output mode (LED)\n", LED_PIN);

    /* Configure PC13 as input with pull-up (button, active-low) */
    gpio_set_mode(SIM_GPIOC, BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull(SIM_GPIOC, BUTTON_PIN, GPIO_PUPD_PULLUP);
    printf("[GPIO] PC%u: input mode, pull-up enabled (button)\n", BUTTON_PIN);

    printf("       MODER = 0x%08X, PUPDR = 0x%08X\n\n",
           (unsigned)SIM_GPIOC[GPIO_MODER_IDX],
           (unsigned)SIM_GPIOC[GPIO_PUPDR_IDX]);

    /* --- Demonstrate active-low button reading --- */
    printf("--- Active-LOW Button (PC13) ---\n");
    printf("    Wiring: VDD -> [pull-up] -> pin -> [button] -> GND\n");
    printf("    Button released = pin HIGH (1)\n");
    printf("    Button pressed  = pin LOW  (0)\n\n");

    /* Simulate: button released (active-low => IDR bit = 1) */
    sim_set_button(SIM_GPIOC, BUTTON_PIN, false, true);
    bool state = gpio_read_debounced(SIM_GPIOC, BUTTON_PIN);
    printf("  Button released: IDR bit %u = %u => %s\n",
           BUTTON_PIN, state ? 1 : 0,
           state ? "NOT pressed" : "PRESSED");

    /* LED mirrors button (LED off when button released) */
    if (!state) {   /* active-low: LOW = pressed */
        gpio_set_pin(SIM_GPIOA, LED_PIN);
    } else {
        gpio_clear_pin(SIM_GPIOA, LED_PIN);
    }
    printf("  LED (PA%u) = %s\n\n",
           LED_PIN,
           (SIM_GPIOA[GPIO_ODR_IDX] & (1U << LED_PIN)) ? "ON" : "OFF");

    /* Simulate: button pressed */
    sim_set_button(SIM_GPIOC, BUTTON_PIN, true, true);
    state = gpio_read_debounced(SIM_GPIOC, BUTTON_PIN);
    printf("  Button pressed:  IDR bit %u = %u => %s\n",
           BUTTON_PIN, state ? 1 : 0,
           state ? "NOT pressed" : "PRESSED");

    if (!state) {
        gpio_set_pin(SIM_GPIOA, LED_PIN);
    } else {
        gpio_clear_pin(SIM_GPIOA, LED_PIN);
    }
    printf("  LED (PA%u) = %s\n\n",
           LED_PIN,
           (SIM_GPIOA[GPIO_ODR_IDX] & (1U << LED_PIN)) ? "ON" : "OFF");

    /* --- Demonstrate full port read --- */
    printf("--- Full Port Read (IDR) ---\n");

    /* Inject a pattern into GPIOC IDR to simulate multiple inputs */
    SIM_GPIOC[GPIO_IDR_IDX] = 0x00A5U;  /* Some pins high, some low */
    printf("  GPIOC IDR = 0x%04X\n", gpio_read_port(SIM_GPIOC));
    printf("  Individual pins:\n");
    for (int pin = 0; pin < 16; pin++) {
        if (gpio_read_pin(SIM_GPIOC, (uint32_t)pin)) {
            printf("    PC%-2d = HIGH\n", pin);
        }
    }

    /* --- Demonstrate pull-down configuration --- */
    printf("\n--- Pull-Down Configuration ---\n");
    printf("    Used for active-HIGH buttons: pin -> [button] -> VDD\n");
    printf("    Pin pulled LOW when button is released\n");

    gpio_set_pull(SIM_GPIOC, 0, GPIO_PUPD_PULLDOWN);
    printf("  PC0 PUPDR = pull-down (PUPDR = 0x%08X)\n",
           (unsigned)SIM_GPIOC[GPIO_PUPDR_IDX]);

    /* Simulate active-high button on PC0 */
    sim_set_button(SIM_GPIOC, 0, false, false);
    printf("  PC0 released: %s\n",
           gpio_read_pin(SIM_GPIOC, 0) ? "HIGH" : "LOW");

    sim_set_button(SIM_GPIOC, 0, true, false);
    printf("  PC0 pressed:  %s\n",
           gpio_read_pin(SIM_GPIOC, 0) ? "HIGH" : "LOW");

    printf("\n=== GPIO Input demo complete ===\n");
    return 0;
}
