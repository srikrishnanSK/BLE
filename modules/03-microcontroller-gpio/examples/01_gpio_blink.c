/**
 * Module 03 - Example 01: GPIO LED Blink (Simulated)
 *
 * Demonstrates blinking an LED using direct register access on an
 * STM32F4-style GPIO peripheral.  The hardware registers are simulated
 * with volatile uint32_t arrays so this program compiles and runs on a
 * desktop with gcc -- no real hardware required.
 *
 * Concepts covered:
 *   - Memory-mapped I/O (volatile pointers)
 *   - RCC clock enable
 *   - MODER register (pin mode selection)
 *   - ODR vs BSRR for setting/clearing outputs
 *   - Simple busy-wait delay
 *
 * Build:  gcc -Wall -Wextra -std=c99 -o gpio_blink 01_gpio_blink.c
 * Run:    ./gpio_blink
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Section 1 : Simulated Hardware Registers
 *
 * On real hardware these would be fixed addresses in the peripheral
 * memory region (0x4000_0000 - 0x5FFF_FFFF).  Here we use plain arrays
 * and access them through pointers, preserving the volatile semantics
 * that are critical in embedded C.
 * ----------------------------------------------------------------------- */

/* GPIO register offsets (index into uint32_t array) */
#define GPIO_MODER_IDX    0   /* 0x00  Mode register           */
#define GPIO_OTYPER_IDX   1   /* 0x04  Output type             */
#define GPIO_OSPEEDR_IDX  2   /* 0x08  Output speed            */
#define GPIO_PUPDR_IDX    3   /* 0x0C  Pull-up / pull-down     */
#define GPIO_IDR_IDX      4   /* 0x10  Input data (read-only)  */
#define GPIO_ODR_IDX      5   /* 0x14  Output data             */
#define GPIO_BSRR_IDX    6   /* 0x18  Bit set/reset           */
#define GPIO_LCKR_IDX     7   /* 0x1C  Lock register           */
#define GPIO_AFRL_IDX     8   /* 0x20  Alternate function low  */
#define GPIO_AFRH_IDX     9   /* 0x24  Alternate function high */
#define GPIO_REG_COUNT   10

/* Simulated GPIO port A registers */
static volatile uint32_t GPIOA_REGS[GPIO_REG_COUNT];

/* Simulated RCC AHB1ENR register (peripheral clock enable) */
static volatile uint32_t RCC_AHB1ENR;

/* MODER field values (2 bits per pin) */
#define GPIO_MODE_INPUT   0x00U
#define GPIO_MODE_OUTPUT  0x01U
#define GPIO_MODE_AF      0x02U
#define GPIO_MODE_ANALOG  0x03U

/* Helper: pointer to a simulated GPIO port (mimics GPIO_TypeDef*) */
typedef volatile uint32_t *GPIO_Port;

#define SIM_GPIOA  ((GPIO_Port)GPIOA_REGS)

/* -----------------------------------------------------------------------
 * Section 2 : Clock Enable
 *
 * On STM32F4 the GPIO clocks live on the AHB1 bus.  Each port has a
 * dedicated enable bit in RCC->AHB1ENR:
 *   Bit 0 = GPIOA, Bit 1 = GPIOB, ..., Bit 7 = GPIOH
 *
 * Without enabling the clock, all register reads return 0 and writes
 * are silently ignored.
 * ----------------------------------------------------------------------- */

#define RCC_AHB1ENR_GPIOAEN  (1U << 0)

static void rcc_enable_gpioa(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    printf("[RCC] GPIOA clock enabled (AHB1ENR = 0x%08X)\n",
           (unsigned)RCC_AHB1ENR);
}

/* -----------------------------------------------------------------------
 * Section 3 : GPIO Configuration Helpers
 * ----------------------------------------------------------------------- */

/**
 * Configure a pin's mode (input / output / AF / analog).
 *
 * MODER uses 2 bits per pin:
 *   Bits [2n+1 : 2n] for pin n.
 */
static void gpio_set_mode(GPIO_Port port, uint32_t pin, uint32_t mode)
{
    uint32_t pos = pin * 2;
    port[GPIO_MODER_IDX] &= ~(0x3U << pos);   /* Clear the 2-bit field */
    port[GPIO_MODER_IDX] |=  (mode  << pos);   /* Set new mode          */
}

