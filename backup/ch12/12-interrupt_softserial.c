#include <avr/io.h>
#include <avr/interrupt.h>

#define BIT_TIME 1667 // 16MHz / 9600bps (Prescaler 1)
#define TX_PIN PB1  //9
#define RX_PIN PB0  //8

// 송수신 상태 정의
volatile enum { IDLE, START, DATA, STOP } tx_state = IDLE, rx_state = IDLE;
volatile uint8_t tx_data, rx_data;
volatile uint8_t tx_bit_idx, rx_bit_idx;
volatile uint8_t rx_ready = 0;

void soft_uart_init() {
    DDRB |= (1 << TX_PIN);   // TX 출력
    DDRB &= ~(1 << RX_PIN);  // RX 입력
    PORTB |= (1 << TX_PIN);  // TX Idle(HIGH)

    // Pin Change Interrupt 설정 (RX 핀의 변화 감지)
    PCICR |= (1 << PCIE0);   // Port B 인터럽트 활성화
    PCMSK0 |= (1 << PCINT0); // PB0(RX_PIN) 감시

    // Timer1 설정 (CTC 모드 아님, 자유 카운팅)
    TCCR1A = 0;
    TCCR1B = (1 << CS10);    // Prescaler 1
    TIMSK1 = 0;              // 처음엔 타이머 인터럽트 끔

    sei(); // 전역 인터럽트 활성화
}

// 송신 함수 (논블로킹 시작점)
void soft_uart_putc_nonblocking(uint8_t c) {
    while (tx_state != IDLE); // 이전 송신이 끝날 때까지 대기(버퍼링 미구현 시)
    
    tx_data = c;
    tx_bit_idx = 0;
    tx_state = START;
    
    OCR1A = TCNT1 + BIT_TIME; // 첫 인터럽트 예약
    TIFR1 |= (1 << OCF1A);    // 플래그 초기화
    TIMSK1 |= (1 << OCIE1A);  // TX 인터럽트 활성화
}

// [TX 인터럽트] 1비트씩 처리
ISR(TIMER1_COMPA_vect) {
    OCR1A += BIT_TIME; // 다음 비트 시간 예약

    switch (tx_state) {
        case START:
            PORTB &= ~(1 << TX_PIN); // Start bit (LOW)
            tx_state = DATA;
            break;
        case DATA:
            if (tx_data & (1 << tx_bit_idx)) PORTB |= (1 << TX_PIN);
            else                             PORTB &= ~(1 << TX_PIN);
            
            if (++tx_bit_idx > 7) tx_state = STOP;
            break;
        case STOP:
            PORTB |= (1 << TX_PIN); // Stop bit (HIGH)
            tx_state = IDLE;
            TIMSK1 &= ~(1 << OCIE1A); // TX 인터럽트 종료
            break;
        default: break;
    }
}

// [RX 시작 감지] Falling Edge (Start Bit)
ISR(PCINT0_vect) {
    if (!(PINB & (1 << RX_PIN)) && rx_state == IDLE) { // 하강 에지 확인
        rx_state = START;
        rx_bit_idx = 0;
        rx_data = 0;
        
        // 1.5 비트 시간 후에 첫 데이터 비트 중앙 샘플링
        OCR1B = TCNT1 + BIT_TIME + (BIT_TIME / 2);
        TIFR1 |= (1 << OCF1B);
        TIMSK1 |= (1 << OCIE1B); // RX 타이머 인터럽트 활성화
    }
}

// [RX 샘플링 인터럽트]
ISR(TIMER1_COMPB_vect) {
    OCR1B += BIT_TIME; // 다음 비트 중앙으로 예약

    if (rx_state == START) {
        // 데이터 비트 수집 시작
        if (PINB & (1 << RX_PIN)) rx_data |= (1 << rx_bit_idx);
        
        if (++rx_bit_idx > 7) {
            rx_state = STOP;
        }
    } else if (rx_state == STOP) {
        rx_ready = 1; // 수신 완료 플래그
        rx_state = IDLE;
        TIMSK1 &= ~(1 << OCIE1B); // RX 타이머 종료
    }
}

int main() {
    soft_uart_init();
    
    while (1) {
        // 메인 루프는 자유롭게 돌아감!
        if (rx_ready) {
            uint8_t received = rx_data;
            rx_ready = 0;
            
            // 받은 데이터를 다시 보냄 (에코)
            soft_uart_putc_nonblocking(received);
        }
        
        // 여기에 다른 코드(LED 깜빡이기 등)를 넣어도 통신이 끊기지 않음
    }
}