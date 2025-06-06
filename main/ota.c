#include <string.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_common_events.h>
#include <app_network.h>

static const char *TAG = "ESP-LIGHT-OTA";


/* Event handler for catching RainMaker events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == RMAKER_EVENT)
    {
        switch (event_id)
        {
        case RMAKER_EVENT_INIT_DONE:
            ESP_LOGI(TAG, "RainMaker Initialised.");
            break;
        case RMAKER_EVENT_CLAIM_STARTED:
            ESP_LOGI(TAG, "RainMaker Claim Started.");
            break;
        case RMAKER_EVENT_CLAIM_SUCCESSFUL:
            ESP_LOGI(TAG, "RainMaker Claim Successful.");
            break;
        case RMAKER_EVENT_CLAIM_FAILED:
            ESP_LOGI(TAG, "RainMaker Claim Failed.");
            break;
        case RMAKER_EVENT_LOCAL_CTRL_STARTED:
            ESP_LOGI(TAG, "Local Control Started.");
            break;
        case RMAKER_EVENT_LOCAL_CTRL_STOPPED:
            ESP_LOGI(TAG, "Local Control Stopped.");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled RainMaker Event: %" PRIi32, event_id);
        }
    }
    else if (event_base == RMAKER_COMMON_EVENT)
    {
        switch (event_id)
        {
        case RMAKER_EVENT_REBOOT:
            ESP_LOGI(TAG, "Rebooting in %d seconds.", *((uint8_t *)event_data));
            break;
        case RMAKER_EVENT_WIFI_RESET:
            ESP_LOGI(TAG, "Wi-Fi credentials reset.");
            break;
        case RMAKER_EVENT_FACTORY_RESET:
            ESP_LOGI(TAG, "Node reset to factory defaults.");
            break;
        case RMAKER_MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected.");
            break;
        case RMAKER_MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected.");
            break;
        case RMAKER_MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT Published. Msg id: %d.", *((int *)event_data));
            break;
        default:
            ESP_LOGW(TAG, "Unhandled RainMaker Common Event: %" PRIi32, event_id);
        }
    }
    else if (event_base == APP_NETWORK_EVENT)
    {
        switch (event_id)
        {
        case APP_NETWORK_EVENT_QR_DISPLAY:
            ESP_LOGI(TAG, "Provisioning QR : %s", (char *)event_data);
            break;
        case APP_NETWORK_EVENT_PROV_TIMEOUT:
            ESP_LOGI(TAG, "Provisioning Timed Out. Please reboot.");
            break;
        case APP_NETWORK_EVENT_PROV_RESTART:
            ESP_LOGI(TAG, "Provisioning has restarted due to failures.");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled App Wi-Fi Event: %" PRIi32, event_id);
            break;
        }
    }
    else if (event_base == RMAKER_OTA_EVENT)
    {
        switch (event_id)
        {
        case RMAKER_OTA_EVENT_STARTING:
            ESP_LOGI(TAG, "Starting OTA.");
            break;
        case RMAKER_OTA_EVENT_IN_PROGRESS:
            ESP_LOGI(TAG, "OTA is in progress.");
            break;
        case RMAKER_OTA_EVENT_SUCCESSFUL:
            ESP_LOGI(TAG, "OTA successful.");
            break;
        case RMAKER_OTA_EVENT_FAILED:
            ESP_LOGI(TAG, "OTA Failed.");
            break;
        case RMAKER_OTA_EVENT_REJECTED:
            ESP_LOGI(TAG, "OTA Rejected.");
            break;
        case RMAKER_OTA_EVENT_DELAYED:
            ESP_LOGI(TAG, "OTA Delayed.");
            break;
        case RMAKER_OTA_EVENT_REQ_FOR_REBOOT:
            ESP_LOGI(TAG, "Firmware image downloaded. Please reboot your device to apply the upgrade.");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled OTA Event: %" PRIi32, event_id);
            break;
        }
    }
    else
    {
        ESP_LOGW(TAG, "Invalid event received!");
    }
}

void app_ota_start()
{

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init()
     */
    app_network_init();

    /* Register an event handler to catch RainMaker events */
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_COMMON_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(APP_NETWORK_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_OTA_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_network_init() but before app_nenetworkk_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP LIGHT OTA", "OTA");
    if (!node)
    {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    /* Enable OTA */
    esp_rmaker_ota_enable_default();

    /* Enable timezone service which will be require for setting appropriate timezone
     * from the phone apps for scheduling to work correctly.
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_timezone_service_enable();

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_network_start(POP_TYPE_RANDOM);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    
}