/**
 * Module 04 - Example 01: Classes and RAII in Embedded C++
 *
 * Demonstrates:
 * - GPIO class with constructor/destructor for pin lifecycle
 * - RAII pattern for SPI transaction locking
 * - Deterministic resource cleanup
 *
 * Compile: g++ -std=c++17 -O2 -Wall -Wextra -o 01_classes_raii 01_classes_raii.cpp
 */

#include <cstdint>
#include <cstdio>
#include <cassert>

// ---------------------------------------------------------------------------
// Simulated hardware registers (in real embedded, these are memory-mapped)
// ---------------------------------------------------------------------------
namespace hw_sim {

static uint32_t gpio_mode_reg[8]   = {};  // 8 ports
static uint32_t gpio_output_reg[8] = {};
static uint32_t gpio_input_reg[8]  = {};
static bool     spi_locked         = false;
static int      spi_lock_count     = 0;
static int      gpio_init_count    = 0;
static int      gpio_deinit_count  = 0;

void reset() {
    for (auto& r : gpio_mode_reg) r = 0;
    for (auto& r : gpio_output_reg) r = 0;
    for (auto& r : gpio_input_reg) r = 0;
    spi_locked = false;
    spi_lock_count = 0;
    gpio_init_count = 0;
    gpio_deinit_count = 0;
}

}  // namespace hw_sim

// ---------------------------------------------------------------------------
// Pin configuration types
// ---------------------------------------------------------------------------
enum class PinMode : uint8_t {
    Input       = 0x00,
    Output      = 0x01,
    AlternateFunction = 0x02,
    Analog      = 0x03
};

enum class PinState : uint8_t {
    Low  = 0,
    High = 1
};

// ---------------------------------------------------------------------------
// GpioPin class — RAII for GPIO pin lifecycle
// ---------------------------------------------------------------------------
class GpioPin {
public:
    /**
     * Constructor: initializes the GPIO pin.
     * In real hardware, this would configure mode registers, enable clocks, etc.
     */
    GpioPin(uint8_t port, uint8_t pin, PinMode mode)
        : port_(port), pin_(pin), mode_(mode), initialized_(true)
    {
        assert(port < 8 && "Port must be 0-7");
        assert(pin < 32 && "Pin must be 0-31");

        // Set mode bits (2 bits per pin in mode register)
        uint32_t mode_val = static_cast<uint32_t>(mode);
        uint32_t shift = pin * 2;
        hw_sim::gpio_mode_reg[port] &= ~(0x03u << shift);
        hw_sim::gpio_mode_reg[port] |= (mode_val << shift);

        hw_sim::gpio_init_count++;
        printf("[GPIO] Pin P%d.%d initialized as %s\n",
               port, pin, mode_name(mode));
    }

    // Disable copy — a GPIO pin is a unique hardware resource
    GpioPin(const GpioPin&) = delete;
    GpioPin& operator=(const GpioPin&) = delete;

    // Enable move — transfer ownership of pin
    GpioPin(GpioPin&& other) noexcept
        : port_(other.port_), pin_(other.pin_),
          mode_(other.mode_), initialized_(other.initialized_)
    {
        other.initialized_ = false;  // Prevent other's destructor from resetting
    }

    GpioPin& operator=(GpioPin&& other) noexcept {
        if (this != &other) {
            if (initialized_) deinit();
            port_ = other.port_;
            pin_ = other.pin_;
            mode_ = other.mode_;
            initialized_ = other.initialized_;
            other.initialized_ = false;
        }
        return *this;
    }

    /**
     * Destructor: resets the GPIO pin to default (input) state.
     * This is the "R" in RAII — deterministic cleanup.
     */
    ~GpioPin() {
        if (initialized_) {
            deinit();
        }
    }

    void set() {
        assert(initialized_ && mode_ == PinMode::Output);
        hw_sim::gpio_output_reg[port_] |= (1u << pin_);
    }

    void clear() {
        assert(initialized_ && mode_ == PinMode::Output);
        hw_sim::gpio_output_reg[port_] &= ~(1u << pin_);
    }

    void toggle() {
        assert(initialized_ && mode_ == PinMode::Output);
        hw_sim::gpio_output_reg[port_] ^= (1u << pin_);
    }

    PinState read() const {
        assert(initialized_);
        uint32_t reg = (mode_ == PinMode::Output)
                       ? hw_sim::gpio_output_reg[port_]
                       : hw_sim::gpio_input_reg[port_];
        return (reg & (1u << pin_)) ? PinState::High : PinState::Low;
    }

    void write(PinState state) {
        if (state == PinState::High) set();
        else clear();
    }

    uint8_t port() const { return port_; }
    uint8_t pin() const { return pin_; }
    bool is_initialized() const { return initialized_; }

private:
    uint8_t port_;
    uint8_t pin_;
    PinMode mode_;
    bool initialized_;

    void deinit() {
        // Reset pin to input mode (safe default)
        uint32_t shift = pin_ * 2;
        hw_sim::gpio_mode_reg[port_] &= ~(0x03u << shift);
        hw_sim::gpio_output_reg[port_] &= ~(1u << pin_);
        initialized_ = false;
        hw_sim::gpio_deinit_count++;
        printf("[GPIO] Pin P%d.%d deinitialized (reset to input)\n", port_, pin_);
    }

