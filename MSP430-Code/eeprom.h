/**
 *    Filename: eeprom.h
 * Description: Helper for interacting with the 24LC01B EEPROM (USCI_B0).
 *  Created on: Jul 22, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

void eeprom_setup();
void eeprom_write(const uint8_t addr, const uint8_t data);
uint8_t eeprom_read(const uint8_t addr);

#endif /* EEPROM_H_ */
