# Module 04: C++ for Embedded Systems

## Overview

This module covers the effective use of C++ in resource-constrained embedded systems.
C++ offers powerful abstractions that, when used correctly, produce code as efficient as
hand-written C while being safer, more maintainable, and more expressive. The key insight
is **zero-cost abstractions**: features that provide high-level programming constructs
without runtime overhead.

## Prerequisites

- Solid understanding of C programming (Module 03)
- Familiarity with embedded concepts (registers, GPIO, peripherals)
- Basic understanding of compilation and linking

---

## 1. Why C++ in Embedded Systems

### Zero-Cost Abstractions

The C++ design philosophy includes the **zero-overhead principle**:

> "What you don't use, you don't pay for. What you do use, you couldn't hand-code any better."
> — Bjarne Stroustrup

In practice, this means:
- Templates are resolved at compile time — no runtime cost
- `constexpr` computations happen during compilation
- Inline functions eliminate call overhead
- RAII manages resources without garbage collection

### C++ vs C in Embedded Context

| Feature | C | C++ |
|---|---|---|
| Type safety | Weak (implicit casts) | Strong (explicit required) |
| Resource management | Manual (error-prone) | RAII (automatic, deterministic) |
| Code reuse | Macros, function pointers | Templates, inheritance, CRTP |
| Abstraction cost | None (no abstractions) | Zero (when done right) |
| Polymorphism | Function pointers | Virtual (runtime) or CRTP (compile-time) |
| Error handling | Return codes | Return codes, Result types (no exceptions) |
| Binary size | Baseline | Same or slightly larger (templates can bloat) |

---

## 2. Classes and Objects

Classes encapsulate data and behavior. In embedded systems, classes naturally model
hardware peripherals, protocols, and system resources.

```cpp
class GpioPin {
public:
    GpioPin(uint8_t port, uint8_t pin, PinMode mode);
    ~GpioPin();  // Reset pin on destruction

    void set();
    void clear();
    void toggle();
    bool read() const;

private:
    volatile uint32_t* port_base_;
    uint8_t pin_mask_;
};
```

Key points for embedded:
- Keep classes small and focused (Single Responsibility)
- Use `const` methods for read-only operations
- Mark hardware register pointers as `volatile`
- Prefer composition over deep inheritance hierarchies

---

## 3. Constructors and Destructors — The RAII Pattern

**RAII (Resource Acquisition Is Initialization)** is the single most important C++ idiom
for embedded systems. Resources are acquired in constructors and released in destructors,
guaranteeing cleanup even when control flow is complex.

### Why RAII Matters in Embedded

```cpp
// C style — easy to forget cleanup
void transfer_data() {
    spi_lock();
    // ... what if we return early here?
    spi_unlock();  // Might never execute!
}

// C++ RAII — cleanup is guaranteed
void transfer_data() {
    SpiTransaction txn(spi1);  // Locks SPI
    // ... early return is safe
}  // Destructor unlocks SPI automatically
```

RAII use cases in embedded:
- **Interrupt guards**: Disable interrupts in constructor, re-enable in destructor
- **Peripheral locks**: Acquire bus in constructor, release in destructor
- **Power management**: Enable clock in constructor, disable in destructor
- **DMA buffers**: Allocate in constructor, free in destructor

---

## 4. Inheritance and Polymorphism

### Virtual Functions — Runtime Polymorphism

```cpp
class Sensor {
public:
    virtual ~Sensor() = default;
    virtual int32_t read() = 0;          // Pure virtual
    virtual const char* name() const = 0;
};

class TemperatureSensor : public Sensor {
public:
    int32_t read() override;
    const char* name() const override { return "Temperature"; }
};
```

### The Cost of Virtual Functions

Each class with virtual functions carries a **vtable** (table of function pointers):
- **Per class**: One vtable (array of pointers) — typically 4 bytes per virtual function
- **Per object**: One vptr (pointer to vtable) — typically 4 bytes on 32-bit systems
- **Per call**: One indirection (pointer dereference) — prevents inlining

On a system with 4 KB of RAM, adding a 4-byte vptr to every object matters.

### When to Use Virtual Functions

- Use when: Set of types is open-ended, runtime switching needed, small number of objects
- Avoid when: Performance-critical inner loops, many small objects, type is known at compile time

---

## 5. Templates — Compile-Time Polymorphism

Templates generate specialized code at compile time with **zero runtime cost**.

```cpp
template<uint32_t Address, typename Width = uint32_t>
class Register {
public:
    static void write(Width value) {
        *reinterpret_cast<volatile Width*>(Address) = value;
    }
    static Width read() {
        return *reinterpret_cast<volatile Width*>(Address);
    }
};

// Usage — resolved entirely at compile time
using GPIOA_ODR = Register<0x40020014>;
GPIOA_ODR::write(0xFF);
```

### Template Considerations for Embedded

- **Code bloat**: Each template instantiation generates separate code. `Buffer<int, 32>`
  and `Buffer<int, 64>` are two complete copies.
