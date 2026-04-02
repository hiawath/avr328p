#include "lcd1602_i2c.h"
#include <util/delay.h>

// [주의] i2c_start, i2c_write 등 TWI(I2C) 기본 통신 함수가 구현되어 있다고 가정합니다.
extern void i2c_start(void);
extern void i2c_write(uint8_t data);
extern void i2c_stop(void);

static void lcd_write_pcf8574(uint8_t data) {
    i2c_start();
    i2c_write(LCD_ADDR);
    i2c_write(data | (1 << BL_BIT)); // 백라이트 항상 켜짐 유지
    i2c_stop();
}

static void lcd_pulse_enable(uint8_t data) {
    lcd_write_pcf8574(data | (1 << EN_BIT));
    _delay_us(1);
    lcd_write_pcf8574(data & ~(1 << EN_BIT));
    _delay_us(50);
}

static void lcd_send(uint8_t value, uint8_t mode) {
    uint8_t high = (value & 0xF0) | mode;
    uint8_t low = ((value << 4) & 0xF0) | mode;

    lcd_write_pcf8574(high);
    lcd_pulse_enable(high);
    lcd_write_pcf8574(low);
    lcd_pulse_enable(low);
}

void lcd_command(uint8_t cmd) { lcd_send(cmd, 0); }
void lcd_data(uint8_t data) { lcd_send(data, (1 << RS_BIT)); }

void lcd_init(void) {
    _delay_ms(50);
    // 4비트 초기화 시퀀스
    lcd_write_pcf8574(0x30); lcd_pulse_enable(0x30); _delay_ms(5);
    lcd_write_pcf8574(0x30); lcd_pulse_enable(0x30); _delay_us(150);
    lcd_write_pcf8574(0x30); lcd_pulse_enable(0x30);
    lcd_write_pcf8574(0x20); lcd_pulse_enable(0x20); // 4-bit mode 설정

    lcd_command(0x28); // 2 Line, 5x8 Matrix
    lcd_command(0x0C); // Display ON, Cursor OFF
    lcd_command(0x06); // Entry Mode: Increment
    lcd_clear();
}

void lcd_gotoxy(uint8_t x, uint8_t y) {
    uint8_t addr = (y == 0) ? (0x80 + x) : (0xC0 + x);
    lcd_command(addr);
}

void lcd_print(const char* str) {
    while (*str) lcd_data(*str++);
}

void lcd_clear(void) {
    lcd_command(LCD_CLEARDISPLAY);
    _delay_ms(2);
}