/*
 * util.c
 *
 * Created on: 22 feb 2019
 *      Author: Borislav Malkov
 */

#include "defines.h"
#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>


void sendchar(uchar c) {
    while (!(UCSRA & (1<<UDRE)));
    UDR = c;
}

void sendstr(char str[]) {
    uchar i = 0;
    while (str[i] != 0) {
        sendchar(str[i]);
        i++;
    }
    sendchar(0x0d);
    sendchar(0x0a);
}

void rec_wrap (uchar j[], uchar pos) {
    uchar i, t;
    while (pos < REC_LG) {
        t = j[REC_LG - 1];
        for (i = REC_LG - 1; i > 0; i--) {
            j[i] = j[i-1];
        }
        j[0] = t;
        pos++;
    }
}

uchar median (uchar w[3]) {
    uchar mid;
    if ((w[0] <= w[1]) && (w[0] <= w[2]))
        mid = (w[1] <= w[2]) ? w[1] : w[2];
    else if ((w[1] <= w[0]) && (w[1] <= w[2]))
        mid = (w[0] <= w[2]) ? w[0] : w[2];
    else
        mid = (w[0] <= w[1]) ? w[0] : w[1];
    return mid;
}

uint median_int (uint w[3]) {
    uint mid;
    if ((w[0] <= w[1]) && (w[0] <= w[2]))
        mid = (w[1] <= w[2]) ? w[1] : w[2];
    else if ((w[1] <= w[0]) && (w[1] <= w[2]))
        mid = (w[0] <= w[2]) ? w[0] : w[2];
    else
        mid = (w[0] <= w[1]) ? w[0] : w[1];
    return mid;
}

uchar average (uchar w[3]) {
    return (w[0] + w[1] + w[2])/3;
}

interval rec_volt_margins(uchar j[], interval intrv) {
    uchar i;
    interval res = { 0xFF, 0 };
    for (i=intrv.start; i <= intrv.end; i++) {
        if (j[i] < res.start) res.start = j[i];
        if (j[i] > res.end) res.end = j[i];
    }
    return res;
}

interval rec_time_margins(uchar j[]) {
    interval res = {0, REC_LG-1};
    uchar i, k, cnt;
    uchar start = 0;
    uchar end = 0;
    for (i=REC_LG-1; i>4; i--) {
        cnt = 0;
        for (k=0; k<5; k++) if (j[i-k-1] - j[i-k] > 2) cnt++;
        if (cnt == 5) {
            end = i;
            break;
        }
    }
    if (end) {
        for (i=end; i>4; i--) {
            cnt = 0;
            for (k=0; k<5; k++) {
                if(j[i-k-1] - j[i-k] < 2) cnt++;
            }
            if (cnt == 5) {
                start = i;
                break;
            }
        }
    }
    if ((start && end) && ((end-start) > 4)) {
        //res.start = start > 0 ? start - 1 : start;
        //res.end = end < REC_LG-1 ? end + 1 : end;
        res.start = start + 1;
        res.end = end -1;
    }
    return res;
}

void rec_relative (uchar j[], interval intrv) {
    uchar i, min;
    min = rec_volt_margins(j, intrv).start;
    for (i = intrv.start; i <= intrv.end; i++) j[i] -= min;
}

void rec_filter (uchar j[], uchar med) {
    uchar w[3];
    uchar i, t, l;
    l = REC_LG - 1;
    for (i = 0; i <= l; i++) {
        w[0] = (i == 0) ? j[i] : j[i-1];
        w[1] = j[i];
        w[2] = (i == l) ? j[i] : j[i+1];
        if (i > 0) j[i-1] = t;
        t = med ? median(w) : average(w);
        if (i == l) j[i] = t;
    }
}

uint rec_square (uchar j[], interval intrv) {
    uchar i;
    uint res = 0;
    for (i=intrv.start; i<intrv.end; i++) res += j[i];
    return res;
}
