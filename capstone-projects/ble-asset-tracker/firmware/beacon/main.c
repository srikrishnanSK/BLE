/**
 * @file main.c
 * @brief BLE Asset Tracker - Beacon Tag Firmware
 *
 * This firmware runs on nRF52832-based asset tags. It broadcasts BLE
 * non-connectable advertisements containing a unique tag ID and battery
 * level, then enters deep sleep to conserve power. Targets 12–18 months
 * battery life on a CR2032 coin cell.
 *
 * Key features:
 *   - Configurable advertising interval (1s–10s)
 *   - TX power adjustment for range vs. battery tradeoff
 *   - Battery voltage monitoring via internal ADC
 *   - Low-battery alert flag in advertisement payload
 *   - Watchdog timer for reliability
 *
 * Target: nRF52832 (nRF Connect SDK / Zephyr RTOS)
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>

#include "../common/ble_config.h"

LOG_MODULE_REGISTER(beacon, LOG_LEVEL_INF);

/* ---------------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------------- */

/** Advertising interval in milliseconds. Higher = longer battery life. */
#define ADV_INTERVAL_MS         CONFIG_BEACON_ADV_INTERVAL_MS
#ifndef CONFIG_BEACON_ADV_INTERVAL_MS
#undef ADV_INTERVAL_MS
#define ADV_INTERVAL_MS         3000
#endif

/** Number of advertisement bursts before re-reading battery voltage. */
#define BATTERY_CHECK_INTERVAL  100

/** Battery voltage threshold (mV) below which the low-battery flag is set. */
#define BATTERY_LOW_THRESHOLD_MV 2200

/** TX power level in dBm. Range: -40 to +4 for nRF52832. */
#define TX_POWER_DBM            0

/** Watchdog timeout in milliseconds. */
#define WDT_TIMEOUT_MS          30000

/* ---------------------------------------------------------------------------
 * Unique Tag Identifier
 * --------------------------------------------------------------------------- */

/**
 * 4-byte tag ID. In production this would be read from OTP/UICR or
 * provisioned during manufacturing. For development, we derive it from
 * the device address.
 */
static uint8_t tag_id[TAG_ID_LENGTH] = {0};

/* ---------------------------------------------------------------------------
 * Battery Monitoring
 * --------------------------------------------------------------------------- */

/** ADC channel configuration for internal VDDH/VDD measurement. */
static const struct adc_dt_spec adc_channel =
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static uint16_t battery_mv = 3000;  /* Default to nominal until first read */
static bool     battery_low = false;
static uint32_t adv_count   = 0;

/**
 * @brief Read battery voltage using the internal ADC.
 *
 * Configures the ADC for a single-shot read of the supply voltage through
 * the internal 1/6 divider, converts the raw value to millivolts, and
 * updates the battery_low flag.
 *
 * @return Battery voltage in millivolts, or 0 on failure.
 */
static uint16_t battery_read_mv(void)
{
    int16_t raw = 0;
    int ret;

    struct adc_sequence sequence = {
        .buffer      = &raw,
        .buffer_size = sizeof(raw),
    };

    if (!adc_is_ready_dt(&adc_channel)) {
        LOG_WRN("ADC device not ready");
        return 0;
    }

    ret = adc_sequence_init_dt(&adc_channel, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC sequence init failed: %d", ret);
        return 0;
    }

    ret = adc_read_dt(&adc_channel, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC read failed: %d", ret);
        return 0;
    }

    int32_t mv = raw;
    ret = adc_raw_to_millivolts_dt(&adc_channel, &mv);
    if (ret < 0) {
        LOG_WRN("Raw-to-mV conversion failed: %d", ret);
        return 0;
    }

    battery_mv  = (uint16_t)mv;
    battery_low = (battery_mv < BATTERY_LOW_THRESHOLD_MV);

    LOG_INF("Battery: %u mV %s", battery_mv, battery_low ? "(LOW)" : "");
    return battery_mv;
}

