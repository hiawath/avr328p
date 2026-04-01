#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/delay.h>

#define BTN_PIN PD2 // 외부 인터럽트 0 (D2)
#define LED_PIN13 PB5 // 아두이노 D13

// 워치독 설정 함수 (8초 주기, 인터럽트 모드)
void setup_watchdog_8s(void) {

    cli();
    MCUSR &= ~(1 << WDRF); // 리셋 플래그 클리어

    // 설정 변경을 위한 잠금 해제 (Timed Sequence)
    WDTCSR |= (1 << WDCE) | (1 << WDE);

    // WDP3, WDP0 세트로 8초 설정 (WDP3:0 = 1001)
    // WDIE 세트로 인터럽트 모드 활성화
    WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);

    sei();
}

void init_hardware(void) {
    // 1. 입출력 핀 설정 (D2 풀업 저항 활성화)
    DDRB |= (1 << LED_PIN13); // LED 출력 설정
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

    wdt_reset();
    // 1. 바운싱으로 인한 중복 실행을 막기 위해 INT0 감지 임시 차단
    EIMSK &= ~(1 << INT0);

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

// 워치독 인터럽트 서비스 루틴
ISR(WDT_vect) {
    // 하드웨어가 WDIE를 자동으로 0으로 만들 수 있으므로 다시 세트 (보수적 설계)
    WDTCSR |= (1 << WDIE);
}

void enter_sleep(void) {
    // 수면 모드 설정 (가장 깊은 수면: Power-down)
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    sleep_enable();  // 수면 준비

    // 수면 중 갈색 아웃 감지(BOD) 회로를 꺼서 전력 추가 절감 (옵션)
    // sleep_bod_disable();

    sleep_cpu();     // 여기서 CPU가 멈춤 (8초 뒤 WDT 인터럽트로 깨어남)

    // --- 여기서부터는 깨어난 후 실행되는 영역 ---
    sleep_disable(); // 수면 모드 해제
}

int main(void) {

    setup_watchdog_8s();
    init_hardware();

    while (1) {
        // 1. 작업 수행 (LED 토글)
        PORTB |= (1 << LED_PIN13);
        _delay_ms(100); // 잠깐 켜진 것을 확인하기 위함
        PORTB &= ~(1 << LED_PIN13);

        // 2. 수면 모드 진입 (8초간 정지)
        enter_sleep();
    }
    return 0;
}