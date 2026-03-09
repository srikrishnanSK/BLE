/**
 * Module 04 - Example 02: Templates for Embedded Systems
 *
 * Demonstrates:
 * - Template-based register access (Register<address, width>)
 * - Compile-time pin configuration
 * - Type-safe units (Milliseconds, Microseconds)
 * - Zero runtime overhead template patterns
 *
 * Compile: g++ -std=c++17 -O2 -Wall -Wextra -o 02_templates 02_templates.cpp
 */

#include <cstdint>
#include <cstdio>
#include <cassert>
#include <type_traits>

// ---------------------------------------------------------------------------
// Simulated memory-mapped registers
// ---------------------------------------------------------------------------
namespace hw_sim {

// Simulated memory space (represents memory-mapped I/O region)
static uint8_t memory[4096] = {};

template<typename T>
T read_addr(uint32_t addr) {
    T val{};
    // In real embedded: return *reinterpret_cast<volatile T*>(addr);
    if (addr < sizeof(memory) - sizeof(T)) {
        __builtin_memcpy(&val, &memory[addr], sizeof(T));
    }
    return val;
}

template<typename T>
void write_addr(uint32_t addr, T val) {
    // In real embedded: *reinterpret_cast<volatile T*>(addr) = val;
    if (addr < sizeof(memory) - sizeof(T)) {
        __builtin_memcpy(&memory[addr], &val, sizeof(T));
    }
}

void reset() {
    for (auto& b : memory) b = 0;
}

}  // namespace hw_sim

// ---------------------------------------------------------------------------
// 1. Template-based Register Access
// ---------------------------------------------------------------------------

/**
 * Register<Address, Width> — type-safe, zero-overhead register access.
 *
 * Each instantiation (e.g., Register<0x100, uint32_t>) generates inline
 * functions that compile down to a single load/store instruction.
 */
template<uint32_t Address, typename Width = uint32_t>
class Register {
    static_assert(std::is_unsigned_v<Width>,
                  "Register width must be an unsigned integer type");
public:
    static void write(Width value) {
        hw_sim::write_addr<Width>(Address, value);
    }

    static Width read() {
        return hw_sim::read_addr<Width>(Address);
    }

    static void set_bits(Width mask) {
        write(read() | mask);
    }

    static void clear_bits(Width mask) {
        write(read() & ~mask);
    }

    static void modify(Width clear_mask, Width set_mask) {
        write((read() & ~clear_mask) | set_mask);
    }

    static bool test_bit(uint8_t bit) {
        return (read() & (Width(1) << bit)) != 0;
    }
};

// Type aliases for specific registers (like a real STM32 HAL)
namespace regs {
    // Simulated register addresses
    constexpr uint32_t GPIOA_BASE = 0x100;
    constexpr uint32_t GPIOB_BASE = 0x120;

    using GPIOA_MODER = Register<GPIOA_BASE + 0x00, uint32_t>;  // Mode register
    using GPIOA_ODR   = Register<GPIOA_BASE + 0x04, uint32_t>;  // Output data
    using GPIOA_IDR   = Register<GPIOA_BASE + 0x08, uint32_t>;  // Input data
    using GPIOB_MODER = Register<GPIOB_BASE + 0x00, uint32_t>;
    using GPIOB_ODR   = Register<GPIOB_BASE + 0x04, uint32_t>;

    // 8-bit register example
    using STATUS_REG  = Register<0x200, uint8_t>;
}

// ---------------------------------------------------------------------------
// 2. Compile-Time Pin Configuration
// ---------------------------------------------------------------------------

enum class Port : uint8_t { A = 0, B = 1, C = 2, D = 3 };
enum class PinMode : uint8_t { Input = 0, Output = 1, AltFunc = 2, Analog = 3 };

/**
 * Pin<P, N> — compile-time pin definition.
 * All operations are resolved at compile time. No runtime storage needed.
 */
template<Port P, uint8_t N>
class Pin {
    static_assert(N < 16, "Pin number must be 0-15");

    static constexpr uint32_t port_base() {
        return 0x100 + static_cast<uint32_t>(P) * 0x20;
    }

public:
    static constexpr Port port = P;
    static constexpr uint8_t number = N;

    static void set_mode(PinMode mode) {
        using ModeReg = Register<0, uint32_t>;  // Will use direct access
        uint32_t addr = port_base();
        uint32_t val = hw_sim::read_addr<uint32_t>(addr);
        val &= ~(0x03u << (N * 2));
        val |= (static_cast<uint32_t>(mode) << (N * 2));
        hw_sim::write_addr<uint32_t>(addr, val);
    }

    static void set() {
        uint32_t addr = port_base() + 0x04;  // ODR offset
        hw_sim::write_addr<uint32_t>(addr,
            hw_sim::read_addr<uint32_t>(addr) | (1u << N));
    }

