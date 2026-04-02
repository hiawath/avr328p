#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "UART0.h"

#define F_CPU 16000000UL // CPU 클럭 설정 (16MHz)

// DHT11 연결 핀 정의 (PORTD의 2번 핀, Arduino Uno의 D2)
#define DHT11_DDR       DDRD
#define DHT11_PORT      PORTD
#define DHT11_PINREG    PIND
#define DHT11_BIT       PD2

volatile uint32_t g_millis = 0; // 시스템 가동 후 경과 시간(ms)

// DHT11 통신 상태를 정의하는 열거형
typedef enum {
    DHT_IDLE = 0,        // 대기 상태
    DHT_START_LOW,       // 시작 신호 전송 중 (MCU가 Low 출력)
    DHT_WAIT_RESPONSE,   // 센서의 응답 신호 대기 및 확인 중
    DHT_READING,         // 40비트 데이터 비트 수신 중
    DHT_DONE,            // 데이터 수신 완료
    DHT_FAIL             // 오류 발생
} dht_state_t;

volatile dht_state_t dht_state = DHT_IDLE;

// 데이터 저장을 위한 변수들
volatile uint8_t dht_bytes[5];     // 수신된 5바이트(습도 2, 온도 2, 체크섬 1)
volatile uint8_t dht_byte_index = 0; // 현재 저장 중인 바이트 번호
volatile uint8_t dht_bit_index  = 0; // 현재 저장 중인 비트 번호 (0~7)
volatile uint8_t dht_data_ready = 0; // 메인 루프에 데이터 완료를 알리는 플래그
volatile uint8_t dht_error_flag = 0; // 메인 루프에 오류를 알리는 플래그

volatile uint16_t last_edge_time = 0; // 마지막 신호 변화 시점의 Timer1 값
volatile uint8_t response_stage = 0;  // 응답 신호(Low/High) 단계 확인용
volatile uint32_t dht_state_start_ms = 0; // 각 상태가 시작된 시간(ms)

/* PCINT용: 이전 포트 상태를 저장하여 어떤 핀이 바뀌었는지 확인 */
volatile uint8_t prev_portd_state = 0;

/* -------------------- 시간 관리 함수 -------------------- */
uint32_t millis_now(void){
    uint32_t t;
    cli(); // 인터럽트 일시 정지 (원자적 읽기 보장)
    t = g_millis;
    sei(); // 인터럽트 재개
    return t;
}

/* -------------------- Timer0: 1ms 시스템 틱 생성 -------------------- */
void timer0_init_1ms(void){
    TCCR0A = (1 << WGM01);                 // CTC 모드
    TCCR0B = (1 << CS01) | (1 << CS00);   // 분주비 64
    OCR0A  = 249;                         // (16MHz / 64 / 1000) - 1 = 249
    TIMSK0 = (1 << OCIE0A);               // 비교 일치 인터럽트 활성화
}

ISR(TIMER0_COMPA_vect){
    g_millis++; // 1ms마다 카운트 업
}

/* -------------------- Timer1: 신호 길이 정밀 측정 (0.5us 단위) -------------------- */
void timer1_init_pulse_counter(void){
    TCCR1A = 0x00;          // 일반 모드
    TCCR1B = (1 << CS11);   // 분주비 8 => 16MHz/8 = 2MHz (1클럭당 0.5us)
    TCNT1  = 0;             // 카운터 초기화 
}

/* -------------------- PCINT2 설정 (포트 D 변화 감지) -------------------- */
void pcint2_init_for_pd2(void){
    PCICR  |= (1 << PCIE2);       // PORTD 그룹 인터럽트 활성화
    PCMSK2 &= ~(1 << PCINT18);    // 초기에는 PD2 인터럽트 비활성화
}

// 핀 변화 인터럽트 활성화
void dht_pcint_enable(void){
    prev_portd_state = PIND;       // 현재 상태 기록
    PCMSK2 |= (1 << PCINT18);     // PD2(PCINT18) 활성화
}

// 핀 변화 인터럽트 비활성화
void dht_pcint_disable(void){
    PCMSK2 &= ~(1 << PCINT18);
}

