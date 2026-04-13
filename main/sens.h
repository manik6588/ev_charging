#ifndef SENS_H
#define SENS_H

typedef struct {
    float voltage;
    float current;
    float power;
} power_data_t;

void sens_init(void);
void sens_update(void);
power_data_t sens_get_data(void);

#endif