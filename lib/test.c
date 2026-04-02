#define F_CPU 16000000UL // [필수] delay 함수가 정확히 동작하도록 CPU 속도 명시
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

// UART 초기화 (9600bps)
void uart_init(){
    UBRR0H = 0;
    UBRR0L = 103;
    UCSR0B = (1<<TXEN0);
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}
void uart_putc(char c){
    while(!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}
void uart_print(const char *s){ while(*s) uart_putc(*s++); }
void uart_hex(uint8_t v){
    const char h[] = "0123456789ABCDEF";
    uart_putc(h[v>>4]);
    uart_putc(h[v&0xF]);
}

// --- 쓰기 함수 (아두이노 타이밍 클론) ---
void ds1302_write_raw(uint8_t reg, uint8_t data) {
    PORTD &= ~(1 << PD6); // SCLK LOW 유지
    PORTB |= (1 << PB0);  // CE HIGH
    _delay_us(4);

    DDRD |= (1 << PD7);   // IO를 출력으로

    uint8_t val = reg;
    for (int i = 0; i < 8; i++) {
        if (val & 1) PORTD |= (1 << PD7);
        else         PORTD &= ~(1 << PD7);
        
        _delay_us(2);
        PORTD |= (1 << PD6); // SCLK HIGH
        _delay_us(4);        // 아두이노 딜레이 모방
        PORTD &= ~(1 << PD6); // SCLK LOW
        _delay_us(2);
        val >>= 1;
    }

    val = data;
    for (int i = 0; i < 8; i++) {
        if (val & 1) PORTD |= (1 << PD7);
        else         PORTD &= ~(1 << PD7);
        
        _delay_us(2);
        PORTD |= (1 << PD6); // SCLK HIGH
        _delay_us(4);
        PORTD &= ~(1 << PD6); // SCLK LOW
        _delay_us(2);
        val >>= 1;
    }

    PORTB &= ~(1 << PB0); // CE LOW
    _delay_us(4);
}

// --- 읽기 함수 (아두이노 타이밍 클론) ---
uint8_t ds1302_read_raw(uint8_t reg) {
    PORTD &= ~(1 << PD6); // SCLK LOW 유지
    PORTB |= (1 << PB0);  // CE HIGH
    _delay_us(4);

    DDRD |= (1 << PD7);   // IO를 출력으로
    uint8_t val = reg;
    
    // 1. 주소 전송
    for (int i = 0; i < 8; i++) {
        if (val & 1) PORTD |= (1 << PD7);
        else         PORTD &= ~(1 << PD7);
        
        _delay_us(2);
        PORTD |= (1 << PD6); // SCLK HIGH
        _delay_us(4);
        PORTD &= ~(1 << PD6); // SCLK LOW
        _delay_us(2);
        val >>= 1;
    }

    // 2. 아두이노처럼 안전하게 입력 전환 후 휴식
    DDRD &= ~(1 << PD7);  // IO를 입력으로 전환
    PORTD &= ~(1 << PD7); // 풀업 해제
    _delay_us(5);         // DS1302가 첫 번째 데이터를 내보낼 시간 제공!

    uint8_t data = 0;
    
    // 3. 데이터 읽기
    for (int i = 0; i < 8; i++) {
        // 클록이 LOW로 떨어진 직후이므로, 이미 데이터가 나와있음. 먼저 읽음.
        if (PIND & (1 << PD7)) {
            data |= (1 << i);
        }
        _delay_us(2);
        PORTD |= (1 << PD6); // SCLK HIGH
        _delay_us(4);
        PORTD &= ~(1 << PD6); // SCLK LOW (다음 비트 방출)
        _delay_us(2);
    }

    PORTB &= ~(1 << PB0); // CE LOW
    _delay_us(4);
    return data;
}

int main(void) {
    DDRB  |= (1 << PB0); // CE 출력
    DDRD  |= (1 << PD6); // SCLK 출력
    PORTB &= ~(1 << PB0); // 초기화 LOW
    PORTD &= ~(1 << PD6); // 초기화 LOW
    _delay_ms(100);

    uart_init();

    // WP 해제 테스트
    ds1302_write_raw(0x8E, 0x00);
    uint8_t wp = ds1302_read_raw(0x8F);
    uart_print("WP: 0x"); uart_hex(wp); uart_print("\r\n");

    // CH 클리어
    ds1302_write_raw(0x80, 0x00);
    uint8_t sec = ds1302_read_raw(0x81);
    uart_print("SEC: 0x"); uart_hex(sec); uart_print("\r\n");

    // 0x30 쓰기/읽기 테스트
    ds1302_write_raw(0x80, 0x30);
    _delay_ms(10);
    uint8_t verify = ds1302_read_raw(0x81);
    uart_print("Verify: 0x"); uart_hex(verify); uart_print("\r\n");

    while(1){}
}