/* -------------------- DHT11 핀 제어 도우미 -------------------- */
void dht_pin_output(void) { DHT11_DDR |= (1 << DHT11_BIT); }
void dht_pin_input_pullup(void) {
    DHT11_DDR &= ~(1 << DHT11_BIT);
    DHT11_PORT |= (1 << DHT11_BIT); // 내부 풀업 저항 활성화
}
void dht_line_low(void) { DHT11_PORT &= ~(1 << DHT11_BIT); }

/* -------------------- 통신 시작 요청 -------------------- */
void dht11_start_request(void){
    memset((void*)dht_bytes, 0, sizeof(dht_bytes)); //수신 배열 초기화
    dht_byte_index = 0; dht_bit_index = 0;          //인덱스 초기화
    dht_data_ready = 0; dht_error_flag = 0; response_stage = 0;// 플래그 초기화

    dht_pin_output();   // 핀을 출력 모드로 변경
    dht_line_low();     // 라인을 Low로 내려서 센서를 깨움

    dht_state = DHT_START_LOW;          // 상태 변경
    dht_state_start_ms = millis_now();  // 시작 시간 기록
}

/* -------------------- DHT 상태 머신 프로세스 -------------------- */
// while(1)에서 계속 실행되며 시간 흐름에 따른 상태 전환을 관리함
/* 비동기 상태 머신 처리 함수 (while문에서 호출) */
void dht11_process(void)
{
    uint32_t now = millis_now();
    switch (dht_state)
    {
        case DHT_IDLE: break; // 대기 중

        case DHT_START_LOW:   // MCU가 Low 신호를 보내는 중
            if ((now - dht_state_start_ms) >= 18) // 18ms가 지났다면
            {
                dht_pin_input_pullup(); // 핀을 입력(풀업)으로 바꿔서 신호 수신 준비

                cli(); TCNT1 = 0; last_edge_time = 0; sei(); // 타이머 초기화

                dht_pcint_enable(); // 이제부터 센서 신호를 인터럽트로 감지

                dht_state = DHT_WAIT_RESPONSE; // 응답 대기 상태로 변경
                dht_state_start_ms = now;
            }
            break;

        case DHT_WAIT_RESPONSE: // 응답/데이터 수신 중 타임아웃 체크
        case DHT_READING:
            if ((now - dht_state_start_ms) > 10) // 10ms 이상 응답 없으면 실패
            {
                dht_pcint_disable();
                dht_state = DHT_FAIL;
                dht_error_flag = 1;
            }
            break;

        case DHT_DONE: // 수신 완료 후 체크섬 확인
        {
            uint8_t checksum = dht_bytes[0] + dht_bytes[1] + dht_bytes[2] + dht_bytes[3];
            if (checksum == dht_bytes[4]) dht_data_ready = 1; // 일치하면 성공
            else dht_error_flag = 1;                         // 다르면 에러
            dht_state = DHT_IDLE;
            break;
        }

        case DHT_FAIL: dht_state = DHT_IDLE; break; // 실패 시 초기화
    }
}

