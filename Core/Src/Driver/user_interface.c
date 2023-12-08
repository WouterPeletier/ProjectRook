#include "stm32f4xx.h"
#include "interrupts.h"
#include <stdbool.h>
#include <string.h>

#include "fonts.h"
#include "ssd1306.h"
#include "user_interface.h"

extern uint8_t PDLC_intensity;
extern uint8_t Address;

/*struct setting_struct {
  char name[max_string_length + 1];
  char display_string[max_string_length + 1];

  uint32_t value_max;
  uint32_t value_min;
  uint32_t* value_ptr;
  int val_digit_count;

  void* fun_ptr;
};*/

struct setting_struct settings[settings_count];
uint8_t settings_current = 0;
bool settings_menu = true;

//enum inputs{CW, CCW, PB}; // input kan; een clockwise rotatie-, counter-clockwise rotatie- of een push button signaal zijn


void exit_UI(void)
{
	/* Clear screen */
	SSD1306_Fill(SSD1306_COLOR_BLACK);

	/* Update screen */
	SSD1306_UpdateScreen();
	return;
}


struct setting_struct generate_display_string(struct setting_struct setting)
{
	strcpy(setting.display_string, setting.name);

	if (setting.value_ptr == 0)
	{
		return setting;
	}

	const uint8_t string_length = strlen(setting.name);

	int i;

	int val_digit_count = 1;

	uint32_t value = *(setting.value_ptr);

	if (value < 0)
	{
		val_digit_count++;
		value *= -1;
	}

	if (value > 9)
	{
		for (i = value; i > 9; i /= 10, val_digit_count++);
	}

	setting.val_digit_count = val_digit_count;

	for (i = string_length; i < max_string_length; i++)
	{
		setting.display_string[i] = ' ';
	}

	char num[setting.val_digit_count + 1];
	itoa(*(setting.value_ptr), num, 10);

	setting.display_string[max_string_length - setting.val_digit_count - 1] = ' ';
	strcpy (setting.display_string + (max_string_length - setting.val_digit_count), num);

	//setting.display_string[max_string_length] = '\0';

	return setting;
}

struct setting_struct new_setting(char* name, int32_t* value_ptr, int32_t value_max, int32_t value_min, void* fun_ptr)
{
	struct setting_struct new_setting;

	strxfrm(new_setting.name, name, max_string_length);

	new_setting.value_ptr = value_ptr;

	new_setting.value_max = value_max;
	new_setting.value_min = value_min;
	new_setting.val_digit_count = 1;

	new_setting.fun_ptr = fun_ptr;

	new_setting = generate_display_string(new_setting);


	return new_setting;
}

void init_settings(void)
{
	settings[0] = new_setting("exit UI", 0, 0, 0, &exit_UI);

	settings[1] = new_setting("intensity", &PDLC_intensity, 100, 0, 0);
	settings[2] = new_setting("ID", &Address, 31, 0, 0);

}

void display_menu(uint8_t n)
{
	SSD1306_Fill (0);

	SSD1306_GotoXY (11, 0);
	SSD1306_Puts (settings[(n + settings_count - 1) % settings_count].display_string, &settings_font, 1);

	SSD1306_GotoXY (0, 22);
	SSD1306_Puts (">", &Font_11x18, 1);

	SSD1306_GotoXY (11, 22);
	SSD1306_Puts (settings[n].display_string, &Font_11x18, 1);

	SSD1306_GotoXY (11, 45);
	SSD1306_Puts (settings[(n + settings_count + 1) % settings_count].display_string, &settings_font, 1);

	SSD1306_UpdateScreen();
}

void display_setting(uint8_t n)
{
	SSD1306_Fill (0);

	int stringLen = strlen(settings[n].name);

	SSD1306_GotoXY (63 - ((stringLen * settings_font.FontWidth)/2), 2);
	SSD1306_Puts (settings[n].name, &settings_font, 1);

	char numbers[settings[n].val_digit_count + 1];

	itoa(*(settings[n].value_ptr), numbers, 10);

	SSD1306_GotoXY (63 - (settings[n].val_digit_count * Font_16x26.FontWidth / 2), 35);
	SSD1306_Puts (numbers, &Font_16x26, 1);

	SSD1306_UpdateScreen();
}

bool iterate_UI(enum inputs input)
{
	if (settings_menu)
	{
		switch(input) {
		  case CW:
			  settings_current = (settings_current + (settings_count + 1)) % settings_count;
			  display_menu(settings_current);
			  break;

		  case CCW:
			  settings_current = (settings_current + (settings_count - 1)) % settings_count;
			  display_menu(settings_current);
		    break;

		  case PB:
			  if (settings[settings_current].fun_ptr == 0){
				  settings_menu = false;
				  display_setting(settings_current);
			  }
			  else
			  {
					(*(settings[settings_current].fun_ptr))();
					return true;
			  }
			  break;

		  //default:
		    // code block
		}
		return false;
	}
	else if (!settings_menu)
	{
		switch(input) {
		  case CW:
			  //*(settings[settings_current].value_ptr) = ((*(settings[settings_current].value_ptr) + (settings[settings_current].value_max + 1)) % (settings[settings_current].value_max)) + settings[settings_current].value_min;
			  if (*(settings[settings_current].value_ptr) < settings[settings_current].value_max)
			  {
				  ++*(settings[settings_current].value_ptr);
			  }
			  else
			  {
				  *(settings[settings_current].value_ptr) = settings[settings_current].value_min;
			  }
			  display_setting(settings_current);
			  break;

		  case CCW:
			  //*(settings[settings_current].value_ptr) = ((*(settings[settings_current].value_ptr) + (settings[settings_current].value_max - 1)) % (settings[settings_current].value_max)) +  settings[settings_current].value_min;

			  if (*(settings[settings_current].value_ptr) > settings[settings_current].value_min)
			  {
				  --*(settings[settings_current].value_ptr);
			  }
			  else
			  {
				  *(settings[settings_current].value_ptr) = settings[settings_current].value_max;
			  }
			  display_setting(settings_current);
			break;

		  case PB:
			  settings_menu = true;
			  settings[settings_current] = generate_display_string(settings[settings_current]);
			  display_menu(settings_current);
			  break;

		  //default:
			// code block
		}

		return false;
	}

}
