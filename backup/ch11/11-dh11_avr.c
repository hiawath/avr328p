#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "UART0.h"
#define DHT_PIN     PD2    // Digital 9번 핀
#define DHT_DDR     DDRD
#define DHT_PIN_REG PIND
#define DHT_PORT    PORTD



FILE OUTPUT \
= FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);

uint8_t dht_data[5];

// 8비트 데이터를 읽어오는 핵심 함수
uint8_t read_dht_byte() {
    uint8_t result = 0;
    for (int i = 0; i < 8; i++) {
        // 1. Low 구간이 끝날 때까지 대기 (50us)
        while (!(DHT_PIN_REG & (1 << DHT_PIN)));

        // 2. High 시작 후 약 30us 대기
        _delay_us(30);

        // 3. 30us 후에도 High라면 비트 '1', Low가 됐으면 '0'
        if (DHT_PIN_REG & (1 << DHT_PIN)) {
            result |= (1 << (7 - i));
            // 나머지 High 구간이 끝날 때까지 대기
            while (DHT_PIN_REG & (1 << DHT_PIN));
        }
    }
    return result;
}

void measure_dht11() {
    // [Step 1] 시작 신호 전송
    DHT_DDR |= (1 << DHT_PIN);
    DHT_PORT &= ~(1 << DHT_PIN);
    _delay_ms(20);             // 18ms 이상 유지

    DHT_PORT |= (1 << DHT_PIN);
    _delay_us(30);
    DHT_DDR &= ~(1 << DHT_PIN); // 입력 모드 전환

    // [Step 2] 센서 응답 확인 (정상 응답 시 Low 80us -> High 80us 발생)
    _delay_us(40);
    if (!(DHT_PIN_REG & (1 << DHT_PIN))) {
        while (!(DHT_PIN_REG & (1 << DHT_PIN))); // Low 구간 끝 대기
        while (DHT_PIN_REG & (1 << DHT_PIN));    // High 구간 끝 대기

        // [Step 3] 5바이트(40비트) 읽기
        for (int i = 0; i < 5; i++) {
            dht_data[i] = read_dht_byte();
        }

        // 체크섬 확인
        if (dht_data[4] == (dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3])) {
            printf("습도: %d%%, 온도: %dC\n", dht_data[0], dht_data[2]);
        }
    }
}

int main() {
	UART0_init(); // UART0 초기화
	stdout = &OUTPUT; // printf 사용 설정
    printf("온습도 측정!!\n");

    while (1) {
        measure_dht11();
        _delay_ms(2000); // DHT11은 최소 2초 간격 측정 권장

    }
    return 0;
}