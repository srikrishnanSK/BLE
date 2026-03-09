/**
 * Module 04 - Example 04: constexpr for Compile-Time Computation
 *
 * Demonstrates:
 * - Compile-time baud rate register calculation
 * - Lookup table generation at compile time
 * - static_assert for configuration validation
 * - constexpr vs const vs #define
 *
 * Compile: g++ -std=c++17 -O2 -Wall -Wextra -o 04_constexpr 04_constexpr.cpp
 */

#include <cstdint>
#include <cstdio>
#include <cassert>
#include <array>
#include <cmath>

// ---------------------------------------------------------------------------
// 1. Compile-Time Baud Rate Calculation
// ---------------------------------------------------------------------------

/**
 * Calculate UART baud rate divisor at compile time.
 * On real hardware, this saves runtime computation and flash reads.
 */
constexpr uint32_t calc_baud_divisor(uint32_t peripheral_clock, uint32_t baud_rate) {
    return (peripheral_clock + (baud_rate / 2)) / baud_rate;
}

constexpr uint32_t calc_actual_baud(uint32_t peripheral_clock, uint32_t divisor) {
    return peripheral_clock / divisor;
}

constexpr uint32_t calc_baud_error_ppm(uint32_t desired, uint32_t actual) {
    // Parts per million error
    return (actual > desired)
        ? ((actual - desired) * 1000000u) / desired
        : ((desired - actual) * 1000000u) / desired;
}

// All computed at compile time — zero runtime cost
constexpr uint32_t SYS_CLOCK = 72000000;  // 72 MHz

constexpr auto BAUD_9600_DIV   = calc_baud_divisor(SYS_CLOCK, 9600);
constexpr auto BAUD_115200_DIV = calc_baud_divisor(SYS_CLOCK, 115200);

constexpr auto BAUD_9600_ACTUAL   = calc_actual_baud(SYS_CLOCK, BAUD_9600_DIV);
constexpr auto BAUD_115200_ACTUAL = calc_actual_baud(SYS_CLOCK, BAUD_115200_DIV);

constexpr auto BAUD_9600_ERR   = calc_baud_error_ppm(9600, BAUD_9600_ACTUAL);
constexpr auto BAUD_115200_ERR = calc_baud_error_ppm(115200, BAUD_115200_ACTUAL);

// Validate at compile time — if error > 2%, compilation fails
static_assert(BAUD_9600_ERR < 20000, "9600 baud error exceeds 2%");
static_assert(BAUD_115200_ERR < 20000, "115200 baud error exceeds 2%");

// ---------------------------------------------------------------------------
// 2. Compile-Time Timer Configuration
// ---------------------------------------------------------------------------

struct TimerConfig {
    uint32_t prescaler;
    uint32_t period;
    uint32_t actual_freq_hz;
};

constexpr TimerConfig calc_timer_config(uint32_t timer_clock, uint32_t desired_freq_hz) {
    // Find prescaler and period to achieve desired frequency
    // timer_freq = timer_clock / ((prescaler + 1) * (period + 1))
    uint32_t best_prescaler = 0;
    uint32_t best_period = 0;
    uint32_t best_error = UINT32_MAX;

    for (uint32_t psc = 0; psc < 65536; psc++) {
        uint32_t period = (timer_clock / ((psc + 1) * desired_freq_hz)) - 1;
        if (period == 0 || period > 65535) continue;

        uint32_t actual = timer_clock / ((psc + 1) * (period + 1));
        uint32_t error = (actual > desired_freq_hz)
                         ? (actual - desired_freq_hz)
                         : (desired_freq_hz - actual);

        if (error < best_error) {
            best_error = error;
            best_prescaler = psc;
            best_period = period;
            if (error == 0) break;
        }
    }

    uint32_t actual = timer_clock / ((best_prescaler + 1) * (best_period + 1));
    return {best_prescaler, best_period, actual};
}

// Timer configs computed entirely at compile time
constexpr auto TIMER_1KHZ = calc_timer_config(72000000, 1000);
constexpr auto TIMER_50HZ = calc_timer_config(72000000, 50);   // Servo PWM

static_assert(TIMER_1KHZ.actual_freq_hz == 1000, "1kHz timer config is exact");
static_assert(TIMER_50HZ.actual_freq_hz == 50, "50Hz timer config is exact");

// ---------------------------------------------------------------------------
// 3. Compile-Time Lookup Tables
// ---------------------------------------------------------------------------

