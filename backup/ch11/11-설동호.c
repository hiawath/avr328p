// main.c
#include <avr/io.h>
#include <UART0.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define DHT_PIN PD2

void start_dht11();
void wait_ack_dht11();
void rx_dht11();
void drive_dht11_watchdog();

volatile uint8_t dht_req = 0;
volatile uint8_t dht_step = 0;
volatile uint8_t dht_buffer[5];
volatile uint8_t bit_cnt= 0;
volatile uint16_t last_tcnt = 0;

int main() {
    UART0_init();
    drive_dht11_watchdog();

    while (1) {
        if (dht_req) {
            start_dht11();
        }

        if (dht_step == 3) {
            uint8_t checksum = 0;
            for (int i = 0; i < 4; i++) {
                UART0_print_1_byte_number(dht_buffer[i]);
                checksum += dht_buffer[i];
                UART0_print_string(" ");
            }
            //data[4] = 0; // nok test
            checksum == dht_buffer[4] ? UART0_print_string(" ok\r\n") : UART0_print_string(" nok\r\n");
            dht_req = 0;
            dht_step = 0;
        }
    }
}

ISR(WDT_vect) {
    WDTCSR |= (1 << WDIE);
    dht_req = 1;
}

void drive_dht11_watchdog() {
    cli();

    wdt_reset();

    wdt_enable(WDTO_2S); // 2s
    WDTCSR |= (1 << WDIE);

    sei();

    return;
}

void enter_sleep() {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    sleep_enable();

    sleep_cpu();

    sleep_disable();

    return;
}

void start_dht11() {
    if (dht_step != 0) return;

    dht_step = 1;
    bit_cnt = 0;
    for(int i = 0; i < 5; i++) {
        dht_buffer[i] = 0;
    }

    DDRD |= (1 << DHT_PIN);
    PORTD &= ~(1 << DHT_PIN);

    OCR1A = 35999; // 18ms
    TCNT1 = 0;
    TCCR1B |= (1 << WGM12) | (1 << CS11); // prescaler 8
    TIMSK1 |= (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
    if (dht_step == 1) {
        PORTD |= (1 << DHT_PIN);
        DDRD &= ~(1 << DHT_PIN);

        TIMSK1 &= ~(1 << OCIE1A); // timer intr off

        EICRA |= (1 << ISC00);    // falling & rising
        EIMSK |= (1 << INT0);

        dht_step = 2; // set receive mode
        last_tcnt = TCNT1;
    }
}

ISR(INT0_vect) {
    uint16_t current_tcnt = TCNT1;
    uint16_t duration = current_tcnt - last_tcnt;
    last_tcnt = current_tcnt;

    if (!(PIND & (1 << DHT_PIN))) { // every falling
        // ack
        if (bit_cnt < 2) {
            bit_cnt++;
        }
        else if (bit_cnt < 42) {
            if (duration > 100) {
                // -2(excluding ack)
                dht_buffer[(bit_cnt - 2) / 8] |= (1 << (7 - ((bit_cnt - 2) % 8)));
            }

            bit_cnt++;

            if (bit_cnt >= 42) {
                EIMSK &= ~(1 << INT0);
                dht_step = 3;
                TCCR1B = 0;
            }
        }
    }
}