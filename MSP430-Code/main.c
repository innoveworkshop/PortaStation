/**
 * Portable Soldering Station
 * This is the most awesome soldering station ever, and it's totally portable.
 *
 *    Filename: main.c
 * Description: The main file.
 *  Created on: Jul 5, 2017
 *      Author: Nathan Campos <nathan@innoveworkshop.com>
 *
 * Copyright (C) 2017 Innove Workshop - All Rights Reserved
 */

// Port 1
#define VISENSE BIT1  // P1.1
#define HEATER  BIT2  // P1.2
#define SENSOR  BIT3  // P1.3
#define SWITCH  BIT4  // P1.4

// Port 2
#define RE_A BIT4  // P2.4
#define RE_B BIT5  // P2.5

// Constants
#define ADC_CONVS   4
#define AVG_TIMES   10
#define ADC_VISENSE 2
#define ADC_SENSOR  0

// Timers.
#define TEMP_SAVE_TIMEOUT_CYCLES  180  // ~6 seconds.
#define LONG_PRESS_TIMEOUT_CYCLES 50   // ~2 seconds.

#include <msp430g2553.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "delay.h"
#include "eeprom.h"
#include "settings.h"
#include "lcd.h"
#include "screens.h"
#include "menu.h"

// Global variables.
int set_temp_val = 0;
unsigned int set_temp = 0;
unsigned int actual_temp = 0;
unsigned int heater_pwm = 0;
char str[15];  // LCD max char = 14 (+ \0)
int counter = 0;
uint8_t last_RE_A = 0;
bool temp_changed = false;
int meas_temp = 0;
int cal_temp[2] = { -1, -1 };
bool defaults_loaded = false;
float adc_res = -1;
unsigned int adc[ADC_CONVS];
unsigned int temp_save_timeout = 0;
unsigned int long_press_timeout = 0;
int8_t current_preset = -1;

// Don't stare at it.
char portastation_line[84] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xfe, 0x90, 0x90, 0x90, 0x60,
	0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c,
	0x00, 0x3e, 0x10, 0x20, 0x20, 0x10,
	0x00, 0x20, 0xfc, 0x22, 0x02, 0x04,
	0x00, 0x04, 0x2a, 0x2a, 0x2a, 0x1e,
	0x00, 0x62, 0x92, 0x92, 0x92, 0x8c,
	0x00, 0x20, 0xfc, 0x22, 0x02, 0x04,
	0x00, 0x04, 0x2a, 0x2a, 0x2a, 0x1e,
	0x00, 0x20, 0xfc, 0x22, 0x02, 0x04,
	0x00, 0x00, 0x22, 0xbe, 0x02, 0x00,
	0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c,
	0x00, 0x3e, 0x10, 0x20, 0x20, 0x1e,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Function prototypes.
void read_adc();
float grab_input_voltage();
void control_heater();
void set_temperature(int temp, const bool print, const uint8_t unit, const bool force);
void set_temperature(int temp, const bool print, const uint8_t unit);
void set_temperature(int temp, const bool print);
void set_adc_temperature(int temp, const bool print, const uint8_t unit);
void heater_bar();
void info_panel();

/**
 * Main stuff.
 *
 * @return Don't ask me.
 */
