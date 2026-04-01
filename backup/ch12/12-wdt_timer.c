#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

volatile uint8_t led_state = 0;

void WDT_init(void)
{
    DDRB |= (1 << PB5);

    cli();
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    
    WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1);
    wdt_enable(WDTO_1S);
    sei();
}

ISR(WDT_vect)
{
    led_state ^= 1;

    if (led_state)
        PORTB |= (1 << PB5);
    else
        PORTB &= ~(1 << PB5);
}

int main(void)
{
    WDT_init();
    while (1)
    {
    }
}