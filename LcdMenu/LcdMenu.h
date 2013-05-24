#ifndef _LCD_MENU
#define _LCD_MENU

#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>

// You have to make sure this exists in your main program.
extern Adafruit_RGBLCDShield lcd;

void message (char const *line1, char const *line2 = "");
uint8_t waitForButton ();
uint32_t readNum (uint8_t x, uint8_t y, uint8_t digits, uint8_t decimals = 0, uint32_t initial = 0);

class action
{
	public:
	virtual int8_t run () = 0;
};

class CB_t : public action
{
	int8_t (*cb)();
	public:
	CB_t (int8_t (*cb)()) : cb (cb) {}
	virtual int8_t run () { return cb (); }
};

#define MenuItem(name) int8_t name ## _impl (); CB_t name (name ## _impl); int8_t name ## _impl ()

template <unsigned num_choices> class Menu : public action
{
	public:
	Menu (char const *title, char const *(names)[num_choices], action *(actions)[num_choices]);
	virtual int8_t run ();

	private:
	char const *title;
	char const **names;
	action **actions;
	uint8_t choice;
	void show ();
};

template <unsigned num_choices> Menu <num_choices>::Menu (char const *title, char const *names[num_choices], action *actions[num_choices])
	: title (title), names (names), actions (actions), choice (0)
{
}

template <unsigned num_choices> void Menu <num_choices>::show ()
{
	lcd.clear ();
	lcd.print (title);
	lcd.setCursor (0,1);
	lcd.print (names[choice]);
}

template <unsigned num_choices> int8_t Menu <num_choices>::run ()
{
	while (true)
	{
		show ();
		uint8_t buttons = waitForButton ();
		lcd.clear ();
		if (buttons & BUTTON_UP)
		{
			choice = (choice + num_choices - 1) % num_choices;
		}
		else if (buttons & BUTTON_DOWN)
		{
			choice = (choice + 1) % num_choices;
		}
		else if (buttons & (BUTTON_SELECT | BUTTON_RIGHT))
		{
			// Use the return value of run as the menu choice adjustment.
			choice = (choice + actions[choice]->run ()) % num_choices;
		}
		else if (buttons & BUTTON_LEFT)
		{
			return 0;
		}
	}
}

#endif
