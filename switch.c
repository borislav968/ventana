/*
 * switch.c
 *
 * Created on: 27 сент. 2018 г.
 *      Author: Borislav Malkov
 */


#include <avr/io.h>

#define SW_MASK ((1<<PD0) | (1<<PD1))

void init_switch_port () {
    PORTD = SW_MASK;
    DDRD &= ~SW_MASK;
}

unsigned char poll_switch () {
    return (~(PIND) & SW_MASK);
}
