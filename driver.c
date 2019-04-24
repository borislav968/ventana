/*
 * driver.c
 *
 * Low-level driver for the H-bridge
 *
 * Created on: 19 дек. 2018 г.
 *      Author: Borislav Malkov
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "defines.h"
#include "util.h"
#include <stdlib.h>

// Stores the H-bridge state
// 3:0 are for the state (on/off) of UL, LL, UR, LR
// 7:4 are for errors of UL, LL, UR, LR
volatile uchar bridge;

// Returns the actual state of bridge controlling pins
uchar get_bridge_state () {
    uchar res;
    res  =   ((MT_PIN>> MT_UL) & 0x01)<<BR_UL;
    res |= (((~MT_PIN)>>MT_LL) & 0x01)<<BR_LL;
    res |=   ((MT_PIN>> MT_UR) & 0x01)<<BR_UR;
    res |= (((~MT_PIN)>>MT_LR) & 0x01)<<BR_LR;
    return res;
}

// Turns the ADC on or off
void adc (uchar on) {
    if (on) {
        DDR_ADC &= ~((1<<ADC_L) | (1<<ADC_R));
        PORT_ADC &= ~((1<<ADC_L) | (1<<ADC_R));
        ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1);
        ADMUX = (1<<REFS0) | (1<<ADLAR);
    } else {
        ADMUX = 0;
        ADCSRA &= ~(1<<ADEN);
    }
}

// Measures the voltage at middle points of the bridge
// 'mux' selects the ADC input
uchar get_voltage (uchar mux) {
    ADMUX &= 0b11110000;            // zero the 3:0 ADMUX bits
    ADMUX |= (mux & 0b00001111);    // raise 3:0 bits as needed
    _delay_ms(1);                   // wait for the signal to get stable
    ADCSRA |= (1<<ADSC);            // start the conversion
    while (ADCSRA & (1<<ADSC)) {};  // ...and wait for it to end
    return ADCH;
}

uchar out_voltage () {
    uchar a, b;
    adc(ON);
    a = get_voltage(0x00);
    b = get_voltage(0x01);
    adc(OFF);
    if (a == b) return 0;
    return a > b ? a - b : b - a;
}

// Since the actual FET outputs not necessarily are matching to 'bridge' bits
// this one will return actual pin of the MOSFET by it's number in the 'bridge'
uchar which_fet (uchar i) {
    switch (i) {
        case 0:
            return MT_LR;
        case 1:
            return MT_UR;
        case 2:
            return MT_LL;
        case 3:
            return MT_UL;
        default:
            return 0xFF;
    }
}

// Checks if the open MOSFET doesn't cause a short circuit due, for example, bad insulation
void fet_chk (uchar i) {
    uchar fet, voltage;
    uchar k, v[3];
    uchar error = 0;
    if (bridge & (1<<(i+4))) return;            // do nothing if there already is an error on this MOSFET
    if ((bridge>>i) & 0x01) {                   // there's no need to do this check for a closed MOSFET
        fet = which_fet(i);
        adc(ON);                                // turn on the ADC
        //voltage = get_voltage(~(i>>1) & 0x01);  // measure voltage on the left or right side
        for (k = 0; k < 3; k++) {
            v[k] = get_voltage(~(i>>1) & 0x01);
        }
        voltage = median(v);
        if (i & 0x01) {                                             // if it is an upper MOSFET
            if (voltage < (MAX_VOLTAGE - HI_THRESHOLD)) error++;    // it should make the voltage rise, so if it's too low, there's an error
        } else {
            if (voltage > LO_THRESHOLD) error++;                    // lower MOSFET should hold the voltage low
        }
        adc(OFF);                               // turn off the ADC
        if (error == 0) return;                 // if no errors, nothing more to do
        if (!(i & 0x01)) bridge |= (1<<(i+4));  // remember the error in the corresponding bit of 'bridge' var
        MT_PORT = (i & 0x01) ? MT_PORT & ~(1<<fet) : MT_PORT | (1<<fet);    // of course the MOSFET should be closed now
    }
}

// Open or close the MOSFET addressed by 'i'
void fet_ctrl (uchar i) {
    uchar fet;
    if (bridge & (1<<(i+4))) return;    // if there is a stored error, do nothing
    fet = which_fet(i);
    if ((bridge>>i) & 0x01) {           // open a MOSFET
        MT_PORT = (i & 0x01) ? MT_PORT | (1<<fet) : MT_PORT & ~(1<<fet);    // for upper MOSFETs open is '1', for lower ones '0'
    } else {
        MT_PORT = (i & 0x01) ? MT_PORT & ~(1<<fet) : MT_PORT | (1<<fet);    // and vice versa for closing them
    }
}

// updates the  bridge state from 'br' by opening and closing the MOSFETs
// with checking for errors
void bridge_update (uchar br) {
    uchar change, i;
    if (br) bridge = br;                                // if no params, update from the 'bridge' global var
    if (bridge & (1<<BR_UL)) bridge &= ~(1<<BR_LL);     // check if there are no collisions
    if (bridge & (1<<BR_UR)) bridge &= ~(1<<BR_LR);     // we don't want to open lower and upper MOSFETs on the same side
    change = bridge ^ get_bridge_state();               // make the mask of changing bits
    for (i=0; i<4; i++) if (change & (1<<i) && !(bridge & (1<<i))) fet_ctrl(i); // first we do all closing MOSFETs
    for (i=0; i<4; i++) if (change & (1<<i) && bridge & (1<<i)) {               // and after that, all the opening ones
        fet_ctrl(i);
        _delay_ms(20);
        fet_chk(i);
    }
}

// Checks the MOSFETs that are already open
// used when idle, 'cause at idle the lower MOSFETs are open, and we shoud check if there's no shorts from +12V to middle points
void bridge_chk () {
    uchar i;
    for (i=0; i<4; i++) if (bridge & (1<<i)) fet_chk(i);
}
