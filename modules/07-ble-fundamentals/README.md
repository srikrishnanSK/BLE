# Module 07: BLE Fundamentals

## Overview

Bluetooth Low Energy (BLE), introduced in the Bluetooth 4.0 specification, is a wireless
technology designed for short-range communication with an emphasis on ultra-low power
consumption. Unlike Classic Bluetooth (BR/EDR), BLE was built from the ground up for
IoT devices, sensors, wearables, and any application where battery life is critical.

This module covers the BLE protocol stack in depth, from the physical radio layer to
the application-level profiles, using the ESP32 with NimBLE as the development platform.

---

## 1. BLE vs Classic Bluetooth

### Key Differences

```
+---------------------+-------------------+------------------------+
| Feature             | Classic Bluetooth | Bluetooth Low Energy   |
+---------------------+-------------------+------------------------+
| Optimized for       | Streaming data    | Short bursts of data   |
| Data rate           | 1-3 Mbps          | 1-2 Mbps (BLE 5.0)    |
| Range               | ~100 m            | ~100-400 m (BLE 5.0)   |
| Peak current        | ~30 mA            | ~15 mA                 |
| Avg current         | Varies            | Micro-amps (uA)        |
| Latency             | ~100 ms           | ~6 ms (min)            |
| Channels            | 79 (1 MHz)        | 40 (2 MHz)             |
| Voice capable       | Yes               | No (until LE Audio)    |
| Topology            | Point-to-point    | P2P, Broadcast, Mesh   |
| Pairing complexity  | High              | Simplified             |
| Power profile       | Continuous        | Sleep + short bursts   |
+---------------------+-------------------+------------------------+
```

### When to Use BLE

- **Sensors and beacons**: Temperature, humidity, proximity, asset tracking
- **Wearables**: Fitness bands, smartwatches, health monitors
- **Remote controls**: Simple HID devices, smart home remotes
- **Smart home**: Door locks, light switches, environmental sensors
- **Medical devices**: Heart rate monitors, glucose meters, pulse oximeters
- **Industrial IoT**: Equipment monitoring, predictive maintenance

### When Classic Bluetooth Is Better

- Audio streaming (A2DP) -- though LE Audio is changing this
- High-throughput file transfer
- Legacy device compatibility
- Continuous bidirectional data streams

---

## 2. BLE Protocol Stack

The BLE protocol stack is organized into three main groups: the Controller, the Host,
and the Application. Here is the full layered architecture:

```
+===================================================================+
|                       APPLICATION LAYER                           |
|                  (Profiles & User Application)                    |
+===================================================================+
|                                                                   |
|                          HOST                                     |
|                                                                   |
|  +-----------------------------+  +----------------------------+  |
|  |            GAP              |  |           GATT             |  |
|  |  (Generic Access Profile)   |  | (Generic Attribute Profile)|  |
|  |                             |  |                            |  |
|  |  - Device roles             |  |  - Service framework       |  |
|  |  - Discovery                |  |  - Characteristics         |  |
|  |  - Connection establishment |  |  - Client/Server model     |  |
|  |  - Security procedures      |  |  - Read/Write/Notify       |  |
|  +-----------------------------+  +----------------------------+  |
|                                                                   |
|  +-----------------------------+  +----------------------------+  |
|  |            SMP              |  |           ATT              |  |
|  | (Security Manager Protocol) |  | (Attribute Protocol)       |  |
|  |                             |  |                            |  |
|  |  - Pairing                  |  |  - Attribute database      |  |
|  |  - Key distribution         |  |  - PDU operations          |  |
|  |  - Encryption setup         |  |  - Handle-based access     |  |
|  +-----------------------------+  +----------------------------+  |
|                                                                   |
|  +-------------------------------------------------------------+ |
|  |                        L2CAP                                 | |
|  |          (Logical Link Control & Adaptation Protocol)        | |
|  |                                                              | |
|  |  - Protocol multiplexing (CID-based)                        | |
|  |  - Segmentation and reassembly                              | |
|  |  - Fixed channels for ATT (0x0004) and SMP (0x0006)         | |
|  +-------------------------------------------------------------+ |
|                                                                   |
+===================================================================+
|                                                                   |
|  +-------------------------------------------------------------+ |
|  |                    HCI (Host Controller Interface)           | |
|  |          (Standardized interface between Host & Controller)  | |
|  +-------------------------------------------------------------+ |
|                                                                   |
+===================================================================+
|                                                                   |
|                        CONTROLLER                                 |
|                                                                   |
|  +-------------------------------------------------------------+ |
|  |                     Link Layer (LL)                          | |
|  |                                                              | |
|  |  - State machine (Standby, Advertising, Scanning,           | |
|  |    Initiating, Connection, Synchronization)                  | |
|  |  - Packet format & CRC                                      | |
|  |  - Adaptive frequency hopping                               | |
|  |  - Connection management                                    | |
|  |  - Flow control                                             | |
|  +-------------------------------------------------------------+ |
|  |                                                              | |
|  |                   Physical Layer (PHY)                       | |
|  |                                                              | |
|  |  - 2.4 GHz ISM band (2400-2483.5 MHz)                      | |
|  |  - 40 RF channels (2 MHz spacing)                           | |
|  |  - GFSK modulation                                          | |
|  |  - 1 Mbps (LE 1M), 2 Mbps (LE 2M), 500/125 kbps (Coded)   | |
|  +-------------------------------------------------------------+ |
|                                                                   |
+===================================================================+
```

