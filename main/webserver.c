#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "WEB";

/* ================= SPIFFS ================= */

void init_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
    ESP_LOGI(TAG, "SPIFFS mounted");
}

/* ================= UTIL ================= */

static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static const char* get_content_type(const char *path)
{
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".js"))   return "application/javascript";
    if (strstr(path, ".css"))  return "text/css";
    if (strstr(path, ".json")) return "application/json";
    if (strstr(path, ".svg"))  return "image/svg+xml";
    return "text/plain";
}

/* ================= FILE SEND ================= */

static esp_err_t send_file(httpd_req_t *req, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGW(TAG, "File not found: %s", path);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, get_content_type(path));

    if (strstr(path, ".gz")) {
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    }

    if (strstr(path, "/assets/")) {
        httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=31536000");
    }

    char buffer[1024];
    size_t read_bytes;

    while ((read_bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }

    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* ================= HANDLERS ================= */

// Serve index.html for ALL React pages
static esp_err_t index_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Route: %s", req->uri);

    if (file_exists("/spiffs/index.html.gz"))
        return send_file(req, "/spiffs/index.html.gz");

    return send_file(req, "/spiffs/index.html");
}

/* ---- ASSETS ---- */

static esp_err_t index_js_handler(httpd_req_t *req)
{
    if (file_exists("/spiffs/assets/index.js.gz"))
        return send_file(req, "/spiffs/assets/index.js.gz");

    return send_file(req, "/spiffs/assets/index.js");
}

static esp_err_t index_css_handler(httpd_req_t *req)
{
    if (file_exists("/spiffs/assets/index.css.gz"))
        return send_file(req, "/spiffs/assets/index.css.gz");

    return send_file(req, "/spiffs/assets/index.css");
}

static esp_err_t manifest_handler(httpd_req_t *req)
{
    if (file_exists("/spiffs/manifest.json.gz"))
        return send_file(req, "/spiffs/manifest.json.gz");

    return send_file(req, "/spiffs/manifest.json");
}

static esp_err_t sw_handler(httpd_req_t *req)
{
    if (file_exists("/spiffs/sw.js.gz"))
        return send_file(req, "/spiffs/sw.js.gz");

    return send_file(req, "/spiffs/sw.js");
}

static esp_err_t favicon_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/favicon.svg");
}

static esp_err_t icons_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/icons.svg");
}

/* ================= SERVER ================= */

httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Server start failed");
        return NULL;
    }

    /* ---- ALL ROUTES MANUAL ---- */

    httpd_uri_t routes[] = {

        // Root
        { .uri = "/", .method = HTTP_GET, .handler = index_handler },

        // Customer routes
        { .uri = "/history", .method = HTTP_GET, .handler = index_handler },
        { .uri = "/settings", .method = HTTP_GET, .handler = index_handler },

        // Debug routes
        { .uri = "/debug", .method = HTTP_GET, .handler = index_handler },
        { .uri = "/debug/motor", .method = HTTP_GET, .handler = index_handler },
        { .uri = "/debug/station", .method = HTTP_GET, .handler = index_handler },

        // Assets
        { .uri = "/assets/index.js", .method = HTTP_GET, .handler = index_js_handler },
        { .uri = "/assets/index.css", .method = HTTP_GET, .handler = index_css_handler },

        // Other static files
        { .uri = "/manifest.json", .method = HTTP_GET, .handler = manifest_handler },
        { .uri = "/sw.js", .method = HTTP_GET, .handler = sw_handler },
        { .uri = "/favicon.svg", .method = HTTP_GET, .handler = favicon_handler },
        { .uri = "/icons.svg", .method = HTTP_GET, .handler = icons_handler },
    };

    for (int i = 0; i < sizeof(routes)/sizeof(routes[0]); i++) {
        httpd_register_uri_handler(server, &routes[i]);
    }

    ESP_LOGI(TAG, "Webserver started (FULL MANUAL MODE)");
    return server;
}