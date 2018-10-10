/*
 * defines.h
 *
 * Created on: 27 сент. 2018 г.
 *      Author: Borislav Malkov
 */

#ifndef DEFINES_H
#define DEFINES_H

#define ST_MOVE     (1<<0)
#define ST_DIR      (1<<1)
#define ST_HOLD     (1<<2)
#define ST_EDGE     (1<<3)
#define ST_SPDUP    (1<<4)

#if defined __AVR_ATmega8__
    #define SW_PORT PORTD
    #define SW_DDR  DDRD
    #define SW_PIN  PIND
    #define SW_0    PD0
    #define SW_1    PD1
    #define MT_PORT PORTB
    #define MT_DDR  DDRB
    #define MT_UL   PB0
    #define MT_UR   PB3
    #define MT_LL   PB1
    #define MT_LR   PB2
#elif defined __AVR_ATmega16__
    #define SW_PORT PORTA
    #define SW_DDR  DDRA
    #define SW_PIN  PINA    
    #define SW_0    PA0
    #define SW_1    PA1
    #define MT_PORT PORTD
    #define MT_DDR  DDRD
    #define MT_UL   PD6
    #define MT_UR   PD7
    #define MT_LL   PD4
    #define MT_LR   PD5
# else
    #error "This source is for atmega8 and atmega16 MCUs"
#endif
    

// motor run max duration, ~seconds
#define T_DURATION  10

#endif
