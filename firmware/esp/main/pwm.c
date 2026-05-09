#include "pwm.h"

#include <esp_log.h>
#include <esp_err.h>
#include <math.h>
#include <esp_intr_alloc.h>
#include <esp_clk_tree.h>

#define TAG "pwm"

#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_CLK_SRC LEDC_USE_XTAL_CLK
#define CLK_SRC_PRECISION ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED

uint8_t last_channel = LEDC_CHANNEL_0; // channel to start from
ledc_timer_bit_t duty_resolution = 0;

void pwm_init(uint32_t freq)
{
    // get chosen clock's frequency
    uint32_t clk_freq = 0;
    ESP_ERROR_CHECK(esp_clk_tree_src_get_freq_hz(LEDC_CLK_SRC, CLK_SRC_PRECISION, &clk_freq));

    duty_resolution = ledc_find_suitable_duty_resolution(clk_freq, freq);

    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .clk_cfg = LEDC_CLK_SRC,
        .freq_hz = freq,
        .duty_resolution = duty_resolution,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_fade_func_install(ESP_INTR_FLAG_LOWMED);

    ESP_LOGI(TAG, "init pwm.\n clk_freq: %lu, freq: %lu, duty_res: %lu\n", clk_freq, freq, duty_resolution);
}

ledc_channel_t pwm_enable(gpio_num_t pin)
{
    // pick a new channel up to the maximum each time init is called
    int channel = (last_channel == LEDC_CHANNEL_MAX - 1) ? last_channel : last_channel++;
    // ESP_LOGI(TAG, "last_channel: %i", last_channel);
    if (channel == LEDC_CHANNEL_MAX - 1)
    {
        ESP_LOGW(TAG, "last channel used up: %i", channel);
    }

    ledc_channel_config_t channel_config = {
        .gpio_num = pin,
        .channel = channel,
        .timer_sel = LEDC_TIMER,
        .duty = 0,   // start at 0%
        .hpoint = 0, // no phase shift
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config)); // configure channel

    ESP_LOGI(TAG, "enable pwm for pin %i on channel %i\n", pin, channel);

    return channel;
}

void pwm_update_duty(ledc_channel_t channel, double duty_percentage)
{
    uint32_t duty;
    if (duty_percentage == 1.0 && duty_resolution > 1)
    {
        duty = (1 << duty_resolution) - 1; // prevent potential overflow at maximum
    }
    else if (duty_percentage == 0.5)
    {
        duty = 1 << (duty_resolution - 1); // set duty to alf
    }
    else if (duty_percentage == 0.0)
    {
        duty = 0;
    }
    else
    {
        duty = (uint32_t)ceil(duty_percentage * (1 << duty_resolution)); // calculate duty from the percentage
    }

    ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_MODE, channel, duty, 0));
    ESP_LOGI(TAG, "updated duty on channel %i: %lu\n", channel, duty);
}
