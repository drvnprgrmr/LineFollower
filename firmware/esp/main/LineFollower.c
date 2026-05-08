#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_log.h>

#include <wifi_provisioner.h>
#include <mdns.h>

#include "http_server.h"

#define TAG "LineFollower"

#define LED_PIN GPIO_NUM_8

uint8_t led_state = 1;
uint32_t led_timer = 0;
uint32_t led_interval = 500 * 1000;

void start_mdns_service()
{
    // initialize mDNS service
    esp_err_t err = mdns_init();
    if (err)
    {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    // set hostname
    mdns_hostname_set("line-follower");

    // set default instance
    mdns_instance_name_set(TAG);
}

void app_main(void)
{
    // Configure and start the provisioner
    wifi_prov_config_t config = WIFI_PROV_DEFAULT_CONFIG();
    config.ap_ssid = TAG;

    ESP_ERROR_CHECK(wifi_prov_start(&config));

    // Block until connected (or use the event callback for non-blocking)
    wifi_prov_wait_for_connection(portMAX_DELAY);

    start_mdns_service();

    // start webserver
    httpd_handle_t server = start_webserver();
    register_uris(server);

    gpio_set_direction(GPIO_MODE_OUTPUT, LED_PIN);
    gpio_set_level(LED_PIN, 0);

    while (true)
    {
        vTaskDelay(1);
    }
}
