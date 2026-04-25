#ifndef MOTOR_H
#define MOTOR_H

void motor_init();

void motorA_forward(int speed);
void motorA_backward(int speed);

void motorB_forward(int speed);
void motorB_backward(int speed);

void motor_stop_all();

#endif