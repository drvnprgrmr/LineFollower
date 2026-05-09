#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_log.h>

#include <wifi_provisioner.h>
#include <mdns.h>

#include "http_server.h"
#include "pwm.h"

#define TAG "LineFollower"

#define LED_PIN GPIO_NUM_8

uint8_t led_state = 1;
uint32_t led_timer = 0;
uint32_t led_interval = 500 * 1000;

#define MOTOR_PWM_FREQUENCY 20 * 1000 // 20kHz

#define ENABLE_PIN GPIO_NUM_0

#define LEFT_MOTOR_A_PIN GPIO_NUM_6
#define LEFT_MOTOR_B_PIN GPIO_NUM_7

#define RIGHT_MOTOR_A_PIN GPIO_NUM_8
#define RIGHT_MOTOR_B_PIN GPIO_NUM_9

typedef enum Wheel
{
    LEFT_WHEEL,
    RIGHT_WHEEL,
} Wheel;

void enableMotors()
{
    gpio_set_level(ENABLE_PIN, 1);
}

void disableMotors()
{
    gpio_set_level(ENABLE_PIN, 1);
}

void updateWheel(Wheel wheel, double speed)
{
}

// todo: gradually increase and decrese left and right motor speeds
void testMotors()
{
    uint32_t updateInterval = 100 * 1000;

    for (double i = 0.0; i <= 1.0; i += 0.1)
    {
    }
}

void initMotors()
{
    pwm_init(MOTOR_PWM_FREQUENCY);

    // configure pins
    gpio_set_direction(ENABLE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(ENABLE_PIN, 1);

    gpio_set_direction(LEFT_MOTOR_A_PIN, GPIO_MODE_OUTPUT);
    pwm_enable(LEFT_MOTOR_A_PIN);
    gpio_set_direction(LEFT_MOTOR_B_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LEFT_MOTOR_B_PIN, 0);

    gpio_set_direction(RIGHT_MOTOR_A_PIN, GPIO_MODE_OUTPUT);
    pwm_enable(RIGHT_MOTOR_A_PIN);
    gpio_set_direction(RIGHT_MOTOR_A_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RIGHT_MOTOR_B_PIN, 0);
}

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

    initMotors();
    while (true)
    {
        vTaskDelay(1);
    }
}
