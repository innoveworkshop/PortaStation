/**
 *    Filename: settings.h
 * Description: Helps to keep things customizable and fun.
 *  Created on: Jul 17, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdint.h>

// Presets
#define NUM_TEMP_PRESETS 4

// Units
#define CELSIUS    0
#define FAHRENHEIT 1
#define KELVIN     2

// Calibration ADC points.
#define CAL_LOW_TEMP_ADC  565
#define CAL_HIGH_TEMP_ADC 785

// Temperature limits. (using ADC units)
#define MIN_SET_TEMP 490
#define MAX_SET_TEMP 1000

typedef struct {
	int temp_preset[NUM_TEMP_PRESETS];
	int cal_var[2];

	uint8_t temp_unit;
	char temp_unit_symbol[3];

	bool sense_when_off;
	unsigned int last_set_temp;

	float vref;
	float rheater;
	float vin_ratio;
} SettingsData;

// Make it global!
extern SettingsData settings;
extern bool save_next_time;

// Interpolation stuff.
void interpolate_adc_temp(const int temp1, const int temp2,
						  const unsigned int adc1, const unsigned int adc2);
void interpolate_temp_adc(const int temp1, const int temp2,
						  const unsigned int adc1, const unsigned int adc2);
void perform_interpolations();

// ADC and temperature conversions.
int conv_adc_temp(const unsigned int value, const uint8_t unit);
int conv_adc_temp(const unsigned int value);
unsigned int conv_temp_adc(const int temp, const uint8_t unit);
unsigned int conv_temp_adc(const int temp);

// Units stuff.
void set_temp_unit(const uint8_t unit);
char get_temp_unit(const uint8_t unit);

// Memory operations.
void load_default_settings();
void load_settings();
void commit_settings();

#endif /* SETTINGS_H_ */
