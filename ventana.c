/*
 * ventana.c
 *
 * Created on: 27 сент. 2018 г.
 *      Author: Borislav Malkov
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "motor.h"
#include "switch.h"
#include "defines.h"
#include "fuses.h"


volatile unsigned int cnt = 0;

int main () {
    init_switch_port();
    // Disable analog comparator for energy saving
    ACSR |= (1<<ACD);
    asm("sei");
    unsigned char input = 0, prev = 0;
    while (1) {
        input = poll_switch();
        if (input != prev) {
            prev = input;
            switch (input) {
                case CMD_UP:
                    if (state & ST_DIR) {           // if direction is the same in state
                        if (state & ST_EDGE) break; // do nothing if it already has reached the edge
                        if (state & ST_MOVE) {      // restart motor if it was in spindown state
                            motor_start();
                            break;
                        }
                    }
                    if (state & ST_MOVE) {          // if direction was other in state
                        motor_stop();               // spin the motor down
                        while (speed) {};           // wait for it to stop
                    }
                    state &= ~(ST_EDGE | ST_HOLD);  // clear edge and hold flags
                    state |= ST_DIR;                // start motor in needed direction
                    motor_start();
                    break;
                case CMD_DN:
                    if (!(state & ST_DIR)) {           // if direction is the same in state
                        if (state & ST_EDGE) break; // do nothing if it already has reached the edge
                        if (state & ST_MOVE) {      // restart motor if it was in spindown state
                            motor_start();
                            break;
                        }
                    }
                    if (state & ST_MOVE) {          // if direction was other in state
                        motor_stop();               // spin the motor down
                        while (speed) {};           // wait for it to stop
                    }
                    state &= ~(ST_EDGE | ST_HOLD);  // clear edge and hold flags
                    state &= ~ST_DIR;               // start motor in needed direction
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
                        while (speed) {};
                    }
                    break;
                default:
                    state = 0;
                    motor_stop();
            }
        }
    }
}