/* -------------------- 실시간 신호 해석 (ISR) -------------------- */
ISR(PCINT2_vect) // 포트D의 핀 상태가 바뀔 때마다 실행
{
    uint8_t curr_portd = PIND;              // 현재 포트D 값 읽기
    uint8_t changed = curr_portd ^ prev_portd_state; // XOR 연산: 이전과 비교해서 바뀐 핀만 1로 표시

    if (changed & (1 << PD2)) // PD2 핀이 바뀐 것이라면
    {
        uint16_t now = TCNT1;               // 현재 타이머1 값 (0.5us 단위)
        uint16_t pulse_width = now - last_edge_time; // 이전 변화 후 흐른 시간 계산
        last_edge_time = now;               // 현재 시간 저장

        uint8_t pin_now = (curr_portd & (1 << PD2)) ? 1 : 0; // 현재 핀이 H인지 L인지 확인

        if (dht_state == DHT_WAIT_RESPONSE) // 현재 통신 상태가 '응답 대기'라면
        {
            
            if (response_stage == 0 && pin_now == 0) { response_stage = 1; } // 처음으로 핀이 Low가 되면 '응답 시작' 기록
            else if (response_stage == 1 && pin_now == 1) // Low였다가 다시 High로 올라가면 (센서의 Low 신호 끝)
            {
                //Low 유지 시간이 약 80us(160틱) 전후인지 확인 (120~220 범위)
                if (pulse_width > 120 && pulse_width < 220) response_stage = 2; // 약 80us 확인
                else { dht_state = DHT_FAIL; dht_error_flag = 1; dht_pcint_disable(); } // 너무 짧거나 길면 가짜 신호! 실패 처리
            }
            else if (response_stage == 2 && pin_now == 0) // 응답 High 끝, 데이터 시작
            {
                if (pulse_width > 120 && pulse_width < 220) { dht_state = DHT_READING; dht_state_start_ms = millis_now(); } // 이제부터 데이터를 읽을 준비 완료
                else { dht_state = DHT_FAIL; dht_error_flag = 1; dht_pcint_disable(); } // 읽기 시작 시간 기록
            }
        }
        else if (dht_state == DHT_READING) // 현재 상태가 '데이터 읽기'라면
        {
            if (pin_now == 0) // 신호가 High에서 Low로 떨어질 때 (비트 길이 판별)
            {
                // 100틱(50us)보다 길게 High를 유지했다면 비트 '1', 짧았다면 '0'으로 판정
                uint8_t bit_value = (pulse_width > 100) ? 1 : 0; 

                dht_bytes[dht_byte_index] <<= 1; // 비트를 왼쪽으로 밀고
                dht_bytes[dht_byte_index] |= bit_value; // 새로운 비트 추가
                dht_bit_index++; // 읽은 비트 개수 하나 증가

                if (dht_bit_index >= 8) // 비트를 8개 모아서 1바이트를 다 채웠다면
                {
                    dht_bit_index = 0; // 비트 카운트 초기화
                    dht_byte_index++; // 다음 바구니(배열 인덱스)로 이동
                    if (dht_byte_index >= 5) { //총 5바이트(40비트)를 모두 읽었다면
                         dht_pcint_disable(); // 더 이상 인터럽트가 필요 없으니 핀 감시 중단
                          dht_state = DHT_DONE; } // 완료
                }
            }
        }
    }
    prev_portd_state = curr_portd; // 다음 인터럽트 때 비교를 위해 현재 포트 상태를 '이전 상태'로 저장
}

/* -------------------- 메인 루프 -------------------- */
int main(void)
{
    UART0_init();               // 시리얼 통신 시작
    timer0_init_1ms();          // 시스템 틱 가동
    timer1_init_pulse_counter(); // 정밀 타이머 가동
    pcint2_init_for_pd2();      // 핀 인터럽트 준비

    sei(); // 전체 인터럽트 활성화 (매우 중요)

    char buffer[64];
    uint32_t last_request_time = 0;
    uint8_t humi_int = 0, humi_dec = 0, temp_int = 0, temp_dec = 0;

    UART0_print_string("DHT11 PCINT2 example start\r\n");

    while (1)
    {
        uint32_t now = millis_now();

        // 2초(2000ms) 간격으로 센서 측정 시작
        if ((dht_state == DHT_IDLE) && ((now - last_request_time) >= 2000))
        {
            last_request_time = now;
            dht11_start_request();
        }

        dht11_process(); // 백그라운드에서 상태 머신 처리

        if (dht_data_ready) // 데이터가 정상적으로 수집되었다면
        {
            cli(); // 값 복사 중 인터럽트 방지
            humi_int = dht_bytes[0]; humi_dec = dht_bytes[1];
            temp_int = dht_bytes[2]; temp_dec = dht_bytes[3];
            dht_data_ready = 0;
            sei();

            // 시리얼로 온도/습도 출력
            sprintf(buffer, "temp: %d.%02d C, humi: %d.%02d %%\r\n", temp_int, temp_dec, humi_int, humi_dec);
            UART0_print_string(buffer);
        }

        if (dht_error_flag) // 에러 발생 시 출력
        {
            dht_error_flag = 0;
            UART0_print_string("DHT11 read error\r\n");
        }
    }
}