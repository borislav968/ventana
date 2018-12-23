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

volatile uchar bridge;

void led_on () {
    DDRD |= (1<<PD3);
    PORTD |= (1<<PD3);    
}

void led_off () {
    PORTD &= ~(1<<PD3);
}

uchar get_bridge_state () {
    uchar res;
    res  =   ((MT_PIN>> MT_UL) & 0x01)<<BR_UL;
    res |= (((~MT_PIN)>>MT_LL) & 0x01)<<BR_LL; 
    res |=   ((MT_PIN>> MT_UR) & 0x01)<<BR_UR;
    res |= (((~MT_PIN)>>MT_LR) & 0x01)<<BR_LR;
    return res;
}

void check_bridge () {
}

void adc (uchar on) {
    if (on) {
        DDR_ADC &= ~((1<<ADC_L) | (1<<ADC_R));
        PORT_ADC &= ~((1<<ADC_L) | (1<<ADC_R));
        ADMUX = (1<<REFS0) | (1<<ADLAR);
        ADCSRA = (1<<ADEN) | (1<<ADPS2);
    } else {
        ADCSRA = 0;
        ADMUX = 0;
    }
}

uchar get_voltage (uchar mux) {
    ADMUX &= 0b11110000;
    ADMUX |= (mux & 0b00001111);
    _delay_ms(1);
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC)) {};
    return ADCH;
}

void fet_ctrl (uchar i) {
    uchar fet, voltage;
    uchar error = 0;
    if (bridge & (1<<(i+4))) return;
    switch (i) {
        case 0: 
            fet = MT_LR;
            break;
        case 1:
            fet = MT_UR;
            break;
        case 2:
            fet = MT_LL;
            break;
        case 3:
            fet = MT_UL;
            break;
        default:
            return;
    }
    if ((bridge>>i) & 0x01) {
        led_off();
        adc(ON);
        MT_PORT = (i & 0x01) ? MT_PORT | (1<<fet) : MT_PORT & ~(1<<fet);
        voltage = get_voltage(~(i>>1) & 0x01);
        if (i & 0x01) {
            if (voltage < 130) error++;
        } else {
            if (voltage > 20) error++;
        }
        adc(OFF);
        if (error == 0) return;
        if (!(i & 0x01)) bridge |= (1<<(i+4));
        led_on();
    }
    MT_PORT = (i & 0x01) ? MT_PORT & ~(1<<fet) : MT_PORT | (1<<fet);
}

void bridge_update () {
    uchar change, i;
    if (bridge & (1<<BR_UL)) bridge &= ~(1<<BR_LL);
    if (bridge & (1<<BR_UR)) bridge &= ~(1<<BR_LR);
    change = bridge;
    check_bridge();
    change = bridge ^ get_bridge_state();;
    for (i=0; i<4; i++) if (change & (1<<i) && !(bridge & (1<<i))) fet_ctrl(i);
    for (i=0; i<4; i++) if (change & (1<<i) && bridge & (1<<i)) fet_ctrl(i);
}

void bridge_chk () {
    uchar i;
    for (i=0; i<4; i++) if (!(bridge & (1<<i)) fet_ctrl(i);
    for (i=0; i<4; i++) if (bridge & (1<<i) fet_ctrl(i);
}
