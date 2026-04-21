#ifndef PINS_H
#define PINS_H

/* ---------------- MOTOR A (X AXIS) ---------------- */
#define PIN_AIN1   19
#define PIN_AIN2   21
#define PIN_PWMA   22

/* ---------------- MOTOR B (Y AXIS) ---------------- */
#define PIN_BIN1   17   // changed from 12 (boot-safe)
#define PIN_BIN2   16
#define PIN_PWMB   4

/* ---------------- SHARED ---------------- */
#define PIN_STBY   18

/* ---------------- LIMIT SWITCHES ---------------- */
/* ⚠️ Use pins with internal pull-up support */
#define PIN_X_MIN  34
#define PIN_X_MAX  33   // avoid 34–39 if using internal PU
#define PIN_Y_MIN  32
#define PIN_Y_MAX  35

/* ---------------- STATION (HIN / LIN) ---------------- */
#define PIN_HIN    25
#define PIN_LIN    26

#endif