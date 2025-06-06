#include <stdio.h>
#include "app_priv.h"
#include "esp_now_hub.h"
#include "esp_log.h"
#include "ota.h"

void app_main(void)
{
    app_ota_start();
    app_espnow_start();
    app_light_init();
    ESP_LOGI("app_main", "Light device started, waiting for button presses...");
}
