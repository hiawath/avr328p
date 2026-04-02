#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// 스캔된 주소를 담을 변수 (기본값 0x27)
byte foundAddr = 0x27; 
LiquidCrystal_I2C *lcd;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println("\n--- I2C Scanner & LCD Start ---");
  
  // 1. I2C 주소 스캔 로직
  bool deviceFound = false;
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("I2C device found at address: 0x");
      Serial.println(address, HEX);
      foundAddr = address;
      deviceFound = true;
      break; // 첫 번째로 발견된 장치(보통 LCD)를 사용
    }
  }

  if (!deviceFound) {
    Serial.println("No I2C devices found. Check wiring!");
    return;
  }

  // 2. 발견된 주소로 LCD 초기화
  lcd = new LiquidCrystal_I2C(foundAddr, 16, 2);
  lcd->init();
  lcd->backlight();

  // 3. 메시지 출력
  lcd->setCursor(0, 0);
  lcd->print("Addr: 0x");
  lcd->print(foundAddr, HEX);
  
  lcd->setCursor(0, 1);
  lcd->print("Scanner Success!");
}

void loop() {
  // 1초 간격으로 백라이트 깜빡이며 동작 확인
  //lcd->noBacklight();
  delay(500);
  lcd->backlight();
  delay(500);
}