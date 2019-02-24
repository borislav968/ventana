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
#include <stdlib.h>
#include <stdio.h>
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

char buffer[50];

record journal;
uchar logptr = 0;
uchar logsize = 0;
interval time_int = {0, REC_LG-1};
uint hardness = 0;



void savelog (uchar v) {
    //itoa(v, buffer, 10);
    //sendstr(buffer);
    journal[logptr] = v;
    logptr = (logptr + 1 + REC_LG) % REC_LG;
    if (logsize < REC_LG) logsize++;
}

void showlog (uchar j[], uchar start, uchar end) {
    uchar i;
    char b[20];
    for (i = start; i <= end; i++) {
        itoa(j[i], b, 10);
        sendstr(b);
    }
}

void analyze (uchar j[]) {
    //interval intrv;
    uint h;
    if (logsize < (REC_LG - time_int.start) || ((max_duration - duration) < 20)) return;

    //rec_wrap(journal, logptr);
    //rec_filter(journal, 1);
    //rec_filter(journal, 0);
    //rec_relative(journal);

    //sendstr("");
    //sendstr("0: Volt log raw dta:");
    //showlog(journal, 0, REC_LG-1);

    //sendstr("");
    //sendstr("1: Wrap to ptr:");
    rec_wrap(journal, logptr);
    //showlog(j, 0, REC_LG-1);

    //sendstr("");
    //sendstr("2: Median fltr :");
    rec_filter(journal, 1);
    //showlog(j, 0, REC_LG-1);

    //sendstr("");
    //sendstr("3: Avg fltr :");
    rec_filter(journal, 0);
    //showlog(j, 0, REC_LG-1);

    //sendstr("");
    //sprintf(buffer, "4: Volt mrgns: min=%u, max=%u", rec_volt_margins(j, time_int).start, rec_volt_margins(j, time_int).end);
    //sendstr(buffer);

    //sendstr("");
    //sendstr("3: Rltive data:");
    rec_relative(journal, time_int);
    //showlog(j, 0, REC_LG-1);

    //sendstr("");
    //intrv = rec_time_margins(journal);
    //sprintf(buffer, "4: Tme mrgns: start=%u, end=%u", intrv.start, intrv.end);
    //sendstr(buffer);
    //sprintf(buffer, "Svd mrgns: start=%u, end=%u", time_int.start, time_int.end);
    //sendstr(buffer);


    sendstr("");
    sendstr("5: Final data:");
    showlog(journal, time_int.start, time_int.end);

    h = rec_square(j, time_int);
    sendstr("");
    //sprintf(buffer, "6: Hardness: %u", rec_square(j, time_int));
    itoa(h, buffer, 10);
    sendstr(buffer);
    if (h < hardness) {
        sendstr("Rollback");
        state |= ST_BACK;
    }
    else sendstr("OK");

    sendstr("Anlsis end");

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
        sendstr("Motor stopped");
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
        //speed = 0;
        state &= ~(ST_SPDUP | ST_HOLD);
        sendstr("Motor run timeout");
    }
}

// Comparator output falling edge
// Occurs when voltage at AIN1 > AIN0 (motor overload)
ISR (ANA_COMP_vect) {
    state |= ST_EDGE;
    speed = 0;
    state &= ~(ST_SPDUP | ST_HOLD);
    sendstr("Current sensor");
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
    sendstr(state & ST_DIR ? "Mvng up" : "Mvng down");
}

