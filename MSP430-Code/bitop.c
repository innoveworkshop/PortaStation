/**
 *    Filename: bitop.c
 * Description: Some bit and port operation help.
 *  Created on: Jul 22, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#include "bitop.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * Get a single bit from a byte.
 *
 * @param b A byte.
 * @param pos The bit position to be extracted.
 * @return A bit.
 */
unsigned int get_bit(char b, unsigned int pos) {
	return (b & (1 << pos));
}

/**
 * Puts the desired byte into a pin.
 *
 * @param c The character.
 * @param pos Bit position.
 * @param port Pin port.
 * @param pin The pin to be set.
 */
void byte_to_pin(char c, unsigned int pos, volatile unsigned char *port,
				 unsigned int pin) {
	if (get_bit(c, pos)) {
		*port |= pin;
	} else {
		*port &= ~pin;
	}
}

/**
 * Puts the desired byte into a pin.
 *
 * @param c The character.
 * @param pos Bit position.
 * @param port Pin port.
 * @param port_dir Pin port setup.
 * @param pin The pin to be set.
 */
void byte_to_pullup_pin(char c, unsigned int pos, volatile unsigned char *port,
						volatile unsigned char *port_dir, unsigned int pin) {
	if (get_bit(c, pos)) {
		*port_dir &= ~pin;
	} else {
		*port_dir |= pin;
		*port &= ~pin;
	}
}
