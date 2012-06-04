#ifndef TOUCHPAD_H
#define TOUCHPAD_H

#include "ch.h"
#include "hal.h"

#define CONVERSIONS 3

#define SET_CS(a)		(TP_PORT->BSRR = 1 << (TP_CS + (a ? 0 : 16)))

volatile struct cal {
	int16_t xm;
	int16_t ym;
	int16_t xn;
	int16_t yn;
};

void tpInit(void);
uint8_t tpIRQ(void);
uint16_t tpReadX(void);
uint16_t tpReadY(void);
void tpCalibrate(void);
void tpCalibrate2(void);
void tpDrawCross(uint16_t x, uint16_t y);

#endif
