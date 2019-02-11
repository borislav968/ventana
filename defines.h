/*
 * defines.h
 *
 * Created on: 27 сент. 2018 г.
 *      Author: Borislav Malkov
 */

#ifndef DEFINES_H
#define DEFINES_H

#define uchar unsigned char

#define ON          0x01
#define OFF         0x00

#define ST_MOVE     (1<<0)
#define ST_DIR      (1<<1)
#define ST_HOLD     (1<<2)
#define ST_EDGE     (1<<3)
#define ST_SPDUP    (1<<4)

#define CMD_UP      (1<<0)
#define CMD_DN      (1<<1)
#define CMD_HOLD    (CMD_UP | CMD_DN)
#define CMD_CLOSE   (1<<2)

#define BR_LR       0
#define BR_UR       1
#define BR_LL       2
#define BR_UL       3
#define BR_ERROR    0b11110000

#if defined __AVR_ATmega8__
    #define SW_PORT     PORTD
    #define SW_DDR      DDRD
    #define SW_PIN      PIND
    #define SW_0        PD0
    #define SW_1        PD1
    #define SW_2        PD2
    #define MT_PORT     PORTB
    #define MT_DDR      DDRB
    #define MT_PIN      PINB
    #define MT_UL       PB0
    #define MT_UR       PB6
    #define MT_LL       PB1
    #define MT_LR       PB2
    #define CMP_PORT    PORTD
    #define CMP_DDR     DDRD
    #define AIN0        PD6
    #define AIN1        PD7
    #define PORT_ADC    PORTC
    #define DDR_ADC     DDRC
    #define ADC_L       PC0
    #define ADC_R       PC1
# else
    #error "This source is for atmega8 and atmega16 MCUs"
#endif

// motor run max duration, ~seconds
#define T_DURATION 6

// PWM resolution - the bigger it is, the lower will be PWM frequency.
// Freq = F_CPU/N*(PWM_RES+1) = 8000000 / 8 * (63 + 1) = 15625Hz
// Also affects spinup/spindown time
#define PWM_RES 0x3F

#endif
