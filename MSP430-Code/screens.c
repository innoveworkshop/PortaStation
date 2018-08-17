/**
 *    Filename: screens.c
 * Description: Helps with the screen management.
 *  Created on: Jul 16, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#include <msp430g2553.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "screens.h"
#include "lcd.h"
#include "version.h"

uint8_t current_screen = SPLASH_SCREEN;
bool screen_setup = true;

/**
 * Changes into a new screen.
 *
 * @param screen New screen ID.
 * @return New screen ID.
 */
uint8_t change_screen(const uint8_t screen) {
	TA0CCR1 = 0;  // Turn off the heater.
	lcd_clear();  // Clear the screen for a new one to be drawn.

	current_screen = screen;
	screen_setup = true;

	return current_screen;
}

/**
 * Displays the splash screen of the device.
 */
void splash_screen() {
	char str[15];

	lcd_print("  Porta");
	lcd_set_pos(0, 1);
	lcd_print("     Station");

	lcd_set_pos(0, 3);
	lcd_print("    Innove");
	lcd_set_pos(0, 4);
	lcd_print("   Workshop");

	snprintf(str, sizeof(str), "          v%s", VERSION);
	lcd_set_pos(0, 5);
	lcd_print(str);
}

/**
 * Displays the recovery screen.
 */
void recovery_screen() {
	// Print inverted title.
	lcd_print("   Recovery   ", INVERTED);

	// Print text.
	lcd_set_pos(0, 1);
	lcd_print("Do you wish to");
	lcd_set_pos(0, 2);
	lcd_print("reset all the");
	lcd_set_pos(0, 3);
	lcd_print("settings?");

	// Print OK.
	lcd_set_pos(0, 5);
	lcd_print("     ");
	lcd_print(" OK ", INVERTED);
}

void about_screen() {
	char str[15];

	snprintf(str, sizeof(str), "HW Rev.: %c", HW_REVISION);
	lcd_set_pos(0, 0);
	lcd_print(str);

	snprintf(str, sizeof(str), "SW Ver.: %s", VERSION);
	lcd_set_pos(0, 1);
	lcd_print(str);

	snprintf(str, sizeof(str), "Build: %s", BUILD_NUM);
	lcd_set_pos(0, 2);
	lcd_print(str);

	lcd_set_pos(0, 3);
	lcd_print(" PortaStation ");

	lcd_set_pos(0, 4);
	lcd_print("Made with love");
	lcd_set_pos(0, 5);
	lcd_print("by IWC in 2017");
}
