#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint32_t sys_tick = 0; // 1ms마다 1씩 증가할 전역 시간 변수

void timer0_tick_init(void)
{
    TCCR0A = (1 << WGM01); // CTC
    OCR0A = 249;           // 1ms
    TIMSK0 = (1 << OCIE0A);
    TCCR0B = (1 << CS01) | (1 << CS00); // 분주비 64
}

ISR(TIMER0_COMPA_vect)
{
    sys_tick++;
}

// 현재 시간(ms)을 안전하게 읽어오는 함수
uint32_t get_millis(void)
{
    uint32_t m;
    cli();
    m = sys_tick;
    sei();
    return m;
}

void servo_timer1_init(void)
{
    DDRB |= (1 << PB1);

    // Fast PWM MODE 14 (WGM13,12,11,10: 1110) (CS: 010 = 분주비 8)
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

    ICR1 = 39999; // 20ms 주기 세팅 (MODE14에서는 캡처 X TOP역할)
    OCR1A = 1000; // 초기 각도 (0도)
}

void servo_set_angle(uint16_t angle)
{
    if (angle > 180)
        angle = 180;
    OCR1A = 1000 + (angle * 21); // SG90 맞춤 공식 (0.5ms ~ 2.4ms)
}

// void servo_set_angle(uint16_t angle) {
//     // 데이터시트에 나온 대로 했으나 안됨
//     if (angle > 180) angle = 180;

//     // 0도(맨 왼쪽, 1.0ms) = 2000틱
//     // 180도(맨 오른쪽, 2.0ms) = 4000틱
//     // 1도당 11.1틱씩 증가
//     OCR1A = 2000 + (angle * 11);
// }

int main(void)
{
    timer0_tick_init();
    servo_timer1_init();
    sei();

    uint32_t last_time = 0;
    uint8_t step = 0; // 모터 상태 0, 1, 2

    while (1)
    {
        if (get_millis() - last_time >= 1000)
        {
            last_time = get_millis();

            if (step == 0)
            {
                servo_set_angle(0);
                step = 1;
            }
            else if (step == 1)
            {
                servo_set_angle(90);
                step = 2;
            }
            else if (step == 2)
            {
                servo_set_angle(180);
                step = 0;
            }
        }
    }
}