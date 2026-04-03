/**
 * my_uart.h
 * AVR ATmega328P UART0 드라이버 헤더
 *
 * 사용법:
 *   uart_init();
 *   stdout = uart_get_stdout();   // printf → UART 연결
 *   uart_print("hello\n");
 */

#ifndef MY_UART_H
#define MY_UART_H

#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>

/* ─────────────────────────────────────────
   보드레이트 설정 (기본 9600, F_CPU 필수)
──────────────────────────────────────────*/
#ifndef UART_BAUD
#define UART_BAUD  9600UL
#endif

#define UART_UBRR_VAL  (F_CPU / 16 / UART_BAUD - 1)

/* ─────────────────────────────────────────
   공개 API
──────────────────────────────────────────*/

/**
 * @brief UART0 초기화 (8N1, UART_BAUD)
 */
void uart_init(void);

/**
 * @brief 1바이트 송신 (\n 앞에 \r 자동 삽입)
 */
void uart_putchar(char c);

/**
 * @brief 문자열 송신
 */
void uart_print(const char *str);

/**
 * @brief 1바이트 수신 (블로킹)
 * @return 수신된 문자
 */
char uart_getchar(void);

/**
 * @brief printf 연결용 FILE 스트림 포인터 반환
 *        stdout = uart_get_stdout(); 으로 사용
 */
FILE *uart_get_stdout(void);

#endif /* MY_UART_H */