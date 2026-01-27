#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "driver/rmt_tx.h"
#include "led_strip.h"

#include "bus_i2c.h"
#include "dev_mcp_h11.h"
#include "dev_dac7571.h"

static const char *TAG = "app_main";

#define APP_I2C_PORT        I2C_NUM_0
#define APP_I2C_SDA_GPIO    GPIO_NUM_5
#define APP_I2C_SCL_GPIO    GPIO_NUM_4
#define APP_I2C_FREQ_HZ     400000

#define BLINK_GPIO          38

static void app_init_devices(led_strip_handle_t *led_strip, mcp_h11_t *mcp, dac7571_t *dac)
{
    // 0) init LED strip
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, led_strip));
    ESP_ERROR_CHECK(led_strip_clear(*led_strip));

    // 1) init I2C bus
    bus_i2c_t i2c = {0};
    bus_i2c_config_t bus_cfg = {
        .port = APP_I2C_PORT,
        .sda_io = APP_I2C_SDA_GPIO,
        .scl_io = APP_I2C_SCL_GPIO,
        .clk_speed_hz = APP_I2C_FREQ_HZ,
        .enable_internal_pullups = true,
        .glitch_ignore_cnt = 7,
    };
    ESP_ERROR_CHECK(bus_i2c_init(&i2c, &bus_cfg));

    // 2) init MCP-H11
    mcp_h11_config_t mcp_cfg = {
        .i2c_addr_7bit = 0x36,          // MCP-H11 datasheet: 7-bit 0x36
        .scl_speed_hz = APP_I2C_FREQ_HZ,
        .a = 50.0f,
        .b = -5.0f,
    };
    ESP_ERROR_CHECK(mcp_h11_init(mcp, i2c.bus, &mcp_cfg));

    // 3) init DAC7571
    dac7571_config_t dac_cfg = {
        .i2c_addr_7bit = 0x4D,          // DAC7571: A0=0 => 0x4C；A0=1 => 0x4D
        .scl_speed_hz = APP_I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(dac7571_init(dac, i2c.bus, &dac_cfg));
}

void app_main(void)
{
    ESP_LOGI(TAG, "====== Boot ======");

    led_strip_handle_t led_strip = NULL;
    mcp_h11_t mcp_h11 = {0};
    dac7571_t dac7571 = {0};
    app_init_devices(&led_strip, &mcp_h11, &dac7571);

    uint16_t dac_code = 0;

    while (1) {
        mcp_h11_sample_t s = {0};
        esp_err_t err = mcp_h11_read_sample(&mcp_h11, &s);

        if (err == ESP_OK) {
            // 先用锯齿波验证 DAC 写入稳定
            dac_code = (dac_code + 0x010) & 0x0FFF;
            ESP_ERROR_CHECK(dac7571_write(&dac7571, dac_code, DAC7571_PD_NORMAL));

            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, s.pressure_kpa*10, 0, 0));
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));

            ESP_LOGI(TAG,
                     "status=0x%02X p=%.3f kPa t=%.2f C dac=%.3f V",
                     s.status, (double)s.pressure_kpa, (double)s.temp_c, (dac_code/4095.0*3.3));
        } else {
            ESP_LOGW(TAG, "mcp read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}