/**
 *    Filename: settings.c
 * Description: Helps to keep things customizable and fun.
 *  Created on: Jul 17, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#include <msp430g2553.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "settings.h"
#include "delay.h"
#include "eeprom.h"

// Settings memory positions.
#define MCAL_VAR1H      0
#define MCAL_VAR1L      1
#define MCAL_VAR2H      2
#define MCAL_VAR2L      3
#define MTEMP_UNIT      4
#define MLAST_SET_TEMPH 5
#define MLAST_SET_TEMPL 6
#define MSENSEWHENOFF   7
#define MTEMP_PRESETH   8
#define MTEMP_PRESETL   9

// Global variables.
SettingsData settings;
bool save_next_time = false;
float adc_temp_params[2] = { 0.0, 0.0 };
float temp_adc_params[2] = { 0.0, 0.0 };

/**
 * Loads the settings from the EEPROM into the settings variable.
 */
void load_settings() {
	// Calibration variables.
	settings.cal_var[0] = (eeprom_read(MCAL_VAR1H) << 8) + eeprom_read(MCAL_VAR1L);
	settings.cal_var[1] = (eeprom_read(MCAL_VAR2H) << 8) + eeprom_read(MCAL_VAR2L);

	// Perform the necessary interpolations.
	perform_interpolations();

	// Temperature stuff.
	settings.temp_unit = eeprom_read(MTEMP_UNIT);
	settings.temp_unit_symbol[0] = 0x7f;
	settings.temp_unit_symbol[1] = get_temp_unit(settings.temp_unit);
	settings.temp_unit_symbol[2] = '\0';

	// Last set temperature and sense setting.
	settings.last_set_temp = (eeprom_read(MLAST_SET_TEMPH) << 8) + eeprom_read(MLAST_SET_TEMPL);
	settings.sense_when_off = eeprom_read(MSENSEWHENOFF);

	// Constants.
	settings.vref = 3.253;
	settings.rheater = 12.36;
	settings.vin_ratio = 0.0929735;

/*	// Voltage reference value.
	settings.vref = (eeprom_read(MVREF_F3) << 24) + (eeprom_read(MVREF_F2) << 16) +
			(eeprom_read(MVREF_F1) << 8) + eeprom_read(MVREF_F0);

	// Heater resistance value.
	settings.rheater = (eeprom_read(MRHEATER_F3) << 24) + (eeprom_read(MRHEATER_F2) << 16) +
			(eeprom_read(MRHEATER_F1) << 8) + eeprom_read(MRHEATER_F0);

	// Input voltage ratio.
	settings.vin_ratio = (eeprom_read(MVIN_RATIO_F3) << 24) + (eeprom_read(MVIN_RATIO_F2) << 16) +
			(eeprom_read(MVIN_RATIO_F1) << 8) + eeprom_read(MVIN_RATIO_F0);*/

	// Temperature presets.
	for (uint8_t i = 0; i < NUM_TEMP_PRESETS; i++) {
		settings.temp_preset[i] = (eeprom_read(MTEMP_PRESETH + (i * 2)) << 8) +
								  eeprom_read(MTEMP_PRESETL + (i * 2));
	}
}

/**
 * Loads the default settings into the settings variable.
 */
void load_default_settings() {
	// Calibration variables.
	settings.cal_var[0] = 270;
	settings.cal_var[1] = 415;

	// Perform the necessary interpolations.
	perform_interpolations();

	// Temperature presets.
	settings.temp_preset[0] = conv_temp_adc(300, CELSIUS);
	settings.temp_preset[1] = conv_temp_adc(320, CELSIUS);
	settings.temp_preset[2] = conv_temp_adc(350, CELSIUS);
	settings.temp_preset[3] = conv_temp_adc(425, CELSIUS);

	// Temperature stuff.
	settings.temp_unit = CELSIUS;
	settings.temp_unit_symbol[0] = 0x7f;
	settings.temp_unit_symbol[1] = 'C';
	settings.temp_unit_symbol[2] = '\0';

	// Last set temperature and sense setting.
	settings.last_set_temp = conv_temp_adc(250);
	settings.sense_when_off = 1;

	// Constants.
	settings.vref = 3.253;
	settings.rheater = 12.36;
	settings.vin_ratio = 0.0929735;
}

/**
 * Commits the settings to a permanent storage. Never use this when in the menu
 * screen, for some reason it corrupts the memory and does all sorts of crazy
 * shit, don't ask me why.
 */
