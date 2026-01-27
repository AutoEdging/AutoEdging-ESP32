#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_err.h"

#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

static const char *TAG = "ble_belt";

typedef struct {
    const uint8_t *data;
    uint16_t len;
} adv_payload_t;

#define ADV_PL(...) \
    { .data = (const uint8_t[]){ __VA_ARGS__ }, .len = sizeof((const uint8_t[]){ __VA_ARGS__ }) }

/* 写死的 payload（保持与你测试代码一致） */
static const adv_payload_t g_vibrate_payloads[10] = {
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xD4,0x1F,0x5D, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xD7,0x84,0x6F, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xD6,0x0D,0x7E, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xD1,0xB2,0x0A, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xD0,0x3B,0x1B, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xD3,0xA0,0x29, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xD2,0x29,0x38, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xDD,0xDE,0xC0, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xDC,0x57,0xD1, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xD5,0x96,0x4C, 0x03,0x03,0x8F,0xAE), // stop vibrate
};

static const adv_payload_t g_swing_payloads[10] = {
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xA4,0x98,0x2E, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xA7,0x03,0x1C, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xA6,0x8A,0x0D, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xA1,0x35,0x79, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xA0,0xBC,0x68, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xA3,0x27,0x5A, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xA2,0xAE,0x4B, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xAD,0x59,0xB3, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xAC,0xD0,0xA2, 0x03,0x03,0x8F,0xAE),
    ADV_PL(0x02,0x01,0x01, 0x0E,0xFF,0xFF,0x00, 0x6D,0xB6,0x43,0xCE,0x97,0xFE,0x42,0x7C,0xA5,0x11,0x3F, 0x03,0x03,0x8F,0xAE), // stop swing
};

static const uint8_t g_adv_handle = 0x02;
static uint8_t g_rand_addr[6] = {0xC2,0x11,0x22,0x33,0x44,0x55};

static bool g_inited = false;
static bool g_scan_rsp_configured = false;

static SemaphoreHandle_t g_lock;
static EventGroupHandle_t g_evt;

#define EVT_READY        (1U << 0)
#define EVT_DATA_SET_OK  (1U << 1)
#define EVT_START_OK     (1U << 2)
#define EVT_FAIL         (1U << 15)

static const adv_payload_t *g_pending_payload = NULL;

static esp_ble_gap_ext_adv_params_t g_ext_adv_params = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_LEGACY |
            ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE |
            ESP_BLE_GAP_SET_EXT_ADV_PROP_SCANNABLE,
    .interval_min = 0x00A0,  // 100ms
    .interval_max = 0x00A0,
    .channel_map = ADV_CHNL_ALL,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .tx_power = ESP_BLE_PWR_TYPE_DEFAULT,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_1M,
    .sid = 0,
    .scan_req_notif = false,
};

static void start_ext_adv(void)
{
    esp_ble_gap_ext_adv_t adv = {
        .instance = g_adv_handle,
        .duration = 0,
        .max_events = 0,
    };
    // enable advertising set(s)
    esp_err_t err = esp_ble_gap_ext_adv_start(1, &adv);  // :contentReference[oaicite:4]{index=4}
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ext_adv_start failed: %s", esp_err_to_name(err));
        xEventGroupSetBits(g_evt, EVT_FAIL);
    }
}

static void stop_ext_adv(void)
{
    uint8_t inst[1] = { g_adv_handle };
    (void)esp_ble_gap_ext_adv_stop(1, inst); // :contentReference[oaicite:5]{index=5}
}

static esp_err_t config_payload(const adv_payload_t *pl)
{
    if (!pl || !pl->data || pl->len == 0) return ESP_ERR_INVALID_ARG;

    // 配置 raw adv data
    esp_err_t err = esp_ble_gap_config_ext_adv_data_raw(g_adv_handle, pl->len, pl->data); // :contentReference[oaicite:6]{index=6}
    if (err != ESP_OK) return err;

    // 只配置一次“空 scan rsp”
    if (!g_scan_rsp_configured) {
        err = esp_ble_gap_config_ext_scan_rsp_data_raw(g_adv_handle, 0, NULL); // :contentReference[oaicite:7]{index=7}
        if (err != ESP_OK) return err;
        g_scan_rsp_configured = true;
    }
    return ESP_OK;
}

