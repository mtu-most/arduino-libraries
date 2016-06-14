#include "LcdMenu.h"
#include <string.h>

void message (char const *line1, char const *line2)
{
	DBG_MENU("message1 ", line1);
	DBG_MENU("message2 ", line2);
	lcd.setCursor (0, 0);
	for (unsigned i = 0; i < 8 - strlen(line1) / 2; ++i)
		lcd.print(" ");
	lcd.print (line1);
	for (unsigned i = 0; i < 8 - strlen(line1) / 2 + strlen(line1); ++i)
		lcd.print(" ");
	lcd.setCursor (0, 1);
	lcd.print (line2);
	for (unsigned i = 0; i < 8 - strlen(line2) / 2 + strlen(line2); ++i)
		lcd.print(" ");
#ifdef SERIAL_LCD
	Serial.print("Message: ");
	Serial.print(line1);
	Serial.print(" / ");
	Serial.println(line2);
#endif
}

static uint8_t getDigit (uint32_t num, uint32_t power)
{
	DBG_MENU("getDigit1 ", num);
	DBG_MENU("getDigit2 ", power);
	return (num / power) % 10;
}

static void shownum (uint8_t x, uint8_t y, uint32_t num, uint8_t digits, uint8_t decimals, uint8_t pos)
{
	DBG_MENU("shownum ", num);
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
#ifdef SERIAL_LCD
	Serial.print("Show number: ");
	Serial.print(num);
	Serial.print("e");
	Serial.println(decimals);
#endif
}

uint32_t readNum (uint8_t x, uint8_t y, uint8_t digits, uint8_t decimals, uint32_t initial)
{
	DBG_MENU("readnum ", initial);
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
		uint8_t b = checkButton (true);
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
