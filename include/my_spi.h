/**
 * spi.h
 * AVR ATmega328P 범용 Hardware SPI 드라이버
 *
 * 핀 (고정, 하드웨어 SPI):
 *   SCK  -> PB5 (D13)
 *   MOSI -> PB3 (D11)
 *   MISO -> PB4 (D12)  입력
 *   SS   -> 사용자 정의 GPIO (소프트웨어 제어)
 */

#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <util/delay.h>

/* ═══════════════════════════════════════════
   SPI 모드 (CPOL / CPHA 조합)
═══════════════════════════════════════════*/
typedef enum
{
    SPI_MODE0 = 0, /* CPOL=0, CPHA=0 — 상승 엣지 샘플, RC522·SD카드 등  */
    SPI_MODE1 = 1, /* CPOL=0, CPHA=1 — 하강 엣지 샘플                    */
    SPI_MODE2 = 2, /* CPOL=1, CPHA=0 — 하강 엣지 샘플                    */
    SPI_MODE3 = 3  /* CPOL=1, CPHA=1 — 상승 엣지 샘플, MAX31855 등       */
} spi_mode_t;

/* ═══════════════════════════════════════════
   SPI 클럭 분주비 (fosc 기준)
═══════════════════════════════════════════*/
typedef enum
{
    SPI_CLK_DIV4 = 0,   /*  4 MHz @ 16 MHz (기본값)  */
    SPI_CLK_DIV16 = 1,  /*  1 MHz                    */
    SPI_CLK_DIV64 = 2,  /*  250 kHz                  */
    SPI_CLK_DIV128 = 3, /*  125 kHz                  */
    SPI_CLK_DIV2 = 4,   /*  8 MHz (SPI2X)            */
    SPI_CLK_DIV8 = 5,   /*  2 MHz (SPI2X)            */
    SPI_CLK_DIV32 = 6,  /*  500 kHz (SPI2X)          */
    /* DIV64x2 는 DIV64 와 동일하므로 생략 */
} spi_clk_div_t;

/* ═══════════════════════════════════════════
   비트 순서
═══════════════════════════════════════════*/
typedef enum
{
    SPI_MSB_FIRST = 0, /* 일반 장치 대부분 */
    SPI_LSB_FIRST = 1  /* 일부 LED 드라이버 등 */
} spi_bit_order_t;

/* ═══════════════════════════════════════════
   SPI 설정 구조체
═══════════════════════════════════════════*/
typedef struct
{
    spi_mode_t mode;
    spi_clk_div_t clk_div;
    spi_bit_order_t bit_order;
} spi_config_t;

/* ═══════════════════════════════════════════
   슬레이브 선택 핀 구조체 (GPIO 직접 제어)
   여러 장치를 동일 SPI 버스에 연결할 때 사용
═══════════════════════════════════════════*/
typedef struct
{
    volatile uint8_t *ddr;  /* ex) &DDRB  */
    volatile uint8_t *port; /* ex) &PORTB */
    uint8_t pin;            /* ex) PB2    */
} spi_cs_t;

/* ═══════════════════════════════════════════
   기본 설정 매크로 (RC522 등 MODE0, 4 MHz)
═══════════════════════════════════════════*/
#define SPI_CONFIG_DEFAULT \
    {SPI_MODE0, SPI_CLK_DIV4, SPI_MSB_FIRST}

/* ═══════════════════════════════════════════
   CS 핀 편의 매크로
═══════════════════════════════════════════*/
#define SPI_CS_INIT(cs) spi_cs_init(cs)
#define SPI_CS_LOW(cs) (*((cs).port) &= ~(1 << (cs).pin))
#define SPI_CS_HIGH(cs) (*((cs).port) |= (1 << (cs).pin))

/* ═══════════════════════════════════════════
   공개 API
═══════════════════════════════════════════*/

/**
 * @brief SPI 하드웨어 초기화 (마스터 모드)
 * @param cfg  설정 구조체 포인터 (NULL 이면 기본값 사용)
 */
void spi_init(const spi_config_t *cfg);

/**
 * @brief 현재 설정 변경 (버스 재초기화)
 * @param cfg  새 설정
 */
void spi_reconfigure(const spi_config_t *cfg);

/**
 * @brief 현재 SPI 설정 가져오기
 * @param cfg  결과 저장 포인터
 */
void spi_get_config(spi_config_t *cfg);

/**
 * @brief CS 핀 출력 초기화 + 비선택 상태(HIGH)
 * @param cs  CS 핀 구조체
 */
void spi_cs_init(spi_cs_t cs);

/**
 * @brief 1바이트 송수신 (Full-Duplex)
 * @param data  보낼 바이트
 * @return 수신 바이트
 */
uint8_t spi_transfer_byte(uint8_t data);

/**
 * @brief 버퍼 전체 송수신
 *   tx_buf 가 NULL 이면 0xFF 를 송신 (읽기 전용)
 *   rx_buf 가 NULL 이면 수신 데이터 버림 (쓰기 전용)
 * @param tx_buf  송신 버퍼 (NULL 가능)
 * @param rx_buf  수신 버퍼 (NULL 가능)
 * @param len     바이트 수
 */
void spi_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len);

/**
 * @brief 쓰기 전용 (수신 무시)
 */
void spi_write_buf(const uint8_t *tx_buf, size_t len);

/**
 * @brief 읽기 전용 (0xFF 송신)
 */
void spi_read_buf(uint8_t *rx_buf, size_t len);

/**
 * @brief CS LOW → 버퍼 송수신 → CS HIGH (원자적 트랜잭션)
 * @param cs      CS 핀 구조체
 * @param tx_buf  송신 버퍼 (NULL 가능)
 * @param rx_buf  수신 버퍼 (NULL 가능)
 * @param len     바이트 수
 */
void spi_transaction(spi_cs_t cs,
                     const uint8_t *tx_buf, uint8_t *rx_buf,
                     size_t len);

/**
 * @brief 레지스터 쓰기  (addr 1바이트 + data 1바이트)
 *   RC522, 가속도 센서 등 레지스터 기반 장치에 활용
 * @param cs    CS 핀
 * @param addr  레지스터 주소
 * @param data  쓸 데이터
 */
void spi_write_reg(spi_cs_t cs, uint8_t addr, uint8_t data);

/**
 * @brief 레지스터 읽기  (addr 1바이트 → data 1바이트)
 * @param cs    CS 핀
 * @param addr  레지스터 주소 (읽기 플래그는 호출자가 OR 해서 전달)
 * @return 수신 데이터
 */
uint8_t spi_read_reg(spi_cs_t cs, uint8_t addr);

/**
 * @brief 연속 레지스터 읽기 (burst read)
 * @param cs      CS 핀
 * @param addr    시작 레지스터 주소
 * @param rx_buf  수신 버퍼
 * @param len     읽을 바이트 수
 */
void spi_read_reg_burst(spi_cs_t cs, uint8_t addr,
                        uint8_t *rx_buf, size_t len);

/**
 * @brief SPI 비활성화 (저전력 대기)
 */
void spi_disable(void);

/**
 * @brief SPI 재활성화 (spi_disable 후 복구)
 */
void spi_enable(void);

#endif /* SPI_H */