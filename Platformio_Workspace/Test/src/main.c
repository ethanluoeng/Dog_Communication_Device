#include <stdint.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_log.h"

// ESP32-S3-DevKitC-1 onboard addressable RGB LED (WS2812), driven directly
// over RMT -- no external "led_strip" component needed.
//   - v1.1 boards: onboard LED on GPIO38 (current default below)
//   - v1.0 boards: onboard LED on GPIO48
// If nothing lights up, check your board's silkscreen/schematic and flip this.
#define LED_GPIO            48
#define RMT_RESOLUTION_HZ   10000000 // 10 MHz -> 1 tick = 100 ns

static const char *TAG = "rgb_fade";

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

static void led_strip_init(void)
{
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = LED_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    // WS2812 bit timings (well within datasheet tolerance), MSB first.
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .duration0 = 3, // T0H = 0.3us
            .level0 = 1,
            .duration1 = 9, // T0L = 0.9us
            .level1 = 0,
        },
        .bit1 = {
            .duration0 = 9, // T1H = 0.9us
            .level0 = 1,
            .duration1 = 3, // T1L = 0.3us
            .level1 = 0,
        },
        .flags.msb_first = 1,
    };
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder));

    ESP_ERROR_CHECK(rmt_enable(led_chan));
}

static void led_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    // WS2812 wants GRB byte order, one 24-bit frame for our single pixel.
    // Our update cadence (tens of ms) is far longer than the >50us reset gap
    // the chip needs between frames, so no separate reset symbol is required.
    uint8_t pixel[3] = { g, r, b };
    rmt_transmit_config_t tx_config = { .loop_count = 0 };
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, pixel, sizeof(pixel), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, 100));
}

// Minimal HSV -> RGB conversion (h: 0-360 degrees, s/v: 0-1)
static void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    float c = v * s;
    float x = c * (1 - fabsf(fmodf(h / 60.0f, 2.0f) - 1));
    float m = v - c;
    float rf = 0, gf = 0, bf = 0;

    if (h < 60)       { rf = c; gf = x; bf = 0; }
    else if (h < 120) { rf = x; gf = c; bf = 0; }
    else if (h < 180) { rf = 0; gf = c; bf = x; }
    else if (h < 240) { rf = 0; gf = x; bf = c; }
    else if (h < 300) { rf = x; gf = 0; bf = c; }
    else              { rf = c; gf = 0; bf = x; }

    *r = (uint8_t)((rf + m) * 255);
    *g = (uint8_t)((gf + m) * 255);
    *b = (uint8_t)((bf + m) * 255);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Fading onboard RGB LED through the color wheel");
    led_strip_init();

    float hue = 0.0f;
    while (1) {
        uint8_t r, g, b;
        hsv_to_rgb(hue, 1.0f, 0.15f, &r, &g, &b); // 15% brightness: comfortable to look at
        led_set_rgb(r, g, b);

        hue += 1.0f;
        if (hue >= 360.0f) {
            hue = 0.0f;
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // ~7.2s per full rainbow cycle
    }
}
