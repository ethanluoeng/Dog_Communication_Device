#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const char *TAG = "tone";

// ---- YOUR WIRING (matches the pinout we set) ----
#define I2S_BCLK   GPIO_NUM_5    // amp BCLK
#define I2S_WS     GPIO_NUM_6    // amp LRC / word select
#define I2S_DOUT   GPIO_NUM_7    // amp DIN

// ---- TONE SETTINGS ----
#define SAMPLE_RATE   44100
#define TONE_HZ       440.0f     // A4, an easy-to-hear beep
#define AMPLITUDE     6000       // int16 peak; 32767 = max. START LOW.

static i2s_chan_handle_t tx_chan;

static void i2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK,
            .ws   = I2S_WS,
            .dout = I2S_DOUT,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}

void app_main(void)
{
    i2s_init();
    ESP_LOGI(TAG, "I2S up — playing %.0f Hz at amplitude %d", TONE_HZ, AMPLITUDE);

    const int frames = 256;              // frames per write
    int16_t buf[frames * 2];             // *2 because stereo (L,R,L,R...)
    float phase = 0.0f;
    const float phase_inc = 2.0f * M_PI * TONE_HZ / SAMPLE_RATE;

    while (1) {
        for (int i = 0; i < frames; i++) {
            int16_t s = (int16_t)(AMPLITUDE * sinf(phase));
            buf[2 * i]     = s;          // left channel
            buf[2 * i + 1] = s;          // right channel (same = mono sound)
            phase += phase_inc;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
        size_t written = 0;
        i2s_channel_write(tx_chan, buf, sizeof(buf), &written, portMAX_DELAY);
    }
}