#include <avr/io.h>
#include <avr/interrupt.h>
#include "UART0.h"

#define DHT_PIN PD5

volatile uint32_t count = 0; 

// 타이머0 비교 일치 인터럽트 서비스 루틴
ISR(TIMER0_COMPA_vect) {
    count++;
	//TCNT0 = 0;
}

static void timer0_init_ctc_1ms(void)
{
    TCCR0A = (1 << WGM01);                   // CTC
    TCCR0B = (1 << CS01) | (1 << CS00);      // 분주 64
    OCR0A  = 249;                            // 16MHz / 64 / 1000 - 1
    TIMSK0 = (1 << OCIE0A);                  // OCIE0 설정
}

static uint32_t millis_ctc(void)
{   // ai 자동완성
    uint32_t v;
    uint8_t sreg = SREG;
    cli();
    v = count;
    SREG = sreg;
    return v;
}

// timer1 ai
static void timer1_init(void)
{
    TCCR1A = 0x00;
    TCCR1B = (1 << CS11);   // 분주 8
    TCNT1  = 0;
}

static inline uint16_t t1_now(void)
{
    return TCNT1;
}

static inline uint16_t us_to_ticks(uint16_t us)
{
    return (uint16_t)(us * 2);   // 0.5us / tick
}

static void delay_us_t1(uint16_t us)
{
    uint16_t start = t1_now();
    uint16_t target = us_to_ticks(us);

    while ((uint16_t)(t1_now() - start) < target) {
        ;
    }
}

uint8_t dht11(uint8_t *data) {
    uint8_t ok = 0;
    uint8_t sreg;
    uint16_t pulse;

    // 초기화
    for (int i = 0; i < 5; i++) data[i] = 0;

    // 시작 신호
    DDRD |= (1 << DHT_PIN);     // 출력 모드로 설정
    PORTD &= ~(1 << DHT_PIN);   // LOW 신호
    delay_us_t1(20000);         // 20ms
    //_delay_ms(20);            // 18ms 이상
    
    sreg = SREG;
    cli(); 

    PORTD |= (1 << DHT_PIN);    // 내부 풀업 활성화
    delay_us_t1(30);            // 30us
    //_delay_us(30);            // 20-40us
    DDRD &= ~(1 << DHT_PIN);    // 입력 모드로 설정  // 센서 응답 기다림

    // DHT11 응답 신호 대기
    uint32_t t;
    t = count;
    while (PIND & (1 << DHT_PIN)) {   
            if (count - t > 1000) {
                UART0_print_string("Timeout waiting for LOW signal\n\r");
                goto out;
            } 
        }  // LOW 기다림
    t = count;
    while (!(PIND & (1 << DHT_PIN))) {   
            if (count - t > 1000) {
                UART0_print_string("Timeout waiting for HIGH signal\n\r");  
                goto out;
            } 
        }  // HIGH 기다림
    t = count;
    while (PIND & (1 << DHT_PIN)) {   
            if (count - t > 1000) {
                UART0_print_string("Timeout waiting for LOW signal again\n\r");
                goto out;
            } 
        }   // 다시 LOW


    // 40비트 데이터 읽기
    for (uint8_t i = 0; i < 40; i++) {
        // HIGH 시작 대기
        t = count;
        while (!(PIND & (1 << DHT_PIN))) {
            if (count - t > 1000) {
                UART0_print_string("Timeout waiting for HIGH data signal\n\r");
                goto out;
            } 
        }

        uint16_t start = t1_now();

        // HIGH 끝날 때까지 대기        
        t = count;
        while (PIND & (1 << DHT_PIN)) {
            if (count - t > 1000) {
                UART0_print_string("Timeout waiting for LOW datasignal\n\r");
                goto out;
            }
        }

        pulse = (uint16_t)(t1_now() - start);

        if (pulse > us_to_ticks(50)) {   // 70us 이상이면 1, 아니면 0
            data[i/8] |= (1 << (7 - (i % 8))); // 1
        }

        ok = 1;
    }

out:
    SREG = sreg;    // 인터럽트 복원

    if (!ok) return 0;

    // 체크섬 확인
    if (data[4] != (uint8_t)((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return 0;
    }

    return 1;
}

int main(void) {	
    UART0_init();
    timer0_init_ctc_1ms();
    timer1_init();
    sei();

    char str[] = "DHT11 Sensor Test";
	
	UART0_print_string(str);		// 문자열 출력
	UART0_print_string("\n\r");

    uint8_t data[5];
    uint32_t last_read = 0;


    while (1) {
        uint32_t now = millis_ctc();

        if ((uint32_t)(now - last_read) >= 1000) {
            last_read = now;
            
            if (dht11(data)) {
                UART0_print_string("Humidity: ");
                UART0_print_1_byte_number(data[0]);
                UART0_print_string("%, Temperature: ");
                UART0_print_1_byte_number(data[2]);
                UART0_print_string("°C\n\r");
            } else {
                UART0_print_string("DHT11 read failed!\n\r");
            }
        }
        //_delay_ms(1000);
    }
    return 0;
}