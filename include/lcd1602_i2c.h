#ifndef LCD1602_I2C_H_
#define LCD1602_I2C_H_

#include <avr/io.h>

// I2C 주소 (PCF8574: 0x27, PCF8574A: 0x3F) - 쓰기 모드 위해 왼쪽 1비트 시프트
#define LCD_ADDR (0x27 << 1)

// PCF8574 핀 매핑
#define RS_BIT 0
#define RW_BIT 1
#define EN_BIT 2
#define BL_BIT 3

// 명령어 매크로
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME   0x02
#define LCD_SETDDRAMADDR 0x80

// 함수 선언
void lcd_init(void);
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t data);
void lcd_gotoxy(uint8_t x, uint8_t y);
void lcd_print(const char* str);
void lcd_clear(void);

#endif