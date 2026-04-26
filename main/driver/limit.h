#ifndef LIMIT_H
#define LIMIT_H

#include <stdint.h>

void limit_init(void);

// Raw read (no debounce)
int limit_read_raw(uint8_t pin);

// Debounced read (recommended)
int x_min_pressed(void);
int x_max_pressed(void);
int y_min_pressed(void);
int y_max_pressed(void);

// Optional: combined state
uint8_t limit_get_state(void);

#endif