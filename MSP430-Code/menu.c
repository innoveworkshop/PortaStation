/**
 *    Filename: menu.c
 * Description: Handles all the menu related stuff.
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

#include "delay.h"
#include "settings.h"
#include "menu.h"
#include "screens.h"
#include "lcd.h"

uint8_t current_menu = MENU_MAIN;
uint8_t current_menu_item = 0;
bool editing_menu_item = false;
char str[15];

/**
 * Builds the menu list.
 *
 * @param menu Menu ID.
 */
void build_menu(const int8_t menu) {
	uint8_t effect = NORMAL;

	for (uint8_t i = 0; i < menu_num_items[menu]; i++) {
		// Set the line position.
		lcd_set_pos(0, i + 1);

		// Check if the current item is selected.
		if ((i == current_menu_item) && !editing_menu_item) {
			effect = INVERTED;
		} else {
			effect = NORMAL;
		}

		// Print the menu item.
		switch (menu) {
		case MENU_MAIN:
			lcd_print(main_items[i], effect);
			break;
		case MENU_TEMPPRESETS:
			lcd_print(tempset_items[i], effect);

			if (i < NUM_TEMP_PRESETS) {
				// Check if the current item is selected.
				if ((editing_menu_item) && (i == current_menu_item)) {
					effect = UNDERLINED;
				} else {
					effect = NORMAL;
				}

				snprintf(str, sizeof(str), "%d%s",
						 conv_adc_temp(settings.temp_preset[i]),
						 settings.temp_unit_symbol);

				lcd_set_pos(9 * (FONT_WIDTH + 1), i + 1);
				lcd_print(str, effect);
			}
			break;
		case MENU_CALIBRATION:
			lcd_print(calibration_items[i], effect);

			// Print the calibration values.
			if ((i == 1) || (i == 2)) {
				// Check if the current item is selected.
				if ((editing_menu_item) && (i == current_menu_item)) {
					effect = UNDERLINED;
				} else {
					effect = NORMAL;
				}

				snprintf(str, sizeof(str), "%d", settings.cal_var[i - 1]);
				lcd_set_pos(11 * (FONT_WIDTH + 1), i + 1);
				lcd_print(str, effect);
			}
			break;
		case MENU_UNITS:
			lcd_print(units_items[i], effect);
			break;
		}
	}
}

/**
 * Updates the value of the current edited menu item.
 *
 * @param counter Rotary encoder change counter.
 */
void edit_current_menu_item(const int counter) {
	switch (current_menu) {
	case MENU_TEMPPRESETS:
		if ((counter > 0) && (settings.temp_preset[current_menu_item] < MAX_SET_TEMP)) {
			settings.temp_preset[current_menu_item]++;
		} else if ((counter < 0) && (settings.temp_preset[current_menu_item] > MIN_SET_TEMP)) {
			settings.temp_preset[current_menu_item]--;
		}
		break;
	case MENU_CALIBRATION:
		if ((counter > 0) && (settings.cal_var[current_menu_item - 1] < 1023)) {
			settings.cal_var[current_menu_item - 1]++;
		} else if ((counter < 0) && (settings.cal_var[current_menu_item - 1] > 0)) {
			settings.cal_var[current_menu_item - 1]--;
		}
		break;
	}

	// Redraw the menu items.
	build_menu(current_menu);
}

/**
 * Loads a menu screen.
 *
 * @param menu Menu ID (MENU_CURRENT if current screen).
 * @param counter Rotary encoder change counter.
 */
void load_menu_screen(const int8_t menu, const int counter) {
	// Check if the screen changed.
	if ((menu != current_menu) && (menu != MENU_CURRENT)) {
		current_menu = menu;
	}

	// Prepare the screen.
	lcd_clear();
	lcd_print(menu_titles[current_menu], INVERTED);

	// Select the current item.
	if ((counter > 0) && (current_menu_item < (menu_num_items[current_menu] - 1))) {
		current_menu_item++;
	} else if ((counter < 0) && (current_menu_item > 0)) {
		current_menu_item--;
	} else if (counter == 0) {
		current_menu_item = 0;
	}

	// Display the menu items.
	build_menu(current_menu);
}

/**
 * Performs the action reserved for the current item.
 *
 * @param type Type of the action (click or long-press).
 */
void menu_action(const uint8_t type) {
	switch (current_menu) {
	case MENU_MAIN:
		switch (current_menu_item) {
		case 0:
			// Temperature Presets
			load_menu_screen(MENU_TEMPPRESETS, 0);
			break;
		case 1:
			// Calibration
			load_menu_screen(MENU_CALIBRATION, 0);
			break;
		case 2:
			// Units
			load_menu_screen(MENU_UNITS, 0);
			break;
		case 3:
			// About
			change_screen(ABOUT_SCREEN);
			break;
		case 4:
			// Save
			save_next_time = true;
			change_screen(MAIN_SCREEN);
			break;
		}
		break;
	case MENU_TEMPPRESETS:
		switch (current_menu_item) {
		case 0:
		case 1:
		case 2:
		case 3:
			if (editing_menu_item) {
				editing_menu_item = false;
			} else {
				editing_menu_item = true;
			}

			build_menu(current_menu);
			break;
		case 4:
			load_menu_screen(MENU_MAIN, 0);
			break;
		}
		break;
	case MENU_CALIBRATION:
		switch (current_menu_item) {
		case 0:
			change_screen(CALIBRATION_SCREEN);
			break;
		case 1:
		case 2:
			if (editing_menu_item) {
				editing_menu_item = false;
			} else {
				editing_menu_item = true;
			}

			build_menu(current_menu);
			break;
		case 3:
			load_menu_screen(MENU_MAIN, 0);
			break;
		}
		break;
	case MENU_UNITS:
		switch (current_menu_item) {
		case 0:
		case 1:
		case 2:
			set_temp_unit(current_menu_item);
			load_menu_screen(MENU_MAIN, 0);
			break;
		case 3:
			load_menu_screen(MENU_MAIN, 0);
			break;
		}
		break;
	}
}