/**
 * @brief Convert millivolts to a 0–100% battery percentage.
 *
 * Uses a simple linear mapping for CR2032:
 *   3000 mV = 100%, 2000 mV = 0%.
 */
static uint8_t battery_mv_to_percent(uint16_t mv)
{
    if (mv >= 3000) return 100;
    if (mv <= 2000) return 0;
    return (uint8_t)(((uint32_t)(mv - 2000) * 100) / 1000);
}

/* ---------------------------------------------------------------------------
 * BLE Advertisement
 * --------------------------------------------------------------------------- */

/**
 * Custom manufacturer-specific advertisement payload.
 *
 * Layout (inside Manufacturer Specific Data AD type):
 *   [0..1]  Company ID (ASSET_TRACKER_COMPANY_ID, little-endian)
 *   [2]     Protocol version (PROTOCOL_VERSION)
 *   [3]     Packet type (PKT_TYPE_BEACON)
 *   [4..7]  Tag ID (4 bytes)
 *   [8]     Battery percentage (0–100)
 *   [9]     Flags (bit 0 = low battery)
 *   [10]    TX power (signed dBm, for RSSI-to-distance calibration)
 */
static uint8_t mfg_data[MFG_DATA_BEACON_LENGTH];

static const struct bt_data adv_data[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
};

/**
 * @brief Build the manufacturer-specific data payload.
 *
 * Encodes tag ID, battery status, and TX power into the advertisement
 * packet so gateways can identify this tag and assess its health.
 */
static void build_adv_payload(void)
{
    /* Company ID (little-endian) */
    mfg_data[0] = (uint8_t)(ASSET_TRACKER_COMPANY_ID & 0xFF);
    mfg_data[1] = (uint8_t)(ASSET_TRACKER_COMPANY_ID >> 8);

    /* Protocol header */
    mfg_data[2] = PROTOCOL_VERSION;
    mfg_data[3] = PKT_TYPE_BEACON;

    /* Tag ID */
    memcpy(&mfg_data[4], tag_id, TAG_ID_LENGTH);

    /* Battery */
    mfg_data[8] = battery_mv_to_percent(battery_mv);

    /* Flags */
    uint8_t flags = 0;
    if (battery_low) {
        flags |= FLAG_BATTERY_LOW;
    }
    mfg_data[9] = flags;

    /* TX power for RSSI calibration at receivers */
    mfg_data[10] = (uint8_t)((int8_t)TX_POWER_DBM);
}

/**
 * @brief Initialize the tag ID from the BLE device address.
 *
 * In production, this would be replaced with a provisioning scheme
 * (OTP fuses, NFC tap, or cloud enrollment).
 */
static void init_tag_id(void)
{
    bt_addr_le_t addr;
    size_t count = 1;

    bt_id_get(&addr, &count);
    if (count > 0) {
        /* Use last 4 bytes of the 6-byte BLE address as tag ID */
        memcpy(tag_id, &addr.a.val[0], TAG_ID_LENGTH);
    } else {
        /* Fallback: use a compile-time default */
        tag_id[0] = 0xDE;
        tag_id[1] = 0xAD;
        tag_id[2] = 0xBE;
        tag_id[3] = 0xEF;
    }

    LOG_INF("Tag ID: %02X:%02X:%02X:%02X",
            tag_id[0], tag_id[1], tag_id[2], tag_id[3]);
}

/* ---------------------------------------------------------------------------
 * Watchdog
 * --------------------------------------------------------------------------- */

static const struct device *wdt_dev;
static int wdt_channel_id = -1;

/**
 * @brief Initialize the hardware watchdog timer.
 *
 * If the main loop stalls (e.g., BLE stack hang), the watchdog will reset
 * the MCU after WDT_TIMEOUT_MS milliseconds.
 */
