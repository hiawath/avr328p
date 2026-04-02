#include <avr/io.h>
#include <util/delay.h>

// _delay_us 함수가 상수값만 받을 수 있어서
void delay_us_var(uint16_t us)
{
    while (us--)
    {
        _delay_us(1);
    }
}

// 20ms 주기의 펄스 생성
void servo_pulse(uint16_t pulse_us)
{
    // HIGH
    PORTB |= (1 << PB1);
    delay_us_var(pulse_us);

    // LOW
    PORTB &= ~(1 << PB1);
    delay_us_var(20000 - pulse_us);
}

int main(void)
{
    DDRB |= (1 << PB1);

    uint16_t pulses[3] = {550, 1200, 1800};
    int state = 0;

    while (1)
    {
        // 20ms * 50 = 1s
        for (int i = 0; i < 50; i++)
        {
            servo_pulse(pulses[state]);
        }

        state++;
        if (state >= 3)
            state = 0;
    }
}