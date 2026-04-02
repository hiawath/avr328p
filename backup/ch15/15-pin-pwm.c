#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

// 핀 정의: PB0 (Arduino Digital 8)
#define SERVO_PIN   PB1
#define SERVO_PORT  PORTB
#define SERVO_DDR   DDRB

// 각도별 High 유지 시간 (마이크로초 단위)
// SG90 기준: 0도=1000us, 90도=1500us, 180도=2000us
void send_servo_pulse(uint16_t pulse_us) {
    SERVO_PORT |= (1 << SERVO_PIN);  // High
    
    // 가변 인자를 사용할 수 없으므로 루프로 미세 조절하거나 
    // _delay_us는 상수가 필요하므로 아래와 같이 처리
    for (uint16_t i = 0; i < pulse_us; i++) {
        _delay_us(1);
    }
    
    SERVO_PORT &= ~(1 << SERVO_PIN); // Low
    
    // 전체 주기 20ms(20000us)를 맞추기 위한 나머지 시간 대기
    for (uint16_t i = 0; i < (20000 - pulse_us); i++) {
        _delay_us(1);
    }
}

int main(void) {
    SERVO_DDR |= (1 << SERVO_PIN); // PB0를 출력으로 설정

    while (1) {
        // 0도 이동 (약 1초간 펄스 유지 - 50번 반복하면 약 1초)
        for (int i = 0; i < 50; i++) send_servo_pulse(500);
        
        // 90도 이동
        for (int i = 0; i < 50; i++) send_servo_pulse(1200);
        
        // 180도 이동
        for (int i = 0; i < 50; i++) send_servo_pulse(1800);
        
    }
}