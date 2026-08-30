#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(500));   // let the monitor attach

#if CONFIG_SPIRAM
    size_t psram = esp_psram_get_size();
    printf(">> PSRAM: %u bytes (%.2f MB)\n", (unsigned)psram, psram / (1024.0 * 1024.0));
#else
    printf(">> PSRAM NOT enabled in sdkconfig!\n");
#endif

    uint32_t flash = 0;
    esp_flash_get_size(NULL, &flash);
    printf(">> Flash: %lu bytes (%.2f MB)\n", (unsigned long)flash, flash / (1024.0 * 1024.0));

    printf(">> PSRAM in heap: %u bytes\n",
           (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
}