static int watchdog_init(void)
{
    wdt_dev = DEVICE_DT_GET(DT_ALIAS(watchdog0));
    if (!device_is_ready(wdt_dev)) {
        LOG_WRN("Watchdog device not ready — continuing without WDT");
        return -ENODEV;
    }

    struct wdt_timeout_cfg cfg = {
        .window = {
            .min = 0,
            .max = WDT_TIMEOUT_MS,
        },
        .callback = NULL,  /* System reset on timeout */
        .flags    = WDT_FLAG_RESET_SOC,
    };

    wdt_channel_id = wdt_install_timeout(wdt_dev, &cfg);
    if (wdt_channel_id < 0) {
        LOG_ERR("WDT install failed: %d", wdt_channel_id);
        return wdt_channel_id;
    }

    int ret = wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (ret < 0) {
        LOG_ERR("WDT setup failed: %d", ret);
        return ret;
    }

    LOG_INF("Watchdog initialized (%u ms timeout)", WDT_TIMEOUT_MS);
    return 0;
}

static inline void watchdog_feed(void)
{
    if (wdt_dev && wdt_channel_id >= 0) {
        wdt_feed(wdt_dev, wdt_channel_id);
    }
}

/* ---------------------------------------------------------------------------
 * Main
 * --------------------------------------------------------------------------- */

/**
 * @brief Application entry point.
 *
 * Lifecycle:
 *   1. Initialize BLE subsystem.
 *   2. Read battery voltage.
 *   3. Build advertisement payload.
 *   4. Start advertising.
 *   5. Sleep for the configured interval.
 *   6. Repeat from step 3 (re-read battery every BATTERY_CHECK_INTERVAL cycles).
 */
int main(void)
{
    int ret;

    LOG_INF("=== BLE Asset Tracker - Beacon Tag ===");
    LOG_INF("Adv interval: %u ms, TX power: %d dBm",
            ADV_INTERVAL_MS, TX_POWER_DBM);

    /* --- BLE Initialization --- */
    ret = bt_enable(NULL);
    if (ret) {
        LOG_ERR("BLE init failed: %d", ret);
        return ret;
    }
    LOG_INF("BLE initialized");

    /* --- Tag ID --- */
    init_tag_id();

    /* --- Initial battery reading --- */
    battery_read_mv();

    /* --- Watchdog --- */
    watchdog_init();

    /* --- Set TX power --- */
    /* Note: exact API varies by SDK version; this is the Zephyr HCI approach */
    struct bt_hci_cp_vs_write_tx_power_level *cp;
    struct net_buf *buf;

    buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL, sizeof(*cp));
    if (buf) {
        cp = net_buf_add(buf, sizeof(*cp));
        cp->handle_type = BT_HCI_VS_LL_HANDLE_TYPE_ADV;
        cp->handle      = 0;
        cp->tx_power_level = TX_POWER_DBM;
        bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL, buf, NULL);
    }

    /* --- Advertising parameters --- */
    struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
        BT_LE_ADV_OPT_USE_IDENTITY,    /* Non-connectable, use public addr */
        BT_GAP_ADV_FAST_INT_MIN_2,      /* Min interval */
        BT_GAP_ADV_FAST_INT_MAX_2,      /* Max interval */
        NULL                             /* No peer (undirected) */
    );

    /* --- Main loop --- */
    while (1) {
        /* Periodically refresh battery reading */
        if ((adv_count % BATTERY_CHECK_INTERVAL) == 0) {
            battery_read_mv();
        }

        /* Build and update advertisement payload */
        build_adv_payload();

        /* Start advertising (burst) */
        ret = bt_le_adv_start(&adv_param, adv_data, ARRAY_SIZE(adv_data),
                              NULL, 0);
        if (ret && ret != -EALREADY) {
            LOG_ERR("Advertising start failed: %d", ret);
        }

        /* Let the radio send a few advertisement events */
        k_sleep(K_MSEC(100));

        /* Stop advertising to save power during sleep */
        bt_le_adv_stop();

        adv_count++;
        watchdog_feed();

        /* Deep sleep until next advertisement cycle */
        k_sleep(K_MSEC(ADV_INTERVAL_MS - 100));

        watchdog_feed();
    }

    return 0; /* Never reached */
}
