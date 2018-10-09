/*  
 *   motor.c
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "defines.h"

// Device state bits are stored here
volatile unsigned char state = 0;

// Current speed for PWM control
volatile unsigned char speed = 0;

// Time counter for motor run time limiting
volatile unsigned char duration;



// Port setup for driver's output
void init_out () {
    //  Signal  M8  M16
    //  OC1A    PB1 PD4
    //  OC1B    PB2 PD5
    #if defined __AVR_ATmega8__
        PORTB &= ~((1<<PB1) | (1<<PB2));
        DDRB |= (1<<PB1) | (1<<PB2);
    #elif defined __AVR_ATmega16__
        PORTD &= ~((1<<PD4) | (1<<PD5));
        DDRD |= (1<<PD4) | (1<<PD5);
    # else
        #error "This source is for atmega8 and atmega16 MCUs"
    #endif
}

// Comparator set up for motor current sense for stopping it at edges
void init_sensor () {
    //  Signal  M8  M16
    //  AIN0    PD6 PB2
    //  AIN1    PD7 PB3
    #if defined __AVR_ATmega8__
        PORTD &= ~((1<<PD6) | (1<<PD7));
        DDRD  &= ~((1<<PD6) | (1<<PD7));
    #elif defined __AVR_ATmega16__
        PORTB &= ~((1<<PB2) | (1<<PB3));
        DDRB  &= ~((1<<PB2) | (1<<PB3));
    # else
        #error "This source is for atmega8 and atmega16 MCUs"
    #endif
    // Enable comparator multiplexer
    SFIOR &= ~(1<<ACME);
    // Enable interrupt on falling edge
    ACSR  |=  (1<<ACIS1) | (1<<ACIE);    
}

// I/O init. Has to be invoked at main program start
void init_motor () {
    init_out ();
    init_sensor ();
}

// Soft stop - just remove ST_SPDUP bit
// The rest will be done in Timer 0 overflow interrupt
void motor_stop () {
    state &= ~ST_SPDUP;
}

// Timer 0 interrupt
// PWM control routine
ISR (TIMER0_OVF_vect) {
    // Increase or decrease speed depending on ST_SPDUP
    if (state & ST_SPDUP) {
        if (speed < 0xFF) speed++;
    }
    else
    {
        if (speed > 0) speed--;
    };
    // Set PWM duty on both channels
    OCR1A = speed;
    OCR1B = speed;
    // Complete stop routine
    if (speed == 0) {
        // Disable timers
        TIMSK &= ~((1<<TOIE0) | (1<<TOIE2));
        TCCR0 = 0;
        TCCR1A = 0;
        TCCR1B = 0;
        TCCR2 = 0;
        // Clear ST_MOVE flag
        state &= ~ST_MOVE;
        // Important thing: since timer interrupts (including this one) are
        // disabled during the interrupt, we need to do SEI in order to return from here
        asm("sei");
    }
}

// Timer2 overflow
ISR (TIMER2_OVF_vect) {    
    // Check if it's time to stop motor
    if (duration > 0) {
        duration--;
    }
    else
    {
        // Acting like current sensor triggered
        state |= ST_EDGE;
        // Immediate stop without soft spindown
        speed = 0;
        motor_stop();
        asm("sei");
    }
}

// Comparator output falling edge
// Occurs when voltage at AIN1 > AIN0 (motor overload)
ISR (ANA_COMP_vect) {
    state |= ST_EDGE;
    speed = 0;
    motor_stop();
}


void motor_start () {
    // Enable speed up and move bits in status
    state |= ST_SPDUP | ST_MOVE;
    // Timer 0 is for motor speed increase/decrease
    // Prescaler is 1/1024 (101) ~3.8Hz
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
    // Timer 2 is for motor run time limiting (e.g. if stop by current sensor comparator didn't happen)
    TCCR2 = (1<<CS22) | (1<<CS21) | (1<<CS20); // Set prescaling to 1/1024 ~ 3.8 overflows/sec
    TIMSK |= (1<<TOIE2); // Allow overflow interrupt    
    duration = T_DURATION * 3.8;    
}
