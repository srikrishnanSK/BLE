/**
 * @file 06_preprocessor.c
 * @brief Preprocessor directives: macros, guards, parameterized macros.
 *
 * The C preprocessor is heavily used in embedded systems for configuration,
 * hardware abstraction, and conditional compilation. This example covers
 * #define, #ifdef, parameterized macros, and common embedded macro patterns.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o 06_preprocessor 06_preprocessor.c
 * With defines: gcc -DTARGET_STM32 -DDEBUG_ENABLED -o 06_preprocessor 06_preprocessor.c
 */

#include <stdint.h>
#include <stdio.h>

/* =======================================================================
 * Section 1: Simple #define Constants
 * ======================================================================= */

/* Pin definitions — common in every embedded project */
#define LED_RED_PIN     0
#define LED_GREEN_PIN   1
#define LED_BLUE_PIN    2
#define BUTTON_PIN      7

/* Peripheral configuration */
#define UART_BAUD_RATE  115200
#define ADC_RESOLUTION  12
#define SPI_CLOCK_HZ    1000000

/* Buffer sizes */
#define RX_BUFFER_SIZE  64
#define TX_BUFFER_SIZE  128
#define CMD_MAX_LENGTH  32

/* Protocol constants */
#define PACKET_START    0xAA
#define PACKET_END      0x55
#define PACKET_ESCAPE   0x1B

/* =======================================================================
 * Section 2: Parameterized Macros
 * ======================================================================= */

/* Basic math macros (use parentheses around all parameters!) */
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#define ABS(x)          (((x) < 0) ? -(x) : (x))
#define CLAMP(x, lo, hi) (MIN(MAX((x), (lo)), (hi)))

/* Array utility */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Bit manipulation macros — the core of embedded programming */
#define BIT(n)                  (1UL << (n))
#define SET_BIT(reg, n)         ((reg) |= BIT(n))
#define CLEAR_BIT(reg, n)       ((reg) &= ~BIT(n))
#define TOGGLE_BIT(reg, n)      ((reg) ^= BIT(n))
#define CHECK_BIT(reg, n)       (((reg) >> (n)) & 1UL)
#define WRITE_BIT(reg, n, val)  ((val) ? SET_BIT(reg, n) : CLEAR_BIT(reg, n))

/* Bit field macros */
#define BITMASK(width)          ((1UL << (width)) - 1UL)
#define SET_FIELD(reg, shift, width, val) \
    ((reg) = ((reg) & ~(BITMASK(width) << (shift))) | \
             (((uint32_t)(val) & BITMASK(width)) << (shift)))
#define GET_FIELD(reg, shift, width) \
    (((reg) >> (shift)) & BITMASK(width))

/* Alignment macros */
#define ALIGN_UP(x, align)      (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(x, align)    ((x) & ~((align) - 1))
#define IS_ALIGNED(x, align)    (((x) & ((align) - 1)) == 0)

/* Conversion macros */
#define MS_TO_TICKS(ms, freq)   ((uint32_t)(ms) * (uint32_t)(freq) / 1000UL)
#define TICKS_TO_MS(ticks, freq) ((uint32_t)(ticks) * 1000UL / (uint32_t)(freq))

/* =======================================================================
 * Section 3: Conditional Compilation
 * ======================================================================= */

/* Simulate target selection (in practice, defined via compiler flag -D) */
#if !defined(TARGET_STM32) && !defined(TARGET_MSP430) && !defined(TARGET_ARDUINO)
#define TARGET_STM32  /* Default target for this example */
#endif

/* Target-specific configuration */
#ifdef TARGET_STM32
    #define CPU_FREQ_HZ     168000000UL
    #define FLASH_SIZE      (512 * 1024)
    #define RAM_SIZE        (128 * 1024)
    #define TARGET_NAME     "STM32F4"
#elif defined(TARGET_MSP430)
    #define CPU_FREQ_HZ     16000000UL
    #define FLASH_SIZE      (48 * 1024)
    #define RAM_SIZE        (2 * 1024)
    #define TARGET_NAME     "MSP430G2553"
#elif defined(TARGET_ARDUINO)
    #define CPU_FREQ_HZ     16000000UL
    #define FLASH_SIZE      (32 * 1024)
    #define RAM_SIZE        (2 * 1024)
    #define TARGET_NAME     "Arduino Uno (ATmega328P)"
#endif

/* Debug/Release configuration */
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 1  /* Default to debug mode for this example */
#endif

