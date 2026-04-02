#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "i2c.h"
#include "UART0.h" // 이전에 만든 인터럽트 기반 UART 라이브러리 가정

void i2c_scan() {
    char buf[32];
    uint8_t address;
    uint8_t devices_found = 0;

    UART0_print_string("\r\n--- I2C Scanner Starting ---\r\n");

    for (address = 1; address < 127; address++) {
        // 1. START 신호
        TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
        while (!(TWCR & (1 << TWINT)));

        // 2. 주소 전송 (Write 모드: address << 1)
        TWDR = (address << 1);
        TWCR = (1 << TWINT) | (1 << TWEN);
        while (!(TWCR & (1 << TWINT)));

        // 3. 상태 확인 (0x18 은 ACK 수신 성공을 의미)
        if ((TWSR & 0xF8) == 0x18) {
            sprintf(buf, "Found device at: 0x%02X\r\n", address);
            UART0_print_string(buf);
            devices_found++;
        }

        // 4. STOP 신호
        TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
        _delay_ms(5); // 안정화를 위한 짧은 지연
    }

    if (devices_found == 0) {
        UART0_print_string("No I2C devices found.r\n");
    } else {
        UART0_print_string("Scan complete.\r\n");
    }
}

int main(void) {
    UART0_init(); // 9600 bps 등 설정
    i2c_init();  // 100kHz 설정

    while (1) {
        i2c_scan();
        _delay_ms(5000); // 5초마다 재검사
    }
}