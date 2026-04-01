#include <avr/io.h>
#include <avr/interrupt.h>

#define LED_PIN PB5 // 아두이노 내장 LED (D13)
#define BTN_PIN PD2 // 외부 인터럽트 0 (D2)

void init_hardware(void) {
    // 1. 입출력 핀 설정 (D2 풀업 저항 활성화)
    DDRB |= (1 << LED_PIN);
    DDRD &= ~(1 << BTN_PIN);
    PORTD |= (1 << BTN_PIN);

    // 2. INT0 외부 인터럽트 설정 (하강 에지)
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    EIMSK |= (1 << INT0); 

    // 3. Timer0 초기화 (아직 타이머는 가동하지 않음)
    TCCR0A = 0x00; // 일반(Normal) 모드
    TCCR0B = 0x00; // 타이머 정지 상태
    
    sei(); // 전역 인터럽트 활성화
}

// [핵심 1] INT0 외부 인터럽트 (버튼이 눌리는 순간 1회만 실행됨)
ISR(INT0_vect) {
    // 1. 바운싱으로 인한 중복 실행을 막기 위해 INT0 감지 임시 차단
    EIMSK &= ~(1 << INT0);


    // 2. 실제 원하는 작업 수행
    PORTB ^= (1 << LED_PIN); 

    //__vector_1();

    // 3. 디바운싱 타이머(Timer0) 가동 설정
    TCNT0 = 0;              // 타이머 카운트 0으로 초기화
    TIFR0 |= (1 << TOV0);   // 이전 오버플로우 플래그 강제 지움
    TIMSK0 |= (1 << TOIE0); // Timer0 오버플로우 인터럽트 켜기
    
    // 프리스케일러 1024 적용하여 타이머 시작 
    // (16MHz / 1024 = 15625Hz. 256번 카운트 시 약 16.38ms 경과)
    TCCR0B = (1 << CS02) | (1 << CS00);
}

// [핵심 2] Timer0 오버플로우 인터럽트 (약 16.38ms 후 스위치 안정화 시점)
ISR(TIMER0_OVF_vect) {
    // 1. 타이머 정지 및 인터럽트 끄기 (1회용 지연 역할 완료)
    TCCR0B = 0x00; 
    TIMSK0 &= ~(1 << TOIE0);

    // 2. 대기하는 16ms 동안 발생했던 노이즈(바운싱) 인터럽트 플래그 강제 삭제
    EIFR |= (1 << INTF0);

    // 3. INT0 외부 인터럽트 다시 켜기 (새로운 버튼 클릭 받을 준비 완료)
    EIMSK |= (1 << INT0);
}

int main(void) {
    init_hardware();

    while(1) {
        // 메인 루프는 완전히 비어 있습니다. 
        // 딜레이 없이 100%의 CPU 성능을 다른 기능(UART, 센서 읽기 등)에 쏟을 수 있습니다.
    }
    return 0;
}