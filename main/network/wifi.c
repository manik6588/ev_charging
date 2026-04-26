#include "wifi.h"
#include "secrets.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "lwip/inet.h"

void wifi_init_ap()
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASS,
            .max_connection = 2,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK}};

    if (strlen(AP_PASS) == 0)
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI("WIFI", "AP Started: %s", AP_SSID);
    ESP_LOGI("WIFI", "IP: %s", AP_IP);
}

esp_err_t wifi_set_ap_ip(const char *ip, const char *gw, const char *netmask)
{
    if (!ip || !gw || !netmask) return ESP_ERR_INVALID_ARG;

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    if (!netif) {
        ESP_LOGE("NET", "Failed to get AP netif");
        return ESP_FAIL;
    }

    esp_netif_ip_info_t ip_info;

    ip_info.ip.addr = inet_addr(ip);
    ip_info.gw.addr = inet_addr(gw);
    ip_info.netmask.addr = inet_addr(netmask);

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(netif));

    ESP_LOGI("NET", "AP IP set → %s", ip);

    return ESP_OK;
}