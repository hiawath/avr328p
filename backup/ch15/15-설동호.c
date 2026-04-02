#ifndef F_CPU
#define F_CPU 16000000UL // 시스템 클럭 정의 (16MHz)
#endif

#include <avr/io.h>
#include <util/delay.h>

volatile int8_t step = 0;
volatile uint16_t counter = 0;

void drive_servo(uint16_t, uint16_t);
void step_and_count();

int main() {
    DDRB |= (1 << PB1); // uno pin 9

    while (1) {
        switch (step) {
            case 0: // 0
                drive_servo(600, 19400);
                break;
            case 1: // 90
                drive_servo(1500, 18500);
                break;
            case 2: // 180
                drive_servo(2400, 17600);
                break;
            default:
                break;
        }
        step_and_count();
    }
    return 0;
}

void drive_servo(uint16_t high, uint16_t low) {
    PORTB |= (1 << PB1);
    _delay_us(high);
    PORTB &= ~(1 << PB1);
    _delay_us(low);
}

void step_and_count() {
    if (counter >= 1000) {
        step = (step + 1) % 3;
        counter = 0;
    }
    else {
        counter += 20;
    }
}