#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "UART0.h"

volatile uint16_t time_tick = 0;    // 10us 단위 타이머 카운터
volatile uint8_t bit_count = 0;     // 수신된 비트 수 (0~40)
volatile uint8_t dht_data[5] = {0}; // 센서 데이터 (습도 정수/소수, 온도 정수/소수, 체크섬)
volatile uint8_t dht_ready = 0;     // 데이터 수신 완료 플래그
volatile uint8_t is_reading = 0;    // 센서 읽기 모드 활성화 플래그

void timer0_ctc_10us_init(void) {
    TCCR0A = (1 << WGM01);              // CTC 모드 설정
    OCR0A = 19;                         // 16MHz / 8분주 기준 20 카운트 (10us)
    TIMSK0 = (1 << OCIE0A);             // 비교 일치 인터럽트 활성화
    TCCR0B = (1 << CS01);               // 분주비 8 설정 및 타이머 구동
}

ISR(TIMER0_COMPA_vect) {
    time_tick++; 
}

void pcint0_init(void) {
    PCICR |= (1 << PCIE0);              // PCINT[7:0] 그룹 인터럽트 활성화
    PCMSK0 |= (1 << PCINT0);            // PB0(PCINT0) 핀 변화 감지 활성화
}

// 기존 함수 3개 DHT11의 데이터 핀 ISR에 병합
ISR(PCINT0_vect) {
    if (!is_reading) return; // 수신 모드가 아닐 경우 인터럽트 무시

    if (PINB & (1 << PB0)) {
        // RISING EDGE: 핀 상태가 HIGH로 변환 시 타이머 초기화
        time_tick = 0;
    } else {
        // FALLING EDGE: 핀 상태가 LOW로 변환 시 HIGH 유지 시간 측정
        uint16_t duration = time_tick; 

        // 10us 단위 기준 (오차 범위 포함)
        if (duration >= 7 && duration <= 10) {
            if (bit_count == 0) {
                // 1. DHT11 응답 신호(ACK, 약 80us) 처리 (데이터 비트에 포함하지 않음)
            } else {
                // 2. HIGH 구간 약 70us 유지 시 논리 '1'로 판별
                dht_data[bit_count / 8] <<= 1; 
                dht_data[bit_count / 8] |= 1;  
                bit_count++;
            }
        } else if (duration >= 2 && duration <= 5) {
            // 3. HIGH 구간 약 26~28us 유지 시 논리 '0'으로 판별
            dht_data[bit_count / 8] <<= 1; 
            bit_count++;
        }

        // 40비트(5바이트) 수신 완료 시 처리
        if (bit_count >= 40) {
            dht_ready = 1;
            is_reading = 0; // 추가 인터럽트 방지를 위해 수신 모드 해제
        }
    }
}

void dht11_start(void) {
    // 수신 버퍼 및 플래그 초기화
    dht_ready = 0;
    bit_count = 0;
    for(int i = 0; i < 5; i++) dht_data[i] = 0;
    
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);
    _delay_ms(20); 
    
    PORTB |= (1 << PB0);
    DDRB &= ~(1 << PB0);
    
    // 수신 모드 활성화 (PCINT0에서 응답 처리 시작)
    is_reading = 1; 
}

int main(void) {
    UART0_init();
    timer0_ctc_10us_init();
    pcint0_init();
    
    sei();

    UART0_print_string("DHT11 Interrupt Mode Initialized\r\n");

    while (1) {
        dht11_start();
        
        _delay_ms(2000); 

        if (dht_ready) {
            uint8_t checksum = dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3];
            
            if (checksum == dht_data[4]) {
                UART0_print_sensor_data("Humidity", dht_data[0], dht_data[1], "%");
                UART0_print_sensor_data("Temperature", dht_data[2], dht_data[3], "C");
                UART0_print_string("----------------------\r\n");
            } else {
                UART0_print_string("Error: Checksum mismatch\r\n");
            }
        } else {
             UART0_print_string("Error: Sensor timeout or not responding\r\n");
        }
    }
    
    return 0;
}