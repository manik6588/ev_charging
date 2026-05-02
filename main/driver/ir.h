#ifndef IR_H
#define IR_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t ir_init(void);
esp_err_t ir_read(int *value);
int ir_read_filtered(void);
bool ir_detected(void);

#endif