/*
 * user_interface.h
 *
 *  Created on: Dec 8, 2023
 *      Author: gino
 */
#include "stm32f4xx.h"
#include "interrupts.h"
#include <stdbool.h>
#include <string.h>
#include "gpio.h"
#include "fonts.h"
#include "ssd1306.h"
#include "user_interface.h"

#define pushDebounceCount 5000

#define settings_count 4


extern bool transmit;
extern uint8_t PDLC_intensity;
extern uint8_t Address;

struct setting_struct settings[settings_count];
uint8_t settings_current = 0;
bool settings_menu = true;

void UI_interrupt(void)
{
	__disable_irq();

	enable_UI();

	while(1)
	{
		enum inputs input_button;

		uint16_t PB_count = 0;

		bool bouncy = true;

		while (bouncy)
		{
			bool ROTA_state = gpio_read(inputPort, ROTA_pin);
			bool PB_state = gpio_read(inputPort, PB_pin);
			bool ROTB_state = gpio_read(inputPort, ROTB_pin);

			if (ROTA_state ^ ROTB_state)
			{
				if (ROTA_state)
				{
					input_button = CCW;

				}
				else
				{
					input_button = CW;
				}
				bouncy = false;
			}

			if (PB_state)
			{
				++PB_count;

				if (PB_count >= pushDebounceCount)
				{
					input_button = PB;
					//while (gpio_read(GPIOC, 8));
					bouncy = false;
				}
			}
			else if (PB_count > 0)
			{
				--PB_count;
			}


		}

		if (iterate_UI(input_button)) // verwerkt de input in de UI
		{
			break; // als iterate_UI 1 returned is de gebruiker klaar met de UI
		}

	}
	__enable_irq();
	EXTI->PR |= (0x01 << ROTA_pin) | (0x01 << PB_pin) | (0x01 << ROTB_pin); // reset all pending IO button IRQ flags
}

struct setting_struct generate_display_string(struct setting_struct setting) // this function places the name and the value of a setting in a string to be displayed in the settings menu
{
	strcpy(setting.display_string, setting.name); // copy the name of the setting into the display string

	if (setting.value_ptr == 0) // if this is a function setting we are done already and can return
	{
		return setting;
	}

	const uint8_t string_length = strlen(setting.name); // how long is the string?

	int i;

	int val_digit_count = 1; // this variable will represent how much digits the setting's value has

	uint32_t value = *(setting.value_ptr); // this variable is used to calculate val_digit_count

	if (value < 0) // negative numbers have an extra 'digit' (the '-' sign)
	{
		val_digit_count++;
		value *= -1; // we do the rest of the val_digit_count calculation as a positive number
	}

	if (value > 9) // if the value is greater than 9 it has more than 1 digit
	{
		for (i = value; i > 9; i /= 10, val_digit_count++); // for everytime the value is still higher than 9, we divide the value by 10 and increment val_digit_count
	}

	setting.val_digit_count = val_digit_count; // insert the final val_digit_count into the setting's struct


	for (i = string_length; i < max_string_length; i++) // fill every character after the setting's name with a blank space in the display string
	{
		setting.display_string[i] = ' ';
	}

	char num[setting.val_digit_count + 1]; // character array to copy the value of the setting into a string
	itoa(*(setting.value_ptr), num, 10); // copy the value into a string

	setting.display_string[max_string_length - setting.val_digit_count - 1] = ' '; // put a blank space in between the name and the value in the display string
	strcpy (setting.display_string + (max_string_length - setting.val_digit_count), num);// add the value string to the display string

	return setting; // done is kees
}

struct setting_struct new_setting(char* name, int32_t* value_ptr, int32_t value_max, int32_t value_min, void* fun_ptr) // fill a fresh struct with the desired parameters for a new setting
{
	struct setting_struct new_setting; // make new struct

	strxfrm(new_setting.name, name, max_string_length); // copy the first max_string_length characters of the name parameter into the struct's name member

	new_setting.value_ptr = value_ptr; // assign the address of the setting's value pointer

	new_setting.value_max = value_max; // assign the maximum value of the setting
	new_setting.value_min = value_min; // assign the minimum value of the setting

	*(new_setting.value_ptr) = new_setting.value_min;

	new_setting.val_digit_count = 1; // preset the val_digit_count to 1 (the minimum value for this member)

	new_setting.fun_ptr = fun_ptr; // assign the function pointer's address

	new_setting = generate_display_string(new_setting); // generate the display string (to be displayed in the menu)

	return new_setting; // finito
}

void display_menu(uint8_t n) // display the settings menu with the current, previous and next selected setting and their values
{
	SSD1306_Fill (0); // clear screen

	// place the previous setting's display string:
	SSD1306_GotoXY (11, 0);
	SSD1306_Puts (settings[(n + settings_count - 1) % settings_count].display_string, &settings_font, 1);

	// place the selection indicator
	SSD1306_GotoXY (0, 22);
	SSD1306_Puts (">", &Font_11x18, 1);

	// place the current setting's display string:
	SSD1306_GotoXY (11, 22);
	SSD1306_Puts (settings[n].display_string, &settings_font, 1);

	// place the next setting's display string:
	SSD1306_GotoXY (11, 45);
	SSD1306_Puts (settings[(n + settings_count + 1) % settings_count].display_string, &settings_font, 1);

	SSD1306_UpdateScreen(); // display all changes on the screen
}

