#include "i2c.h"

void i2c_init(void) {
    // SCL 주파수 계산: SCL = F_CPU / (16 + 2 * TWBR * Prescaler)
    // 16MHz CPU에서 100kHz를 만들기 위해 TWBR을 72로 설정 (Prescaler 1)
    TWSR = 0x00; // Prescaler = 1
    TWBR = 72;
    TWCR = (1 << TWEN); // TWI 활성화
}

void i2c_start(void) {
    // START 조건 전송
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    // 작업 완료 대기 (TWINT 비트가 1이 될 때까지)
    while (!(TWCR & (1 << TWINT)));
}

void i2c_stop(void) {
    // STOP 조건 전송
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    // STOP은 하드웨어가 자동으로 처리하므로 대기 루프가 필요 없음
}

void i2c_write(uint8_t data) {
    // 데이터를 데이터 레지스터에 로드
    TWDR = data;
    // 전송 시작
    TWCR = (1 << TWINT) | (1 << TWEN);
    // 전송 완료 대기
    while (!(TWCR & (1 << TWINT)));
}

uint8_t i2c_read_ack(void) {
    // 데이터를 읽고 ACK 전송 응답 설정
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t i2c_read_nack(void) {
    // 데이터를 읽고 NACK 전송 응답 설정 (마지막 바이트 읽기)
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}