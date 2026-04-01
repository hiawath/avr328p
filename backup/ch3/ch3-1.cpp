#include <Arduino.h>
#define F_CPU 16000000L
#include <avr/io.h>
#include <util/delay.h>

void setup()
{
}

void loop()
{
	DDRB |= (1 << PB5); 

	while (1)
	{
		PORTB |= 0x20;
		_delay_ms(100);
		PORTB &= ~0x20;
		_delay_ms(100);
	}
}