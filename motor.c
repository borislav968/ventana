/*  
 * motor.c
 * 
 * Created on: 27 сент. 2018 г.
 *      Author: Borislav Malkov
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


#define PWM_RES 0x3F


// Port setup for driver's output
void init_out () {
    MT_PORT |= (1<<MT_LL) | (1<<MT_LR);
    MT_DDR |= (1<<MT_UL) | (1<<MT_UR) | (1<<MT_LL) | (1<<MT_LR);
}

// Comparator set up for motor current measurement for stopping it at edges
void init_sensor () {
    CMP_PORT &= ~((1<<AIN0) | (1<<AIN1));
    CMP_DDR  &= ~((1<<AIN0) | (1<<AIN1));
    // Enable comparator multiplexer
    SFIOR &= ~(1<<ACME);
    // Enable interrupt on falling edge
    // Will fire if AIN0 > AIN1 (AIN1 has to be on tunable reference voltage)
    ACSR  |=  (1<<ACIS1) | (1<<ACIE);    
}

// I/O init. Has to be invoked at main program start
void init_motor () {
    init_out ();
    init_sensor ();
}

// Soft stop - just remove ST_SPDUP bit
// The rest will be done in Timer 0 interrupt
void motor_stop () {
    state &= ~ST_SPDUP;
}

// Timer 0 interrupt
// PWM control routine
ISR (TIMER0_COMP_vect) {
    // Increase or decrease speed depending on ST_SPDUP
    if (state & ST_SPDUP) {
        if (speed < PWM_RES) speed++;
    }
    else
    {
        if (speed > 0) speed--;
    };
    // Set PWM duty on appropriate channel
    *(state & ST_DIR ? &OCR1A : &OCR1B) = speed;
    // Complete stop routine
    if (speed == 0) {
        MT_PORT &= ~((1<<MT_UL) | (1<<MT_UR));
        // Disable timers
        TIMSK &= ~((1<<OCIE0) | (1<<TOIE2));
        TCCR0 = 0;
        TCCR1A = 0;
        TCCR1B = 0;
        TCCR2 = 0;
        // Clear ST_MOVE flag
        state &= ~ST_MOVE;
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
        state &= ~ST_SPDUP;
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
    // Prescaler is 1/256 (100)
    TCCR0 = (1<<CS02) | (1<<WGM01);
    OCR0 = T_SPEEDUP * 64;
    // Enable timer 0 overflow interrupt
    //TIMSK |= (1<<TOIE0);
    TIMSK |= (1<<OCIE0);
    // Timer 1 is used for PWM control of the motor
    // Mode 14 - fast PWM with top at ICR1
    ICR1 = PWM_RES;
    TCCR1A = (1<<WGM11);
    TCCR1B = (1<<WGM12) | (WGM13);
    // Timer 1 prescaler 1 (001)
    // F_PWM will be F_CPU/(N*(TOP+1)) = 1000000/64 = 15625 Hz
    TCCR1B |= (1<<CS10);
    // Now raising COM1A1 and COM1B1 in TCCR1A will connect PB1 or PB2 to the timer
    // Duty can be adjusted by changing OCR1A and OCR1B    
    if (state & ST_DIR) 
    {
        PORTD |= (1<<MT_UL) | (1<<MT_LR);
        PORTD &= ~(1<<MT_UR);
        TCCR1A |= (1<<COM1A0) | (1<<COM1A1);
    }
    else
    {
        PORTD |= (1<<MT_UR) | (1<<MT_LL);
        PORTD &= ~(1<<MT_UL);        
        TCCR1A |= (1<<COM1B0) | (1<<COM1B1);
    }
    // Timer 2 is for motor run time limiting (e.g. if stop by current sensor comparator didn't happen)
    TCCR2 = (1<<CS22) | (1<<CS21) | (1<<CS20); // Set prescaling to 1/1024 ~ 3.8 overflows/sec
    TIMSK |= (1<<TOIE2); // Allow overflow interrupt    
    duration = T_DURATION * 3.8;    
}
