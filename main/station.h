#ifndef STATION_H
#define STATION_H

#include <stdbool.h>

typedef enum {
    STATION_STATE_OFF = 0,
    STATION_STATE_RUNNING,
    STATION_STATE_FAULT
} station_state_t;

void station_init(void);

/**
 * Set PWM duty (0–100%)
 * Returns false if invalid or fault state
 */
bool station_set_duty(float duty);

/**
 * Enable output (start PWM safely)
 */
void station_start(void);

/**
 * Stop PWM (both outputs LOW)
 */
void station_stop(void);

/**
 * Emergency shutdown (latch fault)
 */
void station_fault_shutdown(void);

/**
 * Clear fault and allow restart
 */
void station_clear_fault(void);

/**
 * Get current state
 */
station_state_t station_get_state(void);

#endif