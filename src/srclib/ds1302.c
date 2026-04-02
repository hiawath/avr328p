#include "ds1302.h"
#include <util/delay.h>

#define DS_CE_PORT   PORTB
#define DS_CE_DDR    DDRB
#define DS_CE_PIN    PB0

#define DS_IO_PORT   PORTD
#define DS_IO_DDR    DDRD
#define DS_IO_IN     PIND
#define DS_IO_PIN    PD7

#define DS_SCLK_PORT PORTD
#define DS_SCLK_DDR  DDRD
#define DS_SCLK_PIN  PD6

static uint8_t dec2bcd(uint8_t v){ return ((v/10)<<4)|(v%10); }
static uint8_t bcd2dec(uint8_t v){ return ((v>>4)*10)+(v&0x0F); }

static inline void clk_high(){ DS_SCLK_PORT |=  (1<<DS_SCLK_PIN); }
static inline void clk_low() { DS_SCLK_PORT &= ~(1<<DS_SCLK_PIN); }

void ds1302_write_reg(uint8_t reg, uint8_t data) {
    DS_CE_PORT |= (1 << DS_CE_PIN);
    _delay_us(4);

    DS_IO_DDR |= (1 << DS_IO_PIN);

    // ── 주소 전송
    for (uint8_t i = 0; i < 8; i++) {
        if (reg & (1 << i)) DS_IO_PORT |=  (1 << DS_IO_PIN);
        else                 DS_IO_PORT &= ~(1 << DS_IO_PIN);
        _delay_us(1);
        DS_SCLK_PORT |= (1 << DS_SCLK_PIN);
        _delay_us(2);
        DS_SCLK_PORT &= ~(1 << DS_SCLK_PIN);
        _delay_us(2);
    }

    // ── 데이터 전송
    for (uint8_t i = 0; i < 8; i++) {
        if (data & (1 << i)) DS_IO_PORT |=  (1 << DS_IO_PIN);
        else                  DS_IO_PORT &= ~(1 << DS_IO_PIN);
        _delay_us(1);
        DS_SCLK_PORT |= (1 << DS_SCLK_PIN);
        _delay_us(2);
        DS_SCLK_PORT &= ~(1 << DS_SCLK_PIN);
        _delay_us(2);
    }

    DS_CE_PORT &= ~(1 << DS_CE_PIN);
    _delay_us(4);
}

uint8_t ds1302_read_reg(uint8_t reg) {
    uint8_t data = 0;

    DS_CE_PORT |= (1 << DS_CE_PIN);
    _delay_us(4);

    DS_IO_DDR |= (1 << DS_IO_PIN);

    // ── 주소 전송
    for (uint8_t i = 0; i < 8; i++) {
        if (reg & (1 << i)) DS_IO_PORT |=  (1 << DS_IO_PIN);
        else                 DS_IO_PORT &= ~(1 << DS_IO_PIN);

        _delay_us(1);
        DS_SCLK_PORT |= (1 << DS_SCLK_PIN);
        _delay_us(2);

        if (i == 7) {
            // ✅ 마지막 비트: CLK↓ 직전에 입력 전환
            //    → DS1302가 CLK↓와 동시에 IO 드라이브할 때 충돌 없음
            DS_IO_DDR  &= ~(1 << DS_IO_PIN);
            DS_IO_PORT &= ~(1 << DS_IO_PIN);  // 풀업 OFF
        }

        DS_SCLK_PORT &= ~(1 << DS_SCLK_PIN); // ← 이 하강엣지에서 DS1302가 bit0 출력
        _delay_us(2);
    }

    // ── 데이터 수신 (LSB first, CLK LOW 상태에서 읽기)
    for (uint8_t i = 0; i < 8; i++) {
        if (DS_IO_IN & (1 << DS_IO_PIN)) data |= (1 << i);

        if (i < 7) {
            DS_SCLK_PORT |= (1 << DS_SCLK_PIN);
            _delay_us(2);
            DS_SCLK_PORT &= ~(1 << DS_SCLK_PIN);
            _delay_us(2);
        }
    }

    DS_CE_PORT &= ~(1 << DS_CE_PIN);
    _delay_us(4);

    return data;
}

void ds1302_init(void){
    DS_CE_DDR   |= (1<<DS_CE_PIN);
    DS_SCLK_DDR |= (1<<DS_SCLK_PIN);

    clk_low();
    DS_CE_PORT &= ~(1<<DS_CE_PIN);

    _delay_ms(100);  // ✅ 10ms → 100ms

    ds1302_write_reg(0x8E, 0x00);  // WP off
    ds1302_write_reg(0x80, 0x00);  // CH bit clear
}

void ds1302_set_time(RTC_Time *t){
    ds1302_write_reg(0x8E, 0x00);
    ds1302_write_reg(0x8C, dec2bcd(t->year));
    ds1302_write_reg(0x88, dec2bcd(t->month));
    ds1302_write_reg(0x86, dec2bcd(t->day));
    ds1302_write_reg(0x84, dec2bcd(t->hour));
    ds1302_write_reg(0x82, dec2bcd(t->min));
    ds1302_write_reg(0x80, dec2bcd(t->sec));
    ds1302_write_reg(0x8E, 0x80);
}

void ds1302_get_time(RTC_Time *t){
    t->sec   = bcd2dec(ds1302_read_reg(0x81) & 0x7F);
    t->min   = bcd2dec(ds1302_read_reg(0x83) & 0x7F);
    t->hour  = bcd2dec(ds1302_read_reg(0x85) & 0x3F);
    t->day   = bcd2dec(ds1302_read_reg(0x87) & 0x3F);
    t->month = bcd2dec(ds1302_read_reg(0x89) & 0x1F);
    t->year  = bcd2dec(ds1302_read_reg(0x8D));
}