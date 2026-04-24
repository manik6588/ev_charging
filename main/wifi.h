#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================= CONFIG ================= */

/**
 * @brief Initialize ESP32 in Access Point (AP) mode
 *
 * Creates WiFi hotspot with default IP (192.168.4.1)
 */
void wifi_init_ap(void);

/**
 * @brief Set custom AP IP address
 *
 * Call AFTER wifi_init_ap()
 *
 * Example:
 * wifi_set_ap_ip("192.168.5.1");
 */
esp_err_t wifi_set_ap_ip(const char *ip, const char *gw, const char *netmask);

/**
 * @brief Get current AP IP as string
 */
const char* wifi_get_ip(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_H