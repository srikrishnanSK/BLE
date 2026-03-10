/**
 * Module 05 - Example 01: Microsecond Delay Using Timer
 *
 * Demonstrates how to use a hardware timer for precise microsecond-level
 * delays without relying on software loop counting (which varies with
 * compiler optimization and clock speed).
 *
 * Target: STM32F4 (adaptable to any Cortex-M with timer peripheral)
 * Timer:  TIM2 (32-bit general-purpose timer)
 * Clock:  84 MHz APB1 timer clock
 */

#include "stm32f4xx.h"

/* --------------------------------------------------------------------------
 * Timer Initialization for Microsecond Counting
 *
 * Configure TIM2 to count at 1 MHz (1 tick = 1 microsecond).
 *
 * Timer Frequency = APB1_Timer_Clock / (PSC + 1)
 * 1,000,000 Hz    = 84,000,000 / (83 + 1)
 * -------------------------------------------------------------------------- */
void Timer_Init(void)
{
    /* Enable TIM2 clock on APB1 bus */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* Set prescaler: 84 MHz / 84 = 1 MHz (1 us per tick) */
    TIM2->PSC = 84 - 1;

    /* Set auto-reload to maximum (32-bit timer: 0xFFFFFFFF) */
    TIM2->ARR = 0xFFFFFFFF;

    /* Reset counter */
    TIM2->CNT = 0;

    /* Generate update event to load prescaler immediately */
    TIM2->EGR = TIM_EGR_UG;

    /* Clear update flag caused by EGR write */
    TIM2->SR &= ~TIM_SR_UIF;

    /* Enable the timer */
    TIM2->CR1 |= TIM_CR1_CEN;
}

/* --------------------------------------------------------------------------
 * Microsecond Delay (Blocking)
 *
 * Uses the running timer to create a precise delay.
 * Maximum delay: ~4294 seconds (32-bit counter at 1 MHz)
 *
 * How it works:
 *   1. Read current counter value
 *   2. Calculate target = current + delay
 *   3. Wait until counter reaches target
 *   4. Handles 32-bit overflow correctly via unsigned subtraction
 * -------------------------------------------------------------------------- */
void delay_us(uint32_t us)
{
    uint32_t start = TIM2->CNT;

    /*
     * Unsigned subtraction handles wrap-around automatically:
     *   If start = 0xFFFFFFF0 and us = 0x20
     *   target would "overflow" but (CNT - start) still works
     *   because unsigned subtraction wraps correctly.
     */
    while ((TIM2->CNT - start) < us)
    {
        /* Wait -- the timer hardware does all the work */
    }
}

/* --------------------------------------------------------------------------
 * Millisecond Delay (built on microsecond delay)
 * -------------------------------------------------------------------------- */
void delay_ms(uint32_t ms)
{
    while (ms--)
    {
        delay_us(1000);
    }
}

/* --------------------------------------------------------------------------
 * Get Elapsed Microseconds
 *
 * Returns elapsed time since a given start timestamp.
 * Useful for non-blocking timeout checks.
 * -------------------------------------------------------------------------- */
uint32_t elapsed_us(uint32_t start)
{
    return TIM2->CNT - start;
}

/* --------------------------------------------------------------------------
 * Get Current Timestamp (microseconds)
 * -------------------------------------------------------------------------- */
uint32_t micros(void)
{
    return TIM2->CNT;
}

/* --------------------------------------------------------------------------
 * Example: Measure execution time of a function
 * -------------------------------------------------------------------------- */
void measure_execution_time(void)
{
    uint32_t start, elapsed;

    start = micros();

    /* -- Code to measure goes here -- */
    volatile uint32_t sum = 0;
    for (volatile int i = 0; i < 1000; i++)
    {
        sum += i;
    }
    /* -- End of code to measure -- */

    elapsed = micros() - start;

    /*
     * 'elapsed' now contains the execution time in microseconds.
     * In a real system you would send this over UART or display it.
     */
    (void)elapsed;  /* Prevent unused variable warning */
}

/* --------------------------------------------------------------------------
 * Main: Blink LED with precise 500ms on / 500ms off
 * -------------------------------------------------------------------------- */
int main(void)
{
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Configure PA5 (on-board LED) as output */
    GPIOA->MODER &= ~GPIO_MODER_MODER5;
    GPIOA->MODER |= (1U << 10);  /* Output mode */

    /* Initialize the microsecond timer */
    Timer_Init();

    while (1)
    {
        GPIOA->ODR ^= GPIO_ODR_OD5;   /* Toggle LED */
        delay_ms(500);                  /* Wait 500 ms */
    }
}
