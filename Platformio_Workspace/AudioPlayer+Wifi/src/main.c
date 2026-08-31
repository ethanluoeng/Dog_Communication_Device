#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

static const char *TAG = "dog";

// ---------------- audio ----------------
#define I2S_BCLK   GPIO_NUM_5
#define I2S_WS     GPIO_NUM_6
#define I2S_DOUT   GPIO_NUM_7
#define WAV_SAMPLE_RATE 16000

static float g_volume = 0.6f;   // 0.0..1.0 loudness cap

extern const uint8_t singing_wav_start[] asm("_binary_singing_wav_start");
extern const uint8_t singing_wav_end[]   asm("_binary_singing_wav_end");

typedef struct { const char *name; const uint8_t *start; const uint8_t *end; } clip_t;
static const clip_t clips[] = {
    { "singing", singing_wav_start, singing_wav_end },
    // add more: { "sit", sit_wav_start, sit_wav_end }, ...
};
#define N_CLIPS (sizeof(clips)/sizeof(clips[0]))

static i2s_chan_handle_t tx_chan;
static QueueHandle_t     clip_queue;

static void i2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(WAV_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = { .mclk=I2S_GPIO_UNUSED, .bclk=I2S_BCLK, .ws=I2S_WS, .dout=I2S_DOUT,
                      .din=I2S_GPIO_UNUSED, .invert_flags={false,false,false} },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    // channel is enabled per-clip, not here
}

static bool wav_data_span(const uint8_t *buf, size_t len, const uint8_t **pcm, size_t *pcm_len)
{
    if (len < 12 || memcmp(buf, "RIFF", 4) || memcmp(buf+8, "WAVE", 4)) return false;
    size_t pos = 12;
    while (pos + 8 <= len) {
        const uint8_t *id = buf + pos;
        uint32_t sz = buf[pos+4] | (buf[pos+5]<<8) | (buf[pos+6]<<16) | ((uint32_t)buf[pos+7]<<24);
        pos += 8;
        if (!memcmp(id, "data", 4)) {
            if (pos + sz > len) sz = len - pos;
            *pcm = buf + pos; *pcm_len = sz; return true;
        }
        pos += (sz + 1) & ~1u;
    }
    return false;
}

