#include "websocket.hpp"

extern "C" {
#include "esp_log.h"
}

#include <cstring>
#include <cstdlib>

static const char *TAG = "WS";

/* ================= STATIC MEMBERS ================= */

httpd_handle_t WebSocketServer::server = nullptr;
WebSocketServer::MessageCallback WebSocketServer::messageCallback = nullptr;

/* ================= HANDLER ================= */

esp_err_t WebSocketServer::handler(httpd_req_t *req)
{
    // 🔥 WebSocket handshake
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Client connected");
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {};
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.final = true;
    frame.fragmented = false;

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Recv len failed");
        return ret;
    }

    if (frame.len > 0) {
        char *buf = (char*) malloc(frame.len + 1);
        if (!buf) return ESP_ERR_NO_MEM;

        frame.payload = (uint8_t *)buf;

        httpd_ws_recv_frame(req, &frame, frame.len);
        buf[frame.len] = 0;

        ESP_LOGI(TAG, "RX: %s", buf);

        if (messageCallback) {
            messageCallback(std::string(buf));
        }

        free(buf);
    }

    return ESP_OK;
}

/* ================= INIT ================= */

esp_err_t WebSocketServer::init(httpd_handle_t srv)
{
    if (!srv) return ESP_FAIL;

    server = srv;

    httpd_uri_t ws_uri = {};   // 🔥 FIX

    ws_uri.uri = "/ws";
    ws_uri.method = HTTP_GET;
    ws_uri.handler = handler;
    ws_uri.user_ctx = nullptr;

    ws_uri.is_websocket = true;
    ws_uri.handle_ws_control_frames = false;
    ws_uri.supported_subprotocol = nullptr;

    ESP_LOGI(TAG, "Registering WS endpoint");
    return httpd_register_uri_handler(server, &ws_uri);
}

/* ================= CALLBACK ================= */

void WebSocketServer::onMessage(MessageCallback cb)
{
    messageCallback = cb;
}

/* ================= SEND ================= */

esp_err_t WebSocketServer::send(int sock, const std::string &data)
{
    if (!server) return ESP_FAIL;

    httpd_ws_frame_t frame = {};   // 🔥 FIX

    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.payload = (uint8_t*)data.c_str();
    frame.len = data.length();
    frame.final = true;
    frame.fragmented = false;

    return httpd_ws_send_frame_async(server, sock, &frame);
}

/* ================= BROADCAST ================= */

esp_err_t WebSocketServer::broadcast(const std::string &data)
{
    if (!server) return ESP_FAIL;

    size_t max_clients = 8;
    int client_fds[8];

    if (httpd_get_client_list(server, &max_clients, client_fds) != ESP_OK)
        return ESP_FAIL;

    for (int i = 0; i < max_clients; i++)
    {
        if (httpd_ws_get_fd_info(server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET)
        {
            send(client_fds[i], data);
        }
    }

    return ESP_OK;
}

/* ================= CLIENT COUNT ================= */

size_t WebSocketServer::getClientCount()
{
    if (!server) return 0;

    size_t count = 8;
    int client_fds[8];

    if (httpd_get_client_list(server, &count, client_fds) != ESP_OK)
        return 0;

    size_t ws_count = 0;

    for (int i = 0; i < count; i++)
    {
        if (httpd_ws_get_fd_info(server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET)
        {
            ws_count++;
        }
    }

    return ws_count;
}