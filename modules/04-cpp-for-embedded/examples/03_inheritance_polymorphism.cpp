/**
 * Module 04 - Example 03: Inheritance and Polymorphism
 *
 * Demonstrates:
 * - Sensor base class with virtual read()
 * - Derived TemperatureSensor and PressureSensor
 * - Driver interface pattern
 * - vtable memory cost discussion and measurement
 *
 * Compile: g++ -std=c++17 -O2 -Wall -Wextra -o 03_inheritance 03_inheritance_polymorphism.cpp
 */

#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>

// ---------------------------------------------------------------------------
// Simulated sensor data
// ---------------------------------------------------------------------------
namespace hw_sim {
    static int32_t temp_raw = 2500;    // 25.00 degrees C
    static int32_t pressure_raw = 101325;  // Pascals
    static int32_t humidity_raw = 6500;    // 65.00 %
}

// ---------------------------------------------------------------------------
// 1. Abstract Sensor Base Class (Runtime Polymorphism)
// ---------------------------------------------------------------------------

/**
 * Sensor — abstract base class defining the sensor interface.
 *
 * Memory cost:
 * - Each Sensor object contains a hidden vptr (4 bytes on 32-bit, 8 on 64-bit)
 * - One vtable per derived class (array of function pointers)
 * - Each virtual call goes through one pointer indirection
 */
class Sensor {
public:
    virtual ~Sensor() = default;

    // Pure virtual — must be implemented by derived classes
    virtual int32_t read_raw() = 0;
    virtual const char* name() const = 0;
    virtual const char* unit() const = 0;

    // Non-virtual — common to all sensors
    float read_scaled() {
        return static_cast<float>(read_raw()) / scale_factor();
    }

protected:
    virtual float scale_factor() const { return 100.0f; }
};

// ---------------------------------------------------------------------------
// 2. Derived Sensor Classes
// ---------------------------------------------------------------------------

class TemperatureSensor : public Sensor {
public:
    explicit TemperatureSensor(int32_t* raw_source)
        : source_(raw_source) {}

    int32_t read_raw() override {
        return *source_;
    }

    const char* name() const override { return "Temperature"; }
    const char* unit() const override { return "°C"; }

protected:
    float scale_factor() const override { return 100.0f; }

private:
    int32_t* source_;
};

class PressureSensor : public Sensor {
public:
    explicit PressureSensor(int32_t* raw_source)
        : source_(raw_source) {}

    int32_t read_raw() override {
        return *source_;
    }

    const char* name() const override { return "Pressure"; }
    const char* unit() const override { return "Pa"; }

protected:
    float scale_factor() const override { return 1.0f; }  // Already in Pa

private:
    int32_t* source_;
};

class HumiditySensor : public Sensor {
public:
    explicit HumiditySensor(int32_t* raw_source)
        : source_(raw_source) {}

    int32_t read_raw() override {
        return *source_;
    }

    const char* name() const override { return "Humidity"; }
    const char* unit() const override { return "%RH"; }

private:
    int32_t* source_;
};

// ---------------------------------------------------------------------------
// 3. Driver Interface Pattern
// ---------------------------------------------------------------------------

/**
 * CommunicationDriver — interface for different communication buses.
 *
 * This pattern is useful when you need to support multiple bus types
 * (I2C, SPI, UART) with the same high-level API.
 */
class CommunicationDriver {
public:
    virtual ~CommunicationDriver() = default;

    virtual bool init() = 0;
    virtual bool write(uint8_t addr, const uint8_t* data, uint32_t len) = 0;
    virtual bool read(uint8_t addr, uint8_t* data, uint32_t len) = 0;
    virtual const char* bus_name() const = 0;
};

class I2CDriver : public CommunicationDriver {
public:
    explicit I2CDriver(uint8_t bus_num) : bus_num_(bus_num), initialized_(false) {}

    bool init() override {
        initialized_ = true;
        printf("  [I2C%d] Initialized\n", bus_num_);
        return true;
    }

    bool write(uint8_t addr, const uint8_t* data, uint32_t len) override {
        if (!initialized_) return false;
        printf("  [I2C%d] Write %u bytes to 0x%02X:", bus_num_, len, addr);
        for (uint32_t i = 0; i < len; i++) printf(" %02X", data[i]);
        printf("\n");
        return true;
    }

    bool read(uint8_t addr, uint8_t* data, uint32_t len) override {
        if (!initialized_) return false;
        // Simulate reading
        for (uint32_t i = 0; i < len; i++) data[i] = 0xAA;
        printf("  [I2C%d] Read %u bytes from 0x%02X\n", bus_num_, len, addr);
        return true;
    }

    const char* bus_name() const override { return "I2C"; }

private:
    uint8_t bus_num_;
    bool initialized_;
};

class SPIDriver : public CommunicationDriver {
public:
    explicit SPIDriver(uint8_t bus_num) : bus_num_(bus_num), initialized_(false) {}

    bool init() override {
        initialized_ = true;
        printf("  [SPI%d] Initialized\n", bus_num_);
        return true;
    }

