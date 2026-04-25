#pragma once

#include "esp_http_server.h"

class WebServer
{
public:
    static void init();
    static void initSPIFFS();
    static httpd_handle_t start();

private:
    static bool fileExists(const char *path);
    static const char *getContentType(const char *path);
    static esp_err_t sendFile(httpd_req_t *req, const char *path);

    // Handlers
    static esp_err_t indexHandler(httpd_req_t *req);
    static esp_err_t indexJsHandler(httpd_req_t *req);
    static esp_err_t indexCssHandler(httpd_req_t *req);
    static esp_err_t manifestHandler(httpd_req_t *req);
    static esp_err_t swHandler(httpd_req_t *req);
    static esp_err_t faviconHandler(httpd_req_t *req);
    static esp_err_t iconsHandler(httpd_req_t *req);
};