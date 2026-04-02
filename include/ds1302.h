#ifndef DS1302_H_
#define DS1302_H_

#include <avr/io.h>

// 시간 데이터를 담을 직관적인 구조체
typedef struct {
    uint8_t year;   // 00-99
    uint8_t month;  // 1-12
    uint8_t day;    // 1-31
    uint8_t hour;   // 0-23
    uint8_t min;    // 0-59
    uint8_t sec;    // 0-59
} RTC_Time;

void ds1302_init(void);
void ds1302_set_time(RTC_Time *time);
void ds1302_get_time(RTC_Time *time);
uint8_t ds1302_read_reg(uint8_t reg);
void ds1302_write_reg(uint8_t reg, uint8_t data);



#endif