void commit_settings() {
	eeprom_write(MCAL_VAR1H, settings.cal_var[0] >> 8);
	eeprom_write(MCAL_VAR1L, settings.cal_var[0] & 0xFF);
	eeprom_write(MCAL_VAR2H, settings.cal_var[1] >> 8);
	eeprom_write(MCAL_VAR2L, settings.cal_var[1] & 0xFF);
	eeprom_write(MTEMP_UNIT, settings.temp_unit);
	eeprom_write(MLAST_SET_TEMPH, settings.last_set_temp >> 8);
	eeprom_write(MLAST_SET_TEMPL, settings.last_set_temp & 0xFF);
	eeprom_write(MSENSEWHENOFF, settings.sense_when_off);

	/*eeprom_write(MVREF_F3, (settings.vref & 0xFF000000) >> 24);
	eeprom_write(MVREF_F2, (settings.vref & 0x00FF0000) >> 16);
	eeprom_write(MVREF_F1, (settings.vref & 0x0000FF00) >> 8);
	eeprom_write(MVREF_F0, settings.vref & 0x000000FF);

	eeprom_write(MRHEATER_F3, (settings.rheater & 0xFF000000) >> 24);
	eeprom_write(MRHEATER_F2, (settings.rheater & 0x00FF0000) >> 16);
	eeprom_write(MRHEATER_F1, (settings.rheater & 0x0000FF00) >> 8);
	eeprom_write(MRHEATER_F0, settings.rheater & 0x000000FF);

	eeprom_write(MVIN_RATIO_F3, (settings.vin_ratio & 0xFF000000) >> 24);
	eeprom_write(MVIN_RATIO_F2, (settings.vin_ratio & 0x00FF0000) >> 16);
	eeprom_write(MVIN_RATIO_F1, (settings.vin_ratio & 0x0000FF00) >> 8);
	eeprom_write(MVIN_RATIO_F0, settings.vin_ratio & 0x000000FF);*/

	for (uint8_t i = 0; i < NUM_TEMP_PRESETS; i++) {
		eeprom_write(MTEMP_PRESETH + (i * 2), settings.temp_preset[i] >> 8);
		eeprom_write(MTEMP_PRESETL + (i * 2), settings.temp_preset[i] & 0xFF);
	}
}

/**
 * Interpolates the function for getting a temperature from the ADC value.
 *
 * @param temp1 First temperature measured.
 * @param temp2 Second temperature measured.
 * @param adc1 First ADC reference value.
 * @param adc2 Second ADC reference value.
 */
void interpolate_adc_temp(const int temp1, const int temp2,
						  const unsigned int adc1, const unsigned int adc2) {
	adc_temp_params[0] = (float)(temp2 - temp1) / (float)(adc2 - adc1);
	adc_temp_params[1] = temp1 - (adc_temp_params[0] * (float)adc1);
}

/**
 * Interpolates the function for getting a ADC value from a temperature.
 *
 * @param temp1 First temperature measured.
 * @param temp2 Second temperature measured.
 * @param adc1 First ADC reference value.
 * @param adc2 Second ADC reference value.
 */
void interpolate_temp_adc(const int temp1, const int temp2,
						  const unsigned int adc1, const unsigned int adc2) {
	temp_adc_params[0] = (float)(adc2 - adc1) / (float)(temp2 - temp1);
	temp_adc_params[1] = adc1 - (temp_adc_params[0] * (float)temp1);
}

/**
 * Performs all the interpolations using the variables in the settings.
 */
void perform_interpolations() {
	interpolate_adc_temp(settings.cal_var[0], settings.cal_var[1],//cal_temp[0], cal_temp[1],
						 CAL_LOW_TEMP_ADC, CAL_HIGH_TEMP_ADC);
	interpolate_temp_adc(settings.cal_var[0], settings.cal_var[1],//cal_temp[0], cal_temp[1],
						 CAL_LOW_TEMP_ADC, CAL_HIGH_TEMP_ADC);
}

/**
 * Converts a ADC value to a temperature.
 *
 * @param value ADC value.
 * @return Temperature.
 */
int conv_adc_temp(const unsigned int value) {
	float temp = conv_adc_temp(value, settings.temp_unit);
	return (int)temp;
}

/**
 * Converts a ADC value to a temperature.
 *
 * @param value ADC value.
 * @return Temperature.
 */
int conv_adc_temp(const unsigned int value, const uint8_t unit) {
	float temp = (adc_temp_params[0] * value) + adc_temp_params[1]; //(0.4525 * value) - 5.4299;  // YAY! MAGIC NUMBERS!!

	switch (unit) {
	case FAHRENHEIT:
		temp = (1.8 * temp) + 32;
		break;
	case KELVIN:
		temp += 273.15;
		break;
	}

	return (int)temp;
}

/**
 * Converts a temperature to a ADC value using the unit in the settings.
 *
 * @param temp Temperature value.
 * @return ADC value.
 */
unsigned int conv_temp_adc(const int temp) {
	return conv_temp_adc(temp, settings.temp_unit);
}

/**
 * Converts a temperature to a ADC value using the unit provided.
 *
 * @param temp Temperature value.
 * @param unit Temperature unit ID.
 *
 * @return ADC value.
 */
unsigned int conv_temp_adc(const int temp, const uint8_t unit) {
	float val = (float)temp;

	switch (unit) {
	case FAHRENHEIT:
		val = (val - 32) / 1.8;
		break;
	case KELVIN:
		val -= 273.15;
		break;
	}

	val = (temp_adc_params[0] * val) + temp_adc_params[1]; //(2.21 * val) + 12;
	return (unsigned int)val;
}

/**
 * Sets temperature units.
 *
 * @param unit Temperature unit ID.
 */
void set_temp_unit(const uint8_t unit) {
	settings.temp_unit = unit;

	switch (unit) {
	case CELSIUS:
		settings.temp_unit_symbol[1] = 'C';
		break;
	case FAHRENHEIT:
		settings.temp_unit_symbol[1] = 'F';
		break;
	case KELVIN:
		settings.temp_unit_symbol[1] = 'K';
		break;
	}
}

/**
 * Gets a temperature unit for printing.
 *
 * @param unit Temperature unit ID.
 * @return Temperature unit character.
 */
char get_temp_unit(const uint8_t unit) {
	char c;

	switch (unit) {
	case CELSIUS:
		c = 'C';
		break;
	case FAHRENHEIT:
		c = 'F';
		break;
	case KELVIN:
		c = 'K';
		break;
	}

	return c;
}
