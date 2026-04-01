#include <Arduino.h>

void setup(){
    Serial.begin(9600);
    pinMode(LED_BUILTIN,OUTPUT);
}
unsigned int count = 0;
void loop(){
    
    digitalWrite(LED_BUILTIN,HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN,LOW);
    delay(1000);
	
}