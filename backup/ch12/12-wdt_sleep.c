#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/delay.h>

#define LED_PIN PB5 // 아두이노 D13

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
    DDRB |= (1 << LED_PIN); // LED 출력 설정

    setup_watchdog_8s();

    while (1) {
        // 1. 작업 수행 (LED 토글)
        PORTB |= (1 << LED_PIN);
        _delay_ms(50); // 잠깐 켜진 것을 확인하기 위함
        PORTB &= ~(1 << LED_PIN);

        // 2. 수면 모드 진입 (8초간 정지)
        enter_sleep();
    }
    return 0;
}