#ifndef SENS_H
#define SENS_H

#include <stdint.h>

typedef struct {
    float vin;      // Input DC Voltage
    float iin;      // Input DC Current
    float vout;     // Output AC Voltage (Peak detected)
    float iout;     // Output Current
    float pin;      // Input Power
    float pout;     // Output Power (approx)
} power_data_t;

void sens_init(void);
void sens_update(void);
power_data_t sens_get_data(void);

#endif