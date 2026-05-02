#ifndef MOTOR_H
#define MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

void motor_init(void);
void motor_enable(void);
void motor_disable(void);
void motor_stop_all(void);
void motor_wiggle(void); // Add this line

/* X-Axis (Motor A) */
void motorX_forward(int speed);
void motorX_backward(int speed);

/* Y-Axis (Motor B) */
void motorY_forward(int speed);
void motorY_backward(int speed);

#ifdef __cplusplus
}
#endif

#endif