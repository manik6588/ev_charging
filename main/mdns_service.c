#include "mdns.h"
#include "esp_log.h"

static const char *TAG = "MDNS";

/* ================= INIT ================= */

esp_err_t mdns_service_init(const char *hostname, const char *instance)
{
    if (!hostname)
        return ESP_ERR_INVALID_ARG;

    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(hostname));

    if (instance)
    {
        ESP_ERROR_CHECK(mdns_instance_name_set(instance));
    }

    ESP_LOGI(TAG, "mDNS → http://%s.local", hostname);
    return ESP_OK;
}

/* ================= SERVICES ================= */

esp_err_t mdns_service_add_http(uint16_t port)
{
    return mdns_service_add(NULL, "_http", "_tcp", port, NULL, 0);
}

esp_err_t mdns_service_add_custom(const char *name, const char *type, const char *proto, uint16_t port)
{
    return mdns_service_add(name, type, proto, port, NULL, 0);
}

/* ================= UPDATE ================= */

esp_err_t mdns_service_update_ip(void)
{
    // Not required in most cases
    // mDNS automatically updates after IP change

    ESP_LOGI("MDNS", "IP update triggered (noop)");
    return ESP_OK;
}

/* ================= DEINIT ================= */

void mdns_service_deinit(void)
{
    mdns_free();
}