#include "LcdMenu.h"
#include <string.h>

void message (char const *line1, char const *line2)
{
	lcd.clear ();
	lcd.setCursor ((16 - strlen (line1)) / 2, 0);
	lcd.print (line1);
	lcd.setCursor ((16 - strlen (line2)) / 2, 1);
	lcd.print (line2);
}

uint8_t checkButton(bool block) {
	static bool wait = true;
	if (block) 
		while (lcd.readButtons () != 0) {}
	else if (wait) {
		if (lcd.readButtons () != 0) 
			return 0;
	}
	while (true) {
		uint8_t buttons = lcd.readButtons ();
		if (buttons != 0) {
			wait = true;
				return buttons;                                          
		}
		wait = false;
		if (!block)
			return 0;
	}
}

static uint8_t getDigit (uint32_t num, uint32_t power)
{
	return (num / power) % 10;
}

static void shownum (uint8_t x, uint8_t y, uint32_t num, uint8_t digits, uint8_t decimals, uint8_t pos)
{
	lcd.setCursor (x, y);
	uint32_t power = 1;
	for (uint8_t t = 0; t < digits + decimals; ++t)
		power *= 10;
	for (uint8_t t = 0; t < digits; ++t)
	{
		power /= 10;
		lcd.print (getDigit (num, power));
	}
	if (decimals > 0)
	{
		lcd.print (".");
		for (uint8_t t = 0; t < decimals; ++t)
		{
			power /= 10;
			lcd.print (getDigit (num, power));
		}
	}
	lcd.setCursor (x + pos + (pos >= digits), y);
}

uint32_t readNum (uint8_t x, uint8_t y, uint8_t digits, uint8_t decimals, uint32_t initial)
{
	uint8_t pos;
	uint32_t &num = initial;
	uint32_t power = 1;
	for (pos = digits + decimals - 1; pos > digits - 1; --pos)
		power *= 10;
	lcd.cursor ();
	lcd.blink ();
	while (true)
	{
		shownum (x, y, num, digits, decimals, pos);
		uint8_t b = waitForButton ();
		if (b & BUTTON_UP)
		{
			if (getDigit (num, power) == 9)
				num -= power * 9;
			else
				num += power;
		}
		if (b & BUTTON_DOWN)
		{
			if (getDigit (num, power) == 0)
				num += power * 9;
			else
				num -= power;
		}
		if (b & BUTTON_LEFT)
		{
			if (pos == 0)
				continue;
			pos -= 1;
			power *= 10;
		}
		if (b & BUTTON_RIGHT)
		{
			if (pos == digits + decimals - 1)
				continue;
			pos += 1;
			power /= 10;
		}
		if (b & BUTTON_SELECT)
		{
			lcd.noCursor ();
			lcd.noBlink ();
			return num;
		}
	}
}