int main() {
	WDTCTL = WDTPW + WDTHOLD;  // Disable WDT.
	lcd_setup();               // Setup the LCD pins and reset the damn thing.

	// Setup clock for 16MHz.
	BCSCTL1  = CALBC1_16MHZ;
	DCOCTL   = CALDCO_16MHZ;
	BCSCTL2 &= ~(DIVS_0);

	// Initialize the LCD.
	delay_ms(1);  // Just to make sure the LCD is ready.
	lcd_init();
	lcd_clear();

	// Setup pins.
	P1DIR |= (HEATER);                     // Setup the outputs.
	P1SEL |= (HEATER + SENSOR + VISENSE);  // Select ADCs and PWM channels.

	// Initiate pins.
	P1OUT |= (HEATER);

	// Check if the unit was powered with the button pressed.
	if (!(P1IN & SWITCH)) {
		current_screen = RECOVERY_SCREEN;
	}

	// Setup the EEPROM.
	eeprom_setup();

	// Configure ADCs.
	ADC10CTL1 = INCH_3 + CONSEQ_1;        // Selects A3 to A0 and a single sequence.
	ADC10CTL0 = SREF_0 + ADC10SHT_3 +     // Supply as reference, Sample and Hold 2.
	            MSC + ADC10ON + ADC10IE;  // Multiple samples, ADC on, ADC interrupt enable.
	ADC10AE0  = SENSOR + VISENSE;         // ADC input enable.
	ADC10DTC1 = ADC_CONVS;                // 4 conversions.

	// Configure PWM.
	TA0CCR0  = 500 - 1;          // PWM Period.
	TA0CCTL1 = OUTMOD_3;         // CCR1 set/reset.
	TA0CCR1  = 0;                // CCR1 PWM duty cycle.
	TA0CTL   = TASSEL_2 + MC_1;  // SMCLK, up mode.

	// Configure Port 1 interrupts.
	P1IE  |= (SWITCH);   // Enabled interrupts for SWITCH.
	P1IES |= (SWITCH);   // SWITCH set for a HIGH to LOW transition.
	P1IFG &= ~(SWITCH);  // Cleared SWITCH IFG.

	// Configure Port 2 interrupts.
	P2IE  |= (RE_A + RE_B);   // Enabled interrupts for RE_A and RE_B.
	P2IES &= ~(RE_A + RE_B);  // RE_A and RE_B set for a LOW to HIGH transition.
	P2IFG &= ~(RE_A + RE_B);  // Cleared RE_A and RE_B IFG.

	lcd_setup();
	// Initialize the LCD.
	delay_ms(1);  // Just to make sure the LCD is ready.
	lcd_init();
	lcd_clear();

	// Enable interrupts.
	__enable_interrupt();

	for (;;) {
		// Screen setup.
		if (screen_setup) {
			switch (current_screen) {
			case RECOVERY_SCREEN:
				recovery_screen();
				break;
			case SPLASH_SCREEN:
				splash_screen();

				// Check if the defaults were loaded.
				if (defaults_loaded) {
					commit_settings();
					defaults_loaded = false;

					// Show a small indication that the defaults were loaded.
					lcd_set_pos(0, 5);
					lcd_print("DL");
				}

				load_settings();
				adc_res = settings.vref / 1023.0;
				delay_ms(1000);
				break;
			case MAIN_SCREEN:
				// If it's time to save, then save this shit. I had to do this
				// because saving stuff is weird a likes to corrupt the memory.
				if (save_next_time) {
					commit_settings();
					save_next_time = false;
				}

				// Set the initial temperature and reset the save timer.
				set_temperature(conv_adc_temp(settings.last_set_temp + 1), true,
						settings.temp_unit, true);
				temp_save_timeout = 0;
				break;
			case MENU_SCREEN:
				load_menu_screen(MENU_MAIN, 0);
				break;
			case CALIBRATION_SCREEN:
				// Calibration screen title.
				lcd_set_pos(0, 0);
				lcd_putc(' ', INVERTED);
				lcd_set_pos(3, 0);
				lcd_print(" Calibration  ", INVERTED);
				lcd_set_pos(0, 1);
				lcd_putc(' ');
				break;
			case CONFIRM_CAL_SCREEN:
				// Confirm the calibration screen title.
				lcd_set_pos(0, 0);
				lcd_print("  Calibrated  ", INVERTED);

				// Printing parameters.
				snprintf(str, sizeof(str), "P1: (%d, %d) ", CAL_LOW_TEMP_ADC, cal_temp[0]);
				lcd_set_pos(0, 1);
				lcd_print(str);
				snprintf(str, sizeof(str), "P2: (%d, %d) ", CAL_HIGH_TEMP_ADC, cal_temp[1]);
				lcd_set_pos(0, 2);
				lcd_print(str);

				// Printing OK.
				lcd_set_pos(0, 5);
				lcd_print("     ");
				lcd_print(" OK ", INVERTED);
				break;
			case ABOUT_SCREEN:
				about_screen();
				break;
			}

			counter = 0;
			screen_setup = false;
		}

		// Screen stuff.
		switch (current_screen) {
		case SPLASH_SCREEN:
			change_screen(MAIN_SCREEN);
			break;
		case MAIN_SCREEN:
			// Set the new temperature.
			set_temperature(set_temp_val + counter, true);

			// Save set temperature timeout.
			if (temp_save_timeout > 0) {
				temp_save_timeout--;

				// Check if the timer finished in this cycle.
				if (temp_save_timeout == 0) {
					// Set the last temperature and save to the EEPROM
					settings.last_set_temp = set_temp;
					commit_settings();
				}
			}

			// Long press action timeout.
			if (long_press_timeout > 0) {
				long_press_timeout--;

				// Check if the timer finished in this cycle.
				if (long_press_timeout == 0) {
					settings.last_set_temp = set_temp;
					change_screen(MENU_SCREEN);
					break;
				} else {
					// Check if the switch was released before the timeout.
					if (P1IN & SWITCH) {
						if (current_preset >= (NUM_TEMP_PRESETS - 1)) {
							current_preset = 0;
						} else {
							current_preset++;
						}

						long_press_timeout = 0;
						set_adc_temperature(settings.temp_preset[current_preset],
								true, settings.temp_unit);
					}
				}
			}

			// Read ADC and do stuff with the measured values.
			read_adc();
			actual_temp = adc[ADC_SENSOR];
			control_heater();
			info_panel();

			// Check if the soldering iron is connected.
			lcd_set_pos(0, 3);
			if (actual_temp >= 1023) {
				// Soldering iron disconnected.
				lcd_print(" Disconnected ", INVERTED);
			} else {
				// Printing actual temperature.
				int ac_temp = conv_adc_temp(actual_temp);

				// Prevent non-linear values of temperature from being shown.
				if (ac_temp < 99) {
					snprintf(str, sizeof(str), "Actual:  <99%s ",
							settings.temp_unit_symbol);
					lcd_print(str);
				} else {
					snprintf(str, sizeof(str), "Actual:  %d%s ", ac_temp,
							settings.temp_unit_symbol);
					lcd_print(str);
				}
			}

			// Heater bar!
			heater_bar();
			break;
		case MENU_SCREEN:
			if (counter != 0) {
				if (editing_menu_item) {
					// Editing a menu item.
					edit_current_menu_item(counter);
				} else {
					// Selecting a menu item.
					load_menu_screen(MENU_CURRENT, counter);
				}

				counter = 0;
			}
			break;
		case CALIBRATION_SCREEN:
			// Set the temperature.
			if (cal_temp[0] == -1) {
				set_adc_temperature(CAL_LOW_TEMP_ADC, false, CELSIUS);
			} else {
				set_adc_temperature(CAL_HIGH_TEMP_ADC, false, CELSIUS);
			}

			// Measure the temperature, and do all the control stuff.
			read_adc();
			actual_temp = adc[ADC_SENSOR];
			control_heater();

			if (temp_changed) {
				// Printing the ADC setpoint.
				snprintf(str, sizeof(str), "Setpoint: %d  ", set_temp);
				lcd_set_pos(0, 1);
				lcd_print(str);

				meas_temp = set_temp_val;
			}

			// Printing actual sensed ADC temperature.
			snprintf(str, sizeof(str), "Sense:    %d ", actual_temp);
			lcd_set_pos(0, 2);
			lcd_print(str);

			// Changing the measured temperature.
			meas_temp += counter;
			counter = 0;

			// Printing measured temperature.
			snprintf(str, sizeof(str), "Meas.:   %d  ", meas_temp);
			lcd_set_pos(0, 4);
			lcd_print(str);
			lcd_putc(0x7f);
			lcd_putc('C');

			// Heater bar!
			heater_bar();
			break;
		case ABOUT_SCREEN:
			// Awesome scrolling inverter animation.
			for (uint8_t i = 0; i < 84; i++) {
				portastation_line[i] = ~portastation_line[i];

				lcd_set_pos(i, 3);
				lcd_command(0, portastation_line[i]);
				delay_ms(18);
			}
			break;
		}
	}

	return 0;
}

