#include <avr/io.h>
#include <util/delay.h>
#include <unity.h>

// 1. UART 초기화 (Unity 출력용)
void uart_init(void) {
    UBRR0H = 0;
    UBRR0L = 103; // 16MHz 클럭 기준 9600 bps
    UCSR0B = (1 << TXEN0); // 송신 활성화
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8비트 데이터
}

// 2. Unity가 한 문자를 출력할 때 호출할 함수
void unittest_uart_putchar(char c) {
    while (!(UCSR0A & (1 << UDRE0))); // 송신 버퍼 비워질 때까지 대기
    UDR0 = c;
}

// 테스트 케이스 예시
void test_bit_manipulation(void) {
    uint8_t reg = 0x00;
    reg |= (1 << 3);
    TEST_ASSERT_EQUAL_HEX8(0x08, reg);
}

int main(void) {
    uart_init();
    _delay_ms(2000); // 보드 안정화 대기

    UNITY_BEGIN();
    RUN_TEST(test_bit_manipulation);
    UNITY_END();

    while (1);
    return 0;
}