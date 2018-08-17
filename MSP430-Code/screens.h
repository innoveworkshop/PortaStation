/**
 *    Filename: screens.h
 * Description: Helps with the screen management.
 *  Created on: Jul 16, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#ifndef SCREENS_H_
#define SCREENS_H_

// Screen definitions.
#define SPLASH_SCREEN      0
#define MAIN_SCREEN        1
#define MENU_SCREEN        2
#define CALIBRATION_SCREEN 3
#define CONFIRM_CAL_SCREEN 4
#define RECOVERY_SCREEN    5
#define ABOUT_SCREEN       6

extern uint8_t current_screen;
extern bool screen_setup;

uint8_t change_screen(const uint8_t screen);

void splash_screen();
void recovery_screen();
void about_screen();

#endif /* SCREENS_H_ */
