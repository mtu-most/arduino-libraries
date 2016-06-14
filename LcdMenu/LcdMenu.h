#ifndef _LCD_MENU
#define _LCD_MENU

#include <Arduino.h>
#include "LiquidCrystal_I2C.h"

//#define DBG_MENU(x, m) do { Serial.print(x); Serial.println(m); } while (0)
#define DBG_MENU(x, m) do {} while (0)

// Value to return meaning the menu should go back to the previous.
#define BACK 127
// Value to return meaning the menu entry hasn't finished.
#define WAIT 128

// You have to make sure this exists in your main program.
extern LiquidCrystal_I2C lcd;
extern uint8_t checkButton(bool block);

void message(char const *line1, char const *line2 = "");
uint32_t readNum(uint8_t x, uint8_t y, uint8_t digits, uint8_t decimals = 0, uint32_t initial = 0);

#define BUTTON_NONE 0
#define BUTTON_LEFT 1
#define BUTTON_UP 2
#define BUTTON_RIGHT 4
#define BUTTON_DOWN 8
#define BUTTON_SELECT 16

class action
{
	public:
	virtual int8_t run(bool block, char const *info = "") = 0;
};

class CB_t : public action
{
	int8_t (*cb)();
	public:
	CB_t (int8_t(*cb)()) : cb(cb) {}
	virtual int8_t run(bool block, char const *info = "") { (void)&block; (void)&info; return cb(); }
};

#define MenuItem(name) int8_t name ## _impl(); CB_t name(name ## _impl); int8_t name ## _impl()

template <unsigned num_choices> class Menu : public action
{
	public:
	Menu(char const *title, char const *(names)[num_choices], action *(actions)[num_choices]);
	virtual int8_t run(bool block, char const *info = "");
	virtual int8_t iteration(bool block, char const *info = "");

	private:
	char const *title;
	char const **names;
	action **actions;
	uint8_t current;
	uint8_t choice;
	bool waiting;
	void show(char const *info = "");
};

template <unsigned num_choices> Menu <num_choices>::Menu(char const *title, char const *names[num_choices], action *actions[num_choices])
	: title(title), names(names), actions(actions), current(~0), choice(0), waiting(false)
{
}

template <unsigned num_choices> void Menu <num_choices>::show(char const *info)
{
	DBG_MENU("show ", title);
	lcd.setCursor(0, 0);
	lcd.print(title);
	int8_t len = int8_t(strlen(info));
	for (int8_t i = 0; i < 16 - int8_t(strlen(title)) - len; ++i)
		lcd.print(" ");
	if (len > 0) {
		lcd.setCursor(16 - len, 0);
		lcd.print(info);
	}
	lcd.setCursor(0,1);
	lcd.print(choice);
	lcd.print(":");
	lcd.print(names[choice]);
	for (int8_t i = 0; i < 16 - (choice >= 10 ? 3 : 2) - int8_t(strlen(names[choice])); ++i)
		lcd.print(" ");
#ifdef SERIAL_LCD
	Serial.print("Menu choice: ");
	Serial.println(names[choice]);
#endif
}

template <unsigned num_choices> int8_t Menu <num_choices>::run(bool block, char const *info)
{
	DBG_MENU("run ", title);
	while (true)
	{
		uint8_t ret = iteration(block, info);
		if (ret != WAIT || !block)
			return ret;
	}
}

template <unsigned num_choices> int8_t Menu <num_choices>::iteration(bool block, char const *info)
{
	DBG_MENU("iteration ", title);
	if (waiting) {
		uint8_t c = actions[choice]->run(block, info);
		if (c == WAIT) {
			return WAIT;
		}
		else if (c == BACK) {
			choice = 0;
			waiting = false;
			return 0;
		}
		choice = (choice + c) % num_choices;
	}
	waiting = false;
	show(info);
	uint8_t buttons = checkButton(block);
	if (!buttons) {
		return WAIT;
	}
	if (buttons & BUTTON_UP) {
		if (choice > 0)
			choice -= 1;
	}
	else if (buttons & BUTTON_DOWN) {
		if (choice < num_choices - 1)
			choice += 1;
	}
	else if (buttons & (BUTTON_SELECT | BUTTON_RIGHT)) {
		// Use the return value of run as the menu choice adjustment.
		lcd.clear();
		uint8_t c = actions[choice]->run(block, info);
		if (c == WAIT) {
			waiting = true;
			return WAIT;
		}
		else if (c == BACK) {
			choice = 0;
			waiting = false;
			return 0;
		}
		choice = (choice + c) % num_choices;
	}
	else if (buttons & BUTTON_LEFT) {
		waiting = false;
		return 0;
	}
	return WAIT;
}

#endif