/**
 * Generate a CRC-8 lookup table at compile time.
 * In traditional C, this would require a runtime init function or a
 * pre-generated table. With constexpr, it's computed by the compiler.
 */
constexpr uint8_t crc8_byte(uint8_t data, uint8_t polynomial = 0x07) {
    uint8_t crc = data;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ polynomial;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

constexpr auto generate_crc8_table() {
    std::array<uint8_t, 256> table{};
    for (int i = 0; i < 256; i++) {
        table[i] = crc8_byte(static_cast<uint8_t>(i));
    }
    return table;
}

// Table lives in flash (ROM), generated at compile time
constexpr auto CRC8_TABLE = generate_crc8_table();

// CRC function using the compile-time table
uint8_t crc8(const uint8_t* data, uint32_t len) {
    uint8_t crc = 0x00;
    for (uint32_t i = 0; i < len; i++) {
        crc = CRC8_TABLE[crc ^ data[i]];
    }
    return crc;
}

// ---------------------------------------------------------------------------
// Sine lookup table for DAC output (compile-time)
// ---------------------------------------------------------------------------

constexpr auto generate_sine_table() {
    constexpr int TABLE_SIZE = 64;
    std::array<uint16_t, TABLE_SIZE> table{};
    for (int i = 0; i < TABLE_SIZE; i++) {
        // sin value mapped to 12-bit DAC range (0-4095)
        // Using a polynomial approximation for constexpr compatibility
        double angle = 2.0 * 3.14159265358979 * i / TABLE_SIZE;
        // Normalize sin to 0.0 - 1.0 range, then scale to DAC range
        double normalized = (std::sin(angle) + 1.0) / 2.0;
        table[i] = static_cast<uint16_t>(normalized * 4095.0);
    }
    return table;
}

constexpr auto SINE_TABLE = generate_sine_table();

// ---------------------------------------------------------------------------
// 4. static_assert for Configuration Validation
// ---------------------------------------------------------------------------

// System configuration constants
constexpr uint32_t STACK_SIZE = 2048;
constexpr uint32_t HEAP_SIZE = 0;  // No heap in this system
constexpr uint32_t RAM_SIZE = 8192;
constexpr uint32_t NUM_TASKS = 4;
constexpr uint32_t TASK_STACK = STACK_SIZE / NUM_TASKS;

// Validate configuration at compile time
static_assert(STACK_SIZE <= RAM_SIZE, "Stack exceeds available RAM");
static_assert(STACK_SIZE + HEAP_SIZE <= RAM_SIZE, "Stack + Heap exceeds RAM");
static_assert(STACK_SIZE % 8 == 0, "Stack size must be 8-byte aligned");
static_assert(NUM_TASKS > 0 && NUM_TASKS <= 16, "Task count out of range");
static_assert(TASK_STACK >= 256, "Per-task stack too small (min 256 bytes)");

// Validate struct layout for hardware compatibility
struct __attribute__((packed)) RegisterBlock {
    uint32_t CR1;
    uint32_t CR2;
    uint32_t SR;
    uint32_t DR;
    uint32_t BRR;
};

static_assert(sizeof(RegisterBlock) == 20, "RegisterBlock must be 20 bytes");
static_assert(offsetof(RegisterBlock, SR) == 8, "SR must be at offset 8");
static_assert(offsetof(RegisterBlock, DR) == 12, "DR must be at offset 12");

// ---------------------------------------------------------------------------
// 5. constexpr vs const vs #define
// ---------------------------------------------------------------------------

// #define — preprocessor text replacement (no type, no scope, no debugging)
#define MAX_RETRIES_DEFINE 3

// const — runtime constant (may be computed at runtime, stored in RAM)
const int max_retries_const = 3;

// constexpr — guaranteed compile-time constant (stored in flash/ROM)
constexpr int MAX_RETRIES_CONSTEXPR = 3;

// constexpr function — computed at compile time when inputs are constexpr
constexpr uint32_t power_of_two(uint8_t n) {
    return 1u << n;
}

static_assert(power_of_two(0) == 1);
static_assert(power_of_two(3) == 8);
static_assert(power_of_two(10) == 1024);

// ---------------------------------------------------------------------------
// 6. Compile-time string hashing (for switch on strings)
// ---------------------------------------------------------------------------

constexpr uint32_t fnv1a_hash(const char* str) {
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= static_cast<uint32_t>(*str++);
        hash *= 16777619u;
    }
    return hash;
}

