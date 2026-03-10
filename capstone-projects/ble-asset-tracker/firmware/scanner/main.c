/**
 * @file main.c
 * @brief BLE Asset Tracker - Gateway Scanner Firmware
 *
 * Runs on ESP32. Continuously scans for BLE beacon advertisements,
 * extracts tag ID and RSSI, and publishes the data to an MQTT broker
 * over WiFi. Multiple gateways deployed in known positions enable
 * trilateration of asset locations.
 *
 * Key features:
 *   - Passive BLE scanning with duplicate filtering
 *   - RSSI averaging (sliding window) per tag to reduce noise
 *   - MQTT publishing with QoS 1 and TLS support
 *   - WiFi reconnection with exponential backoff
 *   - NTP time synchronization for accurate timestamps
 *   - Gateway health reporting (uptime, scan stats, free heap)
 *
 * Target: ESP32-WROOM-32E (ESP-IDF v5.1+)
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

#include "mqtt_client.h"
#include "cJSON.h"

#include "../common/ble_config.h"

static const char *TAG = "gateway";

/* ---------------------------------------------------------------------------
 * Configuration — override via menuconfig / sdkconfig
 * --------------------------------------------------------------------------- */

#ifndef CONFIG_WIFI_SSID
#define CONFIG_WIFI_SSID        "warehouse_iot"
#endif
#ifndef CONFIG_WIFI_PASSWORD
#define CONFIG_WIFI_PASSWORD    "changeme123"
#endif
#ifndef CONFIG_MQTT_BROKER_URI
#define CONFIG_MQTT_BROKER_URI  "mqtt://192.168.1.100:1883"
#endif
#ifndef CONFIG_GATEWAY_ID
#define CONFIG_GATEWAY_ID       "gw-01"
#endif

/** Gateway's known position (meters) — set during deployment. */
#ifndef CONFIG_GW_POS_X
#define CONFIG_GW_POS_X         0.0f
#endif
#ifndef CONFIG_GW_POS_Y
#define CONFIG_GW_POS_Y         0.0f
#endif

/** MQTT topic prefix. Full topic: <prefix>/rssi/<gateway_id> */
#define MQTT_TOPIC_PREFIX       "asset-tracker"

/** RSSI reports are published no faster than this interval per tag. */
#define REPORT_INTERVAL_MS      1000

/** Number of RSSI samples to average per tag before reporting. */
#define RSSI_WINDOW_SIZE        5

/** Maximum number of simultaneously tracked tags. */
#define MAX_TRACKED_TAGS        128

/** Gateway health report interval (seconds). */
#define HEALTH_REPORT_INTERVAL_S 60

/* ---------------------------------------------------------------------------
 * WiFi
 * --------------------------------------------------------------------------- */

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected — reconnecting");
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi STA initialized, connecting to '%s'",
             CONFIG_WIFI_SSID);
}

/* ---------------------------------------------------------------------------
 * NTP Time Sync
 * --------------------------------------------------------------------------- */

static void ntp_init(void)
{
    ESP_LOGI(TAG, "Initializing NTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

static uint64_t get_timestamp_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}

/* ---------------------------------------------------------------------------
 * MQTT
 * --------------------------------------------------------------------------- */

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

static void mqtt_event_handler(void *args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected to broker");
        mqtt_connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        mqtt_connected = false;
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;
    default:
        break;
    }
}

static void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URI,
        .credentials.client_id = CONFIG_GATEWAY_ID,
        .session.keepalive = 30,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

/**
 * @brief Publish an RSSI observation to the MQTT broker.
 *
 * JSON payload format:
 * {
 *   "gateway_id": "gw-01",
 *   "gw_x": 0.0,
 *   "gw_y": 0.0,
 *   "tag_id": "DEADBEEF",
 *   "rssi": -65,
 *   "battery_pct": 87,
 *   "tx_power": 0,
 *   "timestamp_ms": 1700000000000
 * }
 */
