#include "motor.h"
#include "pins.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "MOTOR"

static void motor_wiggle(void)
{
    ESP_LOGI(TAG, "Motor wiggle test");

    // Enable driver
    gpio_set_level(PIN_STBY, 1);

    int speed = 120; // moderate PWM (0–255)

    /* ---- Motor A forward ---- */
    motorA_forward(speed);
    vTaskDelay(pdMS_TO_TICKS(300));

    /* ---- Motor A backward ---- */
    motorA_backward(speed);
    vTaskDelay(pdMS_TO_TICKS(300));

    /* ---- Motor B forward ---- */
    motorB_forward(speed);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* ---- Motor B backward ---- */
    motorB_backward(speed);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* ---- Stop ---- */
    motor_stop_all();

    // Disable again for safety
    gpio_set_level(PIN_STBY, 0);

    ESP_LOGI(TAG, "Motor wiggle done");
}

void motor_init(void)
{
    ESP_LOGI(TAG, "Motor Init");

    int pins[] = {PIN_AIN1, PIN_AIN2, PIN_BIN1, PIN_BIN2, PIN_STBY};

    /* ---- Step 1: Configure all pins ---- */
    for (int i = 0; i < 5; i++)
    {
        gpio_set_direction(pins[i], GPIO_MODE_OUTPUT);
    }

    /* ---- Step 2: Force safe state BEFORE enabling ---- */
    gpio_set_level(PIN_AIN1, 0);
    gpio_set_level(PIN_AIN2, 0);
    gpio_set_level(PIN_BIN1, 0);
    gpio_set_level(PIN_BIN2, 0);

    gpio_set_level(PIN_STBY, 0); // 🔴 KEEP DISABLED FIRST

    /* ---- Step 3: Setup PWM ---- */
    ledc_timer_config_t t = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 10000,
        .duty_resolution = LEDC_TIMER_8_BIT};
    ledc_timer_config(&t);

    ledc_channel_config_t chA = {
        .gpio_num = PIN_PWMA,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0};
    ledc_channel_config(&chA);

    ledc_channel_config_t chB = {
        .gpio_num = PIN_PWMB,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0};
    ledc_channel_config(&chB);

    motor_wiggle();

    /* ---- Step 4: DO NOT enable STBY automatically ---- */
    ESP_LOGI(TAG, "Motor ready (STBY still LOW)");
}

static void drive(int in1, int in2, int ch, int speed, const char *name, bool fwd)
{
    ESP_LOGI(TAG, "%s %s %d", name, fwd ? "FWD" : "REV", speed);

    gpio_set_level(in1, fwd);
    gpio_set_level(in2, !fwd);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ch, speed);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ch);
}

void motorA_forward(int s) { drive(PIN_AIN1, PIN_AIN2, LEDC_CHANNEL_0, s, "A", true); }
void motorA_backward(int s) { drive(PIN_AIN1, PIN_AIN2, LEDC_CHANNEL_0, s, "A", false); }
void motorB_forward(int s) { drive(PIN_BIN1, PIN_BIN2, LEDC_CHANNEL_1, s, "B", true); }
void motorB_backward(int s) { drive(PIN_BIN1, PIN_BIN2, LEDC_CHANNEL_1, s, "B", false); }

void motor_stop_all(void)
{
    ESP_LOGW(TAG, "STOP ALL");

    gpio_set_level(PIN_AIN1, 0);
    gpio_set_level(PIN_AIN2, 0);
    gpio_set_level(PIN_BIN1, 0);
    gpio_set_level(PIN_BIN2, 0);
}
