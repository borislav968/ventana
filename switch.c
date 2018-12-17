/*
 * switch.c
 *
 * Created on: 27 сент. 2018 г.
 *      Author: Borislav Malkov
 */


#include <avr/io.h>
#include <avr/eeprom.h>
#include "defines.h"

#define SW_MASK ((1<<SW_0) | (1<<SW_1))

unsigned char dir = 0;

unsigned char poll_switch () {
    unsigned char same = 0;
    unsigned char this = 0, last = 0;
    while (same < 5) {
        this = ~(SW_PIN) & SW_MASK;
        if (this == last) same++;
        last = this;
    }
    last = 0;
    if (this & (1<<SW_0)) last |= dir ? CMD_UP : CMD_DN;
    if (this & (1<<SW_1)) last |= dir ? CMD_DN : CMD_UP;
    return (last);
}

void init_switch_port () {
    SW_PORT |= SW_MASK;
    SW_DDR &= ~SW_MASK;
    dir = eeprom_read_byte(0) & 1;
    if (poll_switch() == CMD_HOLD) {
        dir = dir ^ 1;
        eeprom_write_byte(0, dir);
        //while (poll_switch());
    }
}