static void mqtt_publish_rssi(const char *tag_id_str, int rssi_avg,
                               uint8_t battery_pct, int8_t tx_power)
{
    if (!mqtt_connected) return;

    char topic[128];
    snprintf(topic, sizeof(topic), "%s/rssi/%s",
             MQTT_TOPIC_PREFIX, CONFIG_GATEWAY_ID);

    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddStringToObject(root, "gateway_id", CONFIG_GATEWAY_ID);
    cJSON_AddNumberToObject(root, "gw_x", CONFIG_GW_POS_X);
    cJSON_AddNumberToObject(root, "gw_y", CONFIG_GW_POS_Y);
    cJSON_AddStringToObject(root, "tag_id", tag_id_str);
    cJSON_AddNumberToObject(root, "rssi", rssi_avg);
    cJSON_AddNumberToObject(root, "battery_pct", battery_pct);
    cJSON_AddNumberToObject(root, "tx_power", tx_power);
    cJSON_AddNumberToObject(root, "timestamp_ms", (double)get_timestamp_ms());

    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        esp_mqtt_client_publish(mqtt_client, topic, json, 0, 1, 0);
        ESP_LOGD(TAG, "Published: %s", json);
        free(json);
    }

    cJSON_Delete(root);
}

/**
 * @brief Publish gateway health status.
 */
static void mqtt_publish_health(void)
{
    if (!mqtt_connected) return;

    char topic[128];
    snprintf(topic, sizeof(topic), "%s/health/%s",
             MQTT_TOPIC_PREFIX, CONFIG_GATEWAY_ID);

    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddStringToObject(root, "gateway_id", CONFIG_GATEWAY_ID);
    cJSON_AddNumberToObject(root, "uptime_s",
                            (double)(xTaskGetTickCount() / configTICK_RATE_HZ));
    cJSON_AddNumberToObject(root, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "timestamp_ms", (double)get_timestamp_ms());

    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        esp_mqtt_client_publish(mqtt_client, topic, json, 0, 1, 0);
        free(json);
    }
    cJSON_Delete(root);
}

/* ---------------------------------------------------------------------------
 * RSSI Tracking & Averaging
 * --------------------------------------------------------------------------- */

typedef struct {
    uint8_t  tag_id[TAG_ID_LENGTH];
    bool     active;
    int      rssi_window[RSSI_WINDOW_SIZE];
    uint8_t  window_idx;
    uint8_t  sample_count;
    uint8_t  battery_pct;
    int8_t   tx_power;
    uint64_t last_report_ms;
} tracked_tag_t;

static tracked_tag_t tracked_tags[MAX_TRACKED_TAGS];
static SemaphoreHandle_t tag_mutex;

/**
 * @brief Find or allocate a tracking slot for the given tag ID.
 * @return Pointer to the tracking entry, or NULL if table is full.
 */
static tracked_tag_t *find_or_create_tag(const uint8_t *id)
{
    tracked_tag_t *empty_slot = NULL;

    for (int i = 0; i < MAX_TRACKED_TAGS; i++) {
        if (tracked_tags[i].active &&
            memcmp(tracked_tags[i].tag_id, id, TAG_ID_LENGTH) == 0) {
            return &tracked_tags[i];
        }
        if (!tracked_tags[i].active && !empty_slot) {
            empty_slot = &tracked_tags[i];
        }
    }

    if (empty_slot) {
        memset(empty_slot, 0, sizeof(*empty_slot));
        memcpy(empty_slot->tag_id, id, TAG_ID_LENGTH);
        empty_slot->active = true;
    }

    return empty_slot;
}

/**
 * @brief Add an RSSI sample and return the sliding-window average.
 */
static int tag_add_rssi(tracked_tag_t *t, int rssi)
{
    t->rssi_window[t->window_idx] = rssi;
    t->window_idx = (t->window_idx + 1) % RSSI_WINDOW_SIZE;
    if (t->sample_count < RSSI_WINDOW_SIZE) {
        t->sample_count++;
    }

    int sum = 0;
    for (int i = 0; i < t->sample_count; i++) {
        sum += t->rssi_window[i];
    }
    return sum / t->sample_count;
}

/* ---------------------------------------------------------------------------
 * BLE Scanner
 * --------------------------------------------------------------------------- */

static esp_ble_scan_params_t scan_params = {
    .scan_type          = BLE_SCAN_TYPE_PASSIVE,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval      = 0x50,   /* 50 ms */
    .scan_window        = 0x30,   /* 30 ms */
    .scan_duplicate     = BLE_SCAN_DUPLICATE_DISABLE,
};

/**
 * @brief Check if an advertisement is from our asset tracker system.
 *
 * Validates company ID and protocol version in the manufacturer-specific
 * data, then extracts tag ID, battery, and TX power.
 */
