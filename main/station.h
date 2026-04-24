#ifndef STATION_H
#define STATION_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STATION_STATE_OFF,
    STATION_STATE_RUNNING,
    STATION_STATE_FAULT
} station_state_t;

void station_init(void);
void station_start(void);
void station_stop(void);
bool station_set_duty(float duty);
float station_get_duty(void); // <--- Added
void station_update_pwm(uint32_t freq_hz, uint32_t dead_time_ns); // <--- Added

station_state_t station_get_state(void);
void station_fault_shutdown(void);
void station_clear_fault(void);

#endif