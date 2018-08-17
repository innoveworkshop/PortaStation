/**
 *    Filename: lcd.c
 * Description: PCD8544 (Nokia 5110) LCD driver library.
 *  Created on: Jul 5, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#include "lcd.h"
#include <msp430g2553.h>
#include <stdint.h>
#include <stdbool.h>

// Helpers.
#include "delay.h"
#include "bitop.h"

// Pins.
#define SCLK BIT0  // P2.0
#define MOSI BIT1  // P2.1
#define D_C  BIT2  // P2.2
#define EN   BIT3  // P2.3
#define RST  BIT7  // P2.7

// Properties.
#define LCDWIDTH  84
#define LCDHEIGHT 48
#define LCDAREA   LCDWIDTH * LCDHEIGHT

#ifdef ROTATION_ENABLE
uint8_t pos_x = PCD8544_WIDTH;
uint8_t pos_y = PCD8544_HEIGHT;
#endif

/**
 *  Setup the pins for communication with the LCD driver.
 */
void lcd_setup() {
	P2SEL &= ~(RST);
	P2SEL2 &= ~(RST);
	P2DIR |= (SCLK + MOSI + D_C + EN + RST);

	// Just don't mess with the magic delays OK?
	P2OUT |= RST;
	delay_ms(10);
	P2OUT &= ~(SCLK + MOSI + D_C + RST);
	delay_ms(20);
	P2OUT |= EN + RST;
}

/**
 *  Initializes the LCD with some defaults.
 */
void lcd_init() {
	lcd_command(PCD8544_FUNCTIONSET | PCD8544_EXTINSTRUCTIONS, 0);
	lcd_command(PCD8544_SETVOP | 59, 0);  // 0x3F  75
	lcd_command(PCD8544_SETTEMP | 0x02, 0);
	lcd_command(PCD8544_SETBIAS | 0x04, 0);  // 0x04 0b111
	lcd_command(PCD8544_FUNCTIONSET, 0);
	lcd_command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL, 0);
}

/**
 *  Send a command to the LCD controller. (Simulating SPI)
 *
 *  @param command A command to send.
 *  @param data Some data to be sent.
 */
void lcd_command(const char command, const char data) {
	// Combine both into one byte to be sent.
	char dbyte = (command | data);

	P2OUT &= ~SCLK;  // Put the clock line LOW to start.
	P2OUT &= ~EN;    // Pull the EN pin LOW to start sending a packet.

	// The packet must be most significant bit first, so we need to send the
	// data from bits 7 to 0.
	for (int i = 7; i >= 0; i--) {
		// Check if it's time to set the Data pin and if it'll be needed.
		if ((i == 0) & (command == 0)) {
			P2OUT |= D_C;
		}

		// Set MOSI according to bit.
		byte_to_pin(dbyte, i, &P2OUT, MOSI);

		// Send a clock pulse.
		P2OUT |= SCLK;   // Set clock HIGH to send the bit.
		P2OUT &= ~SCLK;  // Set the clock back to LOW and prepare for the next bit.
	}

	// Finish the packet and clean the mess.
	P2OUT |= EN;
	P2OUT &= ~(SCLK + MOSI + D_C);
}

/**
 *  Prints a character on the screen.
 *
 *  @param c A character.
 */
void lcd_putc(const char c) {
#ifdef ROTATION_ENABLE
	// Move the cursor 6 columns to make space for the letter.
	pos_x -= 6;
	lcd_command(PCD8544_SETXADDR, pos_x);

	// Send a blank column to separate from the next character.
	lcd_command(0, 0);

	// Print each of the 5 columns of pixels in the font.
	for (int8_t i = FONT_WIDTH - 1; i >= 0; i--) {
		// Reset the Y position just in case it moved.
		lcd_command(PCD8544_SETYADDR, pos_y);

		// Send the actual column.
		lcd_command(0, font[c - 0x20][i]);
	}
#else
	for (int8_t i = 0; i < FONT_WIDTH; i++) {
		lcd_command(0, font[c - 0x20][i]);
	}

	lcd_command(0, 0);
#endif
}

/**
 *  Prints a character on the screen.
 *
 *  @param c A character.
 *  @param effect Font effect.
 */
