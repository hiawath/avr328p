#include <avr/io.h>
#include <util/delay.h>
#include "lcd1602_i2c.h"

int main(void) {
    // I2C 하드웨어(TWI) 초기화 함수 필요
    i2c_init();

    // LCD 초기화
    lcd_init();

    // 메시지 출력
    lcd_gotoxy(0, 0);
    lcd_print("Hello, Andrew!");

    lcd_gotoxy(0, 1);
    lcd_print("AVR I2C System");

    while (1) {
        // 백그라운드 작업 수행
    }

    return 0;
}