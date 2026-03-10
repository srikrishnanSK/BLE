# BLE Asset Tracker - Indoor Positioning System

## Business Case

### Problem Statement

Organizations managing large physical spaces — warehouses, hospitals, manufacturing
floors, and corporate campuses — lose significant revenue to misplaced assets, inefficient
workflows, and poor space utilization. Studies show:

- Hospitals spend **$4,000–$8,000 per nurse annually** in time lost searching for equipment.
- Warehouses lose **up to 30% productivity** from mislocated inventory and tools.
- Manufacturing downtime caused by missing assets costs an average of **$260,000/hour**.

### Proposed Solution

A BLE-based Real-Time Location System (RTLS) using low-cost beacon tags attached to
assets and fixed gateway scanners that triangulate positions via RSSI (Received Signal
Strength Indicator). The system provides:

- **Room-level accuracy** (3–5 m) using trilateration with Kalman filtering
- **Zone-based presence detection** for geofencing and workflow automation
- **Battery life of 12–18 months** on coin-cell tags
- **Cloud dashboard** with real-time asset maps and historical tracking

### ROI Analysis

| Metric                      | Value           |
|-----------------------------|-----------------|
| Hardware cost per zone      | ~$150           |
| Tag cost per asset          | ~$8             |
| Deployment (100 assets)     | ~$2,500         |
| Annual maintenance          | ~$500           |
| Estimated annual savings    | $15,000–$50,000 |
| Payback period              | 1–3 months      |

---

## System Architecture

```
┌─────────────┐     BLE Adv     ┌──────────────┐     MQTT/WiFi     ┌─────────────┐
│  Asset Tag   │ ─────────────► │   Gateway     │ ────────────────► │  Cloud/MQTT  │
│  (Beacon)    │                │   (Scanner)   │                   │   Broker     │
│  CR2032      │                │   ESP32 +     │                   │              │
│  nRF52832    │                │   WiFi        │                   └──────┬───────┘
└─────────────┘                └──────────────┘                          │
                                                                          ▼
┌─────────────┐     BLE Adv     ┌──────────────┐              ┌─────────────────┐
│  Asset Tag   │ ─────────────► │   Gateway     │              │  Position Engine │
│  (Beacon)    │                │   (Scanner)   │              │  (Trilateration  │
└─────────────┘                └──────────────┘              │   + Kalman)      │
                                                              └────────┬────────┘
┌─────────────┐     BLE Adv     ┌──────────────┐                      │
│  Asset Tag   │ ─────────────► │   Gateway     │              ┌───────▼────────┐
│  (Beacon)    │                │   (Scanner)   │              │   Dashboard    │
└─────────────┘                └──────────────┘              │   (Web UI)     │
                                                              └────────────────┘
```

### Data Flow

1. **Tags** broadcast BLE advertisement packets every 1–10 seconds (configurable).
2. **Gateways** (minimum 3 per zone) scan for advertisements, record RSSI and timestamp.
3. Gateways publish `{tag_id, rssi, gateway_id, timestamp}` to MQTT broker over WiFi.
4. **Position Engine** subscribes to MQTT, runs trilateration on RSSI from 3+ gateways,
   applies Kalman filter for smoothing, and determines zone presence.
5. **Dashboard** displays real-time positions, zone occupancy, and historical paths.

### Communication Protocols

| Layer         | Protocol       | Details                                      |
|---------------|----------------|----------------------------------------------|
| Tag → Gateway | BLE 5.0 Adv    | Non-connectable, undirected advertisements   |
| Gateway → Cloud | MQTT over TLS | QoS 1, JSON payloads                        |
| Cloud → Dashboard | WebSocket  | Real-time position updates                   |

---

## Bill of Materials Summary

| Component        | Part                | Unit Cost | Qty (pilot) | Total  |
|------------------|---------------------|-----------|-------------|--------|
| Asset Tag MCU    | nRF52832 Module     | $3.50     | 50          | $175   |
| Tag Battery      | CR2032              | $0.40     | 50          | $20    |
| Tag Enclosure    | 3D-printed ABS      | $1.00     | 50          | $50    |
| Gateway MCU      | ESP32-WROOM-32E     | $4.00     | 6           | $24    |
| Gateway PSU      | 5V USB adapter      | $3.00     | 6           | $18    |
| Gateway Enclosure| ABS project box     | $5.00     | 6           | $30    |
| MQTT Broker      | Mosquitto (RPi 4)   | $55.00    | 1           | $55    |
| **Total**        |                     |           |             | **$372** |

See `hardware/bom.md` for detailed BOM with supplier links.

---

## Building and Flashing

### Prerequisites

- nRF Connect SDK v2.5+ (for tags)
- ESP-IDF v5.1+ (for gateways)
- Mosquitto MQTT broker
- Python 3.10+ (for position engine scripts)

### Tag Firmware

```bash
cd firmware/beacon
west build -b nrf52dk_nrf52832
west flash
```

### Gateway Firmware

```bash
cd firmware/scanner
idf.py set-target esp32
idf.py build
idf.py flash
```

---

## Project Structure

```
ble-asset-tracker/
├── README.md                       # This file
├── firmware/
│   ├── beacon/main.c               # Tag firmware
│   ├── scanner/main.c              # Gateway firmware
│   └── common/ble_config.h         # Shared BLE definitions
├── algorithm/
│   ├── trilateration.c             # RSSI-to-position calculation
│   ├── kalman_filter.c             # Position smoothing filter
│   └── zone_detection.c            # Room/zone presence logic
├── hardware/
│   ├── bom.md                      # Detailed bill of materials
│   └── schematic_notes.md          # Circuit design decisions
└── tests/
    ├── test_trilateration.c        # Trilateration unit tests
    └── test_filter.c               # Kalman filter unit tests
```

---

## Learning Objectives

After completing this capstone, you will be able to:

1. Configure BLE advertisement parameters for optimal power/range tradeoff.
2. Implement RSSI-based distance estimation with path-loss modeling.
3. Apply trilateration algorithms to compute 2D positions.
4. Design and tune a Kalman filter for noisy sensor fusion.
5. Architect a multi-node IoT system with MQTT messaging.
6. Optimize embedded firmware for multi-year battery life.
7. Write unit tests for numerical algorithms on embedded targets.

---

## License

This project is provided as educational material for the Embedded Systems Learning
Platform. See the repository root LICENSE for terms.
