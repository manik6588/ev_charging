#include "websocket.hpp"
#include "webserver.hpp"
#include "api_controller.hpp"

extern "C"
{
#include "esp_spiffs.h"
#include "esp_log.h"
}

#include <string.h>
#include <sys/stat.h>

static const char *TAG = "WEB";

/* ================= SPIFFS ================= */

void WebServer::initSPIFFS()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "SPIFFS mounted");
    }
    else if (ret == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "SPIFFS already mounted, skipping");
    }
    else
    {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
    }
}

/* ================= UTIL ================= */

bool WebServer::fileExists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

const char *WebServer::getContentType(const char *path)
{
    if (strstr(path, ".html"))
        return "text/html";
    if (strstr(path, ".js"))
        return "application/javascript";
    if (strstr(path, ".css"))
        return "text/css";
    if (strstr(path, ".json"))
        return "application/json";
    if (strstr(path, ".svg"))
        return "image/svg+xml";
    return "text/plain";
}

/* ================= FILE SEND ================= */

esp_err_t WebServer::sendFile(httpd_req_t *req, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        ESP_LOGW(TAG, "File not found: %s", path);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, getContentType(path));

    if (strstr(path, ".gz"))
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    if (strstr(path, "/assets/"))
        httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=31536000");

    char buffer[1024];
    size_t read_bytes;

    while ((read_bytes = fread(buffer, 1, sizeof(buffer), f)) > 0)
        httpd_resp_send_chunk(req, buffer, read_bytes);

    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* ================= HANDLERS ================= */

esp_err_t WebServer::indexHandler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Route: %s", req->uri);

    if (fileExists("/spiffs/index.html.gz"))
        return sendFile(req, "/spiffs/index.html.gz");

    return sendFile(req, "/spiffs/index.html");
}

esp_err_t WebServer::indexJsHandler(httpd_req_t *req)
{
    if (fileExists("/spiffs/assets/index.js.gz"))
        return sendFile(req, "/spiffs/assets/index.js.gz");

    return sendFile(req, "/spiffs/assets/index.js");
}

esp_err_t WebServer::indexCssHandler(httpd_req_t *req)
{
    if (fileExists("/spiffs/assets/index.css.gz"))
        return sendFile(req, "/spiffs/assets/index.css.gz");

    return sendFile(req, "/spiffs/assets/index.css");
}

esp_err_t WebServer::manifestHandler(httpd_req_t *req)
{
    if (fileExists("/spiffs/manifest.json.gz"))
        return sendFile(req, "/spiffs/manifest.json.gz");

    return sendFile(req, "/spiffs/manifest.json");
}

esp_err_t WebServer::swHandler(httpd_req_t *req)
{
    if (fileExists("/spiffs/sw.js.gz"))
        return sendFile(req, "/spiffs/sw.js.gz");

    return sendFile(req, "/spiffs/sw.js");
}

esp_err_t WebServer::faviconHandler(httpd_req_t *req)
{
    return sendFile(req, "/spiffs/favicon.svg");
}

esp_err_t WebServer::iconsHandler(httpd_req_t *req)
{
    return sendFile(req, "/spiffs/icons.svg");
}

/* ================= SERVER ================= */

void WebServer::init()
{
    // Bind WebSocket → API
    WebSocketServer::onMessage([](const std::string &msg) {
        ApiController::handleWebSocket(msg);
    });
}

static httpd_uri_t make_uri(
    const char *uri,
    httpd_method_t method,
    esp_err_t (*handler)(httpd_req_t *))
{
    httpd_uri_t u = {};
    u.uri = uri;
    u.method = method;
    u.handler = handler;
    u.user_ctx = nullptr;

    // safe defaults for ESP-IDF v5+
    u.is_websocket = false;
    u.handle_ws_control_frames = false;
    u.supported_subprotocol = nullptr;

    return u;
}

httpd_handle_t WebServer::start()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Server start failed");
        return NULL;
    }

    // 🔥 MUST init SPIFFS here
    initSPIFFS();

    /* ================= NORMAL ROUTES ================= */

    httpd_uri_t routes[] = {

        make_uri("/", HTTP_GET, indexHandler),
        make_uri("/history", HTTP_GET, indexHandler),
        make_uri("/settings", HTTP_GET, indexHandler),

        make_uri("/debug", HTTP_GET, indexHandler),
        make_uri("/debug/motor", HTTP_GET, indexHandler),
        make_uri("/debug/station", HTTP_GET, indexHandler),

        make_uri("/assets/index.js", HTTP_GET, indexJsHandler),
        make_uri("/assets/index.css", HTTP_GET, indexCssHandler),

        make_uri("/manifest.json", HTTP_GET, manifestHandler),
        make_uri("/sw.js", HTTP_GET, swHandler),
        make_uri("/favicon.svg", HTTP_GET, faviconHandler),
        make_uri("/icons.svg", HTTP_GET, iconsHandler),
    };

    for (auto &route : routes)
        httpd_register_uri_handler(server, &route);

    /* ================= API (WS + HTTP HYBRID) ================= */

    httpd_uri_t api_uri = {};
    api_uri.uri = "/api";
    api_uri.method = HTTP_GET; // required for WS handshake
    api_uri.handler = WebSocketServer::handler; // unified entry
    api_uri.user_ctx = nullptr;

    api_uri.is_websocket = true;
    api_uri.handle_ws_control_frames = false;
    api_uri.supported_subprotocol = nullptr;

    httpd_register_uri_handler(server, &api_uri);

    /* ================= INIT WEBSOCKET ================= */

    WebSocketServer::init(server);

    ESP_LOGI(TAG, "Webserver started (OOP MODE)");
    return server;
}