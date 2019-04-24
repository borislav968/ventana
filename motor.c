/*
 * motor.c
 *
 * Created on: 27 сент. 2018 г.
 *      Author: Borislav Malkov
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "defines.h"
#include "driver.h"
//#include <stdlib.h>
//#include <stdio.h>
#include <util/delay.h>
#include "util.h"


// Device state bits are stored here
volatile unsigned char state = 0;

// Current speed for PWM control
volatile unsigned char speed = 0;

// Time counter for motor run time limiting
volatile uint duration;

// Maximum motor run duration
volatile uint max_duration;

//char buffer[50];

record journal;                     // motor voltage log
uchar logptr = 0;                   // last log record index
uchar logsize = 0;                  // size of the last log
interval time_int = {0, REC_LG-1};  // voltage drop measurement interval
uint hardness = 0;                  // measured voltage drop function square
uchar voltage_normal = 0;           // measured motor voltage during run

void savelog (uchar v) {
    journal[logptr] = v;
    logptr = (logptr + 1 + REC_LG) % REC_LG;
    if (logsize < REC_LG) logsize++;
}

void analyze (uchar j[]) {
    uint h;
    signed int dv = 0;
    if (logsize < (REC_LG - time_int.start) || ((max_duration - duration) < 20)) return;
    rec_wrap(journal, logptr);
    rec_filter(journal, 1);
    rec_filter(journal, 0);
    dv = /**(time_int.end - time_int.start) **/ (voltage_normal - rec_volt_margins(journal, time_int).end);
    dv *= 5;
    rec_relative(journal, time_int);
    h = rec_square(j, time_int) + dv;
    if (h < hardness) state |= ST_BACK;
}

// Soft stop - just remove ST_SPDUP bit
// The rest will be done in Timer 0 interrupt
void motor_stop () {
    state &= ~ST_SPDUP;
}

// Timer 0 interrupt
// PWM control routine
ISR (TIMER0_OVF_vect) {
    unsigned char s = speed;    // remember current speed
    savelog(out_voltage());
    // Increase or decrease speed depending on ST_SPDUP
    if (state & ST_SPDUP) {
        if (speed < PWM_RES) speed++;
    }
    else
    {
        if (speed > 0) speed--;
    };
    // Set PWM duty on appropriate channel if speed has changed
    if ((s != speed) || (speed == 0)) *(state & ST_DIR ? &OCR1A : &OCR1B) = speed;
    // Complete stop routine
    if (speed == 0) {
        // Disable timers and interrupts
        TIMSK &= ~(1<<TOIE0);
        TCCR0 = 0;
        TCCR1A = 0;
        TCCR1B = 0;
        TCCR2 = 0;
        // Disable bridge drivers
        bridge_update((1<<BR_LL) | (1<<BR_LR));
        // Disable comparator
        ACSR &= ~(1<<ACIE);
        ACSR |= (1<<ACD);
        // Clear ST_MOVE flag
        state &= ~ST_MOVE;
        if ((state & ST_DIR) && (state & ST_EDGE) && (!(state & ST_LEARN)) && (!(state & ST_TMOUT))) analyze(journal);
    }
}

// Timer2 overflow
ISR (TIMER2_OVF_vect) {
    // Check if it's time to stop motor
    if (duration > 0) {
        duration--;
    }
    else
    {
        state |= ST_TMOUT;
        TIMSK &= ~(1<<TOIE2);
        // Acting like current sensor triggered
        state |= ST_EDGE;
        // Immediate stop without soft spindown
        if (!((state & ST_LEARN) || (state & ST_BACK))) speed = 0;
        state &= ~(ST_SPDUP | ST_HOLD);
    }
}

// Comparator output falling edge
// Occurs when voltage at AIN1 > AIN0 (motor overload)
ISR (ANA_COMP_vect) {
    state |= ST_EDGE;
    speed = 0;
    state &= ~(ST_SPDUP | ST_HOLD);
}

void motor_start () {
    logptr = 0;
    logsize = 0;
    // Set up comparator input pins
    CMP_PORT &= ~((1<<AIN0) | (1<<AIN1));
    CMP_DDR  &= ~((1<<AIN0) | (1<<AIN1));
    // Enable analog comparator
    ACSR &= ~(1<<ACD);
    // Enable comparator multiplexer
    SFIOR &= ~(1<<ACME);
    // Enable interrupt on falling edge
    // Will fire if AIN0 > AIN1 (AIN1 has to be on tunable reference voltage)
    ACSR  |=  (1<<ACIS1);
    // Enable comparator interrupt for overcurrent protection
    ACSR |= (1<<ACIE);
    // output pins for drivers
    MT_DDR |= (1<<MT_UL) | (1<<MT_UR) | (1<<MT_LL) | (1<<MT_LR);
    // Enable speed up and move bits in status
    state |= ST_SPDUP | ST_MOVE;
    state &= ~ST_TMOUT;
    // Timer 0 is for motor speed increase/decrease
    // Spinup time = PWM_RES * ((N*256)/F_CPU), timer0 prescaler N is now 256
    TCCR0 |= (1<<CS02);// | (1<<CS00);
    // Enable timer 0 overflow interrupt
    TIMSK |= (1<<TOIE0);
    // Timer 1 is used for PWM control of the motor
    // Mode 14 - fast PWM with top at ICR1
    TCCR1A = (1<<WGM11);
    TCCR1B = (1<<WGM12) | (1<<WGM13);
    ICR1 = PWM_RES;
    // timer1 prescaler 1
    // F_PWM will be F_CPU/(N*(PWM_RES+1)) = 1000000/64 = 15625 Hz
    TCCR1B |= (1<<CS11);
    // Now raising COM1A1 and COM1B1 in TCCR1A will connect PB1 or PB2 to the timer
    // Duty can be adjusted by changing OCR1A and OCR1B
    // Now start the motor in needed direction
    bridge_update((state & ST_DIR) ? (0<<BR_LR) | (1<<BR_UR) : (0<<BR_LL) | (1<<BR_UL));
    TCCR1A = state & ST_DIR ? TCCR1A | ((1<<COM1A0) | (1<<COM1A1)) : TCCR1A | ((1<<COM1B0) | (1<<COM1B1));
    // Timer 2 is for motor run time limiting (e.g. if stop by current sensor comparator didn't happen)
    duration = max_duration;
    TCCR2 = (1<<CS22) | (1<<CS21) | (1<<CS20);  // Set prescaling to 1/1024 ~ 30.5 overflows/sec
    TIMSK |= (1<<TOIE2);                        // Allow overflow interrupt
}

