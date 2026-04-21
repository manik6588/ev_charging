#include "pins.h"
#include "motor.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#define PWM_MAX 255

static inline uint32_t clamp_speed(int speed)
{
    if (speed < 0) return 0;
    if (speed > PWM_MAX) return PWM_MAX;
    return speed;
}

void motor_init()
{
    gpio_set_direction(PIN_AIN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_AIN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_BIN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_BIN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_STBY, GPIO_MODE_OUTPUT);

    // Safe default state
    gpio_set_level(PIN_AIN1, 0);
    gpio_set_level(PIN_AIN2, 0);
    gpio_set_level(PIN_BIN1, 0);
    gpio_set_level(PIN_BIN2, 0);

    gpio_set_level(PIN_STBY, 1);

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .clk_cfg = LEDC_AUTO_CLK
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

/* ================= MOTOR A ================= */

void motorA_forward(int speed)
{
    gpio_set_level(PIN_AIN1, 1);
    gpio_set_level(PIN_AIN2, 0);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, clamp_speed(speed));
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

void motorA_backward(int speed)
{
    gpio_set_level(PIN_AIN1, 0);
    gpio_set_level(PIN_AIN2, 1);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, clamp_speed(speed));
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

/* ================= MOTOR B ================= */

void motorB_forward(int speed)
{
    gpio_set_level(PIN_BIN1, 1);
    gpio_set_level(PIN_BIN2, 0);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, clamp_speed(speed));
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
}

void motorB_backward(int speed)
{
    gpio_set_level(PIN_BIN1, 0);
    gpio_set_level(PIN_BIN2, 1);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, clamp_speed(speed));
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
}

/* ================= STOP ================= */

void motor_stop_all()
{
    // Active brake (strong stop)
    gpio_set_level(PIN_AIN1, 0);
    gpio_set_level(PIN_AIN2, 0);
    gpio_set_level(PIN_BIN1, 0);
    gpio_set_level(PIN_BIN2, 0);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);

    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
}