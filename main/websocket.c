#include "websocket.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "WS";

/* ================= GLOBALS ================= */

static httpd_handle_t ws_server = NULL;
static ws_message_cb_t user_cb = NULL;

/* ================= HANDLER ================= */

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Client connected");
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {0};
    frame.type = HTTPD_WS_TYPE_TEXT;

    // Get length
    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Recv len failed");
        return ret;
    }

    if (frame.len > 0) {
        char *buf = malloc(frame.len + 1);
        if (!buf) return ESP_ERR_NO_MEM;

        frame.payload = (uint8_t *)buf;
        httpd_ws_recv_frame(req, &frame, frame.len);
        buf[frame.len] = 0;

        ESP_LOGI(TAG, "RX: %s", buf);

        // Forward to user callback (api.c)
        if (user_cb) {
            user_cb(buf, frame.len);
        }

        free(buf);
    }

    return ESP_OK;
}

/* ================= URI ================= */

static const httpd_uri_t ws_uri = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = ws_handler,
    .user_ctx = NULL,
    .is_websocket = true
};

/* ================= INIT ================= */

esp_err_t websocket_init(httpd_handle_t server)
{
    if (!server) return ESP_FAIL;

    ws_server = server;

    ESP_LOGI(TAG, "Registering WS endpoint");
    return httpd_register_uri_handler(server, &ws_uri);
}

/* ================= CALLBACK ================= */

void websocket_set_message_callback(ws_message_cb_t cb)
{
    user_cb = cb;
}

/* ================= SEND ================= */

esp_err_t websocket_send(int sock, const char *data)
{
    if (!ws_server || !data) return ESP_FAIL;

    httpd_ws_frame_t frame = {
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)data,
        .len = strlen(data)
    };

    return httpd_ws_send_frame_async(ws_server, sock, &frame);
}

/* ================= BROADCAST ================= */

esp_err_t websocket_broadcast(const char *data)
{
    if (!ws_server || !data) return ESP_FAIL;

    size_t max_clients = 8;
    int client_fds[8];

    if (httpd_get_client_list(ws_server, &max_clients, client_fds) != ESP_OK) {
        return ESP_FAIL;
    }

    for (int i = 0; i < max_clients; i++) {

        if (httpd_ws_get_fd_info(ws_server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {

            websocket_send(client_fds[i], data);
        }
    }

    return ESP_OK;
}

/* ================= CLIENT COUNT ================= */

size_t websocket_get_client_count(void)
{
    if (!ws_server) return 0;

    size_t count = 8;
    int client_fds[8];

    if (httpd_get_client_list(ws_server, &count, client_fds) != ESP_OK) {
        return 0;
    }

    size_t ws_count = 0;

    for (int i = 0; i < count; i++) {
        if (httpd_ws_get_fd_info(ws_server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
            ws_count++;
        }
    }

    return ws_count;
}