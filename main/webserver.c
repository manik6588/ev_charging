#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <stdio.h>

static const char *TAG = "WEB";

// 1. Initialize the SPIFFS Drive (Call this before starting the webserver)
void init_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "SPIFFS mounted successfully");
}

// 2. The new way to send files (using Chunks to save RAM)
static esp_err_t send_spiffs_file(httpd_req_t *req, const char *filepath, const char *content_type, bool is_gzip)
{
    FILE* fd = fopen(filepath, "r");
    if (fd == NULL) {
        ESP_LOGE(TAG, "Failed to read %s", filepath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, content_type);
    if (is_gzip) {
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    }
    // Note: Add your Cache-Control headers here if needed

    // Send the file in 1KB chunks so we don't crash the ESP32's memory
    char chunk[1024];
    size_t chunksize;
    do {
        chunksize = fread(chunk, 1, sizeof(chunk), fd);
        if (chunksize > 0) {
            httpd_resp_send_chunk(req, chunk, chunksize);
        }
    } while (chunksize != 0);
    
    fclose(fd);

    // Sending an empty chunk tells the browser the file is finished
    httpd_resp_send_chunk(req, NULL, 0); 
    return ESP_OK;
}

// 3. Your Handlers become incredibly simple
// HTML
static esp_err_t index_handler(httpd_req_t *req)
{
    return send_spiffs_file(req, "/spiffs/index.html.gz", "text/html", true);
}

// JS
static esp_err_t index_js_handler(httpd_req_t *req)
{
    return send_spiffs_file(req, "/spiffs/assets/index.js.gz", "application/javascript", true);
}

// CSS
static esp_err_t index_css_handler(httpd_req_t *req)
{
    return send_spiffs_file(req, "/spiffs/assets/index.css.gz", "text/css", true);
}

// Manifest
static esp_err_t manifest_handler(httpd_req_t *req)
{
    return send_spiffs_file(req, "/spiffs/manifest.json.gz", "application/json", true);
}

// Service Worker
static esp_err_t sw_handler(httpd_req_t *req)
{
    return send_spiffs_file(req, "/spiffs/sw.js.gz", "application/javascript", true);
}

// Favicon
static esp_err_t favicon_handler(httpd_req_t *req)
{
    return send_spiffs_file(req, "/spiffs/favicon.svg", "image/svg+xml", false);
}

// Icons
static esp_err_t icons_handler(httpd_req_t *req)
{
    return send_spiffs_file(req, "/spiffs/icons.svg", "image/svg+xml", false);
}

/* ================= ROUTER ================= */

httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 12;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE("WEB", "Failed to start server");
        return NULL;
    }

    httpd_uri_t routes[] = {
        { .uri = "/",                   .method = HTTP_GET, .handler = index_handler },
        { .uri = "/assets/index.js",    .method = HTTP_GET, .handler = index_js_handler },
        { .uri = "/assets/index.css",   .method = HTTP_GET, .handler = index_css_handler },
        { .uri = "/manifest.json",      .method = HTTP_GET, .handler = manifest_handler },
        { .uri = "/sw.js",              .method = HTTP_GET, .handler = sw_handler },
        { .uri = "/favicon.svg",        .method = HTTP_GET, .handler = favicon_handler },
        { .uri = "/icons.svg",          .method = HTTP_GET, .handler = icons_handler },
    };

    for (int i = 0; i < sizeof(routes)/sizeof(routes[0]); i++)
    {
        httpd_register_uri_handler(server, &routes[i]);
    }

    ESP_LOGI("WEB", "Web server started!");
    return server;
}