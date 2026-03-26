#include "UART0.h"



void UART0_init(void)
{
    UBRR0H = 0x00;
    UBRR0L = 207;           // 9,600 보율 설정
    UCSR0A |= _BV(U2X0);    // 2배속 모드
    UCSR0C = 0x06;          // 비동기, 8비트 데이터, 패리티 없음, 1비트 정지 비트
    UCSR0B |= _BV(RXEN0);   // 수신 가능
    UCSR0B |= _BV(TXEN0);   // 송신 가능
}

void UART0_transmit(char data)
{
    while(!(UCSR0A & (1<<UDRE0))); // 송신 대기
    UDR0 = data;
}

unsigned char UART0_receive(void)
{
    while(!(UCSR0A & (1<<RXC0))); // 수신 대기
    return UDR0;
}

void UART0_print_string(char *str)
{
    for(int i = 0; str[i]; i++)
        UART0_transmit(str[i]);
}

void UART0_print_1_byte_number(uint8_t n)
{
    char numString[4] = "0";
    int i, index = 0;
    if(n > 0) {
        for(i = 0; n != 0; i++) {
            numString[i] = n % 10 + '0';
            n = n / 10;
        }
        numString[i] = '\0';
        index = i - 1;
    }
    for(i = index; i >= 0; i--)
        UART0_transmit(numString[i]);
}

