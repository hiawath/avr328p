#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>
#include <stdio.h>
#include "UART0.h"

FILE OUTPUT = FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);

volatile int8_t bit_idx = -1;
volatile uint8_t data[5] = {
    0,
};
volatile uint8_t dht_ready = 0;
volatile uint16_t last_time = 0; // 전역으로 빼서 관리

// PD2(INT0) 전용 제어 매크로
#define DHT_PIN PD2
#define DHT_DDR DDRD
#define DHT_PORT PORTD
#define DHT_PIN_REG PIND

#define LED_PIN PB5 // 아두이노 13번 LED (디버깅용)
#define LED_DDR DDRB
#define LED_PORT PORTB

void timer1_init()
{
    TCCR1A = 0;
    TCCR1B = (1 << CS11); // 1틱 = 0.5us
}
// 16비트 타이머를 안전하게 읽는 함수
uint16_t safe_read_tcnt1()
{
    uint16_t val;
    uint8_t sreg = SREG;
    cli();
    val = TCNT1;
    SREG = sreg;
    return val;
}
void dht11_start()
{
    // 1. 인터럽트 완전히 비활성화 (매우 중요)
    EIMSK &= ~(1 << INT0);

    bit_idx = -1;
    dht_ready = 0;
    last_time = 0;
    for (int i = 0; i < 5; i++)
        data[i] = 0;

    // 2. 시작 신호 송신 (Output Low)
    DDRD |= (1 << PD2);
    PORTD &= ~(1 << PD2);
    _delay_ms(18);

    // 3. 핀을 High로 돌리고 입력 모드로 전환
    PORTD |= (1 << PD2);
    _delay_us(20); // 핀이 확실히 High로 안정화될 시간 부여
    DDRD &= ~(1 << PD2);

    // 4. [핵심] 타이머 리셋 및 가짜 플래그 제거
    TCNT1 = 0;
    EIFR |= (1 << INTF0); // 시작 신호 중 발생한 가짜 인터럽트 기록 삭제

    // 5. 이제서야 인터럽트 허용
    EIMSK |= (1 << INT0);
    EICRA = (1 << ISC01); // Falling Edge 설정 확인
    sei(); // 전역 인터럽트 활성화
}

// [수정] INT0 전용 인터럽트 서비스 루틴
ISR(INT0_vect)
{
    uint16_t current_time = TCNT1 / 2; // us 단위 변환
    uint16_t duration = current_time - last_time;
    last_time = current_time;
    LED_PORT ^= (1 << LED_PIN);

    // INT0을 Falling Edge로 설정했으므로, 이 ISR은 항상 하강 에지에서만 실행됨
    // 즉, duration은 직전 High 구간의 길이를 완벽하게 나타냄

    if (bit_idx == -1)
    {

        // printf("duration = %d\n", duration);
        // 센서 응답 신호 (80us High) 확인
        if (70 < duration && duration < 95)
        {

            bit_idx = 0;
        }
    }
    else if (bit_idx >= 0 && bit_idx < 40)
    {
        // 데이터 비트 해석
        if (duration > 60)
        { // '1' 비트 (약 70us)
            data[bit_idx / 8] |= (1 << (7 - (bit_idx % 8)));
        }
        bit_idx++;

        if (bit_idx >= 40)
        {
            dht_ready = 1;
            EIMSK &= ~(1 << INT0); // 수신 완료 후 인터럽트 잠시 끔
        }
    }
}

int main()
{
    UART0_init();     // UART0 초기화
    stdout = &OUTPUT; // printf 사용 설정
    timer1_init();
    sei();

    LED_DDR |= (1 << LED_PIN);

    LED_PORT |= (1 << LED_PIN);
    _delay_ms(2000);
    LED_PORT &= ~(1 << LED_PIN);

    while (1)
    {
        dht11_start();
        _delay_ms(100); // 데이터 수신 대기

        if (dht_ready)
        {
            printf("습도: %d%%, 온도: %dC\n", data[0], data[2]);
        }
        else
        {
            printf("응답 없음 (bit_idx: %d)\n", bit_idx);
        }

        _delay_ms(2000);
    }
}