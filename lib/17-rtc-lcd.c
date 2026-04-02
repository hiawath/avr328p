#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

// 분리된 모듈 헤더 포함
#include "i2c.h"
#include "lcd1602_i2c.h"
#include "ds1302.h"
#include "UART0.h"
FILE OUTPUT \
= FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);


int main(void) {
    UART0_init();
    stdout = &OUTPUT;

    char buf[17];
    RTC_Time currentTime;

    // 1. 하드웨어 초기화 (의존성 순서대로)
    i2c_init();      // I2C 버스 초기화
    lcd_init();      // LCD 초기화
    ds1302_init();   // RTC 칩 활성화

    // 2. 초기 시간 세팅 (필요할 때만 주석 해제하여 1회 실행)
    
    currentTime.year = 26;
    currentTime.month = 4;
    currentTime.day = 2;
    currentTime.hour = 19;
    currentTime.min = 45;
    currentTime.sec = 0;
    //ds1302_set_time(&currentTime);
    

    /* 13 pin led toggle */
    DDRB |= (1 << PB5);
    uint8_t test;

    // 3. 메인 무한 루프
    while (1) {
        // 모듈에서 시간 데이터를 구조체로 깔끔하게 받아옴
        ds1302_get_time(&currentTime);

        // 첫 번째 줄: 20YY/MM/DD
        sprintf(buf, "20%02d/%02d/%02d",
                currentTime.year, currentTime.month, currentTime.day);
        lcd_gotoxy(0, 0);
        lcd_print(buf);

        // 두 번째 줄: TIME HH:MM:SS
        sprintf(buf, "TIME %02d:%02d:%02d",
                currentTime.hour, currentTime.min, currentTime.sec);
        lcd_gotoxy(0, 1);
        lcd_print(buf);

        // 1초 대기 후 갱신
        _delay_ms(1000);
        /* 13 pin led toggle */
        PORTB ^= (1 << PB5);
        test = ds1302_read_reg(0x81);
        printf("%d\n", test);
    }

    return 0;
}