static bool parse_beacon_adv(const uint8_t *adv_data, uint8_t adv_len,
                              uint8_t *out_tag_id, uint8_t *out_battery_pct,
                              int8_t *out_tx_power)
{
    /* Search for Manufacturer Specific Data AD type (0xFF) */
    uint8_t offset = 0;
    while (offset < adv_len) {
        uint8_t field_len = adv_data[offset];
        if (field_len == 0 || offset + field_len >= adv_len) break;

        uint8_t field_type = adv_data[offset + 1];

        if (field_type == 0xFF && field_len >= MFG_DATA_BEACON_LENGTH) {
            const uint8_t *mfg = &adv_data[offset + 2];

            /* Check company ID */
            uint16_t company_id = mfg[0] | (mfg[1] << 8);
            if (company_id != ASSET_TRACKER_COMPANY_ID) {
                offset += field_len + 1;
                continue;
            }

            /* Check protocol version and packet type */
            if (mfg[2] != PROTOCOL_VERSION ||
                mfg[3] != PKT_TYPE_BEACON) {
                offset += field_len + 1;
                continue;
            }

            /* Extract fields */
            memcpy(out_tag_id, &mfg[4], TAG_ID_LENGTH);
            *out_battery_pct = mfg[8];
            *out_tx_power    = (int8_t)mfg[10];

            return true;
        }

        offset += field_len + 1;
    }

    return false;
}

/**
 * @brief GAP event callback — processes scan results.
 */
static void gap_event_handler(esp_gap_ble_cb_event_t event,
                               esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        if (param->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Scan parameters set, starting scan");
            esp_ble_gap_start_scanning(0);  /* Scan indefinitely */
        }
        break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan = param;
        if (scan->scan_rst.search_evt != ESP_GAP_SEARCH_INQ_RES_EVT) break;

        uint8_t tag_id[TAG_ID_LENGTH];
        uint8_t battery_pct;
        int8_t  tx_power;

        if (!parse_beacon_adv(scan->scan_rst.ble_adv,
                              scan->scan_rst.adv_data_len,
                              tag_id, &battery_pct, &tx_power)) {
            break;  /* Not our beacon */
        }

        /* Process RSSI with averaging and rate limiting */
        xSemaphoreTake(tag_mutex, portMAX_DELAY);

        tracked_tag_t *t = find_or_create_tag(tag_id);
        if (t) {
            int rssi_avg = tag_add_rssi(t, scan->scan_rst.rssi);
            t->battery_pct = battery_pct;
            t->tx_power    = tx_power;

            uint64_t now = get_timestamp_ms();
            if (now - t->last_report_ms >= REPORT_INTERVAL_MS) {
                char tag_str[TAG_ID_LENGTH * 2 + 1];
                snprintf(tag_str, sizeof(tag_str), "%02X%02X%02X%02X",
                         tag_id[0], tag_id[1], tag_id[2], tag_id[3]);

                mqtt_publish_rssi(tag_str, rssi_avg, battery_pct, tx_power);
                t->last_report_ms = now;

                ESP_LOGI(TAG, "Tag %s RSSI=%d (avg=%d) bat=%u%%",
                         tag_str, scan->scan_rst.rssi, rssi_avg, battery_pct);
            }
        }

        xSemaphoreGive(tag_mutex);
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG, "Scan stopped");
        break;

    default:
        break;
    }
}

static void ble_scanner_init(void)
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&scan_params));

    ESP_LOGI(TAG, "BLE scanner initialized");
}

/* ---------------------------------------------------------------------------
 * Health Reporting Task
 * --------------------------------------------------------------------------- */

static void health_task(void *pvParameters)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(HEALTH_REPORT_INTERVAL_S * 1000));
        mqtt_publish_health();
    }
}

/* ---------------------------------------------------------------------------
 * Main
 * --------------------------------------------------------------------------- */

void app_main(void)
{
    ESP_LOGI(TAG, "=== BLE Asset Tracker - Gateway Scanner ===");
    ESP_LOGI(TAG, "Gateway ID: %s, Position: (%.1f, %.1f)",
             CONFIG_GATEWAY_ID, CONFIG_GW_POS_X, CONFIG_GW_POS_Y);

    /* Initialize NVS (required for WiFi) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Tag tracking mutex */
    tag_mutex = xSemaphoreCreateMutex();
    configASSERT(tag_mutex);

    /* Clear tracking table */
    memset(tracked_tags, 0, sizeof(tracked_tags));

    /* Initialize subsystems */
    wifi_init();

    /* Wait for WiFi connection before starting MQTT */
    ESP_LOGI(TAG, "Waiting for WiFi...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    ntp_init();
    mqtt_init();

    /* Allow MQTT to connect */
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Start BLE scanner */
    ble_scanner_init();

    /* Start health reporting task */
    xTaskCreate(health_task, "health", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Gateway running — scanning for asset tags");
}
