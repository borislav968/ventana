/*
 * util.h
 *
 * Created on: 22 feb 2019
 *      Author: Borislav Malkov
 */

#ifndef UTIL_H
#define UTIL_H

uchar median (uchar w[3]);
uint median_int (uint w[3]);
uint min_int (uint w[3]);
void rec_wrap (uchar j[], uchar pos);
void rec_filter (uchar j[], uchar med);
void rec_relative (uchar j[], interval intrv);
interval rec_volt_margins(uchar j[], interval intrv);
interval rec_time_margins(uchar j[]);
uint rec_hardness (uchar j[], interval intrv);
uint rec_hardness_slope (uchar j[], interval intrv);

#endif
