#pragma once

#include "esp_http_server.h"
#include <functional>
#include <string>

class WebSocketServer
{
public:
    using MessageCallback = std::function<void(const std::string&)>;

    static esp_err_t init(httpd_handle_t server);
    static void onMessage(MessageCallback cb);

    static esp_err_t send(int sock, const std::string &data);
    static esp_err_t broadcast(const std::string &data);
    static size_t getClientCount();

    static esp_err_t handler(httpd_req_t *req);   // 🔥 MOVE HERE

private:
    static httpd_handle_t server;
    static MessageCallback messageCallback;
};