void lcd_putc(const char c, uint8_t effect) {
#ifdef ROTATION_ENABLE
	// Move the cursor 6 columns to make space for the letter.
	pos_x -= 6;
	lcd_command(PCD8544_SETXADDR, pos_x);

	switch (effect) {
	case INVERTED:
		lcd_command(0, 0xff);
		break;
	case UNDERLINED:
		lcd_command(0, 0b00000001);
		break;
	default:
		// Send a blank column to separate from the next character.
		lcd_command(0, 0);
		break;
	}

	// Print each of the 5 collumns of pixels in the font.
	for (int8_t i = FONT_WIDTH - 1; i >= 0; i--) {
		// Reset the Y position just in case it moved.
		lcd_command(PCD8544_SETYADDR, pos_y);

		// Send the actual column.
		switch (effect) {
		case INVERTED:
			lcd_command(0, ~(font[c - 0x20][i]));
			break;
		case UNDERLINED:
			lcd_command(0, font[c - 0x20][i] ^ 0b00000001);
			break;
		default:
			lcd_command(0, font[c - 0x20][i]);
			break;
		}
	}
#else
	for (int8_t i = 0; i < 5; i++) {
		switch (effect) {
		case INVERTED:
			lcd_command(0, ~(font[c - 0x20][i]));
			break;
		case UNDERLINED:
			lcd_command(0, font[c - 0x20][i] ^ 0b10000000);
			break;
		default:
			lcd_command(0, font[c - 0x20][i]);
			break;
		}
	}

	switch (effect) {
	case INVERTED:
		lcd_command(0, 0xff);
		break;
	case UNDERLINED:
		lcd_command(0, 0b10000000);
		break;
	default:
		lcd_command(0, 0);
		break;
	}
#endif
}

/**
 *  Prints a string on the screen.
 *
 *  @param string A string of characters.
 */
void lcd_print(const char *string) {
	while (*string) {
		lcd_putc(*string++);
	}
}

/**
 *  Prints a string on the screen with a effect.
 *
 *  @param string A string of characters.
 *  @param effect Font effect.
 */
void lcd_print(const char *string, uint8_t effect) {
	while (*string) {
		lcd_putc(*string++, effect);
	}
}

/**
 *  Clears the screen
 */
void lcd_clear() {
	// Start from (0,0).
	lcd_set_pos(0, 0);

	// Fill the whole screen with blank pixels.
	for (unsigned int i = 0; i < (LCDWIDTH * (LCDHEIGHT / 8)); i++) {
		lcd_command(0, 0);
	}

	// Go back to (0,0).
	lcd_set_pos(0, 0);
}

/**
 *  Clears a row of the display.
 *
 *  @param row A display row (Y address).
 */
void lcd_clear_row(unsigned int row) {
#ifdef ROTATION_ENABLE
	// Start from the beginning of the row.
	lcd_set_pos(0, PCD8544_HEIGHT - row);

	// Fill the row with blank pixels.
	for (unsigned int i = 0; i < LCDWIDTH; i++) {
		lcd_command(0, 0);
	}

	// Go back to where everything started.
	lcd_set_pos(0, PCD8544_HEIGHT - row);
#else
	lcd_set_pos(0, row);

	for (unsigned int i = 0; i < LCDWIDTH; i++) {
		lcd_command(0, 0);
	}

	lcd_set_pos(0, row);
#endif
}

/**
 *  Sets the position of the memory cursor in the LCD controller.
 *
 *  @param x The X position (0 >= X <= 83).
 *  @param y The Y position (0 >= Y <= 5).
 */
void lcd_set_pos(unsigned int x, unsigned int y) {
	// Pretty straight forward huh?
#ifdef ROTATION_ENABLE
	pos_x = PCD8544_WIDTH - x;
	pos_y = PCD8544_HEIGHT - y;

	if (pos_x > PCD8544_WIDTH) {
		pos_x = PCD8544_WIDTH;
	}

	if (pos_y > PCD8544_HEIGHT) {
		pos_y = PCD8544_HEIGHT;
	}

	lcd_command(PCD8544_SETXADDR, pos_x);
	lcd_command(PCD8544_SETYADDR, pos_y);
#else
	lcd_command(PCD8544_SETXADDR, x);
	lcd_command(PCD8544_SETYADDR, y);
#endif
}
