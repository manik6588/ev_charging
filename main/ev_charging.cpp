#include <stdio.h>
#include "secrets.h"
#include "webserver.hpp"

extern "C"
{
#include "wifi.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "motor.h"
}

#define TAG "MAIN"

class App
{
private:
    static void initNetwork()
    {
        ESP_LOGI(TAG, "Init NVS");

        esp_err_t ret = nvs_flash_init();

        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ESP_ERROR_CHECK(nvs_flash_init());
        }

        ESP_LOGI(TAG, "Init WiFi");

        wifi_init_ap();
        wifi_set_ap_ip(AP_IP, AP_GW, AP_MASK);

        ESP_LOGI(TAG, "Init SPIFFS");

        WebServer::initSPIFFS();

        ESP_LOGI(TAG, "Init WebServer");

        WebServer::init();

        motor_init();
    }

public:
    static void start()
    {
        initNetwork();

        ESP_LOGI(TAG, "Start WebServer");

        WebServer::start();

        ESP_LOGI(TAG, "System Ready 🚀");
    }
};

extern "C" void app_main(void)
{
    ESP_LOGI("MAIN", "App started");

    App::start();
}