/**
 * Heater control feedback loop.
 */
void control_heater() {
	// Feedback loop.
	if (actual_temp < set_temp) {
		if (heater_pwm < 500) {
			heater_pwm += 10;
		}
	} else if (actual_temp >= set_temp) {
		if (heater_pwm > 100) {
			heater_pwm -= 100;
		} else {
			heater_pwm = 0;
		}
	}

	TA0CCR1 = heater_pwm;
}

/**
 * Sets the target temperature using the default temperature unit.
 *
 * @param temp Temperature in the unit set in the settings.
 * @param print Should the temperature be printed?
 */
void set_temperature(int temp, const bool print) {
	set_temperature(temp, print, settings.temp_unit);
}

/**
 * Sets the target temperature.
 *
 * @param temp Temperature in the unit set in the settings.
 * @param print Should the temperature be printed?
 * @param unit Desired unit to be used.
 */
void set_temperature(int temp, const bool print, const uint8_t unit) {
	set_temperature(temp, print, unit, false);
}

/**
 * Sets the target temperature.
 *
 * @param temp Temperature in the unit set in the settings.
 * @param print Should the temperature be printed?
 * @param unit Desired unit to be used.
 * @param force Forces the change.
 */
void set_temperature(int temp, const bool print, const uint8_t unit, const bool force) {
	if ((temp != set_temp_val) || force) {
		// Perform the important calculations.
		set_temp = conv_temp_adc(temp, unit);

		// Reset the counter and set the readable temperature.
		counter = 0;
		set_temp_val = temp;

		// Limit the temperature values.
		if (set_temp > MAX_SET_TEMP) {
			set_temp = MAX_SET_TEMP;
			set_temp_val = conv_adc_temp(MAX_SET_TEMP, unit);
		} else if (set_temp < MIN_SET_TEMP) {
			set_temp = MIN_SET_TEMP;
			set_temp_val = conv_adc_temp(MIN_SET_TEMP, unit);
		}

		if (print) {
			char str_unit[3];
			str_unit[0] = settings.temp_unit_symbol[0];
			str_unit[1] = get_temp_unit(unit);
			str_unit[2] = settings.temp_unit_symbol[2];

			// Print the new set temperature.
			snprintf(str, sizeof(str), "Set:     %d%s ", set_temp_val, str_unit);
			lcd_set_pos(0, 2);
			lcd_print(str);
		}

		// Set the save timeout timer and the changed temperature flag.
		temp_save_timeout = TEMP_SAVE_TIMEOUT_CYCLES;
		temp_changed = true;
	} else {
		temp_changed = false;
	}
}

