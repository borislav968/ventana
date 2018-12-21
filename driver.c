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
#include "defines.h" 

//volatile uchar br_state = 0;

void led_on () {
    DDRD |= (1<<PD3);
    PORTD |= (1<<PD3);    
}

void led_off () {
    PORTD &= ~(1<<PD3);
}

uchar get_bridge_state () {
    uchar res;
    res  = ((MT_PIN>>MT_UL) & 0x01)<<BR_UL;
    res |= (((~MT_PIN)>>MT_LL) & 0x01)<<BR_LL; 
    res |= ((MT_PIN>>MT_UR) & 0x01)<<BR_UR;
    res |= (((~MT_PIN)>>MT_LR) & 0x01)<<BR_LR;
    return res;
}

void check_conf (uchar *conf) {
    if (*conf & (1<<BR_UL)) *conf &= ~(1<<BR_LL);
    if (*conf & (1<<BR_UR)) *conf &= ~(1<<BR_LR);
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
    led_on();
    ADMUX &= 0b11110000;
    ADMUX |= (mux & 0b00001111);
    TCCR2 = (1<<CS22) | (1<<CS21) | (1<<CS20);
    TIMSK |= (1<<TOIE2);
    asm("sleep");
    TIMSK &= ~(1<<TOIE2);
    TCCR2 = 0;
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC)) {};
    led_off();
    return ADCH;
}

void fet_ctrl (uchar ctrl, uchar i) {
    uchar fet, inv = 0;
    switch (i) {
        case 0: 
            fet = MT_LR;
            inv++;
            break;
        case 1:
            fet = MT_UR;
            break;
        case 2:
            fet = MT_LL;
            inv++;
            break;
        case 3:
            fet = MT_UL;
            break;
        default:
            led_on();
            return;
    }
    if ((ctrl>>i) & 0x01) {
        adc(ON);
        MT_PORT = inv ? MT_PORT & ~(1<<fet) : MT_PORT | (1<<fet);
        
        adc(OFF);    
    } else {
        MT_PORT = inv ? MT_PORT | (1<<fet) : MT_PORT & ~(1<<fet);
    }
}

void bridge_ctrl (uchar conf) {
    static uchar br_state;
    uchar change, i;
    led_off();
    br_state = conf;
    check_conf(&conf);
    if (br_state != conf) led_on();
    br_state = get_bridge_state ();
    change = br_state ^ conf;
    for (i=0; i<4; i++) {
        if (change & (1<<i)) {
            fet_ctrl(conf, i);
        }
    }
}
