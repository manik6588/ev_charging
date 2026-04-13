#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SPIFFS filesystem.
 * Call this in app_main() before starting the webserver.
 */
void init_spiffs(void);

/**
 * @brief Start the HTTP webserver and register all URI handlers.
 * @return httpd_handle_t Instance of the server, or NULL if it failed.
 */
httpd_handle_t start_webserver(void);

#ifdef __cplusplus
}
#endif

#endif /* WEBSERVER_H */