### 2.1 Physical Layer (PHY)

BLE operates in the 2.4 GHz ISM band, divided into **40 channels** of 2 MHz each:

```
Channel Allocation:
+------+------+------+-----+------+------+------+------+------+
|  37  | 0-10 |  38  |11-36|  39  |      |      |      |      |
| ADV  | DATA | ADV  |DATA | ADV  |      |      |      |      |
+------+------+------+-----+------+------+------+------+------+
2402   2404        2426      2428       2478  2480 MHz

  Advertising channels: 37 (2402 MHz), 38 (2426 MHz), 39 (2480 MHz)
  Data channels: 0-36 (remaining 37 channels)

  Note: Advertising channels are spread across the band to
        minimize interference from Wi-Fi channels 1, 6, 11.
```

**PHY Modes (Bluetooth 5.0+):**

| PHY Mode   | Symbol Rate | Data Rate | Range      | Use Case            |
|------------|-------------|-----------|------------|---------------------|
| LE 1M      | 1 Msym/s    | 1 Mbps    | Standard   | General purpose     |
| LE 2M      | 2 Msym/s    | 2 Mbps    | Shorter    | High throughput     |
| LE Coded S2| 1 Msym/s    | 500 kbps  | ~2x range  | Extended range      |
| LE Coded S8| 1 Msym/s    | 125 kbps  | ~4x range  | Maximum range       |

### 2.2 Link Layer (LL)

The Link Layer manages the radio and defines the state machine for BLE communication.

```
Link Layer State Machine:

                    +----------+
                    | Standby  |<---------+
                    +----------+          |
                   /     |     \          |
                  v      v      v         |
          +------+ +--------+ +------+   |
          | Adv  | |Scanning| | Init |   |
          +------+ +--------+ +------+   |
              |         |         |       |
              +----+----+---------+       |
                   |                      |
                   v                      |
              +-----------+               |
              |Connection |---------------+
              | (Central/ |  (disconnect)
              | Peripheral|
              +-----------+
```

**Link Layer Packet Format:**

```
+----------+----------+--------+---------+-----+
| Preamble | Access   | PDU    | Payload | CRC |
| (1-2 B)  | Address  | Header | (0-255B)| (3B)|
|          | (4 B)    | (2 B)  |         |     |
+----------+----------+--------+---------+-----+

Advertising PDU types:
  ADV_IND         - Connectable, scannable undirected
  ADV_DIRECT_IND  - Connectable directed (high/low duty)
  ADV_NONCONN_IND - Non-connectable, non-scannable undirected
  ADV_SCAN_IND    - Scannable undirected
  SCAN_REQ        - Scan request
  SCAN_RSP        - Scan response
  CONNECT_IND     - Connection request
```

