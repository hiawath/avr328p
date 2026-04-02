#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>


#include "UART0.h"

// -----------------------------------------------------------------
// DHT11 센서 변수 및 인터럽트 함수
// -----------------------------------------------------------------
volatile uint8_t bits[40]; 
volatile uint8_t cnt = 0;
volatile uint8_t ready = 0;

ISR(PCINT2_vect) {
    uint8_t time = TCNT0; 
    TCNT0 = 0;

    if (!(PIND & (1 << PD2))) { 
        if (cnt < 2) {
            cnt++;
        } 
        else if (cnt < 42) {
            bits[cnt - 2] = (time > 12) ? 1 : 0;
            cnt++;
            
            if (cnt == 42) {
                PCICR &= ~(1 << PCIE2); 
                ready = 1;
            }
        }
    }
}

void request_dht() {
    for(int i=0; i<40; i++) bits[i] = 0;
    cnt = 0;
    ready = 0;

    DDRD |= (1 << 2);
    PORTD &= ~(1 << 2);
    _delay_ms(18);
    PORTD |= (1 << 2);
    DDRD &= ~(1 << 2);

    TCNT0 = 0;
    PCIFR |= (1 << PCIF2);    
    PCICR |= (1 << PCIE2);    
    PCMSK2 |= (1 << PCINT18); 
}

// -----------------------------------------------------------------
// 메인 루프 시작
// -----------------------------------------------------------------
int main(void) {
    UART0_init(); 
    
    TCCR0B |= (1 << CS01) | (1 << CS00);
    sei(); 
    
    
    UART0_print_string("Serial port COM5 opened\r\n");

    char buf[80];
    
    // 무한 반복(while) 대신 딱 19번만 측정하는 for문으로 수정!
    for (int count = 0; count < 19; count++) {
        _delay_ms(2000);
        request_dht();

        while (ready == 0);

        uint8_t data[5] = {0, 0, 0, 0, 0};
        for (int i = 0; i < 40; i++) {
            if (bits[i]) data[i / 8] |= (1 << (7 - (i % 8)));
        }

        // 체크섬 검사 (데이터가 올바르게 들어왔는지 확인)
        if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) == data[4]) {
            
            // 1. 함수를 써서 숫자 하나씩 출력 (31 0 26 2 ok)
            for (int i = 0; i < 4; i++) {
                UART0_print_1_byte_number(data[i]);
                UART0_print_string(" ");
            }
            UART0_print_string(" ok\r\n");

            // 2.  온습도 형식 출력 (temp: 26.02 C, humi: 31.00 %)
            sprintf(buf, "temp: %d.%02d C, humi: %d.%02d %%\r\n\r\n", data[2], data[3], data[0], data[1]);
            UART0_print_string(buf);
            
        } else {
            UART0_print_string("data error (checksum nok)\r\n");
            count--; // 에러 나면 횟수 세지 않고 다시 시도
        }
    }
    
    
    UART0_print_string("Serial port COM5 closed\r\n");

    while (1) {
        
    }
    return 0;
}