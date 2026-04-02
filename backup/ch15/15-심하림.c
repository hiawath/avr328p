#define F_CPU 16000000L
#include <avr/io.h>
#include <util/delay.h>

void setup_pwm()
{
    // 9번 핀(PB1) 출력 설정
    DDRB |= (1 << PB1);

    // Timer1 설정: Fast PWM 모드 14 (ICR1이 TOP)
    // 비반전 모드, Prescaler 8
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

    // 주기를 20ms(50Hz)로 설정
    ICR1 = 39999;
}

// 각도 설정을 위한 함수 (보정값 적용)
void set_angle(int angle)
{
    // 일반적인 서보 모터(SG90 등)의 보정 범위:
    // 0도: 1000 (0.5ms)
    // 180도: 4800~5000 (2.4~2.5ms)
    // 이 수치들을 조정해서 정확한 90도와 180도를 찾으세요.
    uint16_t min_pulse = 1000;
    uint16_t max_pulse = 4800;

    uint16_t duty = min_pulse + (angle * (uint32_t)(max_pulse - min_pulse) / 180);
    OCR1A = duty;
}

int main(void)
{
    setup_pwm();

    while (1)
    {
        // 0도 이동 후 1초 대기
        set_angle(0);
        _delay_ms(2000);

        // 90도 이동 후 1초 대기
        set_angle(90);
        _delay_ms(2000);

        // 180도 이동 후 1초 대기
        set_angle(180);
        _delay_ms(2000);

        // 다시 90도로 돌아오고 싶다면 아래 코드를 추가하세요
        // set_angle(90);
        // _delay_ms(000);
    }
}