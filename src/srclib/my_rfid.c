/**
 * rfid.c
 * RC522 RFID 드라이버 구현 — spi.h/spi.c 사용
 */

#include "my_rfid.h"

/* ─────────────────────────────────────────
   모듈 내부 상태
──────────────────────────────────────────*/
static spi_cs_t _cs;   /* rfid_init() 에서 설정 */

/* ═══════════════════════════════════════════
   RC522 레지스터 접근
   RC522 SPI 프로토콜:
     주소 바이트 = (reg << 1) & 0x7E          (쓰기)
     주소 바이트 = ((reg << 1) & 0x7E) | 0x80  (읽기)
═══════════════════════════════════════════*/

void rc522_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { (reg << 1) & 0x7E, value };
    spi_transaction(_cs, buf, NULL, 2);
}

uint8_t rc522_read_reg(uint8_t reg)
{
    uint8_t tx[2] = { ((reg << 1) & 0x7E) | 0x80, 0xFF };
    uint8_t rx[2];
    spi_transaction(_cs, tx, rx, 2);
    return rx[1];
}

void rc522_set_bits(uint8_t reg, uint8_t mask)
{
    rc522_write_reg(reg, rc522_read_reg(reg) | mask);
}

void rc522_clear_bits(uint8_t reg, uint8_t mask)
{
    rc522_write_reg(reg, rc522_read_reg(reg) & ~mask);
}

/* ═══════════════════════════════════════════
   RC522 초기화
═══════════════════════════════════════════*/

void rc522_reset(void)
{
    rc522_write_reg(REG_COMMAND, CMD_SOFT_RESET);
    _delay_ms(50);
}

void rc522_antenna_on(void)
{
    uint8_t tx = rc522_read_reg(REG_TX_CONTROL);
    if ((tx & 0x03) != 0x03) {
        rc522_set_bits(REG_TX_CONTROL, 0x03);
    }
}

/**
 * @brief SPI + RC522 전체 초기화
 * @param cs  SS 핀 (spi_cs_t)
 */
void rfid_init(spi_cs_t cs)
{
    /* SPI 초기화 (MODE0, 4 MHz, MSB first) */
    spi_config_t cfg = SPI_CONFIG_DEFAULT;
    spi_init(&cfg);

    /* CS 핀 초기화 */
    _cs = cs;
    spi_cs_init(_cs);

    /* RST 핀 출력 설정 */
    RFID_RST_DDR  |=  (1 << RFID_RST_PIN_BIT);

    /* 하드웨어 리셋: LOW → HIGH */
    RFID_RST_PORT &= ~(1 << RFID_RST_PIN_BIT);
    _delay_ms(10);
    RFID_RST_PORT |=  (1 << RFID_RST_PIN_BIT);
    _delay_ms(50);

    rc522_reset();

    /* 타이머 설정 */
    rc522_write_reg(REG_T_MODE,      0x8D);
    rc522_write_reg(REG_T_PRESCALER, 0x3E);
    rc522_write_reg(REG_T_RELOAD_L,  30);
    rc522_write_reg(REG_T_RELOAD_H,  0);

    /* 100% ASK 변조 */
    rc522_write_reg(REG_TX_ASK, 0x40);

    /* CRC 초기값 0x6363 (ISO 14443-3) */
    rc522_write_reg(REG_MODE, 0x3D);

    rc522_antenna_on();
}

uint8_t rfid_get_version(void)
{
    return rc522_read_reg(REG_VERSION);
}

/* ═══════════════════════════════════════════
   카드 통신 핵심 루틴
═══════════════════════════════════════════*/

