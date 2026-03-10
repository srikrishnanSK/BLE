# Module 04: Communication Protocols (UART, SPI, I2C)

## Overview

Serial communication protocols are the backbone of embedded systems. Nearly every
sensor, memory chip, display, and peripheral communicates through one of three
dominant protocols: **UART**, **SPI**, or **I2C**. This module covers each protocol
in depth -- the electrical signaling, framing, timing, error handling, and practical
usage patterns that every embedded engineer must know.

---

## Table of Contents

1. [UART (Universal Asynchronous Receiver/Transmitter)](#1-uart)
2. [SPI (Serial Peripheral Interface)](#2-spi)
3. [I2C (Inter-Integrated Circuit)](#3-i2c)
4. [Protocol Comparison](#4-protocol-comparison)
5. [Practical Considerations](#5-practical-considerations)

---

## 1. UART

### 1.1 What is UART?

UART is an **asynchronous** serial protocol -- there is no shared clock line. Both
communicating devices must agree on a **baud rate** (bits per second) ahead of time.
UART is point-to-point: one transmitter talks to one receiver.

```
    Device A                    Device B
   +--------+                  +--------+
   |     TX |----------------->| RX     |
   |     RX |<-----------------| TX     |
   |    GND |------------------| GND    |
   +--------+                  +--------+

   Note: TX of one device connects to RX of the other (crossover)
```

### 1.2 Signal Levels

| Standard   | Logic 1 (Mark) | Logic 0 (Space) | Notes                    |
|------------|----------------|-----------------|--------------------------|
| TTL        | 3.3V or 5V     | 0V              | Short distances (<1m)    |
| RS-232     | -3V to -15V    | +3V to +15V     | Inverted, longer range   |
| RS-485     | Differential   | Differential     | Multi-drop, up to 1200m  |

### 1.3 Frame Format

A UART frame consists of a start bit, data bits, optional parity, and stop bit(s):

```
    Idle ___________                                          _____________ Idle
                    |         Data Bits           |Parity|   |
                    | S | D0 | D1 | D2 | D3 | D4 | D5 | D6 | D7 | P | SP |
                    |___|____|____|____|____|____|____|____|____|___|____|
                     ^                                          ^     ^
                  Start Bit                                  Parity  Stop
                  (always 0)                                  Bit   Bit(s)

    Detailed Timing (8N1 at 9600 baud, ~104us per bit):

    Voltage
      |
    1 |____      ____      ____      ____      __________
      |    |    |    |    |    |    |    |    |
    0 |    |____|    |____|    |____|    |____|
      |    |    |    |    |    |    |    |    |    |    |
      +----|----|----|----|----|----|----|----|----|----|--->
         Start  D0   D1   D2   D3   D4   D5   D6   D7  Stop
           0     1    0    1    0    1    0    1    0    1

    Example: Transmitting 0x55 (0b01010101) with 8N1
```

### 1.4 Common Frame Configurations

| Notation | Data Bits | Parity | Stop Bits | Total Bits/Frame |
|----------|-----------|--------|-----------|------------------|
| 8N1      | 8         | None   | 1         | 10               |
| 8E1      | 8         | Even   | 1         | 11               |
| 8O1      | 8         | Odd    | 1         | 11               |
| 8N2      | 8         | None   | 2         | 12               |
| 7E1      | 7         | Even   | 1         | 10               |

### 1.5 Baud Rate

The baud rate defines the number of signal changes (bits) per second. Both devices
must use the same rate. Common values:

| Baud Rate | Bit Period | Byte Period (8N1) | Use Case              |
|-----------|------------|-------------------|-----------------------|
| 9600      | 104.17 us  | 1.04 ms           | GPS, low-speed sensors|
| 19200     | 52.08 us   | 520.8 us          | Legacy equipment      |
| 38400     | 26.04 us   | 260.4 us          | Modems                |
| 115200    | 8.68 us    | 86.8 us           | Debug consoles        |
| 921600    | 1.09 us    | 10.9 us           | High-speed data       |
| 1000000   | 1.00 us    | 10.0 us           | MCU-to-MCU            |

**Baud Rate Generation:**

The UART peripheral divides a reference clock to produce the baud rate:

```
    Baud Rate Divisor = Reference Clock / (Oversampling * Desired Baud)

    Example: 16 MHz clock, 16x oversampling, 9600 baud
    Divisor = 16,000,000 / (16 * 9600) = 104.1667

    Integer part: 104
    Fractional part: 0.1667 * 64 = 10.67 -> 11

    Actual baud = 16,000,000 / (16 * (104 + 11/64))
               = 16,000,000 / (16 * 104.171875)
               = 9601.38 baud

    Error = (9601.38 - 9600) / 9600 = 0.014% (acceptable, < 2%)
```

### 1.6 Parity

Parity provides a single-bit error detection mechanism:

- **Even Parity**: Total number of 1-bits (data + parity) is even
- **Odd Parity**: Total number of 1-bits (data + parity) is odd

```
    Data: 0b01010101 (four 1-bits)
    Even Parity bit = 0  (4 is already even)
    Odd Parity bit  = 1  (need 5 ones to be odd)

    Data: 0b01010111 (five 1-bits)
    Even Parity bit = 1  (need 6 ones to be even)
    Odd Parity bit  = 0  (5 is already odd)
```

Parity detects **single-bit errors** but cannot detect double-bit errors or
identify which bit is wrong. For robust error detection, use CRC at the
application layer.

### 1.7 Flow Control

Flow control prevents buffer overruns when the receiver cannot keep up.

**Hardware Flow Control (RTS/CTS):**

```
    Device A                       Device B
   +--------+                     +--------+
   |     TX |-------------------->| RX     |
   |     RX |<--------------------| TX     |
   |    RTS |-------------------->| CTS    |
   |    CTS |<--------------------| RTS    |
   |    GND |---------------------| GND    |
   +--------+                     +--------+

   RTS = "Request To Send"  (output: "I am ready to receive")
   CTS = "Clear To Send"    (input:  "You may send to me")

   When Device B's buffer is full:
   1. Device B de-asserts its RTS (goes HIGH)
   2. Device A sees CTS go HIGH -> stops transmitting
   3. Device B processes data, frees buffer space
   4. Device B asserts RTS again (goes LOW)
   5. Device A sees CTS go LOW -> resumes transmitting
```

**Software Flow Control (XON/XOFF):**

```
   Receiver sends XOFF (0x13) -> Transmitter pauses
   Receiver sends XON  (0x11) -> Transmitter resumes

   Limitation: Cannot transmit raw 0x11 or 0x13 as data
               (must use escape sequences for binary data)
```

### 1.8 UART on Microcontrollers

Typical UART peripheral registers:

| Register | Purpose                                          |
|----------|--------------------------------------------------|
| BRR      | Baud Rate Register (divisor value)                |
| CR1      | Control Register 1 (enable, word length, parity) |
| CR2      | Control Register 2 (stop bits, clock settings)    |
| CR3      | Control Register 3 (flow control, DMA enable)    |
| SR       | Status Register (TX empty, RX ready, errors)     |
| DR       | Data Register (read = RX data, write = TX data)  |

```
    Typical Transmit Flow:
    1. Check SR.TXE (TX buffer empty) == 1
    2. Write byte to DR
    3. Hardware shifts data out bit-by-bit
    4. SR.TC (Transmit Complete) set when done

    Typical Receive Flow:
    1. SR.RXNE (RX Not Empty) set when byte received
    2. Read DR to get byte (also clears RXNE)
    3. Check SR for errors: FE (Framing), PE (Parity),
       ORE (Overrun), NE (Noise)
```

---

## 2. SPI

### 2.1 What is SPI?

SPI (Serial Peripheral Interface) is a **synchronous**, **full-duplex**, **master-slave**
protocol. It uses a shared clock line driven by the master, enabling much higher
data rates than UART. SPI is commonly used for flash memory, ADCs, DACs, displays,
and SD cards.

```
                           SPI Bus

    Master                                      Slave 0
   +--------+          SCLK                    +--------+
   |   SCLK |--+----------------------------->| SCLK   |
   |   MOSI |--+----------------------------->| MOSI   |
   |   MISO |<-+------------------------------| MISO   |
   |   SS0  |--+----------------------------->| SS/CS  |
   |   SS1  |--+---+                          +--------+
   +--------+  |   |
               |   |                           Slave 1
               |   |                          +--------+
               |   +------------------------->| SCLK   |
               |   +------------------------->| MOSI   |
               |   +<-------------------------| MISO   |
               |   +------------------------->| SS/CS  |
               |                              +--------+

   SCLK = Serial Clock (master drives)
   MOSI = Master Out, Slave In  (data: master -> slave)
   MISO = Master In, Slave Out  (data: slave -> master)
   SS/CS = Slave Select / Chip Select (active LOW)
```

### 2.2 Full-Duplex Operation

SPI is inherently full-duplex: data shifts out on MOSI while simultaneously
shifting in on MISO. The master and slave each have a shift register:

```
    Master                              Slave
   +---+---+---+---+---+---+---+---+  +---+---+---+---+---+---+---+---+
   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
   +---+---+---+---+---+---+---+---+  +---+---+---+---+---+---+---+---+
     |                           ^        |                           ^
     | MOSI                      |        | MISO                      |
     +-------------------------->|        +<--------------------------+
                                 |                                    |
                          (slave receives)                   (master receives)

   After 8 clock cycles, the registers have swapped contents.
   Even "read-only" operations require the master to send 8 clocks
   (typically sends 0xFF or 0x00 as dummy data).
```

### 2.3 Clock Polarity and Phase (CPOL/CPHA)

The four SPI modes define when data is sampled and shifted:

```
    CPOL = Clock Polarity (idle state of SCLK)
    CPHA = Clock Phase (which edge captures data)

    Mode 0 (CPOL=0, CPHA=0):  Most common
    ──────────────────────────
    SCLK idle LOW, data captured on RISING edge, shifted on FALLING edge

         ___     ___     ___     ___
    ____|   |___|   |___|   |___|   |____  SCLK
        ^       ^       ^       ^          Sample points (rising)
    ____X_______X_______X_______X________  MOSI/MISO
       D7      D6      D5      D4         Data bits


    Mode 1 (CPOL=0, CPHA=1):
    ──────────────────────────
    SCLK idle LOW, data captured on FALLING edge, shifted on RISING edge

         ___     ___     ___     ___
    ____|   |___|   |___|   |___|   |____  SCLK
            ^       ^       ^       ^      Sample points (falling)
    ________X_______X_______X_______X____  MOSI/MISO
           D7      D6      D5      D4


    Mode 2 (CPOL=1, CPHA=0):
    ──────────────────────────
    SCLK idle HIGH, data captured on FALLING edge, shifted on RISING edge

    ____     ___     ___     ___     ____
        |___|   |___|   |___|   |___|      SCLK
        ^       ^       ^       ^          Sample points (falling)
    ____X_______X_______X_______X________  MOSI/MISO
       D7      D6      D5      D4


    Mode 3 (CPOL=1, CPHA=1):
    ──────────────────────────
    SCLK idle HIGH, data captured on RISING edge, shifted on FALLING edge

    ____     ___     ___     ___     ____
        |___|   |___|   |___|   |___|      SCLK
            ^       ^       ^       ^      Sample points (rising)
    ________X_______X_______X_______X____  MOSI/MISO
           D7      D6      D5      D4
```

| Mode | CPOL | CPHA | Idle State | Capture Edge | Common Devices          |
|------|------|------|------------|--------------|-------------------------|
| 0    | 0    | 0    | LOW        | Rising       | Most sensors, SD cards  |
| 1    | 0    | 1    | LOW        | Falling      | Some ADCs               |
| 2    | 1    | 0    | HIGH       | Falling      | Some DACs               |
| 3    | 1    | 1    | HIGH       | Rising       | MAX31855, some displays |

### 2.4 Chip Select (CS/SS)

Each slave has its own chip select line, active LOW:

```
    Transaction Timeline:

    CS   ‾‾‾‾\________________________________/‾‾‾‾‾‾‾‾
    SCLK ______/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\________
    MOSI ------< D7 >< D6 >< D5 >< D4 >< D3 >< D2 >< D1 >< D0 >---
    MISO ------< D7 >< D6 >< D5 >< D4 >< D3 >< D2 >< D1 >< D0 >---
              ^                                              ^
           CS goes LOW                                   CS goes HIGH
           (select slave)                                (deselect)

    Rules:
    - Assert CS LOW before first clock edge
    - Keep CS LOW for entire transaction (may span multiple bytes)
    - De-assert CS HIGH after last clock edge
    - Only one slave selected at a time (unless daisy-chained)
```

### 2.5 SPI Clock Speed

SPI has no formal maximum speed in the specification -- it depends on the slave
device and the PCB layout. Typical ranges:

| Application         | Typical Speed | Notes                      |
|---------------------|---------------|----------------------------|
| Low-speed sensors   | 1-5 MHz       | Temperature, pressure      |
| SD cards (SPI mode) | 25 MHz        | Class-dependent            |
| SPI Flash (NOR)     | 50-133 MHz    | Quad-SPI can go faster     |
| Displays (TFT/OLED) | 10-80 MHz    | Controller-dependent       |
| ADC/DAC             | 1-50 MHz      | Device-specific            |

### 2.6 SPI on Microcontrollers

Typical SPI peripheral registers:

| Register | Purpose                                            |
|----------|----------------------------------------------------|
| CR1      | Control: master/slave, baud rate prescaler, CPOL/CPHA |
| CR2      | Control: DMA, interrupt enables, frame format      |
| SR       | Status: TX empty, RX ready, busy, overrun          |
| DR       | Data Register: read/write for full-duplex transfer |

```
    Typical SPI Transfer:
    1. Assert CS (GPIO LOW)
    2. Write command byte to DR
    3. Wait for RXNE (transfer complete -- 8 clocks done)
    4. Read DR (discard if not needed)
    5. Write dummy byte (0xFF) to DR for read
    6. Wait for RXNE
    7. Read DR (this is the slave's response)
    8. De-assert CS (GPIO HIGH)
```

---

## 3. I2C

### 3.1 What is I2C?

I2C (Inter-Integrated Circuit, pronounced "I-squared-C") is a **synchronous**,
**half-duplex**, **multi-master**, **multi-slave** protocol using just two wires.
It was designed by Philips (now NXP) for inter-chip communication on a PCB.

```
                        I2C Bus

          VDD
           |
          [Rp]    [Rp]          Pull-up Resistors (typ. 4.7k)
           |       |
    SDA ---+---+---+---+---+--- SDA (Serial Data)
               |       |   |
    SCL ---+---+---+---+---+--- SCL (Serial Clock)
           |   |       |   |
        +------+ +------+ +------+
        |Master| |Slave | |Slave |
        | 0x-- | | 0x48 | | 0x68 |
        +------+ +------+ +------+

    - Open-drain/open-collector outputs (wired-AND)
    - Pull-up resistors required on both lines
    - Any device can pull the line LOW but not HIGH
    - Bus is idle when both SDA and SCL are HIGH
```

### 3.2 I2C Signaling

I2C uses special conditions for START, STOP, and data transfer:

```
    START Condition:
    SDA falls while SCL is HIGH

    SDA  ‾‾‾‾‾‾\_____________
    SCL  ‾‾‾‾‾‾‾‾‾‾\_________


    STOP Condition:
    SDA rises while SCL is HIGH

    SDA  ___________/‾‾‾‾‾‾‾‾
    SCL  _________/‾‾‾‾‾‾‾‾‾‾


    Repeated START:
    A START issued without a preceding STOP (used to change direction)

    SDA  ____/‾‾\_____________
    SCL  __/‾‾‾‾‾‾\_________


    Data Transfer:
    SDA must be stable while SCL is HIGH (data sampled on rising edge)

    SDA  ---< D7 >< D6 >< D5 >< D4 >< D3 >< D2 >< D1 >< D0 >< A >---
    SCL  __/‾\__/‾\__/‾\__/‾\__/‾\__/‾\__/‾\__/‾\__/‾\__/‾\__/‾\______

              ^     ^     ^     ^     ^     ^     ^     ^     ^
           Data sampled on each rising edge of SCL          ACK/NACK
```

### 3.3 Addressing

I2C uses 7-bit or 10-bit addressing. The first byte after START contains the
slave address and the R/W bit:

```
    7-bit Address Frame:

    +-----+-----+-----+-----+-----+-----+-----+-----+-----+
    | A6  | A5  | A4  | A3  | A2  | A1  | A0  | R/W | ACK |
    +-----+-----+-----+-----+-----+-----+-----+-----+-----+
    |<---------- 7-bit address ---------->|  0=Write  |
                                             1=Read

    Example: BME280 sensor at address 0x76, write operation
    Byte on bus: 0x76 << 1 | 0 = 0xEC = 0b11101100

    Example: Same sensor, read operation
    Byte on bus: 0x76 << 1 | 1 = 0xED = 0b11101101

    Reserved Addresses:
    0x00 - General Call
    0x01 - CBUS address
    0x02 - Reserved for different bus formats
    0x03 - Reserved for future purposes
    0x04-0x07 - High-speed master code
    0x78-0x7B - 10-bit slave addressing
    0x7C-0x7F - Reserved for future purposes
```

### 3.4 ACK and NACK

After every byte, the receiver must send an acknowledgment:

```
    ACK (Acknowledge):
    Receiver pulls SDA LOW during the 9th clock pulse
    Meaning: "Byte received successfully, send more"

    NACK (Not Acknowledge):
    Receiver leaves SDA HIGH during the 9th clock pulse
    Meaning depends on context:
      - Address NACK: No slave at that address
      - Data NACK (master receiving): "Last byte, stop sending"
      - Data NACK (slave receiving): "Cannot accept more data"

    Complete I2C Write Transaction:

    START  Address+W   ACK   Data Byte    ACK   Data Byte    ACK   STOP
    |  S  | A6..A0 | 0 | A | D7..D0    | A |  D7..D0     | A |  P  |
           Slave        |    Register        Value to
           Address     ACK   Address    ACK  Write        ACK

    Complete I2C Read Transaction:

    START  Addr+W  ACK  Reg Addr  ACK  rSTART  Addr+R  ACK  Data  NACK  STOP
    | S | A6..A0|0| A | D7..D0 | A |  Sr  | A6..A0|1| A | D7..D0| NA | P |
         Slave      |  Register  |   Repeated  Slave     |  Read    |
         Addr      ACK  Addr    ACK   Start    Addr     ACK  Data  NACK
```

### 3.5 Clock Stretching

A slave can slow down communication by holding SCL LOW:

```
    Without Clock Stretching:
    SCL (master) ___/‾‾\___/‾‾\___/‾‾\___/‾‾\___
    SDA          ---< D7  >< D6  >< D5  >< D4  >

    With Clock Stretching:
    SCL (master drives) ___/‾‾\___/ ... \___/‾‾\___/‾‾\___
    SCL (actual bus)    ___/‾‾\___________/‾‾\___/‾‾\___
    SDA                 ---< D7  >< ... WAIT ... >< D6  >< D5  >

    The slave holds SCL LOW after receiving a byte while it
    processes data. The master must check SCL before proceeding.
    This is why I2C masters must monitor the SCL line.
```

### 3.6 I2C Speed Modes

| Mode           | Max Speed  | Pull-up   | Notes                    |
|----------------|------------|-----------|--------------------------|
| Standard       | 100 kHz    | 4.7 kOhm | Original specification   |
| Fast           | 400 kHz    | 2.2 kOhm | Most common today        |
| Fast Plus      | 1 MHz      | 2.2 kOhm | Stronger pull-ups needed |
| High Speed     | 3.4 MHz    | --        | Requires master code     |
| Ultra Fast     | 5 MHz      | --        | Push-pull, unidirectional|

### 3.7 Pull-up Resistor Sizing

```
    Too HIGH resistance (e.g., 10k):
    - Slow rise times (RC time constant too large)
    - Signal may not reach VDD before next clock edge
    - Works at low speeds only

    Too LOW resistance (e.g., 1k):
    - Excessive current when pulling LOW
    - Wastes power
    - May exceed GPIO sink current rating

    Calculation:
    Rise time (tr) = 0.8473 * Rp * Cb

    Where:
    - Rp = pull-up resistance
    - Cb = total bus capacitance (wires + pins)

    For Fast Mode (400 kHz): tr < 300 ns
    If Cb = 50 pF:  Rp < 300ns / (0.8473 * 50pF) = 7.08 kOhm
    Typical choice: 4.7 kOhm

    For Standard Mode (100 kHz): tr < 1000 ns
    If Cb = 50 pF:  Rp < 1000ns / (0.8473 * 50pF) = 23.6 kOhm
    Typical choice: 10 kOhm
```

### 3.8 I2C on Microcontrollers

Typical I2C peripheral registers:

| Register | Purpose                                          |
|----------|--------------------------------------------------|
| CR1      | Control: enable, start/stop generation, ACK      |
| CR2      | Control: clock frequency, DMA, interrupts        |
| OAR1/2   | Own Address Register (for slave mode)            |
| DR       | Data Register                                    |
| SR1/SR2  | Status: start sent, address matched, byte done   |
| CCR      | Clock Control Register (speed configuration)     |
| TRISE    | Rise time configuration                          |

---

## 4. Protocol Comparison

### 4.1 Feature Comparison

```
    +------------------+------------+-------------+-------------+
    | Feature          |    UART    |     SPI     |     I2C     |
    +------------------+------------+-------------+-------------+
    | Wires            |   2 (TX/RX)|  4+ (SCLK,  |  2 (SDA,    |
    |                  |            |  MOSI,MISO, |    SCL)      |
    |                  |            |  CS per dev)|              |
    +------------------+------------+-------------+-------------+
    | Clock            | Async (no  | Synchronous | Synchronous |
    |                  |  clock)    | (master)    | (master)    |
    +------------------+------------+-------------+-------------+
    | Duplex           | Full       | Full        | Half        |
    +------------------+------------+-------------+-------------+
    | Speed            | Up to 1Mbps| Up to 100+  | 100kHz to   |
    |                  |            |   MHz       |   3.4MHz    |
    +------------------+------------+-------------+-------------+
    | Topology         | Point-to-  | 1 Master,   | Multi-master|
    |                  |  point     |  N Slaves   | Multi-slave |
    +------------------+------------+-------------+-------------+
    | Addressing       | None       | Hardware    | Software    |
    |                  |            | (CS lines)  | (7/10-bit)  |
    +------------------+------------+-------------+-------------+
    | Acknowledgment   | None       | None        | ACK/NACK    |
    |                  | (no built- | (protocol   | after every |
    |                  |  in ack)   |  level)     |  byte       |
    +------------------+------------+-------------+-------------+
    | Max Devices      | 2          | Limited by  | 112 (7-bit) |
    |                  |            |  CS pins    | 1008(10-bit)|
    +------------------+------------+-------------+-------------+
    | Distance         | Up to 15m  | < 1m (PCB   | < 1m (PCB   |
    |                  | (RS-232)   |  traces)    |  traces)    |
    +------------------+------------+-------------+-------------+
    | Complexity       | Low        | Medium      | Medium-High |
    +------------------+------------+-------------+-------------+
```

### 4.2 When to Use Which

```
    Use UART when:
    +-- You need debug/console output
    +-- Communicating with GPS, Bluetooth, WiFi modules
    +-- Long-distance communication (with RS-232/RS-485)
    +-- Simple point-to-point communication
    +-- Interfacing with PCs (USB-to-UART adapters)

    Use SPI when:
    +-- High speed is required (>1 MHz)
    +-- Full-duplex communication is needed
    +-- Communicating with flash memory, SD cards
    +-- Driving displays (TFT, OLED)
    +-- Reading high-speed ADCs
    +-- Few slaves (limited CS pins available)

    Use I2C when:
    +-- Many devices share the bus (sensors, EEPROMs)
    +-- Pin count is limited (only 2 pins)
    +-- Speed is not critical (< 400 kHz typical)
    +-- Reading temperature, humidity, accelerometer sensors
    +-- Small data transfers (configuration registers)
    +-- Need built-in addressing and acknowledgment
```

---

## 5. Practical Considerations

### 5.1 Error Handling

```
    UART Errors:
    +-----------+--------------------------------+--------------------+
    | Error     | Cause                          | Detection          |
    +-----------+--------------------------------+--------------------+
    | Framing   | Baud rate mismatch, noise      | Stop bit not HIGH  |
    | Parity    | Bit flip during transmission   | Parity check fails |
    | Overrun   | CPU too slow to read DR        | New byte overwrites|
    | Noise     | Electrical interference        | Sampling disagrees |
    | Break     | Line held LOW > 1 frame time   | All zeros received |
    +-----------+--------------------------------+--------------------+

    SPI Errors:
    +-----------+--------------------------------+--------------------+
    | Error     | Cause                          | Detection          |
    +-----------+--------------------------------+--------------------+
    | Overrun   | CPU too slow to read DR        | Check OVR flag     |
    | Mode Fault| Multi-master conflict          | Check MODF flag    |
    | CRC Error | Data corruption (if CRC used)  | Check CRCERR flag  |
    +-----------+--------------------------------+--------------------+

    I2C Errors:
    +-----------+--------------------------------+--------------------+
    | Error     | Cause                          | Detection          |
    +-----------+--------------------------------+--------------------+
    | NACK      | Wrong address, slave busy      | AF (Ack Failure)   |
    | Bus Error | SDA change during SCL HIGH     | BERR flag          |
    |           | (spurious START/STOP)          |                    |
    | Arbitration| Multi-master collision        | ARLO flag          |
    | Timeout   | Slave clock stretching too long| Software timer     |
    | Bus Stuck | Slave holds SDA LOW            | Manual clock pulse |
    +-----------+--------------------------------+--------------------+
```

### 5.2 Bus Recovery (I2C)

If an I2C slave holds SDA LOW (e.g., due to interrupted transfer), the bus is
stuck. Recovery procedure:

```
    1. Toggle SCL up to 9 times while monitoring SDA
    2. If SDA goes HIGH, send a STOP condition
    3. If SDA stays LOW after 9 clocks, the slave is likely
       faulty -- power cycle may be required

    for (int i = 0; i < 9; i++) {
        gpio_set(SCL_PIN);      // SCL HIGH
        delay_us(5);
        if (gpio_read(SDA_PIN)) // SDA released?
            break;
        gpio_clear(SCL_PIN);    // SCL LOW
        delay_us(5);
    }
    // Generate STOP
    gpio_clear(SDA_PIN);
    delay_us(5);
    gpio_set(SCL_PIN);
    delay_us(5);
    gpio_set(SDA_PIN);
```

### 5.3 DMA Integration

All three protocols benefit significantly from DMA (Direct Memory Access):

```
    Without DMA (Polling/Interrupt):
    CPU --[wait]--[read]--[wait]--[read]--[wait]--[read]--
         Each byte requires CPU intervention

    With DMA:
    CPU --[setup]--[free for other tasks]--[done callback]--
         DMA controller handles byte-by-byte transfers
         CPU only involved at start and completion

    Typical DMA setup:
    1. Configure source address (peripheral DR register)
    2. Configure destination address (memory buffer)
    3. Set transfer count (number of bytes)
    4. Set transfer direction (peripheral-to-memory or reverse)
    5. Enable DMA channel
    6. Trigger: each RXNE/TXE triggers one DMA transfer
```

---

## Summary

| Topic              | Key Points                                          |
|--------------------|-----------------------------------------------------|
| UART               | Asynchronous, baud rate agreement, 8N1 most common  |
| UART Flow Control  | Hardware (RTS/CTS) preferred over software (XON/XOFF)|
| SPI                | Synchronous, full-duplex, CPOL/CPHA define 4 modes  |
| SPI Chip Select    | Active LOW, one CS per slave, assert before clocking |
| I2C                | 2-wire, addressed, ACK/NACK, open-drain with pullups|
| I2C Stretching     | Slave holds SCL LOW to slow master                  |
| Protocol Choice    | UART for debug, SPI for speed, I2C for many devices |

---

## Further Reading

- "Serial Communication" chapters in the reference manual for your MCU
- NXP UM10204: I2C-bus specification and user manual
- Motorola/Freescale SPI specification (original)
- TI Application Report SLVA704: Understanding SPI with GPIO
- "Making Embedded Systems" by Elecia White, Ch. 7

---

## Files in This Module

| File                        | Description                         |
|-----------------------------|-------------------------------------|
| `examples/01_uart_echo.c`  | UART echo server                    |
| `examples/02_uart_printf.c`| UART printf redirect                |
| `examples/03_spi_flash.c`  | SPI flash memory read/write         |
| `examples/04_i2c_eeprom.c` | I2C EEPROM driver                   |
| `examples/05_i2c_sensor.c` | I2C BME280 sensor driver            |
| `examples/06_protocol_bridge.c` | UART-to-I2C protocol bridge    |
| `exercises/exercise_01.md` | UART packet framing                 |
| `exercises/exercise_02.md` | UART bootloader                     |
| `exercises/exercise_03.md` | SPI loopback test                   |
| `exercises/exercise_04.md` | SPI SD card raw sector read         |
| `exercises/exercise_05.md` | I2C bus scanner                     |
| `exercises/exercise_06.md` | I2C multi-sensor polling            |
| `exercises/exercise_07.md` | Protocol analyzer                   |
| `exercises/exercise_08.md` | DMA-based UART transfer             |
| `quiz.md`                  | 15-question quiz                    |
| `quiz_answers.md`          | Answer key                          |
| `project/`                 | Multi-Sensor Data Logger project    |