/**
 * Sets the target temperature using a ADC value.
 *
 * @param temp Temperature in ADC units.
 * @param print Should the temperature be printed?
 * @param unit Desired unit to be used.
 */
void set_adc_temperature(int temp, const bool print, const uint8_t unit) {
	if (temp != set_temp) {
		// Perform the important calculations.
		set_temp = temp;

		// Reset the counter and set the readable temperature.
		counter = 0;
		set_temp_val = conv_adc_temp(temp, unit);

		if (print) {
			char str_unit[3];
			str_unit[0] = settings.temp_unit_symbol[0];
			str_unit[1] = get_temp_unit(unit);
			str_unit[2] = settings.temp_unit_symbol[2];

			// Print the new set temperature.
			snprintf(str, sizeof(str), "Set:     %d%s ", set_temp_val, str_unit);
			lcd_set_pos(0, 2);
			lcd_print(str);
		}

		// Set the save timeout timer if in the main screen.
		if (current_screen == MAIN_SCREEN) {
			temp_save_timeout = TEMP_SAVE_TIMEOUT_CYCLES;
		}

		temp_changed = true;
	} else {
		temp_changed = false;
	}
}

/**
 * Prints the information panel at the top of the screen.
 */
void info_panel() {
	uint8_t int_len = 0;

	// Slice the input voltage float. https://stackoverflow.com/a/40401141/126353
	float vin = grab_input_voltage();
	unsigned int vin_dec = (unsigned int)(vin * 10) % 10;

	// Pad the integer part of the input voltage.
	char v_int_str[3];
	int_len = snprintf(v_int_str, sizeof(v_int_str), "%d", (uint8_t)vin);
	if (int_len < 2) {
		snprintf(v_int_str, sizeof(v_int_str), "0%d", (uint8_t)vin);
	}

	// Power calculations and float slicing.
	float power = (vin * vin * (heater_pwm / 500.0)) / settings.rheater;
	unsigned int p_dec = (unsigned int)(power * 10) % 10;

	// Pad the integer part of the power.
	char p_int_str[3];
	int_len = snprintf(p_int_str, sizeof(p_int_str), "%d", (uint8_t)power);
	if (int_len < 2) {
		snprintf(p_int_str, sizeof(p_int_str), "0%d", (uint8_t)power);
	}

	snprintf(str, sizeof(str), "%s.%dW    %s.%dV", p_int_str, p_dec, v_int_str, vin_dec);
	lcd_set_pos(0, 0);
	lcd_print(str);
}