/**
 * Set a pin HIGH using the BSRR register (atomic, single write).
 *
 * BSRR bits [15:0]  = BS (Bit Set)   -- writing 1 sets the ODR bit
 * BSRR bits [31:16] = BR (Bit Reset) -- writing 1 clears the ODR bit
 */
static void gpio_set_pin(GPIO_Port port, uint32_t pin)
{
    port[GPIO_BSRR_IDX] = (1U << pin);

    /* Simulate the effect on ODR (real hardware does this automatically) */
    port[GPIO_ODR_IDX] |= (1U << pin);
}

/**
 * Set a pin LOW using the BSRR register (atomic, single write).
 */
static void gpio_clear_pin(GPIO_Port port, uint32_t pin)
{
    port[GPIO_BSRR_IDX] = (1U << (pin + 16));

    /* Simulate the effect on ODR */
    port[GPIO_ODR_IDX] &= ~(1U << pin);
}

/**
 * Toggle a pin using read-modify-write on ODR.
 *
 * NOTE: On real hardware this is NOT atomic.  An interrupt between the
 * read and the write could cause a race condition.  For interrupt-safe
 * toggling, read ODR, compute new value, and use BSRR to set/clear.
 */
static void gpio_toggle_pin(GPIO_Port port, uint32_t pin)
{
    port[GPIO_ODR_IDX] ^= (1U << pin);
}

/**
 * Read the current output state of a pin from ODR.
 */
static bool gpio_read_output(GPIO_Port port, uint32_t pin)
{
    return (port[GPIO_ODR_IDX] & (1U << pin)) != 0;
}

/* -----------------------------------------------------------------------
 * Section 4 : Simulated Delay
 *
 * On real hardware we would either use a timer interrupt, SysTick, or
 * a calibrated busy-wait loop.  Here we just iterate to simulate the
 * concept of a blocking delay.
 * ----------------------------------------------------------------------- */

static void delay_simulated(volatile uint32_t count)
{
    while (count--) {
        /* Burn CPU cycles -- the volatile qualifier prevents the
         * compiler from optimising this loop away. */
    }
}

/* -----------------------------------------------------------------------
 * Section 5 : Main -- LED Blink on PA5
 *
 * PA5 is the user LED on STM32 Nucleo-F446RE boards.
 * ----------------------------------------------------------------------- */

#define LED_PIN  5

int main(void)
{
    printf("=== GPIO LED Blink (Simulated STM32F4) ===\n\n");

    /* Step 1: Enable the GPIOA peripheral clock */
    rcc_enable_gpioa();

    /* Step 2: Configure PA5 as general-purpose output (MODER = 01) */
    gpio_set_mode(SIM_GPIOA, LED_PIN, GPIO_MODE_OUTPUT);
    printf("[GPIO] PA%u configured as output (MODER = 0x%08X)\n\n",
           LED_PIN, (unsigned)SIM_GPIOA[GPIO_MODER_IDX]);

    /* Step 3: Blink loop */
    printf("--- Blink Loop (8 toggles) ---\n");
    for (int i = 0; i < 8; i++) {
        gpio_toggle_pin(SIM_GPIOA, LED_PIN);
        printf("  Cycle %d: PA%u = %s  (ODR = 0x%04X)\n",
               i, LED_PIN,
               gpio_read_output(SIM_GPIOA, LED_PIN) ? "HIGH (LED ON)" : "LOW  (LED OFF)",
               (unsigned)(SIM_GPIOA[GPIO_ODR_IDX] & 0xFFFF));

        delay_simulated(100000);
    }

    /* Step 4: Demonstrate BSRR set/clear */
    printf("\n--- Using BSRR (Atomic Set/Clear) ---\n");

    gpio_set_pin(SIM_GPIOA, LED_PIN);
    printf("  BSRR set:   PA%u = %s  (ODR = 0x%04X)\n",
           LED_PIN,
           gpio_read_output(SIM_GPIOA, LED_PIN) ? "HIGH" : "LOW",
           (unsigned)(SIM_GPIOA[GPIO_ODR_IDX] & 0xFFFF));

    gpio_clear_pin(SIM_GPIOA, LED_PIN);
    printf("  BSRR clear: PA%u = %s  (ODR = 0x%04X)\n",
           LED_PIN,
           gpio_read_output(SIM_GPIOA, LED_PIN) ? "HIGH" : "LOW",
           (unsigned)(SIM_GPIOA[GPIO_ODR_IDX] & 0xFFFF));

    printf("\n=== Blink complete ===\n");
    return 0;
}