- **Mitigation**: Factor common logic into non-template base functions
- **Compile time**: Heavy template use increases compilation time
- **Debugging**: Template errors can produce verbose compiler messages

---

## 6. constexpr — Compile-Time Computation

`constexpr` tells the compiler to evaluate expressions at compile time:

```cpp
constexpr uint32_t calculate_baud_divisor(uint32_t clock, uint32_t baud) {
    return (clock + (baud / 2)) / baud;
}

// Computed at compile time — no runtime cost
constexpr auto divisor = calculate_baud_divisor(16000000, 115200);
```

### constexpr vs const

- `const`: Value won't change after initialization (may be computed at runtime)
- `constexpr`: Value **must** be computable at compile time

### constexpr Use Cases in Embedded

- Baud rate calculations
- Timer prescaler values
- Lookup tables (sin, CRC, etc.)
- Configuration validation with `static_assert`

---

## 7. Namespaces

Namespaces prevent naming collisions — critical when integrating multiple vendor HALs:

```cpp
namespace hal::gpio {
    void init(Port port, Pin pin, Mode mode);
}

namespace hal::uart {
    void init(Port port, uint32_t baud);
}

// No collision between gpio::init and uart::init
```

---

## 8. References vs Pointers

| Feature | Reference | Pointer |
|---|---|---|
| Can be null | No | Yes |
| Can be reassigned | No | Yes |
| Syntax | `obj.member` | `obj->member` |
| Use in embedded | Passing peripherals, const refs | Hardware addresses, optional params |

Prefer references when:
- The target always exists
- You want to prevent null dereference bugs
- Passing objects to functions (avoid copying)

---

## 9. Operator Overloading for Hardware Registers

Operator overloading makes register manipulation natural and type-safe:

```cpp
class Register32 {
    volatile uint32_t& reg_;
public:
    Register32& operator|=(uint32_t mask) { reg_ |= mask; return *this; }
    Register32& operator&=(uint32_t mask) { reg_ &= mask; return *this; }
};
```

---

## 10. Modern C++ Features for Embedded

### enum class (Scoped Enumerations)

```cpp
enum class PinMode : uint8_t { Input = 0, Output = 1, Alternate = 2, Analog = 3 };
// PinMode::Input — no implicit conversion to int, no namespace pollution
```

### std::array

```cpp
#include <array>
std::array<uint8_t, 64> buffer{};  // Stack-allocated, bounds-checkable, size known
```

### auto Type Deduction

```cpp
auto divisor = calculate_baud_divisor(clock, baud);  // Type deduced at compile time
```

### static_assert

```cpp
static_assert(sizeof(RegisterBlock) == 24, "Register block size mismatch");
```

### nullptr

```cpp
int* p = nullptr;  // Type-safe null pointer (not 0 or NULL)
```

---

## 11. What to AVOID in Embedded C++

### Exceptions

- Stack unwinding requires runtime support (~10-20 KB code overhead)
- Non-deterministic execution time
- Compile with `-fno-exceptions`

### RTTI (Run-Time Type Information)

- `dynamic_cast` and `typeid` require type metadata in binary
- Adds ~5-15% code size overhead
- Compile with `-fno-rtti`

### Dynamic Memory Allocation

- `new`/`delete` and heap allocation cause fragmentation
- Non-deterministic allocation time
- Use stack allocation, static allocation, or memory pools

### Heavy STL Containers

- `std::vector`, `std::map`, `std::string` use heap allocation
- Use `std::array`, fixed-size containers, or embedded-specific libraries (ETL)

---

## 12. Embedded C++ Coding Standards

1. **Use RAII** for all resource management
2. **Prefer templates** over virtual functions for compile-time polymorphism
3. **Use constexpr** for all compile-time computations
4. **Use enum class** instead of plain enums or `#define` constants
5. **Use static_assert** for compile-time configuration validation
6. **Disable exceptions and RTTI** (`-fno-exceptions -fno-rtti`)
7. **Avoid heap allocation** — prefer stack and static storage
8. **Use `std::array`** instead of C arrays
9. **Mark functions `const`** when they don't modify state
10. **Use `override`** on all virtual function overrides

---

## Module Contents

| Directory | Description |
|---|---|
| `examples/` | 7 annotated example programs |
| `exercises/` | 8 hands-on exercises with constraints |
| `solutions/` | Complete solutions for all exercises |
| `tests/` | Automated tests for key concepts |
| `project/` | C++ HAL for Multi-Board Support |

## Recommended Compiler Flags

```bash
g++ -std=c++17 -O2 -fno-exceptions -fno-rtti -Wall -Wextra -Wpedantic
```

## Further Reading

- *Real-Time C++* by Christopher Kormanyos
- *Effective Modern C++* by Scott Meyers
- MISRA C++ 2008 Guidelines
- Embedded Template Library (ETL): https://www.etlcpp.com/
