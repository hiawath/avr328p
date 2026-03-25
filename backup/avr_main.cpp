#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

// 1. 1ms마다 증가할 전역 변수
// ISR 내부에서 값이 변경되므로 컴파일러 최적화 방지를 위해 반드시 volatile 선언
volatile unsigned long timer0_millis = 0;

// 2. 타이머0 하드웨어 초기화 함수
void Timer0_Init(void) {
    TCCR0A = 0x00; // 노멀 모드 (Normal mode)

    // 프리스케일러 64 설정 (CS01, CS00 비트 1)
    TCCR0B = (1 << CS01) | (1 << CS00);

    // 1ms(250틱)를 맞추기 위해 TCNT0 초기값 6 설정 (256 - 250 = 6)
    TCNT0 = 6;

    // 타이머0 오버플로 인터럽트 활성화
    TIMSK0 |= (1 << TOIE0);

    // 전역 인터럽트 활성화 (필수)
    sei();
}

// 3. 타이머0 오버플로 인터럽트 서비스 루틴 (1ms마다 자동 실행)
ISR(TIMER0_OVF_vect) {
    TCNT0 = 6;         // 다음 1ms 간격을 위해 레지스터 초기값 재장전
    timer0_millis++;   // 밀리초 카운터 1 증가
}

// 4. 안전하게 값을 읽어오는 커스텀 millis() 함수
unsigned long millis_custom(void) {
    unsigned long m;
    
    // 상태 레지스터(SREG) 백업
    uint8_t oldSREG = SREG; 

    // 인터럽트 비활성화 (원자적 읽기 보장)
    cli(); 
    
    // 4바이트(unsigned long) 변수 복사
    m = timer0_millis; 
    
    // 인터럽트 상태 원상 복구
    SREG = oldSREG; 

    return m;
}

// 5. 메인 함수 활용 예시 (이전 C 언어 상태 머신 코드와 결합 가능)
int main(void) {
    Timer0_Init(); // 타이머 시작
    
    DDRB |= (1 << PB5); // 우노 내장 LED(13번 핀) 출력 설정

    unsigned long prev_time = 0;

    while (1) {
        unsigned long current_time = millis_custom();

        // 500ms마다 LED 토글 (논블로킹)
        if (current_time - prev_time >= 500) {
            prev_time = current_time;
            PORTB ^= (1 << PB5); // XOR 연산으로 상태 반전
        }
    }
    return 0;
}