/**
 * Sets the size of the heater bar according to the PWM level.
 */
void heater_bar() {
	float val = 0.168 * heater_pwm;

#ifndef ROTATION_ENABLE
	// Set the position.
	lcd_set_pos(0, 5);
#endif

	for (uint8_t i = 0; i < PCD8544_WIDTH + 1; i++) {
#ifdef ROTATION_ENABLE
		// Set the position.
		lcd_set_pos(i, 5);
#endif

		// Print the bar.
		if ((i == 0) || (i == PCD8544_WIDTH)) {
			lcd_command(0, 0b11111111);
		} else if ((i == 1) || (i == PCD8544_WIDTH - 1)) {
			lcd_command(0, 0b10000001);
		} else if (i <= (int)val) {
			lcd_command(0, 0b10111101);
		} else {
			lcd_command(0, 0b10000001);
		}
	}
}

/**
 * Grabs the current input voltage.
 *
 * @return Input voltage value.
 */
float grab_input_voltage() {
	float r = (adc_res * (float)adc[ADC_VISENSE]) / settings.vin_ratio;
	return r;
}

/**
 * Reads the ADC values into the array.
 */
void read_adc() {
	unsigned int val[2] = { 0, 0 };

	if (settings.sense_when_off) {
		TA0CCR1 = 0;  // Disable the heater.
	}

	for (uint8_t i = 0; i < AVG_TIMES; i++) {
		delay_us(100);  // Wait for ADC reference to settle.
		ADC10CTL0 &= ~ENC;
		while (ADC10CTL1 & BUSY);         // Wait until ADC10 core is active.

		ADC10SA = (unsigned int)adc;      // Data buffer start.
		ADC10CTL0 |= ENC + ADC10SC;       // Sampling and conversion start
		__bis_SR_register(CPUOFF + GIE);  // Enter LPM0 with interrupts enabled.

		val[0] += adc[ADC_SENSOR];
		val[1] += adc[ADC_VISENSE];
	}

	// Get the measured averages and re-enable the heater.
	adc[ADC_SENSOR] = (unsigned int)(val[0] / AVG_TIMES);
	adc[ADC_VISENSE] = (unsigned int)(val[1] / AVG_TIMES);

	if (settings.sense_when_off) {
		TA0CCR1 = heater_pwm;
	}
}

// ADC10 interrupt service routine.
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {
	__bic_SR_register_on_exit(CPUOFF);  // Return to active mode.
}

// Port 1 interrupt service routine.
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {
	switch (current_screen) {
	case RECOVERY_SCREEN:
		// Load the default settings.
		load_default_settings();
		defaults_loaded = true;

		change_screen(SPLASH_SCREEN);
		break;
	case MAIN_SCREEN:
		long_press_timeout = LONG_PRESS_TIMEOUT_CYCLES;
		break;
	case MENU_SCREEN:
		menu_action(ACTION_CLICK);
		break;
	case CALIBRATION_SCREEN:
		if (cal_temp[0] == -1) {
			cal_temp[0] = meas_temp;
		} else {
			cal_temp[1] = meas_temp;
			change_screen(CONFIRM_CAL_SCREEN);
		}
		break;
	case CONFIRM_CAL_SCREEN:
		// Set the new calibration variables and perform the interpolations.
		settings.cal_var[0] = cal_temp[0];
		settings.cal_var[1] = cal_temp[1];
		perform_interpolations();

		// Reset the calibration variables.
		cal_temp[0] = -1;
		cal_temp[1] = -1;

		// Save everything and go back to the main screen.
		save_next_time = true;
		change_screen(MAIN_SCREEN);
		break;
	case ABOUT_SCREEN:
		change_screen(MENU_SCREEN);
		break;
	}

	P1IFG &= ~(SWITCH);
}

// Port 2 interrupt service routine.
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void) {
	// Check P2IFG if RE_A is set?
	// Maybe wait a couple of cycles to make sure the switch is steady?

	if ((last_RE_A == 0) && (P2IN & RE_A)) {
		delay_us(10);

		if (P2IN & RE_B) {
			// CCW
			counter--;
		} else {
			// CW
			counter++;
		}
	}

	last_RE_A = (P2IFG & RE_A);
	P2IFG &= ~(RE_A + RE_B);  // Maybe I should clear the IFGs separately inside the if?
}
