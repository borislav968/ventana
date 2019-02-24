/*
 * driver.h
 *
 * Created on: 19 дек. 2018 г.
 *      Author: Borislav Malkov
 */


#ifndef DRIVER_H
#define DRIVER_H

extern volatile uchar bridge;

void bridge_update ();
void bridge_chk();
uchar out_voltage();

#endif
