/**
 * rfid-test.c
 * RC522 RFID 카드 ID 읽기 테스트 (spi.h/rfid.h 기반)
 *
 * 빌드:
 *   avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os \
 *           -o rfid-test.elf rfid-test.c rfid.c spi.c
 *   avr-objcopy -O ihex rfid-test.elf rfid-test.hex
 *   avrdude -c arduino -p m328p -P /dev/ttyUSB0 -b 115200 \
 *           -U flash:w:rfid-test.hex
 */

#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include "my_spi.h"
#include "my_rfid.h"
#include "my_uart.h"


/* ─────────────────────────────────────────
   헬퍼
──────────────────────────────────────────*/
static void print_uid(const rfid_uid_t *uid)
{
    uart_print("UID: ");
    for (uint8_t i = 0; i < uid->size; i++) {
        if (i > 0) uart_putchar(':');
        printf("%02X", uid->bytes[i]);
    }
    uart_putchar('\r');
    uart_putchar('\n');
}

static uint8_t uid_equal(const rfid_uid_t *a, const rfid_uid_t *b)
{
    if (a->size != b->size) return 0;
    return (memcmp(a->bytes, b->bytes, a->size) == 0);
}

/* ─────────────────────────────────────────
   메인
──────────────────────────────────────────*/
int main(void)
{
    uart_init();
    stdout = uart_get_stdout();

    uart_print("\n===========================\n");
    uart_print("  RC522 RFID UID Reader\n");
    uart_print("  SS=PB2(D10), RST=PB1(D9)\n");
    uart_print("===========================\n\n");

    /* SS 핀: PB2 (D10) */
    spi_cs_t rfid_cs = {
        .ddr  = &DDRB,
        .port = &PORTB,
        .pin  = PB2
    };
    rfid_init(rfid_cs);

    uint8_t version = rfid_get_version();
    printf("RC522 Version: 0x%02X", version);
    uart_print((version == 0x91 || version == 0x92) ? "  [OK]\n"
                                                    : "  [WARN]\n");

    uart_print("Ready - tap a card...\n");

    rfid_uid_t uid_current;
    rfid_uid_t uid_last;
    memset(&uid_last, 0, sizeof(uid_last));
    uint8_t same_count = 0;

    while (1) {
        memset(&uid_current, 0, sizeof(uid_current));
        rfid_status_t status = rfid_read_uid(&uid_current);

        if (status == RFID_OK) {
            if (!uid_equal(&uid_current, &uid_last)) {
                print_uid(&uid_current);
                uid_last   = uid_current;
                same_count = 0;
            } else if (++same_count >= 10) {
                memset(&uid_last, 0, sizeof(uid_last));
                same_count = 0;
            }
        } else if (status == RFID_ERROR) {
            uart_print("[ERROR] Communication error\n");
        }

        _delay_ms(100);
    }
    return 0;
}