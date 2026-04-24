#include "api.h"
#include "limit.h"
#include "motor.h"
#include "pins.h"
#include "websocket.h"

#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "driver/gpio.h"

static const char *TAG = "API";

/* ================= GLOBALS ================= */

static httpd_handle_t active_server = NULL;
static api_config_cb_t user_config_cb = NULL;

/* ================= WEBSOCKET HANDLER ================= */

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "WebSocket connected");
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {0};
    frame.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_recv_frame(req, &frame, 0);

    if (frame.len)
    {
        char *buf = malloc(frame.len + 1);
        frame.payload = (uint8_t *)buf;
        httpd_ws_recv_frame(req, &frame, frame.len);
        buf[frame.len] = 0;

        ESP_LOGI(TAG, "RX: %s", buf);

        cJSON *json = cJSON_Parse(buf);
        if (json)
        {
            cJSON *cmd = cJSON_GetObjectItem(json, "command");

            /* ================= MOTOR CONTROL ================= */

            if (cJSON_IsString(cmd) && strcmp(cmd->valuestring, "motor") == 0)
            {

                int a = cJSON_GetObjectItem(json, "motorA")->valueint;
                int b = cJSON_GetObjectItem(json, "motorB")->valueint;

                bool standby = cJSON_IsTrue(cJSON_GetObjectItem(json, "standby"));
                bool driver = cJSON_IsTrue(cJSON_GetObjectItem(json, "driver"));

                cJSON *mode_json = cJSON_GetObjectItem(json, "mode");
                const char *mode = (cJSON_IsString(mode_json)) ? mode_json->valuestring : "production";

                ESP_LOGI(TAG, "Mode:%s Driver:%d Standby:%d A:%d B:%d", mode, driver, standby, a, b);

                /* ---- Safety ---- */

                if (!driver || standby)
                {
                    ESP_LOGW(TAG, "Driver disabled or standby → STOP");
                    motor_stop_all();
                    cJSON_Delete(json);
                    free(buf);
                    return ESP_OK;
                }

                /* ---- Debug mode ---- */

                if (strcmp(mode, "debug") == 0)
                {

                    if (x_max_pressed() && a > 0)
                    {
                        ESP_LOGW(TAG, "X MAX → STOP A");
                        a = 0;
                    }
                    if (x_min_pressed() && a < 0)
                    {
                        ESP_LOGW(TAG, "X MIN → STOP A");
                        a = 0;
                    }

                    if (y_max_pressed() && b > 0)
                    {
                        ESP_LOGW(TAG, "Y MAX → STOP B");
                        b = 0;
                    }
                    if (y_min_pressed() && b < 0)
                    {
                        ESP_LOGW(TAG, "Y MIN → STOP B");
                        b = 0;
                    }
                }

                /* ---- Drive ---- */

                if (a > 0)
                    motorA_forward(a * 2.55);
                else if (a < 0)
                    motorA_backward((-a) * 2.55);

                if (b > 0)
                    motorB_forward(b * 2.55);
                else if (b < 0)
                    motorB_backward((-b) * 2.55);

                if (a == 0 && b == 0)
                {
                    motor_stop_all();
                }
            }

            /* ================= CONFIG CALLBACK ================= */

            if (cJSON_IsString(cmd) && strcmp(cmd->valuestring, "setConfig") == 0)
            {

                if (user_config_cb != NULL)
                {
                    ESP_LOGI(TAG, "Calling user config callback");
                    user_config_cb(NULL); // You can extend struct later
                }
            }

            cJSON_Delete(json);
        }

        free(buf);
    }

    return ESP_OK;
}

/* ================= INIT ================= */

esp_err_t api_init(httpd_handle_t server)
{
    if (!server)
        return ESP_FAIL;

    active_server = server;

    websocket_set_message_callback(api_ws_message_handler);

    ESP_LOGI(TAG, "API init → WS registered");
    return ESP_OK;
}

/* ================= CONFIG CALLBACK ================= */

void api_set_config_callback(api_config_cb_t cb)
{
    user_config_cb = cb;
}

static void api_ws_message_handler(const char *msg, size_t len)
{
    ESP_LOGI("API", "WS MSG: %s", msg);

    cJSON *json = cJSON_Parse(msg);
    if (!json)
        return;

    cJSON *cmd = cJSON_GetObjectItem(json, "command");

    if (cJSON_IsString(cmd) && strcmp(cmd->valuestring, "motor") == 0)
    {

        int a = cJSON_GetObjectItem(json, "motorA")->valueint;
        int b = cJSON_GetObjectItem(json, "motorB")->valueint;

        ESP_LOGI("API", "Motor A:%d B:%d", a, b);

        if (a > 0)
            motorA_forward(a * 2.55);
        else if (a < 0)
            motorA_backward((-a) * 2.55);

        if (b > 0)
            motorB_forward(b * 2.55);
        else if (b < 0)
            motorB_backward((-b) * 2.55);

        if (a == 0 && b == 0)
        {
            motor_stop_all();
        }
    }

    cJSON_Delete(json);
}

/* ================= TELEMETRY ================= */

esp_err_t api_send_telemetry(const telemetry_data_t *data)
{
    if (!active_server || !data)
        return ESP_FAIL;

    cJSON *json = cJSON_CreateObject();

    cJSON_AddNumberToObject(json, "v", data->voltage);
    cJSON_AddNumberToObject(json, "i", data->current);
    cJSON_AddNumberToObject(json, "hinDuty", data->hinDuty);
    cJSON_AddNumberToObject(json, "linDuty", data->linDuty);
    cJSON_AddBoolToObject(json, "hinActive", data->hinActive);
    cJSON_AddBoolToObject(json, "linActive", data->linActive);

    /* ---- Active Pins ---- */

    cJSON *pins = cJSON_CreateArray();

    if (gpio_get_level(PIN_AIN1))
        cJSON_AddItemToArray(pins, cJSON_CreateNumber(PIN_AIN1));
    if (gpio_get_level(PIN_AIN2))
        cJSON_AddItemToArray(pins, cJSON_CreateNumber(PIN_AIN2));
    if (gpio_get_level(PIN_BIN1))
        cJSON_AddItemToArray(pins, cJSON_CreateNumber(PIN_BIN1));
    if (gpio_get_level(PIN_BIN2))
        cJSON_AddItemToArray(pins, cJSON_CreateNumber(PIN_BIN2));

    cJSON_AddItemToObject(json, "activePins", pins);

    char *str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    if (!str)
        return ESP_FAIL;

    size_t clients = 4;
    int fds[4];

    if (httpd_get_client_list(active_server, &clients, fds) == ESP_OK)
    {
        for (int i = 0; i < clients; i++)
        {

            if (httpd_ws_get_fd_info(active_server, fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET)
            {

                httpd_ws_frame_t ws_pkt = {
                    .payload = (uint8_t *)str,
                    .len = strlen(str),
                    .type = HTTPD_WS_TYPE_TEXT};

                httpd_ws_send_frame_async(active_server, fds[i], &ws_pkt);
            }
        }
    }

    free(str);
    return ESP_OK;
}