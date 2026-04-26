#ifndef MOTOR_H
#define MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

// ============================
// Initialization & Power Control
// ============================

/**
 * Initialize motor GPIOs, PWM, and internal state.
 * Does NOT enable motor driver (STBY remains LOW).
 */
void motor_init(void);

/**
 * Enable motor driver (STBY HIGH).
 */
void motor_enable(void);

/**
 * Disable motor driver (STBY LOW).
 */
void motor_disable(void);


// ============================
// Motor Control (Channel A)
// ============================

/**
 * Run Motor A forward with speed (0–100 or PWM scaled).
 */
void motorA_forward(int speed);

/**
 * Run Motor A backward with speed (0–100 or PWM scaled).
 */
void motorA_backward(int speed);


// ============================
// Motor Control (Channel B)
// ============================

void motorB_forward(int speed);
void motorB_backward(int speed);


// ============================
// Stop Control
// ============================

/**
 * Stop both motors (does NOT disable driver).
 */
void motor_stop_all(void);


// ============================
// Optional State Helpers (Recommended)
// ============================

/**
 * Returns true if motor driver is enabled (STBY HIGH).
 */
int motor_is_enabled(void);

/**
 * Set both motors using signed speed:
 * +ve → forward, -ve → backward, 0 → stop
 */
void motor_set(int speedA, int speedB);


#ifdef __cplusplus
}
#endif

#endif // MOTOR_H