    static const char* mode_name(PinMode m) {
        switch (m) {
            case PinMode::Input:             return "Input";
            case PinMode::Output:            return "Output";
            case PinMode::AlternateFunction: return "AlternateFunction";
            case PinMode::Analog:            return "Analog";
            default:                         return "Unknown";
        }
    }
};

// ---------------------------------------------------------------------------
// SpiTransaction — RAII lock for SPI bus
// ---------------------------------------------------------------------------
class SpiTransaction {
public:
    explicit SpiTransaction(const char* owner = "unknown") : owner_(owner) {
        // In real code, this would acquire a mutex or disable interrupts
        assert(!hw_sim::spi_locked && "SPI bus already locked!");
        hw_sim::spi_locked = true;
        hw_sim::spi_lock_count++;
        printf("[SPI] Bus locked by '%s'\n", owner_);
    }

    // Non-copyable, non-movable
    SpiTransaction(const SpiTransaction&) = delete;
    SpiTransaction& operator=(const SpiTransaction&) = delete;

    ~SpiTransaction() {
        hw_sim::spi_locked = false;
        printf("[SPI] Bus unlocked by '%s'\n", owner_);
    }

    void transfer(uint8_t tx_byte, uint8_t& rx_byte) {
        assert(hw_sim::spi_locked);
        // Simulated SPI transfer
        rx_byte = ~tx_byte;  // Simulate: loopback with inversion
        printf("[SPI] TX: 0x%02X  RX: 0x%02X\n", tx_byte, rx_byte);
    }

private:
    const char* owner_;
};

// ---------------------------------------------------------------------------
// Demonstration: RAII guarantees cleanup even with early returns
// ---------------------------------------------------------------------------
bool perform_spi_transfer(bool simulate_error) {
    SpiTransaction txn("transfer_function");

    uint8_t rx = 0;
    txn.transfer(0xAA, rx);

    if (simulate_error) {
        printf("[SPI] Error detected, returning early\n");
        return false;  // SpiTransaction destructor runs — bus is unlocked!
    }

    txn.transfer(0x55, rx);
    return true;
    // SpiTransaction destructor runs here
}

// ---------------------------------------------------------------------------
// InterruptGuard — RAII for critical sections
// ---------------------------------------------------------------------------
namespace hw_sim {
    static bool interrupts_enabled = true;
}

class InterruptGuard {
public:
    InterruptGuard() : was_enabled_(hw_sim::interrupts_enabled) {
        hw_sim::interrupts_enabled = false;
        printf("[IRQ] Interrupts disabled\n");
    }

    ~InterruptGuard() {
        hw_sim::interrupts_enabled = was_enabled_;
        printf("[IRQ] Interrupts %s\n",
               was_enabled_ ? "re-enabled" : "kept disabled");
    }

    InterruptGuard(const InterruptGuard&) = delete;
    InterruptGuard& operator=(const InterruptGuard&) = delete;

private:
    bool was_enabled_;
};

// ---------------------------------------------------------------------------
// Main — demonstrate all RAII patterns
// ---------------------------------------------------------------------------
int main() {
    hw_sim::reset();

    printf("=== GPIO RAII Demo ===\n\n");

    {
        // Create GPIO pins — constructor initializes hardware
        GpioPin led(0, 5, PinMode::Output);
        GpioPin button(0, 13, PinMode::Input);

        led.set();
        assert(led.read() == PinState::High);

        led.clear();
        assert(led.read() == PinState::Low);

        led.toggle();
        assert(led.read() == PinState::High);

        // Simulate button press
        hw_sim::gpio_input_reg[0] |= (1u << 13);
        assert(button.read() == PinState::High);

        printf("\n--- Leaving scope: pins will be auto-deinitialized ---\n");
    }
    // Both pins are now deinitialized (destructors ran)

    assert(hw_sim::gpio_init_count == 2);
    assert(hw_sim::gpio_deinit_count == 2);
    printf("GPIO init/deinit balanced: %d/%d\n\n",
           hw_sim::gpio_init_count, hw_sim::gpio_deinit_count);

    printf("=== Move Semantics Demo ===\n\n");
    hw_sim::reset();

    {
        GpioPin pin1(1, 0, PinMode::Output);
        pin1.set();

        // Move ownership to pin2
        GpioPin pin2 = std::move(pin1);
        assert(!pin1.is_initialized());
        assert(pin2.is_initialized());
        assert(pin2.read() == PinState::High);

        printf("--- pin1 moved to pin2, only pin2 will deinit ---\n");
    }
    assert(hw_sim::gpio_init_count == 1);
    assert(hw_sim::gpio_deinit_count == 1);

    printf("\n=== SPI RAII Demo ===\n\n");
    hw_sim::reset();

    // Normal transfer
    bool ok = perform_spi_transfer(false);
    assert(ok);
    assert(!hw_sim::spi_locked);

    // Transfer with early return — SPI still gets unlocked
    ok = perform_spi_transfer(true);
    assert(!ok);
    assert(!hw_sim::spi_locked);  // RAII guarantees unlock!
    printf("SPI lock count: %d (all unlocked)\n\n", hw_sim::spi_lock_count);

    printf("=== Interrupt Guard Demo ===\n\n");
    hw_sim::interrupts_enabled = true;

    {
        InterruptGuard guard;
        assert(!hw_sim::interrupts_enabled);
        // Critical section work here
    }
    assert(hw_sim::interrupts_enabled);  // Restored!

    printf("\n=== All RAII tests passed! ===\n");
    return 0;
}
