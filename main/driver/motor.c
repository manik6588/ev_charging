#include "motor.h"
#include "pins.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

/* Essential FreeRTOS headers for vTaskDelay and pdMS_TO_TICKS */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "MOTOR"

void motor_wiggle(void) {
    ESP_LOGI(TAG, "Starting motor wiggle test...");

    // 1. Enable driver (STBY HIGH)
    gpio_set_level(PIN_STBY, 1);

    int test_speed = 150; 

    // 2. Test X-Axis (Motor A)
    ESP_LOGI(TAG, "Testing X-Axis...");
    motorX_forward(test_speed);
    vTaskDelay(pdMS_TO_TICKS(300));
    motorX_backward(test_speed);
    vTaskDelay(pdMS_TO_TICKS(300));
    motor_stop_all();
    vTaskDelay(pdMS_TO_TICKS(200));

    // 3. Test Y-Axis (Motor B)
    ESP_LOGI(TAG, "Testing Y-Axis...");
    motorY_forward(test_speed);
    vTaskDelay(pdMS_TO_TICKS(300));
    motorY_backward(test_speed);
    vTaskDelay(pdMS_TO_TICKS(300));
    motor_stop_all();

    // 4. Return to safe state (STBY LOW)
    gpio_set_level(PIN_STBY, 0);
    ESP_LOGI(TAG, "Motor wiggle test complete.");
}

void motor_init(void) {
    ESP_LOGI(TAG, "Motor Init");

    int pins[] = {PIN_AIN1, PIN_AIN2, PIN_BIN1, PIN_BIN2, PIN_STBY};

    // Step 1: Configure all pins
    for (int i = 0; i < 5; i++) {
        gpio_reset_pin(pins[i]);
        gpio_set_direction(pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(pins[i], 0);
    }

    // Step 2: Setup PWM
    gpio_reset_pin(PIN_PWMA);
    gpio_reset_pin(PIN_PWMB);

    ledc_timer_config_t t = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 10000,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&t);

    ledc_channel_config_t chs[2] = {
        {.gpio_num = PIN_PWMA, .channel = LEDC_CHANNEL_0, .speed_mode = LEDC_HIGH_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .duty = 0},
        {.gpio_num = PIN_PWMB, .channel = LEDC_CHANNEL_1, .speed_mode = LEDC_HIGH_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .duty = 0}
    };
    for(int i = 0; i < 2; i++) {
        ledc_channel_config(&chs[i]);
    }

    // Step 3: Run the wiggle test to verify hardware
    motor_wiggle(); 

    ESP_LOGI(TAG, "Motor ready (STBY still LOW)");
}

static void drive(int in1, int in2, int ch, int speed, bool fwd) {
    gpio_set_level(in1, fwd);
    gpio_set_level(in2, !fwd);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ch, speed);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ch);
}

void motorX_forward(int s)  { drive(PIN_AIN1, PIN_AIN2, LEDC_CHANNEL_0, s, true); }
void motorX_backward(int s) { drive(PIN_AIN1, PIN_AIN2, LEDC_CHANNEL_0, s, false); }
void motorY_forward(int s)  { drive(PIN_BIN1, PIN_BIN2, LEDC_CHANNEL_1, s, true); }
void motorY_backward(int s) { drive(PIN_BIN1, PIN_BIN2, LEDC_CHANNEL_1, s, false); }

void motor_stop_all(void) {
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    gpio_set_level(PIN_AIN1, 0);
    gpio_set_level(PIN_AIN2, 0);
    gpio_set_level(PIN_BIN1, 0);
    gpio_set_level(PIN_BIN2, 0);
}

void motor_enable(void)  { gpio_set_level(PIN_STBY, 1); }
void motor_disable(void) { gpio_set_level(PIN_STBY, 0); }