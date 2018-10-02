/*
 * ventana.c
 *
 *  Created on: 27 сент. 2018 г.
 *      Author: elgato
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "motor.h"
#include "switch.h"

#define CMD_UP      (1<<0)
#define CMD_DN      (1<<1)
#define CMD_HOLD    (CMD_UP | CMD_DN)

volatile unsigned char state = 0;
volatile unsigned int cnt = 0;

#define ST_MOVE     (1<<0)
#define ST_DIR      (1<<1)
#define ST_HOLD     (1<<2)
#define ST_EDGE     (1<<3)

void init_sensor () {
    // PD6 and PD7 pins are inputs without pull-up
    PORTD &= ~((1<<PD6) | (1<<PD7));
    DDRD  &= ~((1<<PD6) | (1<<PD7));
    // Enable comparator multiplexer
    SFIOR &= ~(1<<ACME);
    // Enable interrupt on falling edge
    ACSR  |=  (1<<ACIS1) | (1<<ACIE);    
}

void start_timer (unsigned int time) {
    cnt = time;
    SFIOR |= (1<<PSR2); // Reset prescaler
    TIMSK |= (1<<TOIE2); // Allow overflow interrupt    
    TCCR2 = (1<<CS22) | (1<<CS21) | (1<<CS20); // Set prescaling to 1/1024 ~ 3.8 overflows/sec
}

void stop_timer () {
    TCCR2 &= ~((1<<CS22) | (1<<CS21) | (1<<CS20)); // Zeros disconnect timer from prescaler
    TIMSK &= ~(1<<TOIE2); // Disable overflow interrupt
}

// Comparator output falling edge
// Occurs when voltage at AIN1 > AIN0 (motor overload)
ISR (ANA_COMP_vect) {
    stop_timer();
    state |= ST_EDGE;
    motor_stop();
}

// Timer2 overflow
ISR (TIMER2_OVF_vect) {    
    if (cnt > 0) {
        cnt--;
    }
    else
    {
        stop_timer();
        state |= ST_EDGE;
        motor_stop();
    }
}

int main () {
    init_driver_port();
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
                    if (state & ST_HOLD) {
                        motor_stop();
                        _delay_ms(100);
                    }
                    state &= ~(ST_EDGE | ST_HOLD);
                    state |= ST_MOVE | ST_DIR;
                    start_timer(40);
                    move_up();
                    break;
                case CMD_DN:
                    if ((!(state & ST_DIR)) && ((state & ST_EDGE) || (state & ST_MOVE))) break;
                    if (state & ST_HOLD) {
                        motor_stop();
                        _delay_ms(100);
                    }
                    state &= ~(ST_EDGE | ST_DIR | ST_HOLD);
                    state |= ST_MOVE;                    
                    start_timer(40);
                    move_dn();
                    break;
                case CMD_HOLD:
                    if (state & (ST_EDGE | ST_MOVE)) {
                        state |= ST_HOLD;
                    }
                    break;
                case 0:
                    if (!(state & ST_HOLD)) {
                        state &= ~ST_MOVE;
                        stop_timer();
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

