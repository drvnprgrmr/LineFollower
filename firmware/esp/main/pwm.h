#pragma once

#include <driver/ledc.h>
#include <driver/gpio.h>

ledc_channel_t pwm_init(gpio_num_t pin);
void update_pwm_duty(ledc_channel_t channel, double duty_percentage);
void update_pwm_freq(uint32_t freq);