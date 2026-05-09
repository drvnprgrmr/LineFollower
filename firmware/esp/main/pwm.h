#pragma once

#include <driver/ledc.h>
#include <driver/gpio.h>

void pwm_init(uint32_t freq);
ledc_channel_t pwm_enable(gpio_num_t pin);
void update_pwm_duty(ledc_channel_t channel, double duty_percentage);
