/**
 *    Filename: delay.c
 * Description: Simple delay functions to make things easier on us.
 *  Created on: Jul 5, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#include <msp430.h>
#include "delay.h"

#define FREQ 16  // MHz

/**
 * Delay by some milliseconds.
 *
 * @param ms number of milliseconds to delay.
 */
void delay_ms(unsigned int ms) {
	while (ms--) {
		__delay_cycles(1000 * FREQ);
	}
}

/**
 * Delay by some microseconds.
 *
 * @param ms number of microseconds to delay.
 */
void delay_us(unsigned int us) {
	while (us--) {
		__delay_cycles(1 * FREQ);
	}
}