    static void clear() {
        uint32_t addr = port_base() + 0x04;
        hw_sim::write_addr<uint32_t>(addr,
            hw_sim::read_addr<uint32_t>(addr) & ~(1u << N));
    }

    static void toggle() {
        uint32_t addr = port_base() + 0x04;
        hw_sim::write_addr<uint32_t>(addr,
            hw_sim::read_addr<uint32_t>(addr) ^ (1u << N));
    }

    static bool read() {
        uint32_t addr = port_base() + 0x08;  // IDR offset
        return (hw_sim::read_addr<uint32_t>(addr) & (1u << N)) != 0;
    }
};

// Named pins — self-documenting, zero runtime cost
using LED_GREEN  = Pin<Port::A, 5>;
using LED_RED    = Pin<Port::A, 6>;
using BUTTON_USR = Pin<Port::A, 13>;
using SPI_CS     = Pin<Port::B, 0>;

// ---------------------------------------------------------------------------
// 3. Type-Safe Units
// ---------------------------------------------------------------------------

/**
 * Duration<T, Ratio> — type-safe time duration.
 *
 * Prevents mixing milliseconds and microseconds at compile time.
 * Zero runtime overhead — the ratio is a template parameter.
 */
template<typename T, uint32_t RatioToMicroseconds>
class Duration {
public:
    constexpr explicit Duration(T count) : count_(count) {}

    constexpr T count() const { return count_; }

    // Convert to microseconds (common base)
    constexpr uint64_t to_microseconds() const {
        return static_cast<uint64_t>(count_) * RatioToMicroseconds;
    }

    // Arithmetic
    constexpr Duration operator+(Duration other) const {
        return Duration(count_ + other.count_);
    }

    constexpr Duration operator-(Duration other) const {
        return Duration(count_ - other.count_);
    }

    constexpr Duration operator*(T factor) const {
        return Duration(count_ * factor);
    }

    // Comparison
    constexpr bool operator==(Duration other) const { return count_ == other.count_; }
    constexpr bool operator!=(Duration other) const { return count_ != other.count_; }
    constexpr bool operator<(Duration other) const { return count_ < other.count_; }
    constexpr bool operator>(Duration other) const { return count_ > other.count_; }
    constexpr bool operator<=(Duration other) const { return count_ <= other.count_; }
    constexpr bool operator>=(Duration other) const { return count_ >= other.count_; }

private:
    T count_;
};

using Microseconds = Duration<uint32_t, 1>;
using Milliseconds = Duration<uint32_t, 1000>;
using Seconds      = Duration<uint32_t, 1000000>;

// Type-safe delay function — won't accidentally mix units
namespace hal {

void delay_us(Microseconds us) {
    printf("  [Delay] %u microseconds\n", us.count());
    // In real code: configure timer for us.count() microseconds
}

void delay_ms(Milliseconds ms) {
    printf("  [Delay] %u milliseconds (%llu us)\n",
           ms.count(), (unsigned long long)ms.to_microseconds());
}

// This would be a compile error:
// void delay_ms(Microseconds us);  // Different type!

}  // namespace hal

// ---------------------------------------------------------------------------
// 4. Compile-time configuration validation with templates
// ---------------------------------------------------------------------------

template<uint32_t BaudRate>
class UartConfig {
    static_assert(BaudRate >= 1200 && BaudRate <= 4500000,
                  "Baud rate must be between 1200 and 4,500,000");

    static constexpr uint32_t CLOCK_HZ = 16000000;
    static constexpr uint32_t divisor = (CLOCK_HZ + BaudRate / 2) / BaudRate;
    static constexpr uint32_t actual_baud = CLOCK_HZ / divisor;
    static constexpr uint32_t error_permille =
        (actual_baud > BaudRate)
        ? ((actual_baud - BaudRate) * 1000) / BaudRate
        : ((BaudRate - actual_baud) * 1000) / BaudRate;

    static_assert(error_permille < 30,
                  "Baud rate error exceeds 3% — choose a different rate");

public:
    static constexpr uint32_t baud_rate = BaudRate;
    static constexpr uint32_t baud_divisor = divisor;
    static constexpr uint32_t actual_baud_rate = actual_baud;

    static void apply() {
        printf("  UART: requested=%u, divisor=%u, actual=%u, error=%u.%u%%\n",
               baud_rate, baud_divisor, actual_baud_rate,
               error_permille / 10, error_permille % 10);
    }
};

// ---------------------------------------------------------------------------
// 5. Generic circular buffer with template size
// ---------------------------------------------------------------------------

template<typename T, uint32_t Size>
class CircularBuffer {
    static_assert(Size > 0, "Buffer size must be positive");
    static_assert((Size & (Size - 1)) == 0, "Buffer size must be a power of 2");

public:
    constexpr CircularBuffer() : buffer_{}, head_(0), tail_(0), count_(0) {}

