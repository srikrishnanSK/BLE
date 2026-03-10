# Module 08: BLE Advanced — GATT Profiles, Mesh, OTA

## Overview

This module covers advanced Bluetooth Low Energy topics essential for building
production-grade embedded systems. You will learn how to design custom GATT
profiles, build BLE Mesh networks, implement over-the-air firmware updates,
exploit BLE 5.x features, optimize throughput, and manage multiple simultaneous
connections.

---

## Table of Contents

1. [Custom GATT Profile Design](#1-custom-gatt-profile-design)
2. [BLE Mesh Networking](#2-ble-mesh-networking)
3. [Over-the-Air (OTA) Firmware Update](#3-over-the-air-ota-firmware-update)
4. [BLE 5.x Features](#4-ble-5x-features)
5. [Throughput Optimization](#5-throughput-optimization)
6. [Multi-Connection Management](#6-multi-connection-management)
7. [Security Considerations](#7-security-considerations)

---

## 1. Custom GATT Profile Design

### 1.1 GATT Architecture Recap

The Generic Attribute Profile (GATT) defines how two BLE devices exchange data
using Services and Characteristics. The hierarchy is:

```
Profile
 └── Service (identified by UUID)
      └── Characteristic (identified by UUID)
           ├── Value
           ├── Properties (read, write, notify, indicate)
           └── Descriptors
                ├── Client Characteristic Configuration (CCC)
                ├── Characteristic User Description
                └── Custom Descriptors
```

### 1.2 UUID Strategy

BLE defines two UUID sizes:

| Type | Size | Usage |
|------|------|-------|
| SIG-adopted | 16-bit | Standard profiles (Heart Rate, Battery, etc.) |
| Vendor-specific | 128-bit | Custom services and characteristics |

A common pattern for vendor UUIDs is to define a base UUID and vary one octet:

```
Base:  xxxxxxxx-0000-1000-8000-00805F9B34FB   (Bluetooth Base UUID)
Custom: 12340001-5678-9ABC-DEF0-123456789ABC  (your base)
         ^^^^---^ vary these for each characteristic
```

### 1.3 Designing a Custom Profile

When designing a custom GATT profile, follow these principles:

1. **Group related data into Services.** A smart light might have a Light
   Control Service and a Device Information Service.

2. **Choose characteristic properties carefully:**
   - `READ` — for values the client polls (sensor readings, device status)
   - `WRITE` — for commands (turn on/off, set brightness)
   - `WRITE_WITHOUT_RESPONSE` — for latency-sensitive commands (color streaming)
   - `NOTIFY` — for server-initiated updates (state changes, alarms)
   - `INDICATE` — like notify but with acknowledgment (critical alerts)

3. **Use appropriate value formats.** Define data types, byte order (little-endian
   per BLE spec), and valid ranges.

4. **Add CCC descriptors** to every characteristic that supports NOTIFY or
   INDICATE so clients can subscribe.

5. **Document your profile** with a specification including UUID assignments,
   data formats, and state machines.

### 1.4 Example: Smart Light Profile

```
Smart Light Service (UUID: 0xFF10)
 ├── Brightness   (0xFF11) — uint8, READ | WRITE | NOTIFY, range 0-100
 ├── Color RGB    (0xFF12) — uint8[3], READ | WRITE | NOTIFY
 ├── Power State  (0xFF13) — uint8, READ | WRITE | NOTIFY, 0=off 1=on
 ├── Scene ID     (0xFF14) — uint8, READ | WRITE, 0-9
 └── Fade Time    (0xFF15) — uint16 (ms), READ | WRITE, range 0-10000
```

### 1.5 Access Control

Each characteristic can enforce access control:

- **Open** — no authentication required
- **Encrypted** — requires an encrypted link (pairing)
- **Authenticated** — requires MITM-protected pairing
- **Authorized** — application-level authorization check

---

## 2. BLE Mesh Networking

### 2.1 What is BLE Mesh?

BLE Mesh (Bluetooth Mesh Profile, adopted 2017) enables many-to-many
communication over BLE. Unlike point-to-point GATT connections, Mesh uses a
managed flood network where messages are relayed across nodes.

Key properties:

- **No connection required** — uses advertising and scanning
- **Managed flooding** — TTL and message caching prevent infinite loops
- **Relay nodes** — extend network range by forwarding messages
- **Publish/Subscribe model** — nodes publish to groups; subscribers receive
- **Provisioning** — secure process to add nodes to the network
- **Up to 32,767 nodes** per network

### 2.2 Mesh Architecture

```
┌─────────────────────────────────────────────────┐
│                  Applications                    │
├─────────────────────────────────────────────────┤
│              Foundation Models                   │
│  (Configuration, Health, Remote Provisioning)    │
├─────────────────────────────────────────────────┤
│                Access Layer                      │
│  (Encryption/Decryption, Model addressing)       │
├─────────────────────────────────────────────────┤
│             Upper Transport Layer                │
│  (Application-level encryption, segmentation)    │
├─────────────────────────────────────────────────┤
│             Lower Transport Layer                │
│  (Segmentation and reassembly)                   │
├─────────────────────────────────────────────────┤
│               Network Layer                      │
│  (Network encryption, relay, proxy)              │
├─────────────────────────────────────────────────┤
│               Bearer Layer                       │
│  (Advertising bearer, GATT bearer)               │
├─────────────────────────────────────────────────┤
│              BLE Core (HCI)                      │
└─────────────────────────────────────────────────┘
```

### 2.3 Node Roles

| Role | Description |
|------|-------------|
| **Unprovisioned Device** | Not yet part of any mesh network |
| **Node** | Provisioned device, can send/receive mesh messages |
| **Relay Node** | Forwards messages to extend range |
| **Proxy Node** | Bridges GATT clients (phones) into the mesh network |
| **Friend Node** | Stores messages for Low Power Nodes |
| **Low Power Node (LPN)** | Sleeps most of the time, polls Friend for messages |
| **Provisioner** | Adds new devices to the network, assigns addresses and keys |

### 2.4 Addressing

- **Unicast address** (0x0001–0x7FFF) — unique per element
- **Group address** (0xC000–0xFEFF) — multicast groups (e.g., "all kitchen lights")
- **Virtual address** — 128-bit UUID hashed to 16 bits
- **Unassigned** (0x0000) — not allocated

### 2.5 Security in Mesh

BLE Mesh uses three levels of encryption keys:

1. **Network Key (NetKey)** — shared by all nodes in the network; encrypts the
   network layer. A node with the NetKey can relay but not read application data.

2. **Application Key (AppKey)** — shared by nodes that need to exchange
   application data. Bound to a NetKey. Multiple AppKeys allow access control
   (e.g., lights use AppKey-1, locks use AppKey-2).

3. **Device Key (DevKey)** — unique per node, used during provisioning and
   configuration.

### 2.6 Provisioning Process

```
Provisioner                    Unprovisioned Device
     |                                |
     |--- Provisioning Invite ------->|
     |<-- Provisioning Capabilities --|
     |--- Provisioning Start -------->|
     |                                |
     |   (ECDH key exchange)          |
     |--- Provisioning Public Key --->|
     |<-- Provisioning Public Key ----|
     |                                |
     |   (Authentication: OOB/None)   |
     |--- Provisioning Confirmation ->|
     |<-- Provisioning Confirmation --|
     |--- Provisioning Random ------->|
     |<-- Provisioning Random --------|
     |                                |
     |   (Provisioning data, encrypted)|
     |--- Provisioning Data --------->|
     |     (NetKey, IV Index, Address) |
     |<-- Provisioning Complete ------|
```

### 2.7 Models

Mesh functionality is organized into **Models**:

- **SIG Models** — standardized (Generic OnOff, Level, Lightness, CTL, HSL, Sensor)
- **Vendor Models** — custom, identified by Company ID + Model ID

Each model defines:
- **States** (e.g., OnOff state, Level state)
- **Messages** (GET, SET, SET_UNACKNOWLEDGED, STATUS)
- **Bindings** between states

---

## 3. Over-the-Air (OTA) Firmware Update

### 3.1 Why OTA?

Deployed embedded devices need firmware updates for:
- Bug fixes and security patches
- Feature additions
- Compliance updates
- Performance improvements

Without OTA, physical access is required — impractical for deployed fleets.

### 3.2 OTA Architecture

A typical OTA system has these components:

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│  OTA Server  │────>│  BLE Link    │────>│  Target MCU  │
│  (Phone/GW)  │     │              │     │              │
│              │     │  - Chunked   │     │  - Validate  │
│  - Firmware  │     │    transfer  │     │  - Write to  │
│    image     │     │  - Flow      │     │    flash     │
│  - Metadata  │     │    control   │     │  - Verify    │
│              │     │  - Progress  │     │  - Reboot    │
└──────────────┘     └──────────────┘     └──────────────┘
```

### 3.3 Flash Partition Layout

Most OTA implementations use a dual-bank (A/B) scheme:

```
Flash Memory Map:
┌─────────────────┐ 0x00000
│   Bootloader    │
├─────────────────┤ 0x10000
│  Partition      │
│  Table          │
├─────────────────┤ 0x11000
│  OTA Data       │
│  (active slot)  │
├─────────────────┤ 0x12000
│  App Slot A     │
│  (factory/ota0) │
├─────────────────┤ 0x112000
│  App Slot B     │
│  (ota1)         │
├─────────────────┤ 0x212000
│  NVS / Config   │
└─────────────────┘
```

**Boot flow:**
1. Bootloader reads OTA Data partition to determine active slot.
2. Validates image header and hash of the active slot.
3. If valid, boots the active slot. If invalid, falls back to the other slot.

### 3.4 OTA Transfer Protocol

A robust OTA protocol should include:

1. **Metadata exchange** — firmware version, image size, checksum (SHA-256)
2. **Chunked transfer** — split image into MTU-sized blocks
3. **Flow control** — acknowledge each block or use a sliding window
4. **Integrity verification** — CRC per block, SHA-256 over entire image
5. **Commit/rollback** — mark new image as valid only after verification
6. **Resume support** — track last successfully written offset

### 3.5 Security Considerations for OTA

- **Sign firmware images** with a private key; verify signature on device
- **Encrypt the BLE link** (use bonded/encrypted connections)
- **Use secure boot** to prevent running unsigned images
- **Version anti-rollback** — reject images with version <= current
- **Rate-limit retries** to prevent denial-of-service

---

## 4. BLE 5.x Features

### 4.1 BLE 5.0 Key Features

| Feature | BLE 4.2 | BLE 5.0 |
|---------|---------|---------|
| Advertising payload | 31 bytes | 255 bytes (extended) |
| Advertising sets | 1 | Multiple simultaneous |
| PHY options | 1M only | 1M, 2M, Coded (125k/500k) |
| Broadcast range | Standard | 4x (Coded PHY) |
| Broadcast throughput | Standard | 2x (2M PHY) |

### 4.2 PHY Options

BLE 5 introduces three PHY (Physical Layer) options:

| PHY | Symbol Rate | Range | Throughput | Use Case |
|-----|-------------|-------|------------|----------|
| **LE 1M** | 1 Msym/s | Standard | ~1 Mbps | Default, backward compatible |
| **LE 2M** | 2 Msym/s | Shorter | ~2 Mbps | High throughput, low latency |
| **LE Coded S=2** | 1 Msym/s | ~2x | ~500 kbps | Medium range |
| **LE Coded S=8** | 1 Msym/s | ~4x | ~125 kbps | Long range (hundreds of meters) |

### 4.3 Extended Advertising

Extended advertising in BLE 5 enables:

- **Advertising Data up to 1,650 bytes** (chained across fragments)
- **Multiple advertising sets** — advertise different data simultaneously
- **Secondary advertising channels** — uses data channels (ch 0–36) in addition
  to primary channels (ch 37, 38, 39)
- **Periodic advertising** — predictable schedule, no connection needed

```
Primary ADV channels (37, 38, 39):
  └── AUX_ADV_IND (pointer to secondary channel)
        └── Secondary data channel:
              └── Extended ADV data (up to 255 bytes per fragment)
                    └── AUX_CHAIN_IND (if more data)
```

### 4.4 BLE 5.1 — Direction Finding

- **Angle of Arrival (AoA)** — receiver has antenna array, calculates direction
- **Angle of Departure (AoD)** — transmitter has antenna array
- Enables centimeter-level indoor positioning

### 4.5 BLE 5.2 — LE Audio and Isochronous Channels

- **LE Audio** — LC3 codec for high-quality audio over BLE
- **Isochronous Channels** — time-bounded data delivery
- **Multi-Stream Audio** — independent streams to left/right earbuds

### 4.6 BLE 5.3 and 5.4

- **Channel classification** — peripheral can inform central about channel quality
- **Connection subrating** — dynamically adjust connection interval
- **Periodic Advertising with Responses (PAwR)** — bidirectional periodic ads
- **Encrypted Advertising Data (EAD)** — encrypt advertising payload

---

## 5. Throughput Optimization

### 5.1 Theoretical Maximum

BLE throughput depends on several parameters:

```
Throughput = (PDU_payload × packets_per_interval) / connection_interval

Where:
  PDU_payload = min(ATT_MTU - 3, Link_Layer_PDU - 4)
  packets_per_interval depends on connection interval and radio time
```

| Parameter | Value | Effect |
|-----------|-------|--------|
| ATT MTU | 23 (default) → 517 | Higher MTU = fewer headers |
| Connection Interval | 7.5ms – 4s | Shorter = more throughput |
| Data Length Extension | 27 → 251 bytes | Fewer packets needed |
| PHY | 1M / 2M | 2M doubles raw throughput |
| TX Power | -20 to +20 dBm | Higher power = fewer retransmissions |

### 5.2 Practical Optimization Steps

1. **Negotiate maximum MTU** — request 517 bytes (BLE 4.2+)
2. **Enable Data Length Extension (DLE)** — increase LL PDU from 27 to 251 bytes
3. **Use 2M PHY** — double the symbol rate (BLE 5.0+)
4. **Minimize connection interval** — request 7.5ms to 15ms
5. **Use Write Without Response** — avoids per-packet ACK at GATT layer
6. **Use Notifications** — server pushes without waiting for reads
7. **Batch data** — fill each packet to maximum MTU
8. **Minimize slave latency** — set to 0 for throughput-critical transfers

### 5.3 Achievable Throughput

| Configuration | Approximate Throughput |
|---------------|----------------------|
| Default (MTU 23, 1M PHY) | ~32 kbps |
| MTU 247 + DLE + 1M PHY | ~700 kbps |
| MTU 247 + DLE + 2M PHY | ~1.3 Mbps |
| MTU 512 + DLE + 2M PHY | ~1.4 Mbps |

---

## 6. Multi-Connection Management

### 6.1 Central vs. Peripheral Roles

A BLE device can act as:
- **Central** — initiates connections, scans for peripherals
- **Peripheral** — advertises, accepts connections
- **Both simultaneously** — common in gateways and hub devices

### 6.2 Multi-Connection Challenges

| Challenge | Description | Mitigation |
|-----------|-------------|------------|
| Scheduling | Radio is shared among connections | Stagger connection intervals |
| Memory | Each connection uses ~1-4 KB RAM | Limit max connections |
| Power | More connections = more radio time | Increase connection intervals |
| Collision | Events may overlap | Use connection interval offsets |
| Supervision | Must monitor all links | Per-connection timeout handling |

### 6.3 Connection Scheduling

When managing multiple connections, avoid overlap:

```
Connection Interval = 30ms

Conn 0: |---E---|                         |---E---|
Conn 1:          |---E---|                         |---E---|
Conn 2:                   |---E---|                         |---E---|
         0      10       20       30       40       50       60 ms
```

Best practices:
- Use the same connection interval for all connections when possible
- Offset each connection by `interval / N` where N is the number of connections
- Use a connection interval of at least `N × 7.5ms`

### 6.4 ESP-IDF Multi-Connection Limits

| Parameter | ESP32 | ESP32-S3 | ESP32-C3 |
|-----------|-------|----------|----------|
| Max connections (central) | 7 | 7 | 3 |
| Max connections (peripheral) | 3 | 3 | 3 |
| Max total | 9 | 9 | 6 |

---

## 7. Security Considerations

### 7.1 Pairing Methods

| Method | MITM Protection | User Interaction |
|--------|-----------------|------------------|
| Just Works | No | None |
| Passkey Entry | Yes | 6-digit code displayed/entered |
| Numeric Comparison | Yes | User confirms matching numbers |
| Out of Band (OOB) | Yes | NFC tap, QR scan, etc. |

### 7.2 Security Levels

| Level | Description |
|-------|-------------|
| Mode 1 Level 1 | No security (open) |
| Mode 1 Level 2 | Unauthenticated encryption (Just Works) |
| Mode 1 Level 3 | Authenticated encryption (MITM protection) |
| Mode 1 Level 4 | LE Secure Connections + authenticated |

### 7.3 Best Practices

- Use LE Secure Connections (LESC) with ECDH key exchange (BLE 4.2+)
- Enable bonding to store keys for reconnection
- Use authenticated pairing for sensitive applications (locks, medical)
- Implement application-layer encryption for mesh networks
- Rotate keys periodically for long-lived deployments
- Use privacy features (resolvable private addresses) to prevent tracking

---

## Examples

| File | Description |
|------|-------------|
| `examples/01_custom_gatt.c` | Custom GATT profile for a smart light |
| `examples/02_ble_mesh.c` | BLE Mesh light and switch nodes |
| `examples/03_ota_update.c` | OTA firmware update over BLE |
| `examples/04_high_throughput.c` | High-throughput data transfer |
| `examples/05_extended_adv.c` | BLE 5 extended advertising |
| `examples/06_multi_connect.c` | Multi-peripheral connection manager |

## Exercises

| Exercise | Topic |
|----------|-------|
| 01 | Design a GATT profile for a smart plant monitor |
| 02 | Implement a BLE Mesh relay node |
| 03 | OTA update with rollback support |
| 04 | BLE 5 long-range communication test |
| 05 | Throughput benchmarking |
| 06 | BLE security with MITM-protected pairing |

## Project

Build a complete **BLE Mesh Sensor Network** with 3+ nodes: sensor nodes, relay
nodes, a gateway, provisioning, health monitoring, and OTA update capability.

---

## References

- Bluetooth Core Specification v5.4 — bluetooth.com
- Bluetooth Mesh Profile Specification v1.1
- ESP-IDF BLE Mesh documentation — docs.espressif.com
- Bluetooth SIG GATT specifications — bluetooth.com/specifications/gatt
- "Getting Started with Bluetooth Low Energy" by Townsend et al. (O'Reilly)
