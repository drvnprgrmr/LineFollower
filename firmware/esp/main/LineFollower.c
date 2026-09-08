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

#define MOTOR_PWM_FREQUENCY 10000

#define ENABLE_PIN GPIO_NUM_0

#define LEFT_MOTOR_A_PIN GPIO_NUM_8
#define LEFT_MOTOR_B_PIN GPIO_NUM_7

#define RIGHT_MOTOR_A_PIN GPIO_NUM_5
#define RIGHT_MOTOR_B_PIN GPIO_NUM_6

#define LEFT_MAX_SPEED 0.97
#define LEFT_MIN_SPEED 0.5 //! the wheels just whine and don't move at anything below this

#define RIGHT_MAX_SPEED 1.0
#define RIGHT_MIN_SPEED 0.5 //! the wheels just whine and don't move at anything below this

uint8_t left_channel = 0;
uint8_t right_channel = 0;

// sensors
#define LEFT_SENSOR_PIN GPIO_NUM_4
#define MID_SENSOR_PIN GPIO_NUM_3
#define RIGHT_SENSOR_PIN GPIO_NUM_10

// #define LEFT_SENSE_MASK BIT2
// #define MID_SENSE_MASK BIT1
// #define RIGHT_SENSE_MASK BIT0

// int sense3 = 0b000;

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
    gpio_set_level(ENABLE_PIN, 0);
}

void updateWheel(Wheel wheel, double speed)
{
    if (wheel == LEFT_WHEEL)
    {
        pwm_update_duty(left_channel, speed);
    }
    else if (wheel == RIGHT_WHEEL)
    {
        pwm_update_duty(right_channel, speed);
    }
}

void testMotors()
{
    uint16_t update_interval = 1000;
    double update_step = 0.1;

    for (double i = 0.0; i <= 1.0; i += update_step)
    {
        updateWheel(LEFT_WHEEL, i);
        vTaskDelay(update_interval / portTICK_PERIOD_MS);
    }
    updateWheel(LEFT_WHEEL, 0.0);

    vTaskDelay(3000 / portTICK_PERIOD_MS);

    for (double i = 0.0; i <= 1.0; i += update_step)
    {
        updateWheel(RIGHT_WHEEL, i);
        vTaskDelay(update_interval / portTICK_PERIOD_MS);
    }
    updateWheel(RIGHT_WHEEL, 0.0);
}

void initMotors()
{
    pwm_init(MOTOR_PWM_FREQUENCY);

    // configure pins
    gpio_set_direction(ENABLE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(ENABLE_PIN, 1);

    gpio_set_direction(LEFT_MOTOR_A_PIN, GPIO_MODE_OUTPUT);
    left_channel = pwm_enable(LEFT_MOTOR_A_PIN);
    gpio_set_direction(LEFT_MOTOR_B_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LEFT_MOTOR_B_PIN, 0);

    gpio_set_direction(RIGHT_MOTOR_A_PIN, GPIO_MODE_OUTPUT);
    right_channel = pwm_enable(RIGHT_MOTOR_A_PIN);
    gpio_set_direction(RIGHT_MOTOR_B_PIN, GPIO_MODE_OUTPUT);
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

void init3()
{
    gpio_set_direction(LEFT_SENSOR_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(MID_SENSOR_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(RIGHT_SENSOR_PIN, GPIO_MODE_INPUT);
}

void line3()
{
    // Logic for following a black line on white surface
    // where white = 1 and black = 0 per sensor
    int left_sense = !gpio_get_level(LEFT_SENSOR_PIN);
    int mid_sense = !gpio_get_level(MID_SENSOR_PIN);
    int right_sense = !gpio_get_level(RIGHT_SENSOR_PIN);

    ESP_LOGI(TAG, "L: %i, M: %i, R: %i\n", left_sense, mid_sense, right_sense);

    if (left_sense && mid_sense && right_sense) // full stop
    {
        updateWheel(LEFT_WHEEL, 0.0);
        updateWheel(RIGHT_WHEEL, 0.0);
    }

    else if (
        (!left_sense && mid_sense && right_sense) ||
        (!left_sense && !mid_sense && right_sense)) // right turn
    {
        updateWheel(LEFT_WHEEL, LEFT_MAX_SPEED);
        updateWheel(RIGHT_WHEEL, 0.0);
    }

    else if (
        (left_sense && mid_sense && !right_sense) ||
        (left_sense && !mid_sense && !right_sense)) // left turn
    {
        updateWheel(LEFT_WHEEL, 0.0);
        updateWheel(RIGHT_WHEEL, RIGHT_MAX_SPEED);
    }
    else // full throttle
    {
        updateWheel(LEFT_WHEEL, LEFT_MAX_SPEED);
        updateWheel(RIGHT_WHEEL, RIGHT_MAX_SPEED);
    }

    // TODO: Add course correction for 000
}

void app_main(void)
{
    gpio_set_direction(GPIO_MODE_OUTPUT, LED_PIN);
    gpio_set_level(LED_PIN, 0);

    initMotors();
    // testMotors();
    init3();

    while (true)
    {
        line3(); // track a line using 3 sensors
        vTaskDelay(1);
    }
}

//
void _app_main(void)
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

    testMotors();

    while (true)
    {
        vTaskDelay(1);
    }
}