    bool push(const T& item) {
        if (count_ >= Size) return false;
        buffer_[head_ & (Size - 1)] = item;
        head_++;
        count_++;
        return true;
    }

    bool pop(T& item) {
        if (count_ == 0) return false;
        item = buffer_[tail_ & (Size - 1)];
        tail_++;
        count_--;
        return true;
    }

    uint32_t size() const { return count_; }
    uint32_t capacity() const { return Size; }
    bool empty() const { return count_ == 0; }
    bool full() const { return count_ >= Size; }

private:
    T buffer_[Size];
    uint32_t head_;
    uint32_t tail_;
    uint32_t count_;
};

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    hw_sim::reset();

    printf("=== 1. Template Register Access ===\n\n");
    {
        regs::GPIOA_ODR::write(0x00000020);  // Set bit 5 (LED on PA5)
        assert(regs::GPIOA_ODR::read() == 0x00000020);
        assert(regs::GPIOA_ODR::test_bit(5) == true);
        assert(regs::GPIOA_ODR::test_bit(4) == false);

        regs::GPIOA_ODR::set_bits(0x00000040);  // Also set bit 6
        assert(regs::GPIOA_ODR::read() == 0x00000060);

        regs::GPIOA_ODR::clear_bits(0x00000020);  // Clear bit 5
        assert(regs::GPIOA_ODR::read() == 0x00000040);

        // 8-bit register
        regs::STATUS_REG::write(0xAB);
        assert(regs::STATUS_REG::read() == 0xAB);

        printf("  Register access tests passed.\n");
    }

    printf("\n=== 2. Compile-Time Pin Configuration ===\n\n");
    {
        LED_GREEN::set_mode(PinMode::Output);
        LED_GREEN::set();
        LED_GREEN::toggle();
        LED_GREEN::toggle();

        SPI_CS::set_mode(PinMode::Output);
        SPI_CS::set();    // CS high (deselect)

        // Verify pin types are distinct at compile time
        static_assert(LED_GREEN::port == Port::A);
        static_assert(LED_GREEN::number == 5);
        static_assert(SPI_CS::port == Port::B);
        static_assert(SPI_CS::number == 0);

        printf("  Compile-time pin configuration tests passed.\n");
    }

    printf("\n=== 3. Type-Safe Units ===\n\n");
    {
        Milliseconds timeout(100);
        Microseconds pulse(500);
        Seconds period(2);

        hal::delay_ms(timeout);
        hal::delay_us(pulse);

        // Arithmetic works within same type
        auto double_timeout = timeout * 2u;
        assert(double_timeout.count() == 200);

        auto combined = timeout + Milliseconds(50);
        assert(combined.count() == 150);

        // Conversions to common base
        assert(timeout.to_microseconds() == 100000);
        assert(pulse.to_microseconds() == 500);
        assert(period.to_microseconds() == 2000000);

        // This would NOT compile — type safety prevents mixing:
        // hal::delay_ms(pulse);  // Error: Microseconds != Milliseconds
        // auto bad = timeout + pulse;  // Error: different types

        printf("  Type-safe unit tests passed.\n");
    }

    printf("\n=== 4. Compile-Time UART Config ===\n\n");
    {
        UartConfig<9600>::apply();
        UartConfig<115200>::apply();
        UartConfig<1000000>::apply();

        // These would fail static_assert at compile time:
        // UartConfig<500>::apply();      // Error: below minimum
        // UartConfig<10000000>::apply();  // Error: above maximum

        static_assert(UartConfig<9600>::baud_divisor > 0);
        printf("  UART config tests passed.\n");
    }

    printf("\n=== 5. Template Circular Buffer ===\n\n");
    {
        CircularBuffer<uint8_t, 8> rx_buffer;

        assert(rx_buffer.empty());
        assert(rx_buffer.capacity() == 8);

        // Fill buffer
        for (uint8_t i = 0; i < 8; i++) {
            assert(rx_buffer.push(i * 10));
        }
        assert(rx_buffer.full());
        assert(!rx_buffer.push(99));  // Buffer full

        // Read back
        uint8_t val;
        for (uint8_t i = 0; i < 8; i++) {
            assert(rx_buffer.pop(val));
            assert(val == i * 10);
        }
        assert(rx_buffer.empty());

        // Wrap-around test
        for (int i = 0; i < 20; i++) {
            rx_buffer.push(static_cast<uint8_t>(i));
            if (rx_buffer.size() > 4) {
                rx_buffer.pop(val);
            }
        }

        printf("  Circular buffer tests passed.\n");
    }

    printf("\n=== All template tests passed! ===\n");
    return 0;
}
