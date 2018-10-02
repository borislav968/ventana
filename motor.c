/*  
 *   motor.c
 */

#include <avr/io.h>

void init_driver_port () {
    PORTB &= ~((1<<PB1) | (1<<PB2));
    DDRB |= (1<<PB1) | (1<<PB2);
}

void motor_stop () {
    PORTB &= ~((1<<PB1) | (1<<PB2));
}

void move_up () {
    motor_stop();
    PORTB |= (1<<PB1);
}

void move_dn () {
    motor_stop ();
    PORTB |= (1<<PB2);
}

void init_timers () {
    // Timer 1 is used for PWM control of the motor
    // Mode 5 - fast PWM 8-bit with top at 0xFF
    TCCR1A = (1<<WGM10);
    TCCR1B = (1<<WGM12);
    // Timer 1 prescaler 1 (001)
    // F_PWM will be F_CPU/(N*(TOP+1)) = 1000000/256 = 3906.25 Hz
    TCCR1B |= (1<<CS10);
    // Now raising COM1A1 and COM1B1 in TCCR1A will connect PB1 or PB2 to the timer
    // Duty can be adjusted by changing OCR1A and OCR1B
}

void start () {
    // Duty is full for debugging
    OCR1A = 0xFF;
    TCCR1A |= (1<<COM1A1);
    //for (;OCR1A<0xFF;OCR1A++) {
    //  _delay_ms(1);
    //}
}

void stop () {
    TCCR1A &= ~((1<<COM1A1) | (1<<COM1B1));
}
