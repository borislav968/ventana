/*
 * motor.h
 *
 * Created on: 27 сент. 2018 г.
 *      Author: Borislav Malkov
 */


#ifndef MOTOR_H
#define MOTOR_H

extern volatile unsigned char state;
extern volatile unsigned char speed;
extern volatile unsigned char max_duration;

void init_motor ();
void motor_stop ();
void motor_start ();
void sendstr (char str[]);
void learn ();

#endif
