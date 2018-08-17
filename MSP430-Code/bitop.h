/**
 *    Filename: bitop.h
 * Description: Some bit and port operation help.
 *  Created on: Jul 22, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#ifndef BITOP_H_
#define BITOP_H_

unsigned int get_bit(char b, unsigned int pos);
void byte_to_pin(char c, unsigned int pos, volatile unsigned char *port,
				 unsigned int pin);
void byte_to_pullup_pin(char c, unsigned int pos, volatile unsigned char *port,
						volatile unsigned char *port_dir, unsigned int pin);

#endif /* BITOP_H_ */
