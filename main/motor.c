#include "pins.h"
#include "motor.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

void motor_init()
{
    gpio_set_direction(PIN_AIN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_AIN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_BIN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_BIN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_STBY, GPIO_MODE_OUTPUT);

    gpio_set_level(PIN_STBY, 1);

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .duty_resolution = LEDC_TIMER_8_BIT
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t chA = {
        .gpio_num = PIN_PWMA,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0
    };

    ledc_channel_config_t chB = {
        .gpio_num = PIN_PWMB,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0
    };

    ledc_channel_config(&chA);
    ledc_channel_config(&chB);
}

void motorA_forward(int speed)
{
    gpio_set_level(PIN_AIN1, 1);
    gpio_set_level(PIN_AIN2, 0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, speed);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

void motorA_backward(int speed)
{
    gpio_set_level(PIN_AIN1, 0);
    gpio_set_level(PIN_AIN2, 1);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, speed);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

void motorB_forward(int speed)
{
    gpio_set_level(PIN_BIN1, 1);
    gpio_set_level(PIN_BIN2, 0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, speed);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
}

void motorB_backward(int speed)
{
    gpio_set_level(PIN_BIN1, 0);
    gpio_set_level(PIN_BIN2, 1);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, speed);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
}

void motor_stop_all()
{
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);

    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
}