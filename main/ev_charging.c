#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nvs_flash.h"
#include "esp_log.h"

#include "motor.h"
#include "limit.h"
#include "station.h"
#include "wifi.h"
#include "webserver.h"

#define TAG "MAIN"
#define SPEED 200

/* ================= TYPES ================= */

typedef enum {
    CMD_NONE,
    CMD_STOP,
    CMD_FORWARD_X,
    CMD_BACKWARD_X,
    CMD_FORWARD_Y,
    CMD_BACKWARD_Y
} control_cmd_t;

typedef enum {
    SYS_OK,
    SYS_FAULT
} system_state_t;

/* ================= GLOBAL ================= */

static QueueHandle_t control_queue;
static system_state_t system_state = SYS_OK;

/* ================= SAFETY TASK ================= */
// 🔥 Highest priority

void safety_task(void *arg)
{
    while (1)
    {
        if (x_max_pressed() || x_min_pressed() ||
            y_max_pressed() || y_min_pressed())
        {
            // 🚨 Immediate shutdown
            motor_stop_all();
            station_fault_shutdown();

            system_state = SYS_FAULT;

            ESP_LOGE(TAG, "LIMIT TRIGGERED → SYSTEM FAULT");

            // Send STOP command to system
            control_cmd_t cmd = CMD_STOP;
            xQueueOverwrite(control_queue, &cmd);
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // ultra-fast response
    }
}

/* ================= MOTOR TASK ================= */
// ⚡ Motion logic only

void motor_task(void *arg)
{
    control_cmd_t cmd = CMD_NONE;

    while (1)
    {
        if (xQueueReceive(control_queue, &cmd, portMAX_DELAY))
        {
            if (system_state == SYS_FAULT) {
                motor_stop_all();
                continue;
            }

            switch (cmd)
            {
                case CMD_FORWARD_X:
                    motorA_forward(SPEED);
                    break;

                case CMD_BACKWARD_X:
                    motorA_backward(SPEED);
                    break;

                case CMD_FORWARD_Y:
                    motorB_forward(SPEED);
                    break;

                case CMD_BACKWARD_Y:
                    motorB_backward(SPEED);
                    break;

                case CMD_STOP:
                default:
                    motor_stop_all();
                    break;
            }
        }
    }
}

/* ================= STATION TASK ================= */
// 🌐 Low priority (power stage control)

void station_task(void *arg)
{
    while (1)
    {
        if (system_state == SYS_FAULT)
        {
            station_stop();
        }
        else
        {
            // Example logic
            station_set_duty(50);
            station_start();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ================= MAIN ================= */

void app_main(void)
{
    nvs_flash_init();
    init_spiffs();

    motor_init();
    limit_init();
    station_init();

    wifi_init_ap();

    if (start_webserver() != NULL) {
        ESP_LOGI(TAG, "Webserver running");
    }

    /* ================= QUEUE ================= */
    control_queue = xQueueCreate(1, sizeof(control_cmd_t));

    /* ================= TASKS ================= */

    // 🔥 SAFETY (highest priority)
    xTaskCreatePinnedToCore(
        safety_task,
        "safety_task",
        2048,
        NULL,
        10,
        NULL,
        0
    );

    // ⚡ MOTOR CONTROL
    xTaskCreatePinnedToCore(
        motor_task,
        "motor_task",
        4096,
        NULL,
        8,
        NULL,
        0
    );

    // 🌐 STATION / BACKGROUND
    xTaskCreatePinnedToCore(
        station_task,
        "station_task",
        4096,
        NULL,
        3,
        NULL,
        1
    );

    ESP_LOGI(TAG, "System initialized");
}