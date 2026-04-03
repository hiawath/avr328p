/**
 * my_uart.c
 * AVR ATmega328P UART0 드라이버 구현
 */

#include "my_uart.h"

/* ─────────────────────────────────────────
   내부: printf 스트림 콜백
──────────────────────────────────────────*/
static int _uart_stream_putchar(char c, FILE *stream)
{
    (void)stream;
    if (c == '\n') uart_putchar('\r');
    uart_putchar(c);
    return 0;
}

static FILE _uart_stdout =
    FDEV_SETUP_STREAM(_uart_stream_putchar, NULL, _FDEV_SETUP_WRITE);

/* ═══════════════════════════════════════════
   공개 API 구현
═══════════════════════════════════════════*/

void uart_init(void)
{
    UBRR0H = (uint8_t)(UART_UBRR_VAL >> 8);
    UBRR0L = (uint8_t)(UART_UBRR_VAL);
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   /* 8N1 */
}

void uart_putchar(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = (uint8_t)c;
}

void uart_print(const char *str)
{
    while (*str) {
        if (*str == '\n') uart_putchar('\r');
        uart_putchar(*str++);
    }
}

char uart_getchar(void)
{
    while (!(UCSR0A & (1 << RXC0)));
    return (char)UDR0;
}

FILE *uart_get_stdout(void)
{
    return &_uart_stdout;
}