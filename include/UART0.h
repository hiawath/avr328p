#ifndef UART0_H_
#define UART0_H_

#include <avr/io.h>

void UART0_init(void);
void UART0_transmit(char data);
unsigned char UART0_receive(void);
void UART0_print_string(char *str);
void UART0_print_1_byte_number(uint8_t n);


#endif /* UART0_H_ */