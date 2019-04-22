/*
 * switch.c
 *
 * Created on: 27 сент. 2018 г.
 *      Author: Borislav Malkov
 */


#include <avr/io.h>
#include <avr/eeprom.h>
#include "defines.h"
#include "motor.h"

#define SW_MASK ((1<<SW_0) | (1<<SW_1) | (1<<SW_2))

// if bit 0 of this var is 1, it inverts the switch Up and Down positions
unsigned char dir = 0;

unsigned char poll_switch () {
    unsigned char same = 0;
    unsigned char this = 0, last = 0;
    while (same < 5) {                  // Debouncing - read the state 5 times
        this = ~(SW_PIN) & SW_MASK;
        if (this == last) same++;
        last = this;
    }
    last = 0;
    // Up and Down commands depend on 'dir' variable
    if (this & (1<<SW_0)) last |= dir ? CMD_UP : CMD_DN;
    if (this & (1<<SW_1)) last |= dir ? CMD_DN : CMD_UP;
    // Special command from the alarm system - close the window
    if (this & (1<<SW_2)) last = CMD_CLOSE;
    return (last);
}

// Init the input and set up the switch direction
void init_switch_port () {
    SW_PORT |= SW_MASK;
    SW_DDR &= ~SW_MASK;
    dir = eeprom_read_byte((uchar*) 0) & 1;      // Get bit 0 from address 0 of EEPROM
    switch (poll_switch()) {
        case CMD_HOLD:
            dir = dir ^ 1;                  // invert the direction
            eeprom_write_byte((uchar*) 0, dir);      // and save it to EEPROM
            while (poll_switch());
            break;
        case CMD_UP:
            state |= ST_LEARN;
            while (poll_switch());
            break;
    }
}
