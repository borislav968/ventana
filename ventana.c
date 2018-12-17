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

// Enter sleep mode
void sleep () {
    state |= ST_SLEEP;
    // timer2 will wake the MCU after ~1/15 second
    TCCR2 = (1<<CS22) | (1<<CS21);  // Set prescaling to 1/256 ~ 15.2 overflows/sec
    TIMSK |= (1<<TOIE2);            // Allow overflow interrupt
    asm("sleep");                   // go to idle mode
    TIMSK &= ~(1<<TOIE2);           // disable timer2 after awakening
    TCCR2 = 0;
    state &= ~ST_SLEEP;
}

int main () {
    // Allow global interrupts
    asm("sei");
    // Set up the idle mode
    MCUCR &= ~((1<<SM2) | (1<<SM1) | (1<<SM0));
    MCUCR |= (1<<SE);
    init_switch_port(); // See switch.c for details
    ACSR |= (1<<ACD);   // Disable analog comparator for energy saving when idle
    unsigned char input = 0, prev = 0;
    while (1) {
        input = poll_switch();  // Poll current command from switch
        if (input != prev) {    // Do nothing if it didn't change
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
        } else if (!(input || speed)) sleep(); // go to idle mode if nothing is happening
    }
}

