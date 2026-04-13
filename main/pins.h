#ifndef PINS_H
#define PINS_H

/* ---------------- MOTOR A (X AXIS) ---------------- */
#define PIN_AIN1   26
#define PIN_AIN2   27
#define PIN_PWMA   25

/* ---------------- MOTOR B (Y AXIS) ---------------- */
#define PIN_BIN1   16   // changed from 12 (boot-safe)
#define PIN_BIN2   17
#define PIN_PWMB   14

/* ---------------- SHARED ---------------- */
#define PIN_STBY   33

/* ---------------- LIMIT SWITCHES ---------------- */
/* ⚠️ Use pins with internal pull-up support */
#define PIN_X_MIN  32
#define PIN_X_MAX  33   // avoid 34–39 if using internal PU
#define PIN_Y_MIN  18
#define PIN_Y_MAX  19

/* ---------------- STATION (HIN / LIN) ---------------- */
#define PIN_HIN    21
#define PIN_LIN    22

#endif