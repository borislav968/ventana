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

// PWM resolution - the bigger it is, the lower will be PWM frequency.
// Freq = F_CPU/PWM_RES+1 (15625Hz)
// Also affects spinup/spindown time
#define PWM_RES 0x2F

// Soft stop - just remove ST_SPDUP bit
// The rest will be done in Timer 0 interrupt
void motor_stop () {
    state &= ~ST_SPDUP;
}

// Timer 0 interrupt
// PWM control routine
ISR (TIMER0_OVF_vect) {
    unsigned char s = speed;
    // Increase or decrease speed depending on ST_SPDUP
    if ((state & ST_SPDUP) && (speed < PWM_RES)) {
        // Here can be simply increment of speed, but to get the drive started
        // it's better to reach something like 25% of power asap, and then increment it.
        if (speed < (PWM_RES>>2)) speed += 4; else speed++;
    }
    else
    {
        // Feels better when the drive stops faster than starts, so speed decreases 4x faster
        // Lower than ~25% of power there's no need to slowly decrease - the window may be actually stopped already
        if (speed > (PWM_RES>>2)) speed -= 4; else speed = 0;
    };
    // Set PWM duty on appropriate channel
    if (s != speed) *(state & ST_DIR ? &OCR1A : &OCR1B) = speed;
    // Complete stop routine
    if (speed == 0) {
        // Disable upper bridge drivers
        MT_PORT &= ~((1<<MT_UL) | (1<<MT_UR));
        // Enable lower bridge drivers for energy saving
        MT_PORT &= ~((1<<MT_LL) | (1<<MT_LR));
        // Disable timers
        TIMSK &= ~((1<<TOIE0) | (1<<TOIE2));
        TCCR0 = 0;
        TCCR1A = 0;
        TCCR1B = 0;
        TCCR2 = 0;
        // Disable comparator
        ACSR &= ~(1<<ACIE);
        ACSR |= (1<<ACD);
        // Disable output port
        MT_DDR = 0;
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
        state &= ~(ST_SPDUP | ST_HOLD);
    }
}

// Comparator output falling edge
// Occurs when voltage at AIN1 > AIN0 (motor overload)
ISR (ANA_COMP_vect) {
    state |= ST_EDGE;
    speed = 0;
    state &= ~(ST_SPDUP | ST_HOLD);
}

void motor_start () {
    // Set up comparator input pins
    CMP_PORT &= ~((1<<AIN0) | (1<<AIN1));
    CMP_DDR  &= ~((1<<AIN0) | (1<<AIN1));
    // Enable analog comparator
    ACSR &= ~(1<<ACD);
    // Enable comparator multiplexer
    SFIOR &= ~(1<<ACME);
    // Enable interrupt on falling edge
    // Will fire if AIN0 > AIN1 (AIN1 has to be on tunable reference voltage)
    ACSR  |=  (1<<ACIS1);
    // Enable comparator interrupt for overcurrent protection
    ACSR |= (1<<ACIE);
    // output pins for drivers
    MT_DDR |= (1<<MT_UL) | (1<<MT_UR) | (1<<MT_LL) | (1<<MT_LR);
    // Important: due to different type MOSFETs in upper and lower halves of the bridge,
    // upper ones are open with logical 1, and lower ones open with logical 0.
    // Enable speed up and move bits in status
    state |= ST_SPDUP | ST_MOVE;
    // Timer 0 is for motor speed increase/decrease
    // Spinup time = PWM_RES * ((N*256)/F_CPU), timer0 prescaler N is now 64
    TCCR0 |= (1<<CS01) | (1<<CS00);
    // Enable timer 0 overflow interrupt
    TIMSK |= (1<<TOIE0);
    // Timer 1 is used for PWM control of the motor
    // Mode 14 - fast PWM with top at ICR1
    TCCR1A = (1<<WGM11);
    TCCR1B = (1<<WGM12) | (1<<WGM13);
    ICR1 = PWM_RES;
    // timer1 prescaler 1
    // F_PWM will be F_CPU/(N*(PWM_RES+1)) = 1000000/64 = 15625 Hz
    TCCR1B |= (1<<CS10);
    // Now raising COM1A1 and COM1B1 in TCCR1A will connect PB1 or PB2 to the timer
    // Duty can be adjusted by changing OCR1A and OCR1B    
    // Now start the motor in needed direction
    if (state & ST_DIR) 
    {
        MT_PORT |= (1<<MT_LR);                  // close lower right first
        MT_PORT |= (1<<MT_UR);                  // open upper right after. Not vice versa - will cause short circuit!
        TCCR1A |= (1<<COM1A0) | (1<<COM1A1);    // connect cnannel A (lower left) to the PWM timer
    }
    else
    {
        MT_PORT |= (1<<MT_LL);                  // close lower left first
        MT_PORT |= (1<<MT_UL);                 // open upper left after. Not vice versa - will cause short circuit!
        TCCR1A |= (1<<COM1B0) | (1<<COM1B1);    // connect channel B (lower right) to the PWM timer
    }
    // Timer 2 is for motor run time limiting (e.g. if stop by current sensor comparator didn't happen)
    duration = T_DURATION * 3.8;
    TCCR2 = (1<<CS22) | (1<<CS21) | (1<<CS20); // Set prescaling to 1/1024 ~ 3.8 overflows/sec
    TIMSK |= (1<<TOIE2); // Allow overflow interrupt    
}
