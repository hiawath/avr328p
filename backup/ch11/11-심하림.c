#define F_CPU 16000000L
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "UART0.h"

#define DHT_PIN PD2
#define BUTTON_PIN PD3 // 버튼을 3번 핀에 연결

volatile uint8_t read_ready = 0; // 버튼이 눌렸는지 확인하는 플래그
volatile uint16_t pulse_width = 0;
volatile uint8_t data[5] = {0,};
volatile int8_t bit_cnt = -1;

// --- [타이머 및 인터럽트 초기화] ---
void Timer1_init() {
    TCCR1B |= (1 << CS11); // 8분주 (0.5us 단위)
}

void INT0_init() {
    EICRA |= (1 << ISC01); // Falling Edge (DHT11 신호용)
    EIMSK |= (1 << INT0);
}

void PCINT2_init() {
    PCICR |= (1 << PCIE2);     // 포트 D 그룹 인터럽트 허용
    PCMSK2 |= (1 << 3);  // PD3(디지털 3번) 핀만 버튼 감시 대상으로 지정 (PCINT23이 아니라 27입니다!)
}

// --- [인터럽트 서비스 루틴] ---

// 1. 버튼 인터럽트 (PCINT2)
ISR(PCINT2_vect) {
    // 버튼이 눌렸는지 확인 (Low일 때 눌린 것으로 간주)
    if (!(PIND & (1 << BUTTON_PIN))) {
        _delay_ms(20); // 디바운싱 (떨림 방지)
        if (!(PIND & (1 << BUTTON_PIN))) {
            read_ready = 1; // 메인 루프에 "읽어라!"라고 신호 보냄
        }
    }
}

// 2. DHT11 데이터 인터럽트 (INT0)
ISR(INT0_vect) {
    pulse_width = TCNT1;
    TCNT1 = 0;
    if (bit_cnt >= 0 && bit_cnt < 40) {
        if (pulse_width > 160) { 
            data[bit_cnt / 8] |= (1 << (7 - (bit_cnt % 8)));
        }
        bit_cnt++;
    }
}

// --- [DHT11 실행 함수] ---
void request_dht11() {
    bit_cnt = -1;
    for(int i=0; i<5; i++) data[i] = 0;

    DDRD |= (1 << DHT_PIN);
    PORTD &= ~(1 << DHT_PIN);
    _delay_ms(18);
    PORTD |= (1 << DHT_PIN);
    _delay_us(40);
    
    DDRD &= ~(1 << DHT_PIN);
    TCNT1 = 0;
    _delay_us(160);
    bit_cnt = 0;
}

int main(void) {
    UART0_init();
    Timer1_init();
    INT0_init();   // DHT11용 인터럽트
    PCINT2_init(); // 버튼용 인터럽트
    
    // 버튼 핀 설정 (입력 + 내부 풀업 저항 켜기)
    DDRD &= ~(1 << BUTTON_PIN);
    PORTD |= (1 << BUTTON_PIN); 

    sei(); // 전역 인터럽트 허용

    UART0_print_string("Push Button to Read DHT11!\r\n");

    while (1) {
        // 버튼이 눌려야만 실행됨
        if (1) {
            request_dht11();
            _delay_ms(100); // 데이터 수신 대기

            if (bit_cnt >= 40) {
                uint8_t sum = data[0] + data[1] + data[2] + data[3];
                if (sum == data[4] && data[0] != 0) {
                    UART0_print_string("H: ");
                    UART0_print_1_byte_number(data[0]);
                    UART0_print_string("%  T: ");
                    UART0_print_1_byte_number(data[2]);
                    UART0_print_string("C\r\n");
                } else {
                    UART0_print_string("Check Error\r\n");
                }
            }
            read_ready = 0; // 플래그 초기화 (다음 버튼 클릭 대기)
        }
        _delay_ms(2000);
    }
}