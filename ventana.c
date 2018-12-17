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

// Move the window up
void moveup () {
    if (state & ST_DIR) {           // if direction is the same in state
        if (state & ST_EDGE) return; // do nothing if it already has reached the edge
        if (state & ST_MOVE) {      // restart motor if it was in spindown state
            motor_start();
            return;
        }
    }
    if (state & ST_MOVE) {          // if direction was other in state
        motor_stop();               // spin the motor down
        while (speed) {};           // wait for it to stop
    }
    state &= ~(ST_EDGE | ST_HOLD);  // clear edge and hold flags
    state |= ST_DIR;                // start motor in needed direction
    motor_start();
}

// Move the window down
void movedn () {
    if (!(state & ST_DIR)) {           // if direction is the same in state
        if (state & ST_EDGE) return; // do nothing if it already has reached the edge
        if (state & ST_MOVE) {      // restart motor if it was in spindown state
            motor_start();
            return;
        }
    }
    if (state & ST_MOVE) {          // if direction was other in state
        motor_stop();               // spin the motor down
        while (speed) {};           // wait for it to stop
    }
    state &= ~(ST_EDGE | ST_HOLD);  // clear edge and hold flags
    state &= ~ST_DIR;               // start motor in needed direction
    motor_start();
}

// Enter 'hold' mode - now motor will continue working even if no input from switch
void hold () {
    if (state & (ST_EDGE | ST_MOVE)) state |= ST_HOLD;
}

int main () {
    init_switch_port(); // See switch.c for details
    ACSR |= (1<<ACD);   // Disable analog comparator for energy saving when idle
    unsigned char input = 0, prev = 0;
    while (1) {
        input = poll_switch();  // Poll current command from switch
        if (input != prev) {    // No nothing if it didn't change
            prev = input;
            switch (input) {
                case CMD_UP:    // Move up
                    moveup();
                    break;
                case CMD_DN:    // Move down
                    movedn();
                    break;
                case CMD_HOLD:  // Enable 'hold' mode
                    hold();
                    break;
                case CMD_CLOSE: // Move up and Hold together - for the alarm system which will close the windows
                    moveup();
                    hold();
                    break;
                case 0:         // No commands - if not in 'hold' state, stop the motor
                    if (!(state & ST_HOLD)) {
                        motor_stop();
                        while (speed) {};
                    }
                    break;
                default:        // For some weird situations =)
                    state = 0;
                    motor_stop();
            }
        }
    }
}

