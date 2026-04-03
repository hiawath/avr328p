#include <SPI.h> // SPI 통신을 사용할 때 필요한 라이브러리, 직렬 통신 방식 
#include <MFRC522.h>
#include <Arduino.h>

#define SS_PIN 10  // SPI 통신에서 슬레이브 선택(SS)핀 -> SDA핀과 연결
#define RST_PIN 9  // RC522를 리셋할 때 사용하는 핀 -> RST핀과 연결

MFRC522 mfrc522(SS_PIN, RST_PIN); // 객체를 통해 RC522 모듈의 함수를 제어

void setup() {
  Serial.begin(9600);
  SPI.begin();             // SPI 통신 시작
  mfrc522.PCD_Init();      // RC522 초기화, 전원이 들어오거나 
                           // 리셋 되었을 때, 내부 레지스터 값이 엉켜 있거나, 불완전한 상태를 방지
  Serial.println("Scan a card..."); // 카드 태그 대기 상태 안내 메세지
}

void loop() {
  // 카드가 감지되지 않으면 루프 반복
  if (!mfrc522.PICC_IsNewCardPresent()) return; // 리더 범위 내에 새로운 카드가 감지 되었는지
  if (!mfrc522.PICC_ReadCardSerial()) return;   // 감지된 카드의 고유 ID(UID)를 읽음

  Serial.print("UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
  // RFID 카드의 UID에서 한 바이트 값을 가져 왔을때, 16보다 작다면 " 0"(공백+0), 크다면 " " 공백만 붙여라
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "); // RC522는 Tag의 고유 UID 정보를 SPI로 아두이노에 전송
    // 아두이노가 이를 받아 Serial 출력  
    Serial.print(mfrc522.uid.uidByte[i], HEX);  // HEX(16진수) 출력
  }
  Serial.println();

  mfrc522.PICC_HaltA();         // 통신 종료
}