// Verify hash uniqueness at compile time
static_assert(fnv1a_hash("GPIO") != fnv1a_hash("UART"));
static_assert(fnv1a_hash("GPIO") != fnv1a_hash("SPI"));
static_assert(fnv1a_hash("UART") != fnv1a_hash("SPI"));

const char* identify_peripheral(const char* name) {
    switch (fnv1a_hash(name)) {
        case fnv1a_hash("GPIO"): return "General Purpose I/O";
        case fnv1a_hash("UART"): return "Universal Async Receiver/Transmitter";
        case fnv1a_hash("SPI"):  return "Serial Peripheral Interface";
        case fnv1a_hash("I2C"):  return "Inter-Integrated Circuit";
        default: return "Unknown peripheral";
    }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    printf("=== 1. Compile-Time Baud Rate Calculation ===\n\n");
    printf("  System clock: %u Hz\n", SYS_CLOCK);
    printf("  9600 baud:   divisor=%u, actual=%u Hz, error=%u ppm\n",
           BAUD_9600_DIV, BAUD_9600_ACTUAL, BAUD_9600_ERR);
    printf("  115200 baud: divisor=%u, actual=%u Hz, error=%u ppm\n",
           BAUD_115200_DIV, BAUD_115200_ACTUAL, BAUD_115200_ERR);

    printf("\n=== 2. Compile-Time Timer Configuration ===\n\n");
    printf("  1kHz timer: prescaler=%u, period=%u, actual=%u Hz\n",
           TIMER_1KHZ.prescaler, TIMER_1KHZ.period, TIMER_1KHZ.actual_freq_hz);
    printf("  50Hz timer:  prescaler=%u, period=%u, actual=%u Hz\n",
           TIMER_50HZ.prescaler, TIMER_50HZ.period, TIMER_50HZ.actual_freq_hz);

    assert(TIMER_1KHZ.actual_freq_hz == 1000);
    assert(TIMER_50HZ.actual_freq_hz == 50);

    printf("\n=== 3. Compile-Time CRC Table ===\n\n");
    printf("  CRC8 table (first 16 entries):\n  ");
    for (int i = 0; i < 16; i++) {
        printf("0x%02X ", CRC8_TABLE[i]);
    }
    printf("\n");

    // Test CRC computation
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t crc_val = crc8(test_data, sizeof(test_data));
    printf("  CRC8({01,02,03,04}) = 0x%02X\n", crc_val);

    // Verify CRC table values at compile time
    static_assert(CRC8_TABLE[0] == 0x00);
    static_assert(CRC8_TABLE[1] == 0x07);

    printf("\n=== 4. Compile-Time Sine Table ===\n\n");
    printf("  Sine table (64 entries, 12-bit DAC values):\n  ");
    for (int i = 0; i < 16; i++) {
        printf("%4u ", SINE_TABLE[i]);
    }
    printf("...\n");
    // Sine at 0 degrees should be midpoint (2047-2048)
    assert(SINE_TABLE[0] >= 2040 && SINE_TABLE[0] <= 2055);
    // Sine at 90 degrees (quarter table) should be max (4095)
    assert(SINE_TABLE[16] >= 4090);

    printf("\n=== 5. Configuration Validation ===\n\n");
    printf("  Stack size:    %u bytes\n", STACK_SIZE);
    printf("  Heap size:     %u bytes\n", HEAP_SIZE);
    printf("  RAM size:      %u bytes\n", RAM_SIZE);
    printf("  Tasks:         %u\n", NUM_TASKS);
    printf("  Stack/task:    %u bytes\n", TASK_STACK);
    printf("  RegisterBlock: %zu bytes\n", sizeof(RegisterBlock));
    printf("  All static_asserts passed at compile time!\n");

    printf("\n=== 6. Compile-Time String Hashing ===\n\n");
    printf("  GPIO -> %s\n", identify_peripheral("GPIO"));
    printf("  UART -> %s\n", identify_peripheral("UART"));
    printf("  SPI  -> %s\n", identify_peripheral("SPI"));
    printf("  I2C  -> %s\n", identify_peripheral("I2C"));
    printf("  XYZ  -> %s\n", identify_peripheral("XYZ"));

    assert(fnv1a_hash("GPIO") == fnv1a_hash("GPIO"));  // Deterministic

    printf("\n=== All constexpr tests passed! ===\n");
    return 0;
}