### 2.3 L2CAP (Logical Link Control and Adaptation Protocol)

L2CAP provides protocol multiplexing and segmentation/reassembly. In BLE, it uses
**fixed channels** (unlike Classic BT, which uses dynamic channels):

| CID    | Channel Purpose              |
|--------|------------------------------|
| 0x0004 | ATT (Attribute Protocol)     |
| 0x0005 | LE Signaling                 |
| 0x0006 | SMP (Security Manager)       |

BLE 4.2+ adds LE Credit Based Flow Control for connection-oriented channels, enabling
higher throughput data transfer.

### 2.4 ATT (Attribute Protocol)

ATT defines the client-server protocol for accessing attributes on a device.

```
Attribute Structure:
+------------------+----------+-----------------+------------------+
| Attribute Handle | Type     | Value           | Permissions      |
| (16-bit)         | (UUID)   | (variable len)  | (R/W/Auth/Enc)   |
+------------------+----------+-----------------+------------------+

  Handle: Unique 16-bit identifier (0x0001 - 0xFFFF)
  Type:   UUID describing what the attribute represents
  Value:  The actual data (up to 512 bytes)
  Perms:  Access control (read, write, authentication, encryption)
```

**ATT Operations:**

```
Client-initiated:                Server-initiated:
+---------------------+         +---------------------+
| Find Information    |         | Notification        |
| Find By Type Value  |         | (no ACK required)   |
| Read (By Type/Handle|         +---------------------+
|   /Blob/Multiple)   |         | Indication          |
| Write (Request/Cmd/ |         | (ACK required)      |
|   Prepared/Execute) |         +---------------------+
+---------------------+
```

### 2.5 GATT (Generic Attribute Profile)

GATT builds on ATT and defines a hierarchical data structure for organizing attributes.
This is the layer most application developers interact with.

```
GATT Hierarchy:

+-- Profile (e.g., Heart Rate Profile)
|
+---- Service (e.g., Heart Rate Service, UUID 0x180D)
|     |
|     +---- Characteristic (e.g., Heart Rate Measurement, UUID 0x2A37)
|     |     |
|     |     +---- Value (the actual heart rate data)
|     |     |
|     |     +---- Descriptor: CCCD (Client Characteristic Config, 0x2902)
|     |     |     [Used to enable notifications/indications]
|     |     |
|     |     +---- Descriptor: User Description (0x2901)
|     |           [Human-readable name]
|     |
|     +---- Characteristic (e.g., Body Sensor Location, UUID 0x2A38)
|           |
|           +---- Value (chest, wrist, etc.)
|
+---- Service (e.g., Battery Service, UUID 0x180F)
      |
      +---- Characteristic (Battery Level, UUID 0x2A19)
            |
            +---- Value (0-100%)
            |
            +---- Descriptor: CCCD (0x2902)
```

**GATT Roles:**

```
+-------------------+                    +-------------------+
|    GATT Client    |   ATT Requests     |    GATT Server    |
|                   | -----------------> |                   |
| - Discovers svcs  |   ATT Responses    | - Hosts database  |
| - Reads/writes    | <----------------- | - Sends notif/ind |
| - Subscribes to   |   Notifications    | - Manages attrs   |
|   notifications   | <----------------- |                   |
+-------------------+                    +-------------------+

Typical mapping:
  Central (phone/PC) = GATT Client
  Peripheral (sensor) = GATT Server
  (But roles can be reversed or dual!)
```

### 2.6 GAP (Generic Access Profile)

GAP defines how BLE devices discover each other, establish connections, and manage
security. It defines four **roles**:

```
GAP Roles:

  Broadcaster -----> Observer
  (Advertises)       (Scans, no connect)

  Peripheral ------> Central
  (Advertises,       (Scans, initiates
   accepts conn)      connections)
```