void learn() {
    uint t = max_duration;
    interval intrv;
    uchar i, j; // counters
    // vars to store measured values during 3-time calibration run
    uint d[3];  // run duration from lower to upper position
    uint h[3];  // hardness
    uchar s[3]; // interval start
    uchar e[3]; // interval end
    uchar v[3]; // max voltage
    max_duration = T_DURATION * 30.5;               // apply default max_duration to detect if the window reaches its margins earlier than this
    for (i=0; i<3; i++) {                           // make the test run in cycle for 3 times
        _delay_ms(500);
        for (j=0; j<REC_LG; j++) journal[j] = 0;    // zero all the log to make max voltage measurement accurate
        state &= ~(ST_DIR | ST_EDGE);               // start moving window down
        motor_start();
        while (!(state & ST_EDGE));                 // wait until it stops
        if (state & ST_TMOUT) {                     // if it has stopped by timeout, abort calibration
            max_duration = t;
            state = 0;
            return;
        }
        _delay_ms(500);
        state |= ST_DIR;                            // start moving window up
        state &= ~ST_EDGE;
        motor_start();
        while (!(state & ST_EDGE));                 // wait until it stops
        if (state & ST_TMOUT) {                     // if it is stopped by timeout, abort calibraiton
            max_duration = t;
            state = 0;
            return;
        }
        d[i] = max_duration - duration;             // store measured duration
        rec_wrap(journal, logptr);                  // filter the log
        rec_filter(journal, 1);
        rec_filter(journal, 0);
        intrv.start = 0;                            // set the interval to full
        intrv.end = REC_LG - 1;
        v[i] = rec_volt_margins(journal, intrv).end;    // and find the maximum motor voltage during run
        intrv = rec_time_margins(journal);          // find the voltage drop margins
        s[i] = intrv.start;                         // ...and store them
        e[i] = intrv.end;
        rec_relative(journal, intrv);               // make the log zero-relative
        h[i] = rec_square(journal, intrv);          // find the "hardness" of window border
    }
    t = median_int(d);
    if (t < T_MIN_DURATION * 30.5) {
        state = 0;
        return;
    }
    t += t/4;
    eeprom_write_word((uint *) 1, t);
    max_duration = t;
    time_int.start = median(s);
    time_int.end = median(e);
    eeprom_write_byte((uchar*) 3, time_int.start);
    eeprom_write_byte((uchar*) 4, time_int.end);
    hardness = median_int(h);
    hardness -= hardness/13; // 14
    eeprom_write_word((uint*) 5, hardness);
    voltage_normal = median(v);
    eeprom_write_byte((uchar*) 7, voltage_normal);
    _delay_ms(500);
    max_duration = t / 3;
    state &= ~(ST_DIR | ST_EDGE);
    motor_start();
    while(state & ST_MOVE);
    max_duration = t;
    state = 0;
}

// must be run after power-up - reads motor-related settings from the EEPROM
void init_motor () {
    uint t;
    uchar start, end;
    // max_duration
    t = eeprom_read_word((uint*) 1);
    max_duration = ((t > (uint) (T_MIN_DURATION * 30.5)) && (t < (uint) (T_DURATION * 30.5))) ? t : T_DURATION * 30.5;
    // voltage drop interval
    start = eeprom_read_byte((uchar*) 3);
    end = eeprom_read_byte((uchar*) 4);
    if (((start == 0) || (start == 0xFF)) || (end == 0xFF)) {
        time_int.start = 0;
        time_int.end = REC_LG-1;
    } else {
        time_int.start = start;
        time_int.end = end;
    }
    // hardness
    t = eeprom_read_word((uint*) 5);
    if (t < 0xFFFF) hardness = t;
    // voltage_normal
    t = eeprom_read_byte((uchar*) 7);
    voltage_normal = ((t > 70) && (t < 200)) ? t : 146;
    // start calibration routine if requested by switch state during power-up
    if (state & ST_LEARN) learn();
    //motor_start();
}
