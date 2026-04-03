/**
 * rfid.h
 * RC522 RFID 드라이버 헤더 (spi.h 기반)
 *
 * 핀 배선:
 *   SCK  -> D13 (PB5)
 *   MISO -> D12 (PB4)
 *   MOSI -> D11 (PB3)
 *   SS   -> D10 (PB2)  ← SS_PIN_BIT
 *   RST  -> D9  (PB1)  ← RST_PIN_BIT
 *   VCC  -> 3.3V
 *   GND  -> GND
 */

#ifndef RFID_H
#define RFID_H

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "my_spi.h"

/* ─────────────────────────────────────────
   RST 핀 정의 (SS 는 spi_cs_t 로 관리)
──────────────────────────────────────────*/
#define RFID_RST_DDR     DDRB
#define RFID_RST_PORT    PORTB
#define RFID_RST_PIN_BIT PB1

/* ─────────────────────────────────────────
   RC522 레지스터 주소
──────────────────────────────────────────*/
#define REG_COMMAND        0x01
#define REG_COM_IEN        0x02
#define REG_COM_IRQ        0x04
#define REG_DIV_IRQ        0x05
#define REG_ERROR          0x06
#define REG_FIFO_DATA      0x09
#define REG_FIFO_LEVEL     0x0A
#define REG_CONTROL        0x0C
#define REG_BIT_FRAMING    0x0D
#define REG_MODE           0x11
#define REG_TX_CONTROL     0x14
#define REG_TX_ASK         0x15
#define REG_CRC_RESULT_H   0x21
#define REG_CRC_RESULT_L   0x22
#define REG_T_MODE         0x2A
#define REG_T_PRESCALER    0x2B
#define REG_T_RELOAD_H     0x2C
#define REG_T_RELOAD_L     0x2D
#define REG_VERSION        0x37

/* ─────────────────────────────────────────
   RC522 커맨드
──────────────────────────────────────────*/
#define CMD_IDLE           0x00
#define CMD_CALC_CRC       0x03
#define CMD_TRANSCEIVE     0x0C
#define CMD_SOFT_RESET     0x0F

/* ─────────────────────────────────────────
   MIFARE 명령
──────────────────────────────────────────*/
#define PICC_REQA          0x26
#define PICC_ANTICOLL      0x93

/* ─────────────────────────────────────────
   상태 코드
──────────────────────────────────────────*/
typedef enum {
    RFID_OK      = 0,
    RFID_NOTAG   = 1,
    RFID_ERROR   = 2,
    RFID_TIMEOUT = 3
} rfid_status_t;

/* ─────────────────────────────────────────
   카드 UID 구조체
──────────────────────────────────────────*/
#define RFID_UID_MAX_LEN 10

typedef struct {
    uint8_t bytes[RFID_UID_MAX_LEN];
    uint8_t size;
} rfid_uid_t;

/* ─────────────────────────────────────────
   공개 API
──────────────────────────────────────────*/

/**
 * @brief RC522 초기화
 * @param cs  SS 핀 구조체 (spi_cs_t)
 *            spi_init() 은 rfid_init() 내부에서 호출
 */
void rfid_init(spi_cs_t cs);

/**
 * @brief RC522 펌웨어 버전 읽기 (정상: 0x91 또는 0x92)
 */
uint8_t rfid_get_version(void);

/**
 * @brief 카드 UID 읽기
 * @param uid  읽어온 UID 저장
 * @return RFID_OK / RFID_NOTAG / RFID_ERROR
 */
rfid_status_t rfid_read_uid(rfid_uid_t *uid);

/* ─────────────────────────────────────────
   내부 함수 선언
──────────────────────────────────────────*/
void    rc522_write_reg(uint8_t reg, uint8_t value);
uint8_t rc522_read_reg(uint8_t reg);
void    rc522_set_bits(uint8_t reg, uint8_t mask);
void    rc522_clear_bits(uint8_t reg, uint8_t mask);
void    rc522_reset(void);
void    rc522_antenna_on(void);

rfid_status_t rc522_request(uint8_t req_mode, uint8_t *tag_type);
rfid_status_t rc522_anticoll(rfid_uid_t *uid);
rfid_status_t rc522_to_card(uint8_t cmd,
                             uint8_t *send_data, uint8_t send_len,
                             uint8_t *back_data, uint16_t *back_len);

#endif /* RFID_H */