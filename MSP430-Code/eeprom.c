/**
 *    Filename: eeprom.c
 * Description: Helper for interacting with the 24LC01B EEPROM (USCI_B0).
 *  Created on: Jul 22, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#include "eeprom.h"
#include <msp430g2553.h>
#include <stdint.h>
#include <stdbool.h>

// Helpers
#include "settings.h"
#include "delay.h"
#include "bitop.h"

// Pins
#define CLK BIT6  // P1.6
#define DAT BIT7  // P1.7

#define DEVICE_ADDR 0b1010000
#define MAX_BYTES 4

// Temporary variables.
uint8_t rx_buffer = 0;

/**
 * Initializes the EEPROM stuff.
 */
void eeprom_setup() {
	P1SEL     |= (CLK + DAT);                // Assign I2C pins to USCI_B0.
	P1SEL2    |= (CLK + DAT);
	UCB0CTL1  |= UCSWRST;                    // Enable SW reset.
	UCB0CTL0   = UCMST + UCMODE_3 + UCSYNC;  // I2C Master, synchronous mode.
	UCB0CTL1   = UCSSEL_2 + UCSWRST;         // Use SMCLK, keep SW reset.
	UCB0BR0    = 0b10010000;                 // fSCL = SMCLK/400 = ~40kHz.
	UCB0BR1    = 1;
	UCB0I2CSA  = DEVICE_ADDR;                // Set slave address.
	UCB0CTL1  &= ~UCSWRST;                   // Clear SW reset, resume operation.
	IFG2      &= ~(UCB0TXIFG + UCB0RXIFG);   // Clear the TX and RX interrupt flags.
	UCB0I2CIE |= UCNACKIE;                   // Enable the NACK interrupt flag.
	IE2       |= UCB0TXIE;                   // Enable TX ready interrupt.
	IE2       |= UCB0RXIE;                   // Enable RX interrupt.
}

/**
 * Writes a byte to the EEPROM at a specific address.
 *
 * @param addr Word address.
 * @param data Data byte.
 */
void eeprom_write(const uint8_t addr, const uint8_t data) {
	// Start the transmission.
	UCB0CTL1 |= UCTR + UCTXSTT;       // I2C TX + START condition

	// Send the word address.
	UCB0TXBUF = addr;                 // Put the memory address in the TX buffer.
	__bis_SR_register(CPUOFF + GIE);  // Enter LPM0 with interrupts enabled.

	// Send the data.
	UCB0TXBUF = data;                 // Put the data in the TX buffer.
	__bis_SR_register(CPUOFF + GIE);  // Enter LPM0 with interrupts enabled.

	// End the transmission.
	UCB0CTL1 |= UCTXSTP;              // Send a STOP condition.
	delay_ms(10);                     // Wait the write cycle time.
}

/**
 * Reads a byte to the EEPROM at a specific address.
 *
 * @param addr Word address.
 * @return The data byte.
 */
uint8_t eeprom_read(const uint8_t addr) {
	// Sets the current address for a random read.
	UCB0CTL1 |= UCTR + UCTXSTT;       // I2C TX + START condition
	UCB0TXBUF = addr;                 // Put the memory address in the TX buffer.
	__bis_SR_register(CPUOFF + GIE);  // Enter LPM0 with interrupts enabled.

	// Reads the data.
	UCB0CTL1 &= ~UCTR;                // Sets USCI_B0 for receiving data.
	UCB0CTL1 |= UCTXSTT;              // Generates a START condition.
	while (UCB0CTL1 & UCTXSTT);       // Waits for the ACK from the device.
	UCB0CTL1 |= UCTXNACK;             // Setup a NACK single-byte response.
	UCB0CTL1 |= UCTXSTP;              // Send a STOP condition.
	__bis_SR_register(CPUOFF + GIE);  // Enter LPM0 with interrupts enabled.
	while (UCB0CTL1 & UCTXSTP);       // Ensure the STOP condition finished.

	return rx_buffer;
}

/**
 * USCI_B0 data interrupt service routine.
 */
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void) {
	if (IFG2 & UCB0TXIFG) {
		// Clear the TX interrupt flag.
		IFG2 &= ~UCB0TXIFG;
	} else if (IFG2 & UCB0RXIFG) {
		rx_buffer = UCB0RXBUF;  // Reads the RX buffer into memory.
		IFG2 &= ~UCB0RXIFG;     // Clear the RX interrupt flag.
	}

	__bic_SR_register_on_exit(CPUOFF);  // Return to active mode.
}

/**
 * USCI_B0 state interrupt service routine.
 */
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void) {
	if (UCB0STAT & UCNACKIFG) {
		__bic_SR_register_on_exit(CPUOFF);  // Return to active mode.
		UCB0CTL1 |= UCTXSTP;                // Send a STOP condition.
	}

	// Clear the NACK interrupt flag
	UCB0STAT &= ~UCNACKIFG;
}
