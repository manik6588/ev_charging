#ifndef PINS_H
#define PINS_H

/* Motor A (X-Axis) */
#define PIN_AIN1            19
#define PIN_AIN2            4
#define PIN_PWMA            5

/* Motor B (Y-Axis) */
#define PIN_BIN1            17
#define PIN_BIN2            16
#define PIN_PWMB            25 

/* Driver Control */
#define PIN_STBY            18 

/* Limit Switch Pin Definitions */
#define PIN_X_MIN           26
#define PIN_X_MAX           27
#define PIN_Y_MIN           13
#define PIN_Y_MAX           14

/* Bit Positions for state masking */
#define BIT_X_MIN           (1 << 0)
#define BIT_X_MAX           (1 << 1)
#define BIT_Y_MIN           (1 << 2)
#define BIT_Y_MAX           (1 << 3)

/* IR and Sensors */
#define PIN_IR              34
#define PIN_VIN_SENSE       35
#define PIN_IIN_SENSE       32
#define PIN_VOUT_SENSE      36
#define PIN_IOUT_SENSE      39

#endif