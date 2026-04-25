#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_log.h"

#include "motor.h"
#include "limit.h"
#include "station.h"
#include "sens.h"
#include "api.h"
#include "wifi.h"
#include "webserver.h"
#include "websocket.h"
#include "mdns_service.h"
#include "secrets.h"

#define TAG "MAIN"

/* ================= DASHBOARD CALLBACK ================= */

// Called when frontend sends config
void on_dashboard_config_received(const config_data_t *config)
{
    ESP_LOGI(TAG, "Config -> Freq: %dkHz | DeadTime: %dns",
             config->frequency, config->deadTime);

    // Convert kHz → Hz
    uint32_t freq_hz = config->frequency * 1000;

    station_update_pwm(freq_hz, config->deadTime);
}

/* ================= TELEMETRY TASK ================= */

void telemetry_task(void *arg)
{
    telemetry_data_t state = {0};

    while (1)
    {
        /* -------- SENSOR UPDATE -------- */
        sens_update();
        power_data_t pwr = sens_get_data();

        /* -------- TELEMETRY MAPPING -------- */
        // Using OUTPUT side (more relevant for WPT)
        state.voltage = pwr.vout;
        state.current = pwr.iout;

        /* -------- STATION STATE -------- */
        bool is_active = (station_get_state() == STATION_STATE_RUNNING);

        state.hinActive = is_active;
        state.linActive = is_active;

        float duty = station_get_duty();

        state.hinDuty = duty;
        state.linDuty = duty;

        /* -------- SEND DATA -------- */
        api_send_telemetry(&state);

        vTaskDelay(pdMS_TO_TICKS(100)); // 10 Hz
    }
}

/* ================= LIMIT CONTROL ================= */

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

        /* ===== CROSS CONTROL ===== */

        // X limits → control Y motor
        if (x_max && !prev_x_max)
        {
            ESP_LOGW(TAG, "X_MAX → Y reverse");
            motor_stop_all();
            vTaskDelay(pdMS_TO_TICKS(50));
            motorB_backward(200);
        }

        if (x_min && !prev_x_min)
        {
            ESP_LOGW(TAG, "X_MIN → Y forward");
            motor_stop_all();
            vTaskDelay(pdMS_TO_TICKS(50));
            motorB_forward(200);
        }

        // Y limits → control X motor
        if (y_max && !prev_y_max)
        {
            ESP_LOGW(TAG, "Y_MAX → X reverse");
            motor_stop_all();
            vTaskDelay(pdMS_TO_TICKS(50));
            motorA_backward(200);
        }

        if (y_min && !prev_y_min)
        {
            ESP_LOGW(TAG, "Y_MIN → X forward");
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
    /* -------- NVS -------- */
    ESP_ERROR_CHECK(nvs_flash_init());
    init_spiffs();

    ESP_LOGI(TAG, "Initializing Hardware...");

    /* -------- MODULE INIT -------- */
    motor_init();
    limit_init();
    station_init();
    sens_init();

    /* -------- SAFE STATE -------- */
    motor_stop_all();
    station_stop();

    /* -------- NETWORK -------- */

    wifi_init_ap(); // start AP (creates netif)

    wifi_set_ap_ip(AP_IP, AP_GW, AP_MASK); // change IP here

    mdns_service_init("esp32", "EV Charger");
    mdns_service_add_http(80);

    httpd_handle_t server = start_webserver();

    if (server)
    {
        ESP_LOGI(TAG, "Webserver started");
        api_set_config_callback(on_dashboard_config_received);
        api_init(server);
    }

    websocket_init(server);
    api_init(server);

    /* -------- TASKS -------- */
    xTaskCreatePinnedToCore(
        limit_control_task,
        "limit_task",
        4096,
        NULL,
        5,
        NULL,
        0);

    xTaskCreatePinnedToCore(
        telemetry_task,
        "telemetry_task",
        4096,
        NULL,
        4,
        NULL,
        1);

    ESP_LOGI(TAG, "System Ready 🚀");
}