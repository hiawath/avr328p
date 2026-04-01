#define F_CPU 16000000L
#define BIT_DELAY_US 104
#define MY_RX PD2   // Rx pin
#define MY_TX PD3   // Tx pin
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

void UART0_init(void);
// 1. 선언부 수정 (int 반환, FILE *stream 추가)
int UART0_transmit(char data, FILE *stream);
int UART0_receive(FILE *stream);
 
FILE OUTPUT \
= FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);

FILE INPUT \
= FDEV_SETUP_STREAM(NULL, UART0_receive, _FDEV_SETUP_READ);

void UART0_init(void)
{
    UBRR0H = 0x00; 
    UBRR0L = 207;
    UCSR0A |= _BV(U2X0); 
    UCSR0C |= 0x06;
    UCSR0B |= _BV(RXEN0) | _BV(TXEN0);

    // my_uart init
    DDRD &= ~(1 << MY_RX);    // rx
    DDRD |= (1 << MY_TX);     // tx
    PORTD |= (1 << MY_TX);    // high 유지
}

int UART0_transmit(char data, FILE *stream)
{
    if (data == '\n') UART0_transmit('\r', stream);
    
    while( !(UCSR0A & (1 << UDRE0)) ); 
    UDR0 = data; 
    
    return 0;
}

int UART0_receive(FILE *stream)
{
    while( !(UCSR0A & (1 << RXC0)) ); 
    char data = UDR0;
    
    return data;
}

void my_uart_tx(char data){
    // start bit
    PORTD &= ~(1 << MY_TX);
    _delay_us(BIT_DELAY_US);

    for (int i = 0; i < 8; i++) {
        if (data & (1 << i))
            PORTD |= (1 << MY_TX);
        else
            PORTD &= ~(1 << MY_TX);

        _delay_us(BIT_DELAY_US);    // 9600기준 104us 마다
    }

    // stop bit
    PORTD |= (1 << MY_TX);
    _delay_us(500);
}

char my_uart_rx()
{
    char data = 0;

    // data bit 중앙 샘플링
    _delay_us(BIT_DELAY_US/2);

    for (int i = 0; i < 8; i++) {
        if (PIND & (1 << MY_RX))
            data |= (1 << i);

        _delay_us(BIT_DELAY_US);
    }

    // stop bit 대기
    _delay_us(BIT_DELAY_US);

    return data;
}

int main(void)
{
    UART0_init(); 
    stdout = &OUTPUT; 
    stdin = &INPUT;

    char c, c_in;

    while (1)
    {
        // 외부 -> PC
        if (!(PIND & (1 << MY_RX))) { // start bit 기다림
            _delay_us(BIT_DELAY_US);
            c = my_uart_rx();
            printf("%c", c);
        }

        // PC -> 외부
        if (UCSR0A & (1 << RXC0)) {
            scanf("%c", &c_in);
            my_uart_tx(c_in);
        }
    }

    return 0;
}