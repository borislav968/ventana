/*  
 *   motor.c
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "defines.h"

volatile unsigned char state = 0;
volatile unsigned char speed = 0;
volatile unsigned char duration;

void init_motor_port () {
    PORTB &= ~((1<<PB1) | (1<<PB2));
    DDRB |= (1<<PB1) | (1<<PB2);
}

void motor_stop () {
    state &= ~ST_SPDUP;
}


ISR (TIMER0_OVF_vect) {
    if (state & ST_SPDUP) {
        if (speed < 0xFF) speed+=0x01;
    }
    else
    {
        if (speed > 0) speed-=0x01;
    };
    OCR1A = speed;
    OCR1B = speed;
    if (speed == 0) {
        TIMSK &= ~((1<<TOIE0) | (1<<TOIE2));
        TCCR0 = 0;
        TCCR1A = 0;
        TCCR1B = 0;
        TCCR2 = 0;
        asm("sei");
    }
}

// Timer2 overflow
ISR (TIMER2_OVF_vect) {    
    if (duration > 0) {
        duration--;
    }
    else
    {
        state |= ST_EDGE;
        speed = 0;
        motor_stop();
        asm("sei");
    }
}


void motor_start () {
    // Enable speed up bit in status
    state |= ST_SPDUP;
    // Timer 0 is for motor speed increase/decrease
    // Prescaler is 1/1024 (101)
    TCCR0 = (1<<CS01);
    // Enable timer 0 overflow interrupt
    TIMSK |= (1<<TOIE0);
    // Timer 1 is used for PWM control of the motor
    // Mode 5 - fast PWM 8-bit with top at 0xFF
    TCCR1A = (1<<WGM10);
    TCCR1B = (1<<WGM12);
    // Timer 1 prescaler 1 (001)
    // F_PWM will be F_CPU/(N*(TOP+1)) = 1000000/256 = 3906.25 Hz
    TCCR1B |= (1<<CS10);
    // Now raising COM1A1 and COM1B1 in TCCR1A will connect PB1 or PB2 to the timer
    // Duty can be adjusted by changing OCR1A and OCR1B    
    if (state & ST_DIR)
        TCCR1A |= (1<<COM1A1);
    else
        TCCR1A |= (1<<COM1B1);
    duration = T_DURATION;
    TCCR2 = (1<<CS22) | (1<<CS21) | (1<<CS20); // Set prescaling to 1/1024 ~ 3.8 overflows/sec
    TIMSK |= (1<<TOIE2); // Allow overflow interrupt    
}
