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
    unsigned char same = 0;
    unsigned char this = 0, last = 0;
    while (same < 5) {
        this = SW_PIN;
        if (this == last) same++;
        last = this;
    }
    return (~(SW_PIN) & SW_MASK);
}
