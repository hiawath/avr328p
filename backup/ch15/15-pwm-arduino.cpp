#include <Arduino.h>
#include <Servo.h>


Servo myServo;  // 서보 객체 생성

void setup() {
  myServo.attach(9);  // 9번 핀에 서보 모터 연결
}

void loop() { 
  // 0도에서 180도까지 회전
  for (int pos = 0; pos <= 180; pos += 90) {
    myServo.write(pos);              // 각도 명령
    delay(1000);                       // 모터가 움직일 시간 대기
  }

  // 180도에서 0도까지 회전
  for (int pos = 180; pos >= 0; pos -= 90) {
    myServo.write(pos);
    delay(1000);
  }
}