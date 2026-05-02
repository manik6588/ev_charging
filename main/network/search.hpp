#ifndef SEARCH_H
#define SEARCH_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t search_init(void);
void search_start(void);
void search_stop(void);
bool search_is_running(void);

#ifdef __cplusplus
}
#endif

#endif