#if DEBUG_ENABLED
    #define DEBUG_PRINT(fmt, ...) \
        printf("[DEBUG %s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...) ((void)0)  /* Compiles to nothing */
#endif

/* =======================================================================
 * Section 4: Stringification and Token Pasting
 * ======================================================================= */

/* Stringify: convert macro argument to string literal */
#define STRINGIFY(x)    #x
#define TOSTRING(x)     STRINGIFY(x)

/* Token pasting: concatenate tokens */
#define CONCAT(a, b)    a##b
#define PORT_REG(port)  CONCAT(GPIO_PORT_, port)

/* Self-documenting register access */
#define REG_OFFSET(name) offsetof(struct peripheral_regs, name)

/* =======================================================================
 * Section 5: Common Embedded Macro Patterns
 * ======================================================================= */

/* Compile-time assertion (C99 version) */
#define STATIC_ASSERT(cond, msg) \
    typedef char static_assert_##msg[(cond) ? 1 : -1]

/* Unused parameter suppression */
#define UNUSED(x)   ((void)(x))

/* Likely/unlikely branch hints (GCC/Clang) */
#ifdef __GNUC__
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
#endif

/* Memory-mapped I/O register access */
#define REG32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))
#define REG16(addr) (*(volatile uint16_t *)(uintptr_t)(addr))
#define REG8(addr)  (*(volatile uint8_t  *)(uintptr_t)(addr))

/* Packed struct (for protocol parsing) */
#ifdef __GNUC__
    #define PACKED __attribute__((packed))
#else
    #define PACKED
#endif

/* Section placement */
#ifdef __GNUC__
    #define SECTION(name) __attribute__((section(name)))
#else
    #define SECTION(name)
#endif

/* Weak function (allows override) */
#ifdef __GNUC__
    #define WEAK __attribute__((weak))
#else
    #define WEAK
#endif

/* =======================================================================
 * Compile-time checks
 * ======================================================================= */
STATIC_ASSERT(sizeof(uint8_t) == 1,  uint8_is_1_byte);
STATIC_ASSERT(sizeof(uint16_t) == 2, uint16_is_2_bytes);
STATIC_ASSERT(sizeof(uint32_t) == 4, uint32_is_4_bytes);
STATIC_ASSERT(RX_BUFFER_SIZE >= 32,  rx_buffer_large_enough);

/* =======================================================================
 * Demonstrations
 * ======================================================================= */
static void demonstrate_constants(void)
{
    printf("\n=== #define Constants ===\n\n");

    printf("Target: %s\n", TARGET_NAME);
    printf("CPU Frequency: %lu Hz\n", (unsigned long)CPU_FREQ_HZ);
    printf("Flash: %lu KB\n", (unsigned long)(FLASH_SIZE / 1024));
    printf("RAM:   %lu KB\n", (unsigned long)(RAM_SIZE / 1024));
    printf("\nPeripheral config:\n");
    printf("  UART baud: %u\n", UART_BAUD_RATE);
    printf("  ADC resolution: %u bits\n", ADC_RESOLUTION);
    printf("  SPI clock: %u Hz\n", SPI_CLOCK_HZ);
    printf("  RX buffer: %u bytes\n", RX_BUFFER_SIZE);
    printf("  TX buffer: %u bytes\n", TX_BUFFER_SIZE);
}

static void demonstrate_parameterized_macros(void)
{
    printf("\n=== Parameterized Macros ===\n\n");

    /* Math macros */
    printf("MIN(3, 7) = %d\n", MIN(3, 7));
    printf("MAX(3, 7) = %d\n", MAX(3, 7));
    printf("ABS(-42)  = %d\n", ABS(-42));
    printf("CLAMP(150, 0, 100) = %d\n", CLAMP(150, 0, 100));
    printf("CLAMP(-10, 0, 100) = %d\n", CLAMP(-10, 0, 100));
    printf("CLAMP(50, 0, 100)  = %d\n", CLAMP(50, 0, 100));

    /* ARRAY_SIZE */
    uint16_t data[] = {10, 20, 30, 40, 50};
    printf("\nARRAY_SIZE(data) = %zu\n", ARRAY_SIZE(data));

    /* Bit manipulation */
    printf("\nBit manipulation macros:\n");
    uint32_t reg = 0x00000000;
    printf("  Initial reg:     0x%08X\n", reg);

    SET_BIT(reg, 3);
    printf("  SET_BIT(reg, 3): 0x%08X\n", reg);

    SET_BIT(reg, 7);
    printf("  SET_BIT(reg, 7): 0x%08X\n", reg);

    CLEAR_BIT(reg, 3);
    printf("  CLEAR_BIT(3):    0x%08X\n", reg);

    TOGGLE_BIT(reg, 7);
    printf("  TOGGLE_BIT(7):   0x%08X\n", reg);

    printf("  CHECK_BIT(7):    %lu\n", CHECK_BIT(reg, 7));

    /* Bit field macros */
    printf("\nBit field macros (3-bit field at bit 8):\n");
    reg = 0;
    SET_FIELD(reg, 8, 3, 5);  /* Set bits [10:8] to 5 (0b101) */
    printf("  SET_FIELD(8, 3, 5): 0x%08X\n", reg);
    printf("  GET_FIELD(8, 3):    %lu\n", GET_FIELD(reg, 8, 3));

    /* Alignment */
    printf("\nAlignment macros:\n");
    printf("  ALIGN_UP(13, 4)   = %lu\n",  (unsigned long)ALIGN_UP(13, 4));
    printf("  ALIGN_DOWN(13, 4) = %lu\n",  (unsigned long)ALIGN_DOWN(13, 4));
    printf("  IS_ALIGNED(16, 4) = %d\n",   IS_ALIGNED(16, 4));
    printf("  IS_ALIGNED(13, 4) = %d\n",   IS_ALIGNED(13, 4));

    /* Timer conversion */
    printf("\nTimer conversions (freq = %lu Hz):\n", (unsigned long)CPU_FREQ_HZ);
    printf("  1ms = %lu ticks\n",   (unsigned long)MS_TO_TICKS(1, CPU_FREQ_HZ));
    printf("  10ms = %lu ticks\n",  (unsigned long)MS_TO_TICKS(10, CPU_FREQ_HZ));
}

static void demonstrate_conditional_compilation(void)
{
    printf("\n=== Conditional Compilation ===\n\n");

    printf("Compiled for: %s\n", TARGET_NAME);

    #ifdef TARGET_STM32
    printf("  STM32-specific code active\n");
    printf("  FPU: available (Cortex-M4F)\n");
    #endif

    #ifdef TARGET_MSP430
    printf("  MSP430-specific code active\n");
    printf("  FPU: not available\n");
    #endif

    #ifdef TARGET_ARDUINO
    printf("  Arduino-specific code active\n");
    printf("  FPU: not available (AVR)\n");
    #endif

    /* Debug messages */
    printf("\nDebug output (DEBUG_ENABLED=%d):\n", DEBUG_ENABLED);
    DEBUG_PRINT("System initialized");
    DEBUG_PRINT("ADC value: %u", 1234);
    DEBUG_PRINT("Temperature: %d.%d C", 23, 7);

    /* Predefined macros */
    printf("\nPredefined compiler macros:\n");
    printf("  __FILE__:     %s\n", __FILE__);
    printf("  __LINE__:     %d\n", __LINE__);
    printf("  __DATE__:     %s\n", __DATE__);
    printf("  __TIME__:     %s\n", __TIME__);
    #ifdef __GNUC__
    printf("  __GNUC__:     %d\n", __GNUC__);
    #endif
    printf("  __STDC__:     %d\n", __STDC__);
}

static void demonstrate_stringify_paste(void)
{
    printf("\n=== Stringification and Token Pasting ===\n\n");

    /* Stringify */
    printf("STRINGIFY(UART_BAUD_RATE) = \"%s\"\n", STRINGIFY(UART_BAUD_RATE));
    printf("TOSTRING(UART_BAUD_RATE)  = \"%s\"\n", TOSTRING(UART_BAUD_RATE));
    printf("  (STRINGIFY gives the macro name, TOSTRING expands first)\n");

    /* Version string from macros */
    #define VERSION_MAJOR 1
    #define VERSION_MINOR 3
    #define VERSION_PATCH 7
    #define VERSION_STRING \
        TOSTRING(VERSION_MAJOR) "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH)

    printf("\nVersion: %s\n", VERSION_STRING);

    /* Token pasting */
    uint32_t GPIO_PORT_A = 0x40020000;
    uint32_t GPIO_PORT_B = 0x40020400;
    printf("\nToken pasting (PORT_REG macro):\n");
    printf("  PORT_REG(A) = 0x%08X\n", PORT_REG(A));
    printf("  PORT_REG(B) = 0x%08X\n", PORT_REG(B));
}

static void demonstrate_macro_pitfalls(void)
{
    printf("\n=== Macro Pitfalls ===\n\n");

    /* Pitfall 1: Missing parentheses */
    #define BAD_DOUBLE(x)  x * 2
    #define GOOD_DOUBLE(x) ((x) * 2)

    printf("Pitfall 1: Missing parentheses\n");
    printf("  BAD_DOUBLE(3 + 1)  = %d (expected 8, got wrong)\n", BAD_DOUBLE(3 + 1));
    printf("  GOOD_DOUBLE(3 + 1) = %d (correct!)\n", GOOD_DOUBLE(3 + 1));

    /* Pitfall 2: Multiple evaluation of arguments */
    printf("\nPitfall 2: Multiple evaluation\n");
    int x = 5;
    printf("  Before MAX: x = %d\n", x);
    int result = MAX(x++, 3);
    printf("  MAX(x++, 3) = %d, x is now %d (x was incremented multiple times!)\n",
           result, x);
    printf("  Use inline functions to avoid this!\n");

    /* Pitfall 3: Semicolons in multi-statement macros */
    printf("\nPitfall 3: Multi-statement macros need do-while(0)\n");

    /* Bad: */
    /* #define SWAP_BAD(a, b) { int t = a; a = b; b = t; } */
    /* Good: */
    #define SWAP_GOOD(a, b) do { int t_ = (a); (a) = (b); (b) = t_; } while(0)

    int p = 10, q = 20;
    SWAP_GOOD(p, q);
    printf("  After SWAP_GOOD(10, 20): p=%d, q=%d\n", p, q);
}

/* ======================================================================= */
int main(void)
{
    printf("=== Module 01, Example 06: Preprocessor ===\n");

    demonstrate_constants();
    demonstrate_parameterized_macros();
    demonstrate_conditional_compilation();
    demonstrate_stringify_paste();
    demonstrate_macro_pitfalls();

    printf("\n=== End of Example ===\n");
    return 0;
}
