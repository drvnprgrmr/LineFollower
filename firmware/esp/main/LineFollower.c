#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "wifi_provisioner.h"

#define APP_NAME "LineFollower"

void app_main(void)
{
    // Configure and start the provisioner
    wifi_prov_config_t config = WIFI_PROV_DEFAULT_CONFIG();
    config.ap_ssid = APP_NAME;

    ESP_ERROR_CHECK(wifi_prov_start(&config));

    // Block until connected (or use the event callback for non-blocking)
    wifi_prov_wait_for_connection(portMAX_DELAY);

    while (true)
    {

        vTaskDelay(1);
    }
}