void learn() {
    uint t = max_duration;
    interval intrv;
    uchar i;

    uint d[3], h[3];
    uchar s[3], e[3];

    sendstr("Learn proc rquestd");
    max_duration = T_DURATION * 30.5;
    for (i=0; i<3; i++) {
        _delay_ms(500);
        sendstr("1. Mv all the way dn");
        state &= ~(ST_DIR | ST_EDGE);
        motor_start();
        while (!(state & ST_EDGE));
        if (state & ST_TMOUT) {
            sendstr("Didn't rch lwr edge - abrtng");
            max_duration = t;
            state = 0;
            return;
        }
        _delay_ms(500);
        sendstr("2. Mv all the way up");
        state |= ST_DIR;
        state &= ~ST_EDGE;
        motor_start();
        while (!(state & ST_EDGE));
        if (state & ST_TMOUT) {
            sendstr("Didn't rch uppr edge - abrtng");
            max_duration = t;
            state = 0;
            return;
        }
        d[i] = max_duration - duration;
        rec_wrap(journal, logptr);
        rec_filter(journal, 1);
        rec_filter(journal, 0);
        intrv = rec_time_margins(journal);
        s[i] = intrv.start;
        e[i] = intrv.end;
        rec_relative(journal, intrv);
        h[i] = rec_square(journal, intrv);
    }

    t = median_int(d);
    sprintf(buffer, "Msrd run time: %u tmr ovf, or %us", t, (uint)(t/30.5));
    sendstr(buffer);
    if (t < T_MIN_DURATION * 30.5) {
        sprintf(buffer, "this is lower than %us, aborting", T_MIN_DURATION);
        sendstr(buffer);
        state = 0;
        return;
    }
    t += t/4;
    sprintf(buffer, "Added 1/4 of spare time, now max_duration is %u or %us", t, (uint)(t/30.5));
    sendstr(buffer);
    sendstr("3. Save new value");
    eeprom_write_word((uint *) 1, t);
    max_duration = t;

    time_int.start = median(s);
    time_int.end = median(e);

    sprintf(buffer, "Msrd vltge drop time itrvl: %u %u", time_int.start, time_int.end);
    sendstr(buffer);

    sendstr("Save values");
    eeprom_write_byte((uchar*) 3, time_int.start);
    eeprom_write_byte((uchar*) 4, time_int.end);

    hardness = median_int(h);
    sprintf(buffer, "Msrd hardness: %u", hardness);
    sendstr(buffer);
    hardness -= hardness/10;
    sprintf(buffer, "Save value: %u", hardness);
    sendstr(buffer);
    eeprom_write_word((uint*) 5, hardness);

    _delay_ms(500);
    sendstr("4. Move halfway down");
    max_duration = t / 3;
    state &= ~(ST_DIR | ST_EDGE);
    motor_start();
    while(state & ST_MOVE);
    max_duration = t;
    state = 0;
    sendstr("Learning finished");
}

void init_motor () {
    uint t;
    uchar start, end;
    //sendstr("Init motor");
    t = eeprom_read_word((uint*) 1);
    //sprintf(buffer, "Read EEPROM max_duration value: %u", t);
    //sendstr(buffer);
    max_duration = ((t > (uint) (T_MIN_DURATION * 30.5)) && (t < (uint) (T_DURATION * 30.5))) ? t : T_DURATION * 30.5;
    //sprintf(buffer, "Applied value: %u", max_duration);
    //sendstr(buffer);

    start = eeprom_read_byte((uchar*) 3);
    end = eeprom_read_byte((uchar*) 4);
    //sprintf(buffer, "Read EEPROM drop interval data: start=%u, end=%u", start, end);
    //sendstr(buffer);
    if (((start == 0) || (start == 0xFF)) || (end == 0xFF)) {
        time_int.start = 0;
        time_int.end = REC_LG-1;
    } else {
        time_int.start = start;
        time_int.end = end;
    }
    //sprintf(buffer, "Applied values: %u %u", time_int.start, time_int.end);
    //sendstr(buffer);

    t = eeprom_read_word((uint*) 5);
    //sprintf(buffer, "Read hardness: %u", t);
    //sendstr(buffer);
    if (t < 0xFFFF) hardness = t;
    //sprintf(buffer, "Applied vaue: %u", hardness);

    if (state & ST_LEARN) learn();
}