void display_setting(uint8_t n) // display the current setting and it's value
{
	SSD1306_Fill (0); // clear screen

	int stringLen = strlen(settings[n].name); // how long is the name string?

	// place the string in a centered position:
	SSD1306_GotoXY (63 - ((stringLen * settings_font.FontWidth)/2), 2);
	SSD1306_Puts (settings[n].name, &settings_font, 1);

	char numbers[settings[n].val_digit_count + 1];// string to place the value on screen

	itoa(*(settings[n].value_ptr), numbers, 10); // copy the value into the new string

	//place the value of the setting in a centered position:
	SSD1306_GotoXY (63 - (settings[n].val_digit_count * Font_16x26.FontWidth / 2), 35);
	SSD1306_Puts (numbers, &Font_16x26, 1);

	SSD1306_UpdateScreen(); // display all this on the screen
}

void exit_UI(bool* exit, struct setting_struct setting) // function to exit the UI
{
	/* Clear screen */
	SSD1306_Fill(SSD1306_COLOR_BLACK); // make the screen black

	/* Update screen */
	SSD1306_UpdateScreen(); // actually display the black screen

	*exit = true; // signal that we can exit the UI

	return;
}

void toggle_transmission(bool* exit, struct setting_struct* setting) // function to toggle the IR transmission
{
	if (transmit)
	{
		strcpy(setting->name, "start IR");
		transmit = 0;
	}
	else
	{
		strcpy(setting->name, "stop IR");
		transmit = 1;
	}

	*(setting) = generate_display_string(*(setting));
	display_menu(settings_current);

	*exit = false; // signal that we can NOT exit the UI

	return;
}

void init_settings(void) // create all desired settings and insert them into the global settings array
{

	settings[0] = new_setting("exit UI", 0, 0, 0, &exit_UI); // used to exit the menu
	settings[1] = new_setting("stop IR", 0, 0, 0, &toggle_transmission); // should the system be enabled? (1 = transmit IR, 0 = do nothing lmao)
	settings[2] = new_setting("intensity", &PDLC_intensity, 10, 0, 0); // used to pass the desired intensity of the system
	settings[3] = new_setting("ID", &Address, 15, 0, 0); // used to pass the desired ID of this beacon

}

void enable_UI(void) // display the menu
{
	display_menu(settings_current); // display the menu
}

bool iterate_UI(enum inputs input) // process input
{
	if (settings_menu) // are we currently in the menu?
	{
		switch(input) {
		  case CW: // clock wise rotation: make next setting the current setting
			  settings_current = (settings_current + (settings_count + 1)) % settings_count; // make next setting the current setting
			  display_menu(settings_current); // display the menu with the new current setting;
			  break;

		  case CCW:// counter clock wise rotation: make previous setting the current setting
			  settings_current = (settings_current + (settings_count - 1)) % settings_count;// make previous setting the current setting
			  display_menu(settings_current); // display the menu with the new current setting;
		    break;

		  case PB: // button pushed: change the setting or execute it's function
			  if (settings[settings_current].fun_ptr == 0){ // if the setting has no function associated:
				  settings_menu = false; // we are going out of the menu
				  display_setting(settings_current); // display (only) the current setting and it's value to adjust it
			  }
			  else // the setting has a function as opposed to a value
			  {
				  	bool exit = 0; // the function can exit the UI by making exit high
					(*(settings[settings_current].fun_ptr))(&exit, &(settings[settings_current])); // execute the function with exit's address as an argument
					return exit; // return exit
			  }
			  break;
		}
		return false; // return false, so as to stay within the UI
	}
	else if (!settings_menu) // we are not in the menu, so we are changing a setting's value
	{
		switch(input) {
		  case CW: // clockwise: increment the setting's value
			  if (*(settings[settings_current].value_ptr) < settings[settings_current].value_max) // if the value is smaller than it's maximum:
			  {
				  ++*(settings[settings_current].value_ptr); // increment the setting's value
			  }
			  else // the setting's value is greater than or equal to it's maximum value
			  {
				  *(settings[settings_current].value_ptr) = settings[settings_current].value_min; // make the setting's value equivalent to it's minimum value
			  }
			  display_setting(settings_current); // display the setting's name and the new value
			  break;

		  case CCW:// counter clockwise: decrement the setting's value
			  if (*(settings[settings_current].value_ptr) > settings[settings_current].value_min)// if the value is greater than it's minimum:
			  {
				  --*(settings[settings_current].value_ptr); // decrement the setting's value
			  }
			  else // the setting's value is smaller than or equal to it's minimum value
			  {
				  *(settings[settings_current].value_ptr) = settings[settings_current].value_max; // make the setting's value equal to it's maximum vaule
			  }
			  display_setting(settings_current); // display the setting's name and it's new value
			break;

		  case PB: // pressed: return to the menu
			  settings_menu = true; // we return to the menu
			  settings[settings_current] = generate_display_string(settings[settings_current]); // generate a new display string with the adjust setting's value
			  display_menu(settings_current); // display the menu
			  break;
		}

		return false; // return false, so as to stay within the UI
	}

}
