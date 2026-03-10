/**
 * Module 05 - Example 02: Periodic Interrupt Handler
 *
 * Configures TIM3 to generate a 1 ms periodic interrupt.
 * The ISR increments a millisecond tick counter used by the main loop
 * for non-blocking timing.
 *
 * Target: STM32F4 (Cortex-M4)
 * Timer:  TIM3 (16-bit general-purpose timer)
 * Clock:  84 MHz APB1 timer clock
 *
 * Key Concepts:
 *   - Timer update interrupt
 *   - NVIC configuration
 *   - volatile shared variables
 *   - Non-blocking timing in main loop
 */

#include "stm32f4xx.h"

/* --------------------------------------------------------------------------
 * Global Variables (shared between ISR and main)
 *
 * MUST be declared volatile so the compiler does not optimize away reads
 * in the main loop (the value changes asynchronously from the ISR).
 * -------------------------------------------------------------------------- */
volatile uint32_t g_ticks_ms = 0;   /* Millisecond counter              */
volatile uint8_t  g_flag_1s  = 0;   /* Set every 1 second by ISR        */

/* --------------------------------------------------------------------------
 * Timer 3 Interrupt Handler
 *
 * Called every 1 ms by hardware. This name must match the vector table
 * entry defined in the startup file.
 *
 * ISR best practices demonstrated:
 *   1. Clear the interrupt flag FIRST
 *   2. Minimal work -- just increment counter and set flag
 *   3. No blocking calls, no printf, no malloc
 * -------------------------------------------------------------------------- */
void TIM3_IRQHandler(void)
{
    /* Step 1: Check and clear the update interrupt flag */
    if (TIM3->SR & TIM_SR_UIF)
    {
        TIM3->SR &= ~TIM_SR_UIF;   /* Clear flag -- MUST do this! */

        /* Step 2: Increment system tick */
        g_ticks_ms++;

        /* Step 3: Set 1-second flag for main loop */
        if ((g_ticks_ms % 1000) == 0)
        {
            g_flag_1s = 1;
        }
    }
}

/* --------------------------------------------------------------------------
 * Timer 3 Configuration: 1 ms Period
 *
 * Timer clock = 84 MHz (APB1 timer clock on STM32F4)
 * Desired period = 1 ms
 *
 * PSC = 83   -> Timer counts at 84 MHz / 84 = 1 MHz
 * ARR = 999  -> Overflow every 1000 counts = 1 ms
 *
 * Verification: (83+1) * (999+1) / 84,000,000 = 0.001 s = 1 ms
 * -------------------------------------------------------------------------- */
void Timer3_Init(void)
{
    /* Enable TIM3 clock */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* Configure timer */
    TIM3->PSC  = 84 - 1;       /* Prescaler: 1 MHz tick rate         */
    TIM3->ARR  = 1000 - 1;     /* Auto-reload: overflow every 1 ms   */
    TIM3->CNT  = 0;            /* Reset counter                      */
    TIM3->CR1  = 0;            /* Up-counting mode, no other options  */

    /* Enable update interrupt */
    TIM3->DIER |= TIM_DIER_UIE;

    /* Generate update event to load PSC and ARR immediately */
    TIM3->EGR = TIM_EGR_UG;
    TIM3->SR &= ~TIM_SR_UIF;   /* Clear the flag set by EGR */

    /* Configure NVIC for TIM3 */
    NVIC_SetPriority(TIM3_IRQn, 2);   /* Priority 2 (0 = highest) */
    NVIC_EnableIRQ(TIM3_IRQn);

    /* Start the timer */
    TIM3->CR1 |= TIM_CR1_CEN;
}

/* --------------------------------------------------------------------------
 * Utility: Get system tick (milliseconds since init)
 * -------------------------------------------------------------------------- */
uint32_t get_tick(void)
{
    return g_ticks_ms;
}

/* --------------------------------------------------------------------------
 * Utility: Non-blocking delay check
 *
 * Usage:
 *   uint32_t last = get_tick();
 *   ...
 *   if (has_elapsed(last, 500)) {
 *       // 500 ms have passed
 *       last = get_tick();
 *   }
 * -------------------------------------------------------------------------- */
uint8_t has_elapsed(uint32_t start, uint32_t interval_ms)
{
    return ((get_tick() - start) >= interval_ms);
}

/* --------------------------------------------------------------------------
 * Main: Demonstrate periodic interrupt-driven timing
 *
 * - LED toggles every 500 ms using non-blocking check
 * - A second counter is printed (conceptually) every 1 s via flag
 * -------------------------------------------------------------------------- */
int main(void)
{
    uint32_t led_last_toggle = 0;
    uint32_t seconds_count   = 0;

    /* Enable GPIOA clock and configure PA5 as output (on-board LED) */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER &= ~GPIO_MODER_MODER5;
    GPIOA->MODER |= (1U << 10);

    /* Initialize the 1 ms periodic timer */
    Timer3_Init();

    while (1)
    {
        /* Non-blocking LED toggle every 500 ms */
        if (has_elapsed(led_last_toggle, 500))
        {
            led_last_toggle = get_tick();
            GPIOA->ODR ^= GPIO_ODR_OD5;    /* Toggle LED */
        }

        /* Process 1-second flag set by ISR */
        if (g_flag_1s)
        {
            g_flag_1s = 0;      /* Clear the flag */
            seconds_count++;

            /*
             * In a real system, you would transmit seconds_count
             * over UART or update a display here.
             */
            (void)seconds_count;
        }

        /*
         * Main loop is free to do other work here.
         * No CPU cycles are wasted on blocking delays.
         */
    }
}
