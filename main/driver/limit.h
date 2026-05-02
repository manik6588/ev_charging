#ifndef LIMIT_H
#define LIMIT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void limit_init(void);
uint8_t limit_get_state(void);

/* Specific pin checks used by API Controller */
bool x_min_pressed(void);
bool x_max_pressed(void);
bool y_min_pressed(void);
bool y_max_pressed(void);

#ifdef __cplusplus
}
#endif

#endif // LIMIT_H