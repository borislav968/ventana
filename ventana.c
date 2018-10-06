/*
 * ventana.c
 *
 *  Created on: 27 сент. 2018 г.
 *      Author: elgato
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "motor.h"
#include "switch.h"
#include "defines.h"

#define CMD_UP      (1<<0)
#define CMD_DN      (1<<1)
#define CMD_HOLD    (CMD_UP | CMD_DN)

volatile unsigned int cnt = 0;

void init_sensor () {
    // PD6 and PD7 pins are inputs without pull-up
    PORTD &= ~((1<<PD6) | (1<<PD7));
    DDRD  &= ~((1<<PD6) | (1<<PD7));
    // Enable comparator multiplexer
    SFIOR &= ~(1<<ACME);
    // Enable interrupt on falling edge
    ACSR  |=  (1<<ACIS1) | (1<<ACIE);    
}

// Comparator output falling edge
// Occurs when voltage at AIN1 > AIN0 (motor overload)
ISR (ANA_COMP_vect) {
    state |= ST_EDGE;
    speed = 0;
    motor_stop();
}

int main () {
    init_motor_port();
    init_switch_port();
    init_sensor();
    asm("sei");
    unsigned char input = 0, prev = 0;
    while (1) {
        input = poll_switch();
        if (input != prev) {
            prev = input;
            switch (input) {
                case CMD_UP:
                    if ((state & ST_DIR) && ((state & ST_EDGE) || (state & ST_MOVE))) break;
                    if (state & ST_MOVE) {
                        motor_stop();
                        while (speed) {};
                    }
                    state &= ~(ST_EDGE | ST_HOLD);
                    state |= ST_DIR;
                    motor_start();
                    break;
                case CMD_DN:
                    if ((!(state & ST_DIR)) && ((state & ST_EDGE) || (state & ST_MOVE))) break;
                    if (state & ST_MOVE) {
                        motor_stop();
                        while (speed) {};
                    }
                    state &= ~(ST_EDGE | ST_DIR | ST_HOLD);
                    motor_start();
                    break;
                case CMD_HOLD:
                    if (state & (ST_EDGE | ST_MOVE)) {
                        state |= ST_HOLD;
                    }
                    break;
                case 0:
                    if (!(state & ST_HOLD)) {
                       motor_stop();
                    }
                    break;
                default:
                    state = 0;
                    motor_stop();
            }
        }
    }
}

