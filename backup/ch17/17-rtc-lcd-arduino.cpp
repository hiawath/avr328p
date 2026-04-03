#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// 1. DS1302 설정 (IO, SCLK, CE 순서)
ThreeWire myWire(7, 6, 8);
RtcDS1302<ThreeWire> Rtc(myWire);

// 2. LCD 설정 (주소 0x27, 16열 2행)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup()
{
    Serial.begin(9600);

    // LCD 초기화
    lcd.init();
    lcd.backlight();

    // RTC 초기화
    Rtc.Begin();

    // 컴파일 시점의 시간으로 RTC 설정 (처음 한 번만 실행)
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    Rtc.SetDateTime(compiled);

    if (Rtc.GetIsWriteProtected())
    {
        Rtc.SetIsWriteProtected(false);
    }
    if (!Rtc.GetIsRunning())
    {
        Rtc.SetIsRunning(true);
    }
}

void loop()
{
    // RTC에서 현재 시간 가져오기
    RtcDateTime now = Rtc.GetDateTime();

    // LCD 첫 번째 줄: 날짜 출력 (YYYY/MM/DD)
    char dateBuf[16];
    sprintf(dateBuf, "%04d/%02d/%02d", now.Year(), now.Month(), now.Day());
    lcd.setCursor(0, 0);
    lcd.print(dateBuf);

    // LCD 두 번째 줄: 시간 출력 (HH:MM:SS)
    char timeBuf[16];
    sprintf(timeBuf, "TIME: %02d:%02d:%02d", now.Hour(), now.Minute(), now.Second());
    lcd.setCursor(0, 1);
    lcd.print(timeBuf);

    delay(1000); // 1초마다 업데이트
}