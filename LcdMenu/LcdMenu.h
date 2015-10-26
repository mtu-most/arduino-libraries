#ifndef _LCD_MENU
#define _LCD_MENU

//#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>

// Value to return meaning the menu should go back to the previous.
#define BACK 127
// Value to return meaning the menu entry hasn't finished.
#define WAIT 128

// You have to make sure this exists in your main program.
extern Adafruit_RGBLCDShield lcd;

void message(char const *line1, char const *line2 = "");
uint8_t checkButton(bool block);
uint32_t readNum(uint8_t x, uint8_t y, uint8_t digits, uint8_t decimals = 0, uint32_t initial = 0);
// Define this to override the default implementation.
#ifndef CHECKBUTTON
#define CHECKBUTTON checkButton
#endif

class action
{
	public:
	virtual int8_t run() = 0;
};

class CB_t : public action
{
	int8_t (*cb)();
	public:
	CB_t (int8_t(*cb)()) : cb(cb) {}
	virtual int8_t run(bool block) { (void)&block; return cb(); }
};

#define MenuItem(name) int8_t name ## _impl(); CB_t name(name ## _impl); int8_t name ## _impl()

template <unsigned num_choices> class Menu : public action
{
	public:
	Menu(char const *title, char const *(names)[num_choices], action *(actions)[num_choices]);
	virtual int8_t run(bool block);
	virtual int8_t iteration(bool block);

	private:
	char const *title;
	char const **names;
	action **actions;
	uint8_t current;
	uint8_t choice;
	void show();
};

template <unsigned num_choices> Menu <num_choices>::Menu(char const *title, char const *names[num_choices], action *actions[num_choices])
	: title(title), names(names), actions(actions), current(~0), choice(0)
{
}

template <unsigned num_choices> void Menu <num_choices>::show()
{
	lcd.clear();
	lcd.print(title);
	lcd.setCursor(0,1);
	lcd.print(names[choice]);
}

template <unsigned num_choices> int8_t Menu <num_choices>::run()
{
	while (true)
	{
		if (!iteration(true))
			return 0;
	}
}

template <unsigned num_choices> int8_t Menu <num_choices>::iteration(bool block)
{
	if (waiting) {
		uint8_t c = actions[choice]->run(block);
		if (c == WAIT) {
			return WAIT;
		else if (c == BACK) {
			waiting = false;
			return 0;
		}
		choice = (choice + c) % num_choices;
	}
	waiting = false;
	show();
	uint8_t buttons = CHECKBUTTON(block);
	if (!buttons) {
		return WAIT;
	}
	if (buttons & BUTTON_UP)
		choice = (choice + num_choices - 1) % num_choices;
	else if (buttons & BUTTON_DOWN)
		choice = (choice + 1) % num_choices;
	else if (buttons & (BUTTON_SELECT | BUTTON_RIGHT)) {
		// Use the return value of run as the menu choice adjustment.
		lcd.clear();
		uint8_t c = actions[choice]->run(block);
		if (c == WAIT) {
			waiting = true;
			return WAIT;
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
