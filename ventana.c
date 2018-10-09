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

int main () {
    init_motor();
    init_switch_port();
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