**GAP Procedures:**

1. **Discovery**: Finding nearby devices via advertising/scanning
2. **Connection**: Establishing a link between two devices
3. **Bonding**: Storing security keys for future reconnection
4. **Name Discovery**: Reading the device name characteristic

---

## 3. Advertising and Scanning

### 3.1 Advertising

Advertising is how BLE devices make themselves known. A device transmits advertising
packets on channels 37, 38, and 39 in sequence.

```
Advertising Timeline:

Ch37  Ch38  Ch39       Ch37  Ch38  Ch39       Ch37  Ch38  Ch39
 |     |     |          |     |     |          |     |     |
 v     v     v          v     v     v          v     v     v
[ADV] [ADV] [ADV]      [ADV] [ADV] [ADV]      [ADV] [ADV] [ADV]
|<--- adv event --->|  |<--- adv event --->|
|<----------- adv interval ------------>|

  Advertising interval: 20 ms to 10.24 s
  Common values: 100 ms (fast), 1000 ms (slow/power-saving)
  Random delay: 0-10 ms added to prevent collisions
```

**Advertising Data Format (max 31 bytes):**

```
+--------+------+---------+--------+------+---------+
| Length1 | Type1| Data1   | Length2 | Type2| Data2   | ...
+--------+------+---------+--------+------+---------+

Common AD Types:
  0x01 - Flags (discoverability, BR/EDR support)
  0x02 - Incomplete 16-bit Service UUIDs
  0x03 - Complete 16-bit Service UUIDs
  0x06 - Incomplete 128-bit Service UUIDs
  0x07 - Complete 128-bit Service UUIDs
  0x08 - Shortened Local Name
  0x09 - Complete Local Name
  0x0A - TX Power Level
  0xFF - Manufacturer Specific Data
```

**Extended Advertising (BLE 5.0+):**

BLE 5.0 introduced extended advertising with up to 254 bytes in primary advertising
and up to 1650 bytes using auxiliary packets (secondary advertising channels).

### 3.2 Scanning

Scanning is how devices discover advertisers.

```
Passive Scanning:
  Scanner listens on adv channels, receives ADV packets.
  No interaction with advertiser.

Active Scanning:
  Scanner listens, then sends SCAN_REQ to get more data.
  Advertiser responds with SCAN_RSP (additional 31 bytes).

  Scanner                        Advertiser
    |                                |
    |        ADV_IND                 |
    | <----------------------------- |
    |                                |
    |        SCAN_REQ                |
    | -----------------------------> |
    |                                |
    |        SCAN_RSP                |
    | <----------------------------- |
    |                                |

Scan parameters:
  Scan window:   How long to listen per interval
  Scan interval: How often to start listening
  Duty cycle = scan_window / scan_interval
```

### 3.3 Connection Establishment

```
  Central (Initiator)              Peripheral (Advertiser)
       |                                |
       |    Scanning for ADV_IND        |
       |         ADV_IND                |
       | <----------------------------- |
       |                                |
       |       CONNECT_IND              |
       | -----------------------------> |
       |                                |
       |  (Both switch to data channels)|
       |                                |
       |  <== Connected: exchange ==>   |
       |  <== data on hop channels ==>  |
       |                                |
```

---

## 4. Connection Parameters

Once connected, communication is governed by connection parameters:

```
Connection Parameter Timeline:

|<-- conn interval -->|<-- conn interval -->|<-- conn interval -->|
|                     |                     |                     |
+--+              +--+--+              +--+--+              +--+
|TX|              |TX|RX|              |TX|RX|              |TX|
+--+              +--+--+              +--+--+              +--+
                  Connection           Connection           ...
                  Event                Event

  Connection Interval: 7.5 ms - 4000 ms (in 1.25 ms steps)
  Peripheral Latency:  0 - 499 (events peripheral can skip)
  Supervision Timeout: 100 ms - 32 s (disconnect if no response)

  Rule: Supervision Timeout > (1 + Latency) * Interval * 2
```

