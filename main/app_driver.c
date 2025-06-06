/*  Temperature Sensor demo implementation using RGB LED and timer

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <sdkconfig.h>

#include <app_reset.h>
#include "app_priv.h"
#include "esp_now_hub.h"

#include "driver/gpio.h"
#include "iot_button.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "app_driver";

light_state_t g_power = DEFAULT_POWER;

void set_light_state(light_state_t power)
{
    gpio_set_level(LIGHT_GPIO, power);
    ESP_LOGI(TAG, "Light state set to: %s", power ? "ON" : "OFF");
    g_power = power;
}

static void light_driver_cfg(void)
{
    gpio_reset_pin(LIGHT_GPIO);
    gpio_set_direction(LIGHT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LIGHT_GPIO, DEFAULT_POWER);

}

static void push_btn_cb(void *arg)
{
    light_state_t power = DEFAULT_POWER;
    power = !g_power; // Toggle the power state
    ESP_LOGI(TAG, "Button pressed, toggling light state to: %s", power ? "ON" : "OFF");
    // Set the light state and send the new state via ESPNOW
    set_light_state(power);
    espnow_send_light_state(power);
}

void app_light_init()
{
    light_driver_cfg();

    button_handle_t btn_handle = iot_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL);

    if (btn_handle)
    {
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, push_btn_cb, NULL);
        app_reset_button_register(btn_handle, WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
        ESP_LOGI(TAG, "Button initialized successfully");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to initialize button");
    }
}
