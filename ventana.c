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
#include "driver.h"

// TODO: Make those two functions one
// Move the window up
void moveup () {
    if (state & ST_DIR) {           // if direction is the same in state
        if ((state & ST_EDGE) || (state & ST_HOLD)) return; // do nothing if it already has reached the edge
        if (state & ST_MOVE) {      // restart motor if it was in spindown state
            state |= ST_SPDUP;
            return;
        }
    }
    if (state & ST_MOVE) {          // if direction was other in state
        state &= ~ST_HOLD;
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
        if ((state & ST_EDGE) || (state & ST_HOLD)) return; // do nothing if it already has reached the edge
        if (state & ST_MOVE) {      // restart motor if it was in spindown state
            state |= ST_SPDUP;
            return;
        }
    }
    if (state & ST_MOVE) {          // if direction was other in state
        state &= ~ST_HOLD;
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

void rollback () {
    uchar t;
    t = max_duration;
    max_duration = t/6;
    state = 0;
    movedn();
    while (state & ST_MOVE);
    max_duration = t;
    state = 0;
}

ISR (TIMER1_CAPT_vect) {

}

// Enter sleep mode
void sleep () {
    // timer1 will wake the MCU after ~1/15 second
    TCCR1A = 0;
    TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS11) | (1<<CS10);   // set prescaler 64
    TCNT1 = 0;
    ICR1 = 0x1046;          // set 'top' to 4166
    TIMSK |= (1<<TICIE1);   // allow compare match interrupt
    asm("sleep");           // go to idle mode
    TIMSK &= ~(1<<TICIE1);  // disable timer2 after awakening
    TCCR1B = 0;
    bridge_chk();           // check if there are no shorts of bridge middle points to +12V
}

int main () {
    // Allow global interrupts
    asm("sei");
    // Set up the idle mode
    MCUCR &= ~((1<<SM2) | (1<<SM1) | (1<<SM0));
    MCUCR |= (1<<SE);
    bridge = (1<<BR_LL) | (1<BR_LR);
    bridge_update();
    init_switch_port(); // See switch.c for details
    init_motor();
    ACSR |= (1<<ACD);   // Disable analog comparator for energy saving when idle

    unsigned char input = 0, prev = 0;
    while (1) {
        if (state & ST_BACK) rollback();
        input = poll_switch();  // Poll current command from switch
        if (input != prev) {    // Do nothing if it didn't change
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
                    }
                    break;
                default:        // For some weird situations =)
                    state = 0;
                    motor_stop();
            }
            prev = input;
        } else if (!(input || speed)) sleep(); // go to idle mode if nothing is happening
    }
}