**Common Connection Parameter Presets:**

| Use Case            | Interval    | Latency | Timeout | Power     |
|---------------------|-------------|---------|---------|-----------|
| High throughput     | 7.5-15 ms   | 0       | 2 s     | High      |
| Balanced            | 30-50 ms    | 0       | 4 s     | Medium    |
| Low power sensor    | 100-500 ms  | 3-4     | 6 s     | Low       |
| Background monitor  | 1000-4000ms | 10+     | 20 s    | Very low  |

**Connection Parameter Update:**

Either side can request a parameter update, but the Central makes the final decision:

```
Peripheral                              Central
    |                                      |
    | L2CAP Conn Param Update Request      |
    | -----------------------------------> |
    |                                      |
    | L2CAP Conn Param Update Response     |
    | <----------------------------------- |
    |     (Accepted or Rejected)           |
    |                                      |
    | LL_CONNECTION_UPDATE_IND             |
    | <----------------------------------- |
    |  (New params take effect)            |
```

---

## 5. UUIDs (Universally Unique Identifiers)

BLE uses UUIDs to identify services, characteristics, and descriptors.

### 5.1 UUID Formats

```
16-bit UUID (SIG-defined):
  0x180D  (Heart Rate Service)
  Expands to: 0000180D-0000-1000-8000-00805F9B34FB
              ^^^^^^^^
              16-bit value inserted here

128-bit UUID (Custom/Vendor):
  12345678-1234-5678-1234-56789ABCDEF0
  (Generated for custom services/characteristics)

Base UUID (Bluetooth SIG):
  00000000-0000-1000-8000-00805F9B34FB
  All SIG-defined 16/32-bit UUIDs map into this base.
```

### 5.2 Common SIG-Defined UUIDs

| UUID   | Name                          |
|--------|-------------------------------|
| 0x1800 | Generic Access                |
| 0x1801 | Generic Attribute             |
| 0x180A | Device Information            |
| 0x180D | Heart Rate                    |
| 0x180F | Battery Service               |
| 0x1809 | Health Thermometer            |
| 0x2A00 | Device Name (Characteristic)  |
| 0x2A19 | Battery Level                 |
| 0x2A29 | Manufacturer Name             |
| 0x2A37 | Heart Rate Measurement        |
| 0x2A38 | Body Sensor Location          |
| 0x2902 | CCCD (Descriptor)             |

---

## 6. MTU Negotiation

**MTU (Maximum Transmission Unit)** defines the maximum size of an ATT packet.

```
Default MTU: 23 bytes
  ATT Header: 3 bytes
  Usable payload: 20 bytes

Negotiated MTU: up to 517 bytes (BLE 4.2+)
  ATT Header: 3 bytes
  Usable payload: up to 514 bytes

MTU Exchange:

  Client                           Server
    |                                 |
    | ATT Exchange MTU Request (251)  |
    | ------------------------------> |
    |                                 |
    | ATT Exchange MTU Response (512) |
    | <------------------------------ |
    |                                 |
    | Effective MTU = min(251, 512)   |
    |              = 251 bytes        |

  The effective MTU is the minimum of both sides.
```

**Data Length Extension (DLE) - BLE 4.2+:**

```
Without DLE:                    With DLE:
+------+----------+-----+      +------+--------------+-----+
|Header| Payload  | CRC |      |Header| Payload      | CRC |
| 2B   | 27B max  | 3B  |      | 2B   | 251B max     | 3B  |
+------+----------+-----+      +------+--------------+-----+

  Before DLE: Multiple LL packets needed for large ATT PDU
  After DLE:  Fewer LL packets, less overhead, higher throughput
```

**Throughput Calculation Example:**

```
With MTU=23, no DLE (BLE 4.0):
  20 bytes payload / connection event
  At 7.5ms interval: ~2.67 KB/s theoretical

With MTU=251, DLE enabled, LE 2M PHY:
  248 bytes payload / packet
  Multiple packets per event
  Theoretical: ~1400 kbps (practical: ~800 kbps)
```