    bool write(uint8_t addr, const uint8_t* data, uint32_t len) override {
        if (!initialized_) return false;
        printf("  [SPI%d] Write %u bytes to device 0x%02X\n", bus_num_, len, addr);
        return true;
    }

    bool read(uint8_t addr, uint8_t* data, uint32_t len) override {
        if (!initialized_) return false;
        for (uint32_t i = 0; i < len; i++) data[i] = 0xBB;
        printf("  [SPI%d] Read %u bytes from device 0x%02X\n", bus_num_, len, addr);
        return true;
    }

    const char* bus_name() const override { return "SPI"; }

private:
    uint8_t bus_num_;
    bool initialized_;
};

// ---------------------------------------------------------------------------
// 4. Function that works with any sensor (polymorphism in action)
// ---------------------------------------------------------------------------

void read_and_display(Sensor& sensor) {
    int32_t raw = sensor.read_raw();
    float scaled = sensor.read_scaled();
    printf("  %-15s: raw=%d, scaled=%.2f %s\n",
           sensor.name(), raw, scaled, sensor.unit());
}

void configure_device(CommunicationDriver& driver, uint8_t device_addr) {
    driver.init();
    uint8_t config[] = {0x01, 0x02, 0x03};
    driver.write(device_addr, config, sizeof(config));
    uint8_t status[2];
    driver.read(device_addr, status, sizeof(status));
}

// ---------------------------------------------------------------------------
// 5. Memory Cost Analysis
// ---------------------------------------------------------------------------

// Plain struct — no virtual functions
struct PlainData {
    int32_t x;
    int32_t y;
};

// Struct with one virtual function
struct VirtualData {
    virtual ~VirtualData() = default;
    int32_t x;
    int32_t y;
};

void analyze_memory_cost() {
    printf("\n=== Memory Cost Analysis ===\n\n");

    printf("  sizeof(PlainData)  = %zu bytes (no vtable)\n", sizeof(PlainData));
    printf("  sizeof(VirtualData)= %zu bytes (has vptr)\n", sizeof(VirtualData));
    printf("  vptr overhead      = %zu bytes per object\n",
           sizeof(VirtualData) - sizeof(PlainData));
    printf("  sizeof(void*)      = %zu bytes (pointer size)\n", sizeof(void*));

    printf("\n  sizeof(Sensor)             = %zu bytes\n", sizeof(Sensor));
    printf("  sizeof(TemperatureSensor)  = %zu bytes\n", sizeof(TemperatureSensor));
    printf("  sizeof(PressureSensor)     = %zu bytes\n", sizeof(PressureSensor));

    printf("\n  --- Vtable Cost Summary ---\n");
    printf("  Per class:  1 vtable (array of %zu-byte pointers per virtual function)\n",
           sizeof(void*));
    printf("  Per object: 1 vptr  (%zu bytes)\n", sizeof(void*));
    printf("  Per call:   1 indirection (pointer dereference + indirect call)\n");
    printf("  Inlining:   NOT possible through virtual dispatch\n");

    // Demonstrate: 100 sensor objects wastes 100 * sizeof(void*) bytes
    printf("\n  Example: 100 sensor objects\n");
    printf("    vptr overhead = 100 * %zu = %zu bytes\n",
           sizeof(void*), 100 * sizeof(void*));
    printf("    On 4KB RAM system, that's %.1f%% of memory just for vptrs!\n",
           (100.0 * sizeof(void*) / 4096.0) * 100.0);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    printf("=== Sensor Polymorphism Demo ===\n\n");

    // Create sensor instances
    TemperatureSensor temp(&hw_sim::temp_raw);
    PressureSensor press(&hw_sim::pressure_raw);
    HumiditySensor humid(&hw_sim::humidity_raw);

    // Polymorphic usage — single function handles any sensor type
    read_and_display(temp);
    read_and_display(press);
    read_and_display(humid);

    // Array of base class pointers — classic polymorphism
    Sensor* sensors[] = { &temp, &press, &humid };
    printf("\n  Reading all sensors via base pointer array:\n");
    for (auto* s : sensors) {
        printf("    %s = %d\n", s->name(), s->read_raw());
    }

    // Verify values
    assert(temp.read_raw() == 2500);
    assert(press.read_raw() == 101325);
    assert(humid.read_raw() == 6500);
    assert(strcmp(temp.name(), "Temperature") == 0);

    printf("\n=== Driver Interface Pattern ===\n\n");

    I2CDriver i2c(1);
    SPIDriver spi(2);

    printf("  Configuring device via I2C:\n");
    configure_device(i2c, 0x48);

    printf("  Configuring device via SPI:\n");
    configure_device(spi, 0x01);

    // Memory cost analysis
    analyze_memory_cost();

    printf("\n=== Recommendation ===\n\n");
    printf("  Use virtual functions when:\n");
    printf("    - Types are determined at runtime (plugin systems)\n");
    printf("    - Small number of objects\n");
    printf("    - Not in performance-critical loops\n");
    printf("\n  Use CRTP (see example 06) when:\n");
    printf("    - Types are known at compile time\n");
    printf("    - Many objects or tight loops\n");
    printf("    - Zero overhead is required\n");

    printf("\n=== All polymorphism tests passed! ===\n");
    return 0;
}
