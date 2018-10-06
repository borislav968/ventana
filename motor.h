#ifndef MOTOR_H
#define MOTOR_H

extern volatile unsigned char state;
extern volatile unsigned char speed;

void init_motor_port ();
void motor_stop ();
void motor_start ();

#endif
