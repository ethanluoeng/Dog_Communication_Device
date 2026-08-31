#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

static const char *TAG = "audio";

#define I2S_BCLK   GPIO_NUM_5
#define I2S_WS     GPIO_NUM_6
#define I2S_DOUT   GPIO_NUM_7
#define WAV_SAMPLE_RATE 16000    // must match how you exported the clips

// ---- embedded WAV symbols (names derived from the filenames) ----
extern const uint8_t singing_wav_start[] asm("_binary_singing_wav_start");
extern const uint8_t singing_wav_end[]   asm("_binary_singing_wav_end");    

typedef struct {
    const char    *name;
    const uint8_t *start;
    const uint8_t *end;
} clip_t;

static const clip_t clips[] = {
    { "singing", singing_wav_start, singing_wav_end },
};
#define N_CLIPS (sizeof(clips) / sizeof(clips[0]))

static i2s_chan_handle_t tx_chan;

static void i2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(WAV_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK, .ws = I2S_WS, .dout = I2S_DOUT,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { false, false, false },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));  // leave the channel initialized but stopped
}

// Locate the PCM inside a WAV byte buffer (skips header + any extra chunks).
static bool wav_data_span(const uint8_t *buf, size_t len,
                          const uint8_t **pcm, size_t *pcm_len)
{
    if (len < 12 || memcmp(buf, "RIFF", 4) || memcmp(buf + 8, "WAVE", 4)) return false;
    size_t pos = 12;
    while (pos + 8 <= len) {
        const uint8_t *id = buf + pos;
        uint32_t sz = buf[pos+4] | (buf[pos+5]<<8) | (buf[pos+6]<<16) | ((uint32_t)buf[pos+7]<<24);
        pos += 8;
        if (!memcmp(id, "data", 4)) {
            if (pos + sz > len) sz = len - pos;   // clamp to what's really there
            *pcm = buf + pos;
            *pcm_len = sz;
            return true;
        }
        pos += (sz + 1) & ~1u;                    // next chunk, word-aligned
    }
    return false;
}

static float g_volume = 1.0f;

static void play_clip(const char *name)
{
    for (size_t i = 0; i < N_CLIPS; i++) {
        if (strcmp(name, clips[i].name) != 0) continue;

        const uint8_t *pcm; size_t pcm_len;
        size_t total = clips[i].end - clips[i].start;
        if (!wav_data_span(clips[i].start, total, &pcm, &pcm_len)) {
            ESP_LOGE(TAG, "bad WAV: %s", name);
            return;
        }
        ESP_LOGI(TAG, "playing %s (%u bytes)", name, (unsigned)pcm_len);
        ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));   // start clocking

        int16_t mono[256];
        int16_t stereo[512];
        size_t off = 0;
        while (off < pcm_len) {
            size_t chunk = pcm_len - off;
            if (chunk > sizeof(mono)) chunk = sizeof(mono);
            memcpy(mono, pcm + off, chunk);        // memcpy avoids alignment issues
            int n = chunk / 2;
                for (int s = 0; s < n; s++) {
                    int32_t v = (int32_t)(mono[s] * g_volume);   // scale
                    if (v >  32767) v =  32767;                  // clamp (safety)
                    if (v < -32768) v = -32768;
                    int16_t out = (int16_t)v;
                    stereo[2*s]     = out;
                    stereo[2*s + 1] = out;
                }
            size_t written = 0;
            i2s_channel_write(tx_chan, stereo, n * 2 * sizeof(int16_t), &written, portMAX_DELAY);
            off += chunk;
        }

        // flush the DMA with silence so the tail is 0, not a repeated buffer
        memset(stereo, 0, sizeof(stereo));
        for (int k = 0; k < 6; k++) {
            size_t written = 0;
            i2s_channel_write(tx_chan, stereo, sizeof(stereo), &written, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(10));                    // let it drain
        ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));    // stop clocking

        ESP_LOGI(TAG, "done");
        return;
    }
    ESP_LOGE(TAG, "not found: %s", name);
}

void app_main(void)
{
    i2s_init();
    ESP_LOGI(TAG, "Ready. Type a clip name (sit / come) + Enter:");

    char line[64];
    int idx = 0;
    while (1) {
        int c = getchar();
        if (c == EOF) { vTaskDelay(pdMS_TO_TICKS(20)); continue; }
        if (c == '\n' || c == '\r') {
            if (idx > 0) { line[idx] = 0; play_clip(line); idx = 0; }
        } else if (idx < (int)sizeof(line) - 1) {
            line[idx++] = (char)c;
        }
    }
}