rfid_status_t rc522_to_card(uint8_t cmd,
                             uint8_t *send_data, uint8_t send_len,
                             uint8_t *back_data, uint16_t *back_len)
{
    rfid_status_t status = RFID_ERROR;
    uint8_t irq_en   = 0x00;
    uint8_t wait_irq = 0x00;
    uint8_t n;
    uint16_t i;

    if (cmd == CMD_TRANSCEIVE) {
        irq_en   = 0x77;
        wait_irq = 0x30;
    }

    rc522_write_reg(REG_COM_IEN, irq_en | 0x80);
    rc522_clear_bits(REG_COM_IRQ, 0x80);
    rc522_set_bits(REG_FIFO_LEVEL, 0x80);
    rc522_write_reg(REG_COMMAND, CMD_IDLE);

    /* FIFO 적재: CS 유지한 채 주소 1번 → 데이터 N번 연속 전송
       (매 바이트마다 주소를 재전송하면 RC522가 오류 처리함) */
    SPI_CS_LOW(_cs);
    spi_transfer_byte((REG_FIFO_DATA << 1) & 0x7E);   /* 주소 1회 */
    for (uint8_t k = 0; k < send_len; k++) {
        spi_transfer_byte(send_data[k]);               /* 데이터 연속 */
    }
    SPI_CS_HIGH(_cs);

    rc522_write_reg(REG_COMMAND, cmd);
    if (cmd == CMD_TRANSCEIVE) {
        rc522_set_bits(REG_BIT_FRAMING, 0x80);
    }

    i = 2000;
    do {
        n = rc522_read_reg(REG_COM_IRQ);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & wait_irq));

    rc522_clear_bits(REG_BIT_FRAMING, 0x80);

    if (i == 0) return RFID_TIMEOUT;
    if (rc522_read_reg(REG_ERROR) & 0x1B) return RFID_ERROR;

    if (n & wait_irq) {
        if (!(n & 0x08)) {
            if (cmd == CMD_TRANSCEIVE) {
                uint8_t  last_bits;
                uint8_t  fifo_n = rc522_read_reg(REG_FIFO_LEVEL);
                last_bits = rc522_read_reg(REG_CONTROL) & 0x07;
                *back_len = last_bits ? (fifo_n - 1) * 8 + last_bits
                                      : fifo_n * 8;
                if (fifo_n == 0) fifo_n = 1;
                /* burst read로 FIFO 전체 읽기 */
                for (i = 0; i < fifo_n; i++) {
                    back_data[i] = rc522_read_reg(REG_FIFO_DATA);
                }
            }
            status = RFID_OK;
        }
    }

    return status;
}

rfid_status_t rc522_request(uint8_t req_mode, uint8_t *tag_type)
{
    rfid_status_t status;
    uint16_t back_bits;

    rc522_write_reg(REG_BIT_FRAMING, 0x07);
    tag_type[0] = req_mode;
    status = rc522_to_card(CMD_TRANSCEIVE, tag_type, 1, tag_type, &back_bits);

    if ((status != RFID_OK) || (back_bits != 0x10)) {
        return RFID_NOTAG;
    }
    return RFID_OK;
}

rfid_status_t rc522_anticoll(rfid_uid_t *uid)
{
    rfid_status_t status;
    uint8_t  serial_check = 0;
    uint8_t  serial_buf[2];
    uint16_t back_bits;
    uint8_t  back_data[8];

    rc522_write_reg(REG_BIT_FRAMING, 0x00);
    serial_buf[0] = PICC_ANTICOLL;
    serial_buf[1] = 0x20;

    status = rc522_to_card(CMD_TRANSCEIVE,
                           serial_buf, 2,
                           back_data, &back_bits);
    if (status == RFID_OK) {
        for (uint8_t i = 0; i < 4; i++) {
            serial_check ^= back_data[i];
        }
        if (serial_check != back_data[4]) return RFID_ERROR;

        uid->size = 4;
        for (uint8_t i = 0; i < 4; i++) {
            uid->bytes[i] = back_data[i];
        }
    }
    return status;
}

rfid_status_t rfid_read_uid(rfid_uid_t *uid)
{
    uint8_t tag_type[2];

    if (rc522_request(PICC_REQA, tag_type) != RFID_OK) {
        return RFID_NOTAG;
    }
    return rc522_anticoll(uid);
}