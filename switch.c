/*
 * switch.c
 *
 * Created on: 27 сент. 2018 г.
 *      Author: Borislav Malkov
 */


#include <avr/io.h>
#include "defines.h"

#define SW_MASK ((1<<SW_0) | (1<<SW_1))

void init_switch_port () {
    SW_PORT |= SW_MASK;
    SW_DDR &= ~SW_MASK;
}

unsigned char poll_switch () {
    return (~(SW_PIN) & SW_MASK);
}
