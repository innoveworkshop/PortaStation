/**
 *    Filename: menu.h
 * Description: Handles all the menu related stuff.
 *  Created on: Jul 16, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#ifndef MENU_H_
#define MENU_H_

#include <stdint.h>

#define MENU_CURRENT    -1
#define MENU_MAIN        0
#define MENU_TEMPPRESETS 1
#define MENU_CALIBRATION 2
#define MENU_UNITS       3

#define ACTION_CLICK     0
#define ACTION_LONGPRESS 1

//extern uint8_t current_menu;
//extern uint8_t current_menu_item;
extern bool editing_menu_item;

void load_menu_screen(const int8_t menu, const int counter);
void menu_action(const uint8_t type);
void edit_current_menu_item(const int counter);

// Number of items in each menu.
static const uint8_t menu_num_items[] = { 5, 5, 4, 4 };

// Menu titles.
static const char menu_titles[][15] = {
	"   Settings   ",
	" Temp Presets ",
	"  Calibration ",
	"     Units    "
};

// Main menu items.
static const char main_items[][15] = {  // 14 characters + \0
	"Temp. Presets",
	"Calibration",
	"Units",
	"About",
	"Save"
};

// Temperature presets menu items.
static const char tempset_items[][15] = {
	"Preset 1",
	"Preset 2",
	"Preset 3",
	"Preset 4",
	"Back"
};

// Calibration menu items.
static const char calibration_items[][15] = {
	"Cal. Wizard",
	"Var. 1",
	"Var. 2",
	"Back"
};

// Units menu items.
static const char units_items[][15] = {
	"Celsius",
	"Fahrenheit",
	"Kelvin",
	"Back"
};

#endif /* MENU_H_ */