static void play_clip(const char *name)
{
    for (size_t c = 0; c < N_CLIPS; c++) {
        if (strcmp(name, clips[c].name) != 0) continue;

        const uint8_t *pcm; size_t pcm_len;
        size_t total = clips[c].end - clips[c].start;
        if (!wav_data_span(clips[c].start, total, &pcm, &pcm_len)) {
            ESP_LOGE(TAG, "bad WAV: %s", name); return;
        }
        ESP_LOGI(TAG, "playing %s (vol %.2f)", name, g_volume);
        ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

        int16_t mono[256], stereo[512];
        size_t off = 0;
        while (off < pcm_len) {
            size_t chunk = pcm_len - off;
            if (chunk > sizeof(mono)) chunk = sizeof(mono);
            memcpy(mono, pcm + off, chunk);
            int n = chunk / 2;
            for (int s = 0; s < n; s++) {
                int32_t v = (int32_t)(mono[s] * g_volume);
                if (v >  32767) v =  32767;
                if (v < -32768) v = -32768;
                int16_t o = (int16_t)v;
                stereo[2*s] = o; stereo[2*s+1] = o;
            }
            size_t written = 0;
            i2s_channel_write(tx_chan, stereo, n*2*sizeof(int16_t), &written, portMAX_DELAY);
            off += chunk;
        }
        // flush with silence, then stop clocking (kills the buzz + sleeps the amp)
        memset(stereo, 0, sizeof(stereo));
        for (int k = 0; k < 6; k++) {
            size_t written = 0;
            i2s_channel_write(tx_chan, stereo, sizeof(stereo), &written, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
        ESP_LOGI(TAG, "done");
        return;
    }
    ESP_LOGE(TAG, "not found: %s", name);
}

static void audio_task(void *arg)
{
    int idx;
    while (1)
        if (xQueueReceive(clip_queue, &idx, portMAX_DELAY))
            if (idx >= 0 && idx < (int)N_CLIPS) play_clip(clips[idx].name);
}

// ---------------- Wi-Fi SoftAP ----------------
#define AP_SSID    "DogTrainer"
#define AP_PASS    "dogdog1234"   // must be >=8 chars, or "" for an open network
#define AP_CHANNEL 1
#define AP_MAXCONN 4

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wc = {
        .ap = {
            .ssid = AP_SSID, .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL, .password = AP_PASS,
            .max_connection = AP_MAXCONN, .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    if (strlen(AP_PASS) == 0) wc.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "SoftAP '%s' up -> http://192.168.4.1", AP_SSID);
}

// ---------------- HTTP ----------------
static esp_err_t index_get_handler(httpd_req_t *req)
{
    static char page[2048];
    int len = snprintf(page, sizeof(page),
        "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>Dog Trainer</title><style>"
        "body{font-family:sans-serif;text-align:center;background:#111;color:#eee;padding:20px}"
        "button{font-size:1.4em;padding:18px;margin:8px;border:0;border-radius:12px;"
        "background:#2dd47a;color:#000;width:80%%;max-width:400px}"
        "button:active{background:#1a9a55}input{width:80%%;max-width:400px}"
        "</style></head><body><h2>Dog Trainer</h2>");
    for (size_t i = 0; i < N_CLIPS; i++)
        len += snprintf(page+len, sizeof(page)-len,
            "<button onclick=\"fetch('/play?clip=%s')\">%s</button><br>", clips[i].name, clips[i].name);
    len += snprintf(page+len, sizeof(page)-len,
        "<h3>Volume</h3><input type='range' min='0' max='100' value='60' "
        "oninput=\"fetch('/volume?v='+this.value)\"></body></html>");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, page, len);
    return ESP_OK;
}

static esp_err_t play_get_handler(httpd_req_t *req)
{
    char q[64], clip[32];
    if (httpd_req_get_url_query_str(req, q, sizeof(q)) == ESP_OK &&
        httpd_query_key_value(q, "clip", clip, sizeof(clip)) == ESP_OK) {
        for (int i = 0; i < (int)N_CLIPS; i++)
            if (strcmp(clip, clips[i].name) == 0) {
                xQueueSend(clip_queue, &i, 0);   // non-blocking; queue serializes
                httpd_resp_sendstr(req, "OK");
                return ESP_OK;
            }
    }
    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_sendstr(req, "unknown clip");
    return ESP_OK;
}

static esp_err_t volume_get_handler(httpd_req_t *req)
{
    char q[32], v[8];
    if (httpd_req_get_url_query_str(req, q, sizeof(q)) == ESP_OK &&
        httpd_query_key_value(q, "v", v, sizeof(v)) == ESP_OK) {
        int pct = atoi(v);
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;      // <-- LOWER THIS to hard-cap loudness for the dog
        g_volume = pct / 100.0f;
        httpd_resp_sendstr(req, "OK");
        return ESP_OK;
    }
    httpd_resp_sendstr(req, "bad");
    return ESP_OK;
}

static void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(httpd_start(&server, &config));
    httpd_uri_t u_index  = { .uri="/",       .method=HTTP_GET, .handler=index_get_handler };
    httpd_uri_t u_play   = { .uri="/play",   .method=HTTP_GET, .handler=play_get_handler };
    httpd_uri_t u_volume = { .uri="/volume", .method=HTTP_GET, .handler=volume_get_handler };
    httpd_register_uri_handler(server, &u_index);
    httpd_register_uri_handler(server, &u_play);
    httpd_register_uri_handler(server, &u_volume);
}

// ---------------- main ----------------
void app_main(void)
{
    esp_err_t ret = nvs_flash_init();          // Wi-Fi needs NVS
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    i2s_init();
    clip_queue = xQueueCreate(4, sizeof(int));
    xTaskCreate(audio_task, "audio", 4096, NULL, 5, NULL);

    wifi_init_softap();
    start_webserver();
    ESP_LOGI(TAG, "Ready: join Wi-Fi '%s', open http://192.168.4.1", AP_SSID);
}