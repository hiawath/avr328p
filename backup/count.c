#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "UART0.h" // 라이브러리 포함

FILE OUTPUT = FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);

int main(void)
{

    UART0_init();     // 초기화
    stdout = &OUTPUT; // printf 사용 설정

    char str[] = "Test using UART0 Library";
    uint8_t num = 128;

    UART0_print_string(str); // 문자열 출력
    UART0_print_string("\r\n");
    UART0_print_1_byte_number(num); // 숫자 출력
    UART0_print_string("\r\n");

    unsigned int count = 0;

    while (1)
    {

        printf("%d\n\r", count++);
        _delay_ms(1000);
    }
    return 0;
}