#include "pwm.h"

#include <esp_err.h>
#include <math.h>
#include <esp_intr_alloc.h>

#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_CLK_SRC LEDC_USE_XTAL_CLK
#define LEDC_DUTY_RES LEDC_TIMER_3_BIT // duty resolution
#define LEDC_FREQ (10 * 1000U)

uint8_t last_channel = LEDC_CHANNEL_0;


ledc_channel_t pwm_init(gpio_num_t pin)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .clk_cfg = LEDC_CLK_SRC,
        .freq_hz = LEDC_FREQ,
        .duty_resolution = LEDC_DUTY_RES,
    };

    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    // pick a new channel up to the maximum each time init is called
    int channel = last_channel ? (last_channel == LEDC_CHANNEL_MAX - 1) : last_channel++;
    
    ledc_channel_config_t channel_config = {
        .gpio_num = pin,
        .channel = channel,
        .timer_sel = LEDC_TIMER,
        .duty = 0,   // start at 0%
        .hpoint = 0, // no phase shift
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config)); // configure channel
    ledc_fade_func_install(ESP_INTR_FLAG_LOWMED);

    return channel;
}

void update_pwm_duty(ledc_channel_t channel, double duty_percentage)
{
    uint32_t duty;
    if (duty_percentage == 0.5)
    {
        duty = 1 << (LEDC_DUTY_RES - 1);
    }
    else if (duty_percentage == 0.0)
    {
        duty = 0;
    }
    else
    {

        duty = (uint32_t)ceil(duty_percentage * (1 << LEDC_DUTY_RES)); // calculate duty from the percentage
    }

    ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_MODE, channel, duty, 0));
}

void update_pwm_freq(uint32_t freq)
{
    ESP_ERROR_CHECK(ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq));
}