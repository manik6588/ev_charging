#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "esp_http_server.h"
#include "esp_err.h"

/* ================= TYPES ================= */

// Message callback (incoming WS messages)
typedef void (*ws_message_cb_t)(const char *msg, size_t len);

/* ================= API ================= */

// Initialize WebSocket on existing server
esp_err_t websocket_init(httpd_handle_t server);

// Register message handler (used by api.c)
void websocket_set_message_callback(ws_message_cb_t cb);

// Send message to all connected clients
esp_err_t websocket_broadcast(const char *data);

// Send message to specific client (optional)
esp_err_t websocket_send(int sock, const char *data);

// Get active client count
size_t websocket_get_client_count(void);

#endif