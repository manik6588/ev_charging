#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_log.h"

#include "motor.h"
#include "limit.h"
#include "station.h"
#include "wifi.h"
#include "webserver.h"

#define TAG "MAIN"
#define SPEED 200

/* ================= DIRECTION STATE ================= */

typedef enum {
    DIR_STOP,
    DIR_FORWARD,
    DIR_BACKWARD
} motor_dir_t;

static motor_dir_t x_dir = DIR_FORWARD;
static motor_dir_t y_dir = DIR_FORWARD;

/* ================= LIMIT TASK ================= */

void limit_control_task(void *arg)
{
    bool prev_x_min = false, prev_x_max = false;
    bool prev_y_min = false, prev_y_max = false;

    while (1)
    {
        bool x_min = x_min_pressed();
        bool x_max = x_max_pressed();
        bool y_min = y_min_pressed();
        bool y_max = y_max_pressed();

        /* ===== SWAPPED CONTROL ===== */

        // X limits control Y motor
        if (x_max && !prev_x_max)
        {
            ESP_LOGW(TAG, "X_MAX → control Y reverse");
            motor_stop_all();
            vTaskDelay(pdMS_TO_TICKS(50));

            motorB_backward(200);
        }

        if (x_min && !prev_x_min)
        {
            ESP_LOGW(TAG, "X_MIN → control Y forward");
            motor_stop_all();
            vTaskDelay(pdMS_TO_TICKS(50));

            motorB_forward(200);
        }

        // Y limits control X motor
        if (y_max && !prev_y_max)
        {
            ESP_LOGW(TAG, "Y_MAX → control X reverse");
            motor_stop_all();
            vTaskDelay(pdMS_TO_TICKS(50));

            motorA_backward(200);
        }

        if (y_min && !prev_y_min)
        {
            ESP_LOGW(TAG, "Y_MIN → control X forward");
            motor_stop_all();
            vTaskDelay(pdMS_TO_TICKS(50));

            motorA_forward(200);
        }

        prev_x_min = x_min;
        prev_x_max = x_max;
        prev_y_min = y_min;
        prev_y_max = y_max;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ================= MAIN ================= */

void app_main(void)
{
    nvs_flash_init();
    init_spiffs();

    ESP_LOGI(TAG, "Initializing...");

    motor_init();
    limit_init();
    station_init();

    /* SAFE STATE */
    motor_stop_all();
    station_stop();

    /* ENABLE STATION */
    station_start();

    /* START INITIAL MOVEMENT */
    motorA_forward(SPEED);
    motorB_forward(SPEED);

    wifi_init_ap();

    if (start_webserver() != NULL) {
        ESP_LOGI(TAG, "Webserver running");
    }

    /* TASK */
    xTaskCreatePinnedToCore(
        limit_control_task,
        "limit_control",
        4096,
        NULL,
        5,
        NULL,
        0
    );

    ESP_LOGI(TAG, "Auto XY movement started");
}