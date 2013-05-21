#ifndef _LCD_MENU
#define _LCD_MENU

#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>

// You have to make sure this exists in your main program.
extern Adafruit_RGBLCDShield lcd;

void message (char *line1, char *line2 = "");
uint8_t waitForButton ();

class action
{
	public:
	virtual bool run () = 0;
};

class CB_t : public action
{
	bool (*cb)();
	public:
	CB_t (bool (*cb)()) : cb (cb) {}
	virtual bool run () { return cb (); }
};

#define MenuItem(name) bool name ## _impl (); CB_t name (name ## _impl); bool name ## _impl ()

template <unsigned num_choices> class Menu : public action
{
	public:
	Menu (char *title, char *(&names)[num_choices], action *(&actions)[num_choices]);
	virtual bool run ();

	private:
	char *title;
	char **names;
	action **actions;
	uint8_t choice;
	void show ();
};

template <unsigned num_choices> Menu <num_choices>::Menu (char *title, char *(&names)[num_choices], action *(&actions)[num_choices])
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

template <unsigned num_choices> bool Menu <num_choices>::run ()
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
			if (actions[choice]->run ())
			{
				choice = (choice + 1) % num_choices;
			}
		}
		else if (buttons & BUTTON_LEFT)
		{
			return false;
		}
	}
}

#endif
