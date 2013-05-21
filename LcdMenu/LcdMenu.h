#ifndef _LCD_MENU
#define _LCD_MENU

#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>

// You have to make sure this exists in your main program.
extern Adafruit_RGBLCDShield lcd;

class action
{
	public:
	virtual void run () = 0;
};

class CB_t : public action
{
	void (*cb)();
	public:
	CB_t (void (*cb)()) : cb (cb) {}
	virtual void run () { cb (); }
};

#define MenuItem(name) void name ## _impl (); CB_t name (name ## _impl); void name ## _impl ()

template <unsigned num_choices> class Menu : public action
{
	public:
	Menu (char *(&names)[num_choices], action *(&actions)[num_choices]);
	virtual void run ();

	private:
	char **names;
	action **actions;
	uint8_t choice;
	void show ();
};

template <unsigned num_choices> Menu <num_choices>::Menu (char *(&names)[num_choices], action *(&actions)[num_choices])
	: names (names), actions (actions), choice (0)
{
}

template <unsigned num_choices> void Menu <num_choices>::show ()
{
	lcd.clear ();
	lcd.print (">");
	lcd.print (names[choice]);
	lcd.setCursor (15, 0);
	lcd.print ("<");
	lcd.setCursor (1,1);
	lcd.print (names[(choice + 1) % num_choices]);
}

template <unsigned num_choices> void Menu <num_choices>::run ()
{
	show ();
	while (lcd.readButtons ()) {}
	while (true)
	{
		uint8_t buttons = lcd.readButtons ();
		if (!buttons)
			continue;
		lcd.clear ();
		if (buttons & BUTTON_UP)
			choice = (choice - 1) % num_choices;
		else if (buttons & BUTTON_DOWN)
			choice = (choice + 1) % num_choices;
		else if (buttons & (BUTTON_SELECT | BUTTON_RIGHT))
			actions[choice]->run ();
		else if (buttons & BUTTON_LEFT)
			return;
		show ();
		while (lcd.readButtons ()) {}
	}
}

#endif