---

## 7. BLE Security

### 7.1 Security Modes

```
Security Mode 1 (Encryption):
  Level 1: No security (no authentication, no encryption)
  Level 2: Unauthenticated pairing with encryption (Just Works)
  Level 3: Authenticated pairing with encryption (Passkey/OOB)
  Level 4: Authenticated LE Secure Connections (LESC + ECDH)

Security Mode 2 (Data Signing):
  Level 1: Unauthenticated pairing with data signing
  Level 2: Authenticated pairing with data signing
```

### 7.2 Pairing Methods

```
+------------------+-------------------+-----------------------------+
| Method           | Authentication    | Protection                  |
+------------------+-------------------+-----------------------------+
| Just Works       | None              | Encryption only (no MITM)   |
| Passkey Entry    | 6-digit PIN       | MITM protection             |
| Numeric Compare  | User confirms     | MITM protection (BLE 4.2+)  |
| Out-of-Band      | External channel  | MITM protection (NFC, QR)   |
+------------------+-------------------+-----------------------------+

Pairing Process (LE Secure Connections):

  Initiator                         Responder
      |                                 |
      | Pairing Request                 |
      | ------------------------------> |
      |                                 |
      | Pairing Response                |
      | <------------------------------ |
      |                                 |
      | ECDH Public Key Exchange        |
      | <=============================> |
      |                                 |
      | Authentication (method-specific)|
      | <=============================> |
      |                                 |
      | DHKey Check                     |
      | <=============================> |
      |                                 |
      | Encryption with LTK            |
      | <=============================> |
      |                                 |
      | Key Distribution               |
      | <=============================> |
```

### 7.3 Bonding

Bonding stores pairing information (keys) so devices can reconnect securely without
re-pairing:

- **LTK**: Long Term Key (used for encryption)
- **IRK**: Identity Resolving Key (resolves random addresses)
- **CSRK**: Connection Signature Resolving Key (data signing)

```
Bonded Reconnection:

  Central                            Peripheral
      |                                 |
      | (Recognizes bonded device)      |
      |                                 |
      | LL_ENC_REQ (using stored LTK)   |
      | ------------------------------> |
      |                                 |
      | LL_ENC_RSP                      |
      | <------------------------------ |
      |                                 |
      | (Encrypted link restored)       |
      | <=============================> |

  No re-pairing needed! Much faster reconnection.
```

### 7.4 Privacy (Random Addresses)

BLE supports privacy through random addresses to prevent tracking:

```
Address Types:
  Public Address:          Fixed, IEEE-assigned (like MAC)
  Random Static Address:   Fixed per boot, randomly generated
  Resolvable Private Addr: Changes periodically, resolvable with IRK
  Non-resolvable Private:  Changes periodically, not resolvable

  Resolvable Private Address Format:
  +----------+-------------------+
  | hash(3B) | prand(3B)         |
  +----------+-------------------+
  Bonded devices can resolve using shared IRK.
```

---

## 8. ESP32 NimBLE Stack Architecture

The ESP32 uses the Apache NimBLE stack, a compact and efficient BLE implementation:

```
+----------------------------------------------------------+
|                  User Application                        |
+----------------------------------------------------------+
|  NimBLE Host                                             |
|  +----------+ +------+ +------+ +-----+ +-------------+ |
|  |   GAP    | | GATT | | ATT  | | SMP | | L2CAP       | |
|  +----------+ +------+ +------+ +-----+ +-------------+ |
+----------------------------------------------------------+
|  NimBLE Controller  (or ESP32 HW controller)             |
|  +-----------+ +-------------------+                     |
|  | Link Layer| | PHY               |                     |
|  +-----------+ +-------------------+                     |
+----------------------------------------------------------+

Key NimBLE APIs:
  ble_gap_*        - GAP operations (adv, scan, connect)
  ble_gattc_*      - GATT Client operations
  ble_gatts_*      - GATT Server operations
  ble_svc_*        - Standard services (GAP, GATT, etc.)
  ble_store_*      - Persistent storage for bonding data
```

