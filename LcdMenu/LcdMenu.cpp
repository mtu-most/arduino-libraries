#include "LcdMenu.h"
#include <string.h>

void message (char *line1, char *line2)
{
	lcd.clear ();
	lcd.setCursor ((16 - strlen (line1)) / 2, 0);
	lcd.print (line1);
	lcd.setCursor ((16 - strlen (line2)) / 2, 1);
	lcd.print (line2);
}

uint8_t waitForButton ()
{
	while (lcd.readButtons () != 0) {}
	while (true)
	{
		uint8_t buttons = lcd.readButtons ();
		if (buttons != 0)
			return buttons;
	}
}
