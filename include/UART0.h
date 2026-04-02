#ifndef UART0_H_
#define UART0_H_

#include <avr/io.h>

void UART0_init(void);
void UART0_transmit(char data);
unsigned char UART0_receive(void);
void UART0_print_string(char *str);
void UART0_print_1_byte_number(uint8_t n);
void UART0_print_sensor_data(char *sensor_name, uint8_t int_part, uint8_t dec_part, char *unit);
#endif /* UART0_H_ */