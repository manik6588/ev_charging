#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

#include "esp_err.h"

esp_err_t mdns_service_init(const char *hostname, const char *instance);
esp_err_t mdns_service_add_http(uint16_t port);
esp_err_t mdns_service_add_custom(const char *name, const char *type, const char *proto, uint16_t port);
esp_err_t mdns_service_update_ip(void);
void mdns_service_deinit(void);

#endif