/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_now_hub.h"

#define DEFAULT_POWER false
/* This is the button that is used for toggling the power */
#define BUTTON_GPIO          1
#define BUTTON_ACTIVE_LEVEL  0
/* This is the GPIO on which the power will be set */
#define LIGHT_GPIO    2

#define WIFI_RESET_BUTTON_TIMEOUT  3
#define FACTORY_RESET_BUTTON_TIMEOUT  10


void set_light_state(light_state_t power);
void app_light_init();
