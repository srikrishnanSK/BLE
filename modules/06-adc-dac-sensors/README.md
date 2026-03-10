# Module 06: ADC, DAC & Sensor Interfacing

## Overview

Analog-to-Digital Converters (ADC) and Digital-to-Analog Converters (DAC) are the bridge
between the continuous analog world of sensors and the discrete digital world of
microcontrollers. This module covers the theory and practical application of ADC/DAC
peripherals, sensor interfacing, signal conditioning, calibration, and filtering techniques
essential for embedded systems.

---

## Table of Contents

1. [ADC Fundamentals](#1-adc-fundamentals)
2. [ADC Types](#2-adc-types)
3. [Reference Voltages](#3-reference-voltages)
4. [DMA-Driven ADC](#4-dma-driven-adc)
5. [DAC Basics](#5-dac-basics)
6. [Sensor Types](#6-sensor-types)
7. [Signal Conditioning](#7-signal-conditioning)
8. [Calibration](#8-calibration)
9. [Digital Filtering](#9-digital-filtering)
10. [Practical Considerations](#10-practical-considerations)

---

## 1. ADC Fundamentals

An ADC converts a continuous analog voltage into a discrete digital value. Understanding
its key parameters is critical to designing accurate measurement systems.

### 1.1 Resolution

Resolution is the number of bits in the digital output. An N-bit ADC divides the input
voltage range into 2^N discrete levels.

```
    Resolution and Quantization
    ===========================

    Analog Input        3-bit ADC Output       12-bit ADC Output
    (0-3.3V)            (8 levels)             (4096 levels)

    3.3V __|________         111 = 7             0xFFF = 4095
           |       |
    2.83V _|_____  |         110 = 6
           |    |  |
    2.36V _|__  |  |         101 = 5
           | |  |  |
    1.89V _|_|  |  |         100 = 4
           |||  |  |
    1.41V _|||  |  |         011 = 3
           |||  |  |
    0.94V _|||  |  |         010 = 2
           |||  |  |
    0.47V _|||  |  |         001 = 1
           |||  |  |
    0.00V _|||__|__|         000 = 0             0x000 = 0

    LSB = V_ref / 2^N

    3-bit:  LSB = 3.3V / 8   = 0.4125V  (coarse)
    12-bit: LSB = 3.3V / 4096 = 0.000806V (fine)
```

**Key formula:**

```
    Digital Value = (V_in / V_ref) * (2^N - 1)

    V_in = (Digital Value / (2^N - 1)) * V_ref
```

Common resolutions in embedded systems:
- **8-bit**: 256 levels -- simple, fast, low precision
- **10-bit**: 1024 levels -- Arduino standard
- **12-bit**: 4096 levels -- STM32, nRF52 standard
- **16-bit**: 65536 levels -- precision measurement
- **24-bit**: 16,777,216 levels -- instrumentation grade (load cells, strain gauges)

### 1.2 Sampling Rate

The sampling rate (or sample rate) is how many conversions per second the ADC performs,
measured in samples per second (SPS or Sa/s).

```
    Sampling an Analog Signal
    =========================

    Analog signal (sine wave):
        _____
       /     \         /
      /       \       /
     /         \     /
    /           \   /
                 \_/

    Sampled at adequate rate (meets Nyquist):
    *   *   *
      *       *   *
                    *
    *               *
        Reconstructed signal is accurate

    Sampled too slowly (violates Nyquist):
    *           *
          *           *
    Reconstructed signal is WRONG (aliasing!)
```

### 1.3 Nyquist Theorem

The Nyquist-Shannon sampling theorem states:

```
    f_sample >= 2 * f_max

    where:
      f_sample = sampling frequency
      f_max    = highest frequency component in the signal
```

If the sampling rate is below the Nyquist rate, **aliasing** occurs -- high-frequency
components masquerade as lower frequencies, corrupting the measurement.

```
    Aliasing Example
    ================

    Original 900 Hz signal sampled at 1000 Sa/s:

    The ADC "sees" a 100 Hz signal instead!

    Signal:  900 Hz  -----> Appears as: 100 Hz
             f_alias = |f_signal - N * f_sample|

    Solution: Use an anti-aliasing filter (low-pass) BEFORE the ADC
              with cutoff at f_sample / 2
```

**In practice**, oversample by 5-10x the signal bandwidth for good results. For audio
(20 kHz bandwidth), sample at 44.1 kHz or higher.

### 1.4 Signal-to-Noise Ratio (SNR)

SNR describes how much useful signal power exists relative to noise.

```
    SNR (dB) = 6.02 * N + 1.76

    where N = number of bits

    Resolution    Theoretical SNR
    ---------     ---------------
     8-bit         49.9 dB
    10-bit         62.0 dB
    12-bit         74.0 dB
    16-bit         98.1 dB
    24-bit        146.2 dB
```

**Effective Number of Bits (ENOB)** is the real-world resolution after accounting for
noise, non-linearity, and other errors:

```
    ENOB = (SNR_measured - 1.76) / 6.02

    A 12-bit ADC might have ENOB of 10.5 bits in practice
    due to noise on the board, reference inaccuracy, etc.
```

### 1.5 Quantization Error

Every ADC introduces quantization error -- the difference between the true analog value
and the nearest digital level.

```
    Quantization Error
    ==================

    Max error = +/- 0.5 LSB

    For 12-bit ADC with 3.3V reference:
      LSB = 3.3V / 4096 = 0.806 mV
      Max error = +/- 0.403 mV

    Transfer Function:
                          ____
    Digital              |
    Output          _____|
                   |
              _____|
             |
        _____|
       |
    ___|_________________________
       0    1    2    3    4   ... LSBs
              Analog Input

    Each "step" is 1 LSB wide
    The midpoint of each step is the ideal value
```

---

## 2. ADC Types

Different ADC architectures trade off speed, resolution, cost, and power.

### 2.1 Successive Approximation Register (SAR) ADC

The most common type in microcontrollers (STM32, nRF52, AVR).

```
    SAR ADC Block Diagram
    =====================

    V_in ----+
             |    +----------+     +---------+
             +--->| Sample & |---->|         |
                  | Hold     |     |  SAR    |---> Digital Output
             +--->|          |     |  Logic  |     (N bits)
             |    +----------+     |         |
             |                     +----+----+
    V_ref ---+                          |
             |    +----------+          |
             +--->|  DAC     |<---------+
                  | (internal)|
                  +----------+

    Process (12-bit example):
    ========================
    1. Sample & Hold captures V_in
    2. Compare V_in with V_ref/2 (MSB test)
       - If V_in > V_ref/2: bit 11 = 1, subtract V_ref/2
       - If V_in < V_ref/2: bit 11 = 0
    3. Compare remainder with V_ref/4 (next bit)
    4. Repeat for all 12 bits
    5. Total: 12 clock cycles for 12-bit result
```

- **Speed**: 100 kSa/s to 5 MSa/s typical
- **Resolution**: 8 to 18 bits
- **Power**: Low to moderate
- **Used in**: Most microcontrollers

### 2.2 Flash (Parallel) ADC

The fastest architecture, uses 2^N - 1 comparators.

```
    Flash ADC (3-bit example)
    =========================

    V_ref ----+---- R
              |
              +---- Comparator 7 ---> Priority
              |                       Encoder
              +---- R                    |
              |                          |
              +---- Comparator 6 --->    |
              |                          |
              +---- R                    +---> 3-bit
              |                          |     output
              +---- Comparator 5 --->    |
              :                          :
              +---- Comparator 1 --->    |
              |
    GND ------+

    V_in connected to all comparators simultaneously
    Result available in ONE clock cycle!
```

- **Speed**: > 1 GSa/s possible
- **Resolution**: 4 to 8 bits (limited by comparator count)
- **Power**: Very high (2^N - 1 comparators)
- **Used in**: Oscilloscopes, RF receivers

### 2.3 Sigma-Delta (Delta-Sigma) ADC

Trades speed for very high resolution through oversampling and noise shaping.

```
    Sigma-Delta ADC Block Diagram
    ==============================

                  +---+     +---+     +----------+     +--------+
    V_in --->(+)--| I |---->| Q |---->| Decimation|---->| Digital|
              ^   +---+     +---+     | Filter    |     | Output |
              |  Integrator  1-bit    +----------+     +--------+
              |              ADC         (low-pass
              |                           + downsample)
              +-------[ 1-bit DAC ]<----+
                       (feedback)

    Oversampling at 64x-256x the Nyquist rate
    Noise is "shaped" to higher frequencies
    Decimation filter removes the noise
```

- **Speed**: 10 Sa/s to 100 kSa/s
- **Resolution**: 16 to 24 bits
- **Power**: Low to moderate
- **Used in**: Precision measurement, audio, load cells, temperature

### 2.4 Pipeline ADC

Multi-stage architecture, each stage resolves a few bits.

- **Speed**: 1 MSa/s to 250 MSa/s
- **Resolution**: 10 to 16 bits
- **Power**: Moderate to high
- **Used in**: Communications, imaging, data acquisition

### 2.5 Comparison Table

```
    ADC Type        Resolution   Speed         Power    Cost
    -----------     ----------   ----------    ------   ------
    Flash           4-8 bit      > 1 GSa/s     High     High
    Pipeline        10-16 bit    1-250 MSa/s   Medium   Medium
    SAR             8-18 bit     100k-5M Sa/s  Low      Low
    Sigma-Delta     16-24 bit    10-100k Sa/s  Low      Medium
```

---

## 3. Reference Voltages

The reference voltage determines the full-scale range of the ADC. Its accuracy and
stability directly affect measurement accuracy.

### 3.1 Reference Types

```
    Reference Voltage Options
    =========================

    1. Internal Reference (built into MCU)
       +--------+
       | MCU    |
       |  +---+ |
       |  |Vref| |----> Typically 1.2V or 2.5V
       |  +---+ |      Convenient but less accurate
       |        |      (+/- 1-5% typical)
       +--------+

    2. VDD as Reference
       VDD (3.3V) -----> ADC V_ref
       Simple but tracks supply noise and variation

    3. External Reference IC
       +--------+
       | LM4040 |----> 2.048V or 4.096V
       | REF3030|      (+/- 0.05% to 0.1%)
       +--------+      Stable, accurate, costs more

    4. Ratiometric Measurement
       VDD ---+--- Sensor (e.g., potentiometer)
              |
              +--- ADC V_ref
       Errors cancel because sensor and reference
       share the same supply!
```

### 3.2 Reference Accuracy Impact

```
    Example: 12-bit ADC, nominal V_ref = 3.300V

    If actual V_ref = 3.267V (1% low):

    Reading 2048 should mean: 2048/4095 * 3.300 = 1.650V
    Actually means:           2048/4095 * 3.267 = 1.634V
    Error:                    16 mV (1% of reading)

    For a temperature sensor at 10mV/C:
    Error = 1.6 degrees C -- significant!
```

### 3.3 Decoupling and Layout

```
    Proper ADC Reference Decoupling
    ================================

              V_ref
                |
               [ ] 10uF tantalum
                |
               [ ] 100nF ceramic (close to pin!)
                |
                +-----> ADC V_ref pin
                |
               [ ] 10nF ceramic (optional, for HF noise)
                |
               GND (dedicated analog ground)

    PCB Layout Rules:
    - Keep analog and digital grounds separate
    - Star-ground at a single point
    - Route analog traces away from digital signals
    - Use ground planes under analog circuits
```

---

## 4. DMA-Driven ADC

Direct Memory Access (DMA) allows the ADC to transfer conversion results directly to
memory without CPU intervention, essential for high-speed or multi-channel sampling.

### 4.1 Why Use DMA?

```
    Without DMA (Polling / Interrupt):
    ===================================

    CPU: [Start ADC]--[Wait]--[Read]--[Process]--[Start ADC]--[Wait]--...
                       ^^^^    ^^^^
                       CPU is busy or interrupted constantly

    With DMA:
    =========

    CPU:  [Configure]....[Process buffer].........[Process buffer]....
    DMA:  [ADC->MEM][ADC->MEM][ADC->MEM][ADC->MEM][ADC->MEM]...
    ADC:  [Conv][Conv][Conv][Conv][Conv][Conv][Conv][Conv]...

         ^                              ^
         CPU is free!                   DMA interrupt: buffer ready
```

### 4.2 DMA Circular Mode

```
    DMA Circular Buffer for Multi-Channel ADC
    ==========================================

    ADC channels: CH0 (temp), CH1 (light), CH2 (battery)

    DMA Buffer in RAM (circular):
    +------+------+------+------+------+------+------+------+------+
    | CH0  | CH1  | CH2  | CH0  | CH1  | CH2  | CH0  | CH1  | CH2  |
    +------+------+------+------+------+------+------+------+------+
    | Sample 0           | Sample 1           | Sample 2           |
    |<--- Half Complete ->|<--- Full Complete ->|
         (HT interrupt)       (TC interrupt)

    DMA writes continuously in a circle
    CPU processes one half while DMA fills the other
    This is "double buffering" or "ping-pong" buffering
```

### 4.3 DMA Configuration Steps

```
    1. Configure ADC:
       - Enable scan mode (multi-channel)
       - Set sequence and channel order
       - Enable continuous conversion or timer trigger
       - Enable DMA request

    2. Configure DMA:
       - Source: ADC data register
       - Destination: RAM buffer
       - Transfer size: half-word (16-bit for 12-bit ADC)
       - Mode: circular
       - Enable half-transfer and transfer-complete interrupts

    3. Start:
       - Enable DMA channel
       - Start ADC conversion
       - CPU is now free for other tasks
```

---

## 5. DAC Basics

A DAC converts a digital value to an analog voltage -- the reverse of an ADC.

### 5.1 DAC Operation

```
    DAC Transfer Function
    =====================

    V_out = (Digital Value / 2^N) * V_ref

    12-bit DAC with 3.3V reference:
    Value 0    -> 0.000V
    Value 2048 -> 1.650V
    Value 4095 -> 3.299V  (V_ref * (4095/4096))

    Output waveform with buffered updates:
                      ____
                     |    |
                _____|    |____
               |              |
          _____|              |_____
         |                          |
    _____|                          |_____
    0   1024  2048  3072  4095

    Staircase output -- needs low-pass filter for smooth waveforms
```

### 5.2 DAC Architectures

```
    R-2R Ladder DAC (common in MCUs)
    ================================

    MSB                                    LSB
    b3 ---[2R]---+---[2R]---+---[2R]---+---[2R]---+
                 |           |           |           |
                [R]         [R]         [R]         [R]
                 |           |           |           |
                GND         GND         GND         GND

    Simple, monotonic, good linearity
    Used in most microcontroller DACs

    PWM as "Poor Man's DAC"
    =======================

    PWM Output:  _____     _____     _____
                |     |   |     |   |     |
    ____________|     |___|     |___|     |___

    After RC low-pass filter:
    ____________________________________________
                ~~ 2.5V DC (for 75% duty) ~~

    V_out = Duty_Cycle * V_supply
    Cheap but slow settling, limited bandwidth
```

### 5.3 DAC Applications

- **Waveform generation**: sine, triangle, sawtooth, arbitrary
- **Voltage reference**: programmable bias voltages
- **Audio output**: music, tones, speech
- **Control loops**: setting motor speed, LED brightness (analog dimming)
- **Calibration**: generating known test voltages

---

## 6. Sensor Types

### 6.1 Resistive Sensors

Sensors whose resistance changes with the measured quantity.

```
    Voltage Divider for Resistive Sensors
    ======================================

    VDD (3.3V)
     |
    [R_fixed]  (known resistor)
     |
     +---------> ADC input (V_out)
     |
    [R_sensor]  (variable: thermistor, LDR, FSR, etc.)
     |
    GND

    V_out = VDD * R_sensor / (R_fixed + R_sensor)

    Sensor Types:
    +---------------+------------------+-------------------+
    | Sensor        | Measures         | R range           |
    +---------------+------------------+-------------------+
    | NTC Thermistor| Temperature      | 100 - 500k ohm   |
    | PTC Thermistor| Temperature      | varies            |
    | LDR (CdS)    | Light intensity  | 1k - 10M ohm     |
    | FSR           | Force/pressure   | 100 - 10M ohm    |
    | Flex sensor   | Bend angle       | 10k - 100k ohm   |
    | Potentiometer | Position/angle   | fixed total R     |
    +---------------+------------------+-------------------+
```

### 6.2 Voltage Output Sensors

Sensors that directly output an analog voltage.

```
    Common Voltage Output Sensors:
    +-------------------+------------------+------------------+
    | Sensor            | Output           | Example          |
    +-------------------+------------------+------------------+
    | TMP36             | 10mV/C + 500mV   | 25C = 750mV     |
    | LM35              | 10mV/C           | 25C = 250mV     |
    | Sharp IR distance | 0.4V - 3.1V     | non-linear       |
    | Piezo vibration   | AC voltage       | needs rectifier  |
    | Microphone (amp)  | 0 - VDD         | AC coupled       |
    +-------------------+------------------+------------------+

    TMP36 Temperature Sensor:
    V_out = 0.5V + (Temperature * 0.01V/C)
    Temperature = (V_out - 0.5) / 0.01

    Example: V_out = 0.75V => T = (0.75 - 0.5)/0.01 = 25 C
```

### 6.3 Current Output Sensors

4-20 mA industrial sensors.

```
    4-20 mA Current Loop
    =====================

    +24V ----+
             |
        [ Sensor ]  (e.g., pressure transmitter)
             |
             |  4 mA = 0% of range
             | 20 mA = 100% of range
             |
            [R_sense] = 250 ohm
             |
             +------> ADC input
             |         (1V to 5V)
            GND

    V_adc = I_sensor * R_sense
    4 mA  * 250 = 1.0V (minimum)
    20 mA * 250 = 5.0V (maximum)

    Advantage: Noise immune, long cable runs, wire-break detection (0 mA)
```

### 6.4 Digital Sensors (I2C / SPI)

Many modern sensors have built-in ADCs and communicate digitally.

```
    Digital Sensor Examples:
    +-----------------+----------+----------+------------------+
    | Sensor          | Protocol | Measures | Resolution       |
    +-----------------+----------+----------+------------------+
    | BME280          | I2C/SPI  | T, H, P  | 16-20 bit       |
    | MPU6050         | I2C      | Accel+Gyro| 16-bit          |
    | ADS1115         | I2C      | Voltage  | 16-bit ADC      |
    | MAX31855        | SPI      | Thermo-  | 14-bit          |
    |                 |          | couple   |                  |
    | BH1750          | I2C      | Light    | 16-bit lux      |
    +-----------------+----------+----------+------------------+

    Advantages: Higher resolution, built-in calibration,
                no analog routing issues
    Disadvantages: Slower update rates, more complex protocol
```

---

## 7. Signal Conditioning

Raw sensor signals often need conditioning before ADC conversion.

### 7.1 Amplification

```
    Non-Inverting Op-Amp (Signal Amplification)
    ============================================

    Sensor output: 0 - 100mV (too small for 0-3.3V ADC)
    Need gain of 33x

                    +-----+
    V_in ----+----->|+    |
             |      | Op  |----+----> V_out (0 - 3.3V)
             |  +-->|-    |    |
             |  |   +-----+   |
             |  |              |
             |  +---[R2=32k]---+
             |  |
             |  [R1=1k]
             |  |
             | GND
             |
    Gain = 1 + R2/R1 = 1 + 32k/1k = 33

    Instrumentation Amplifier (for differential signals):
    Used with bridge sensors (load cells, strain gauges)
    Gain = 1 + (50k / R_gain)
    CMRR > 80 dB -- rejects common-mode noise
```

### 7.2 Filtering

```
    Anti-Aliasing Low-Pass Filter (RC)
    ====================================

    Sensor --+--[R]--+---> ADC input
             |       |
             |      [C]
             |       |
             |      GND

    f_cutoff = 1 / (2 * pi * R * C)

    Example: R = 10k, C = 100nF
    f_cutoff = 1 / (2 * 3.14159 * 10000 * 0.0000001)
             = 159 Hz

    This removes frequencies above 159 Hz before ADC sampling.

    For better rolloff, use 2nd or 4th order active filters.
```

### 7.3 Level Shifting and Clamping

```
    Level Shifting (bipolar to unipolar)
    =====================================

    Sensor output: -2.5V to +2.5V
    ADC input:      0V to 3.3V

                   R1        R2
    V_sensor ---[10k]---+---[10k]---> V_ref_mid (1.65V)
                        |
                        +-----------> ADC input
                        |
                       [C]  (optional filter)
                        |
                       GND

    V_adc = V_sensor * (R2/(R1+R2)) + V_ref_mid * (R1/(R1+R2))

    Clamping (overvoltage protection):
    ===================================

    V_in ---[R_series]---+---> ADC pin
                         |
                     [D1] to VDD (Schottky)
                         |
                     [D2] to GND (Schottky)

    Clamps input to GND-0.3V ... VDD+0.3V
```

### 7.4 Voltage Divider for High Voltages

```
    Measuring 12V Battery with 3.3V ADC
    =====================================

    V_battery (0-15V)
     |
    [R1 = 30k]
     |
     +---------> ADC input (0 - 3.3V max)
     |
    [R2 = 10k]
     |
    GND

    V_adc = V_battery * R2 / (R1 + R2)
          = V_battery * 10k / 40k
          = V_battery / 4

    V_battery = V_adc * 4

    15V * 10k/40k = 3.75V --> too high! Add margin:
    Use R1=39k, R2=10k: 15V * 10k/49k = 3.06V (safe)
```

---

## 8. Calibration

Calibration corrects systematic errors between the measured and true values.

### 8.1 Single-Point vs Multi-Point Calibration

```
    Single-Point (Offset) Calibration
    ===================================

    Apply known reference, measure offset:
    True value:     1.000V
    Measured:       1.023V
    Offset:        +0.023V

    Corrected = Measured - 0.023V

    Two-Point (Gain + Offset) Calibration
    ======================================

    Point 1: True = 0.000V, Measured = 0.012V (offset)
    Point 2: True = 3.000V, Measured = 3.045V

    Gain error  = (3.045 - 0.012) / (3.000 - 0.000) = 1.011
    Offset      = 0.012V

    Corrected = (Measured - Offset) / Gain
              = (Measured - 0.012) / 1.011

    Multi-Point Calibration (Lookup Table)
    =======================================

    Measured  | True
    ----------+---------
    0.012     | 0.000
    0.520     | 0.500      Use linear interpolation
    1.035     | 1.000      between calibration points
    1.548     | 1.500
    2.060     | 2.000
    2.570     | 2.500
    3.045     | 3.000
```

### 8.2 Thermistor Calibration: Steinhart-Hart

```
    NTC Thermistor: Resistance vs Temperature
    ==========================================

    R(T) = R_25 * exp(B * (1/T - 1/298.15))

    where:
      R_25 = resistance at 25C (e.g., 10k ohm)
      B    = material constant (e.g., 3950 K)
      T    = temperature in Kelvin

    Steinhart-Hart equation (more accurate):
    1/T = A + B*ln(R) + C*(ln(R))^3

    Lookup table approach (fastest on MCU):
    ADC Value  | Temperature (C * 10)
    -----------+--------------------
    100        | 850   (85.0 C)
    200        | 650   (65.0 C)
    500        | 420   (42.0 C)
    1000       | 290   (29.0 C)
    2000       | 150   (15.0 C)
    3000       | 30    (3.0 C)
    3500       | -50   (-5.0 C)
    4000       | -200  (-20.0 C)
```

---

## 9. Digital Filtering

### 9.1 Moving Average Filter

The simplest and most effective filter for removing random noise.

```
    Moving Average Filter
    =====================

    Window size N = 4

    Input:   102  98  105  100  97  103  101  99
                  |----- window ------|
    Output:              101.25

    y[n] = (1/N) * sum(x[n-k], k=0..N-1)

    Efficient implementation (running average):
    y[n] = y[n-1] + (x[n] - x[n-N]) / N

    Removes random noise but adds latency (N/2 samples)

    Frequency Response:
    |H(f)|
    1.0 |****
        |    ***
        |       **
    0.5 |         **
        |           *
        |            **
    0.0 |______________*____________
        0   fs/2N    fs/N   fs/2
        Nulls at multiples of fs/N
```

### 9.2 Exponential Moving Average (EMA)

```
    Exponential Moving Average (IIR filter)
    ========================================

    y[n] = alpha * x[n] + (1 - alpha) * y[n-1]

    alpha = smoothing factor (0 < alpha < 1)

    alpha close to 1: fast response, less filtering
    alpha close to 0: slow response, more filtering

    Equivalent time constant: tau = T_sample / alpha

    Implementation (integer math, no float!):
    // alpha = 1/16 (shift by 4)
    filtered = filtered + (raw - filtered) / 16;

    // Or using fixed-point (alpha = 0.1, scale = 256):
    // alpha_fp = 26 (0.1 * 256)
    filtered = ((256 - 26) * filtered + 26 * raw) / 256;
```

### 9.3 Median Filter

```
    Median Filter (removes impulse noise / spikes)
    ================================================

    Window size = 5

    Input:  100  102  250  101  99
                      ^^^
                      spike!

    Sort window: 99, 100, 101, 102, 250
    Median:      101  (spike removed!)

    Moving Average would give: (100+102+250+101+99)/5 = 130.4
    Median is immune to outliers!

    Best for: salt-and-pepper noise, sensor glitches
    Cost: Requires sorting (O(N log N) or insertion sort O(N^2))
```

### 9.4 Kalman Filter

```
    Kalman Filter (1D simplified)
    ==============================

    Optimal recursive estimator for noisy measurements.

    State: x (estimated value)
    Uncertainty: P (estimation error covariance)

    Predict:
      x_pred = x_prev          (simple model: value stays same)
      P_pred = P_prev + Q      (Q = process noise)

    Update:
      K = P_pred / (P_pred + R)   (K = Kalman gain, R = measurement noise)
      x = x_pred + K * (measurement - x_pred)
      P = (1 - K) * P_pred

    When K is large (high P, low R): trust measurement more
    When K is small (low P, high R): trust prediction more

    The filter automatically adapts its gain over time!

    Tuning:
      Q large -> filter follows changes quickly (more noise)
      Q small -> filter is smooth but slow to respond
      R large -> measurements are noisy, filter more
      R small -> measurements are clean, follow closely
```

---

## 10. Practical Considerations

### 10.1 ADC Input Impedance

```
    ADC Input Impedance Matters!
    ============================

    Most SAR ADCs have a sample-and-hold capacitor (~5-20 pF)
    that must charge to the correct voltage during sampling.

    High-impedance source (> 10k ohm):
    V_source ---[R_source = 100k]---+--- ADC
                                    |
                                   [C_sh = 10pF]
                                    |
                                   GND

    Time constant = R_source * C_sh = 100k * 10pF = 1 us
    Need ~5 tau for settling = 5 us
    If sample time < 5 us, reading will be WRONG!

    Solutions:
    1. Increase ADC sample time (most MCUs allow this)
    2. Add a buffer op-amp (unity gain)
    3. Add a capacitor at ADC input (100nF) to provide charge
    4. Use lower impedance voltage divider resistors
```

### 10.2 Grounding and Noise

```
    Noise Reduction Checklist
    =========================

    [x] Separate analog and digital power supply decoupling
    [x] Use AVDD/AGND pins (not DVDD/DGND) for ADC supply
    [x] Add 100nF + 10uF decoupling on AVDD
    [x] Keep ADC input traces short
    [x] Route analog traces away from PWM, SPI, clock lines
    [x] Use ground plane under analog section
    [x] Sample ADC when noisy peripherals (PWM, radio) are idle
    [x] Average multiple samples to reduce noise by sqrt(N)
    [x] Use shielded cables for remote sensors
```

### 10.3 Oversampling for Extra Resolution

```
    Oversampling and Decimation
    ===========================

    To gain 1 extra bit of resolution:
    Take 4x more samples and average

    To gain N extra bits:
    Take 4^N more samples and average

    Example: 12-bit ADC -> 14-bit effective resolution
    Extra bits needed: 2
    Oversampling ratio: 4^2 = 16
    Take 16 samples, sum them, divide by 4 (shift right 2)

    12-bit ADC (4096 levels) -> effectively 14-bit (16384 levels)

    Requirement: Input signal must have noise >= 1 LSB
    (noise acts as a natural dither)
```

---

## Examples

| File | Description |
|------|-------------|
| `examples/01_adc_single.c` | Single-channel ADC read with polling |
| `examples/02_adc_dma.c` | Multi-channel ADC with DMA circular buffer |
| `examples/03_thermistor.c` | NTC thermistor with lookup table and interpolation |
| `examples/04_light_sensor.c` | LDR light sensor with auto-ranging |
| `examples/05_dac_sine.c` | DAC sine wave generator with timer |
| `examples/06_moving_average.c` | Moving average filter implementation |
| `examples/07_kalman_filter.c` | Simple 1D Kalman filter for sensor data |

## Exercises

| File | Description |
|------|-------------|
| `exercises/exercise_01.md` | ADC resolution vs speed tradeoff experiment |
| `exercises/exercise_02.md` | Battery voltage monitor with low-battery alert |
| `exercises/exercise_03.md` | Joystick analog input (2-axis) |
| `exercises/exercise_04.md` | DAC arbitrary waveform generator |
| `exercises/exercise_05.md` | Digital thermometer with calibration |
| `exercises/exercise_06.md` | Load cell / strain gauge interface |
| `exercises/exercise_07.md` | Sensor fusion: accelerometer + gyroscope complementary filter |

## Project

**Environmental Monitor** -- see `project/README.md`

Read temperature, humidity, light, and air quality sensors. Apply calibration curves and
digital filtering. Display readings with trend graphs on LCD. Alert on configurable
thresholds.

---

## Further Reading

- STM32 ADC Application Note (AN2834)
- "The Scientist and Engineer's Guide to Digital Signal Processing" by Steven W. Smith
- Analog Devices MT-series tutorials on ADC/DAC
- Texas Instruments "Data Converters" application reports
- Kalman Filter explanation: www.bzarg.com/p/how-a-kalman-filter-works-in-pictures/
