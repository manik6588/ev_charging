#ifndef API_H
#define API_H

#include "esp_http_server.h"
#include "esp_err.h"

typedef struct
{
    int frequency;
    int deadTime;
} config_data_t;

typedef struct
{
    float voltage;
    float current;
    float hinDuty;
    float linDuty;
    bool hinActive;
    bool linActive;
} telemetry_data_t;

typedef void (*api_config_cb_t)(const config_data_t *config);

// Updated to match implementation
esp_err_t api_init(httpd_handle_t web_server);
void api_set_config_callback(api_config_cb_t cb);
esp_err_t api_send_telemetry(const telemetry_data_t *data);
static void api_ws_message_handler(const char *msg, size_t len);
#endif