---

## 9. Development with ESP-IDF and NimBLE

### 9.1 Project Configuration

In `sdkconfig` or via `idf.py menuconfig`:

```
Component config -> Bluetooth ->
  [*] Bluetooth
  Bluetooth Host -> NimBLE
  NimBLE Options ->
    BLE Role: Peripheral + Central
    Max Connections: 3
    Max Bonds: 3
    Security Manager: Enabled
```

### 9.2 Typical Application Flow

**Peripheral (GATT Server):**

```
1. Initialize NVS flash
2. Initialize NimBLE host
3. Configure GAP parameters (device name, appearance)
4. Define GATT services and characteristics
5. Register GATT services
6. Configure and start advertising
7. Handle events (connect, disconnect, subscribe, write)
8. Send notifications/indications when data changes
```

**Central (GATT Client):**

```
1. Initialize NVS flash
2. Initialize NimBLE host
3. Start scanning
4. Filter discovered devices
5. Connect to target device
6. Discover services and characteristics
7. Read/write characteristics
8. Subscribe to notifications
9. Handle events (data received, disconnect)
```

---

## 10. Key Concepts Summary

```
+---------------------------------------------------------------+
|  Concept        |  Key Points                                 |
+-----------------+---------------------------------------------+
|  Advertising    |  Broadcast on ch 37,38,39; max 31B data     |
|  Scanning       |  Passive (listen) or Active (request more)  |
|  GATT Server    |  Hosts the attribute database (peripheral)  |
|  GATT Client    |  Reads/writes/subscribes (central)          |
|  Service        |  Collection of characteristics (UUID-based) |
|  Characteristic |  Data point with value + properties          |
|  Descriptor     |  Metadata about a characteristic             |
|  CCCD           |  Enables/disables notifications/indications  |
|  Notification   |  Server pushes data, no ACK (fast)           |
|  Indication     |  Server pushes data, requires ACK (reliable) |
|  MTU            |  Max ATT packet size (default 23, up to 517) |
|  Bonding        |  Storing keys for secure reconnection        |
|  MITM           |  Man-in-the-middle attack protection         |
+-----------------+---------------------------------------------+
```

---

## References

- Bluetooth Core Specification v5.3 (bluetooth.com)
- ESP-IDF NimBLE documentation (docs.espressif.com)
- Apache NimBLE documentation (mynewt.apache.org)
- Bluetooth SIG Assigned Numbers (bluetooth.com/specifications/assigned-numbers)
- "Getting Started with Bluetooth Low Energy" by Kevin Townsend et al.

---

## File Index

| File | Description |
|------|-------------|
| `examples/01_ble_beacon.c` | BLE beacon (advertising only, non-connectable) |
| `examples/02_ble_peripheral.c` | Peripheral with custom GATT service |
| `examples/03_ble_central.c` | Central scanner and GATT client |
| `examples/04_heart_rate.c` | Heart Rate Profile implementation |
| `examples/05_battery_service.c` | Battery Service (SIG-defined) |
| `examples/06_device_info.c` | Device Information Service |
| `examples/07_notifications.c` | Notifications and indications demo |
| `exercises/exercise_01.md` | Custom BLE beacon with manufacturer data |
| `exercises/exercise_02.md` | BLE thermometer peripheral |
| `exercises/exercise_03.md` | BLE-controlled LED (write characteristic) |
| `exercises/exercise_04.md` | BLE button state (notify characteristic) |
| `exercises/exercise_05.md` | BLE RSSI-based proximity detector |
| `exercises/exercise_06.md` | Multi-service peripheral |
| `exercises/exercise_07.md` | Connection parameter optimization |
| `quiz.md` | 15-question BLE knowledge quiz |
| `quiz_answers.md` | Quiz answer key with explanations |
| `project/` | BLE Remote Control capstone project |