static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
        if (param->ext_adv_set_params.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "set_params failed: %d", param->ext_adv_set_params.status);
            xEventGroupSetBits(g_evt, EVT_FAIL);
            break;
        }
        // 设置随机地址（你的抓包流程一致）
        esp_ble_gap_ext_adv_set_rand_addr(g_adv_handle, g_rand_addr); // :contentReference[oaicite:8]{index=8}
        break;

    case ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
        if (param->ext_adv_set_rand_addr.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "set_rand_addr failed: %d", param->ext_adv_set_rand_addr.status);
            xEventGroupSetBits(g_evt, EVT_FAIL);
            break;
        }
        // 默认先装载 payload 0 并启动
        g_pending_payload = &g_vibrate_payloads[0];
        if (config_payload(g_pending_payload) != ESP_OK) {
            xEventGroupSetBits(g_evt, EVT_FAIL);
        }
        break;

    case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
        if (param->ext_adv_data_set.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "adv_data_set failed: %d", param->ext_adv_data_set.status);
            xEventGroupSetBits(g_evt, EVT_FAIL);
            break;
        }
        xEventGroupSetBits(g_evt, EVT_DATA_SET_OK);
        // 数据配置完成后启动广播
        start_ext_adv();
        break;

    case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
        if (param->ext_adv_start.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "adv_start failed: %d", param->ext_adv_start.status);
            xEventGroupSetBits(g_evt, EVT_FAIL);
            break;
        }
        xEventGroupSetBits(g_evt, EVT_START_OK | EVT_READY);
        break;

    default:
        break;
    }
}

static esp_err_t nvs_init_once(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

esp_err_t ble_belt_init(void)
{
    if (g_inited) return ESP_OK;

    g_lock = xSemaphoreCreateMutex();
    g_evt  = xEventGroupCreate();
    if (!g_lock || !g_evt) return ESP_ERR_NO_MEM;

    ESP_ERROR_CHECK(nvs_init_once());

    // 仅用 BLE：释放 Classic BT（可选，但常用） :contentReference[oaicite:9]{index=9}
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_cb));

    // 设置扩展广播参数（触发 SET_PARAMS_COMPLETE） :contentReference[oaicite:10]{index=10}
    ESP_ERROR_CHECK(esp_ble_gap_ext_adv_set_params(g_adv_handle, &g_ext_adv_params));

    // 等待 ready（首次 payload 装载并开始广播）
    EventBits_t bits = xEventGroupWaitBits(
        g_evt, EVT_READY | EVT_FAIL, pdTRUE, pdFALSE, pdMS_TO_TICKS(1500)
    );
    if (bits & EVT_FAIL) return ESP_FAIL;
    if (!(bits & EVT_READY)) return ESP_ERR_TIMEOUT;

    g_inited = true;
    ESP_LOGI(TAG, "init ok");
    return ESP_OK;
}

static esp_err_t send_payload_sync(const adv_payload_t *pl)
{
    if (!g_inited) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(g_lock, portMAX_DELAY);

    xEventGroupClearBits(g_evt, EVT_DATA_SET_OK | EVT_START_OK | EVT_FAIL);

    stop_ext_adv();
    g_pending_payload = pl;

    esp_err_t err = config_payload(pl);
    if (err != ESP_OK) {
        xSemaphoreGive(g_lock);
        return err;
    }

    // 等待：adv data set complete + start complete（顺序由回调驱动）
    EventBits_t bits = xEventGroupWaitBits(
        g_evt, EVT_DATA_SET_OK | EVT_START_OK | EVT_FAIL, pdTRUE, pdFALSE, pdMS_TO_TICKS(500)
    );

    xSemaphoreGive(g_lock);

    if (bits & EVT_FAIL) return ESP_FAIL;
    if ((bits & (EVT_DATA_SET_OK | EVT_START_OK)) != (EVT_DATA_SET_OK | EVT_START_OK)) return ESP_ERR_TIMEOUT;

    return ESP_OK;
}

esp_err_t ble_belt_send_vibrate(int idx)
{
    if (idx < 0 || idx >= 10) return ESP_ERR_INVALID_ARG;
    return send_payload_sync(&g_vibrate_payloads[idx]);
}

esp_err_t ble_belt_send_swing(int idx)
{
    if (idx < 0 || idx >= 10) return ESP_ERR_INVALID_ARG;
    return send_payload_sync(&g_swing_payloads[idx]);
}
