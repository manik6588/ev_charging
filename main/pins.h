#ifndef PINS_H
#define PINS_H

/* ---------------- MOTOR A (X AXIS) ---------------- */
#define PIN_AIN1            19
#define PIN_AIN2            21
#define PIN_PWMA            22

/* ---------------- MOTOR B (Y AXIS) ---------------- */
#define PIN_BIN1            17
#define PIN_BIN2            16
#define PIN_PWMB            4

/* ---------------- SHARED ---------------- */
#define PIN_STBY            18

/* ---------------- LIMIT SWITCHES ---------------- */
#define PIN_X_MIN           13
#define PIN_X_MAX           33
#define PIN_Y_MIN           32
#define PIN_Y_MAX           14

/* ---------------- STATION (HIN / LIN) ---------------- */
#define PIN_HIN             25
#define PIN_LIN             26
#define PIN_FAULT_INPUT     27

/* ---------------- SENSORS ---------------- */

/* Input Side */
#define PIN_VIN_SENSE       34   // Voltage Divider (DC input)
#define PIN_IIN_SENSE       35   // ACS712 (DC current)

/* Output Side */
#define PIN_VOUT_SENSE      36   // Peak detect (AC voltage)
#define PIN_IOUT_SENSE      39   // ACS758 (current)

#endif