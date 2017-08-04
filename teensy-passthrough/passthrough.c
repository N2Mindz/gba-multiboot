#include <avr/interrupt.h>
#include <stdio.h>
#include <usb_serial.h>
#include <util/delay.h>

#define GBA_CLK  0b00000001
#define GBA_MOSI 0b00000010
#define GBA_MISO 0b00000100

int main() {
    // CPU Prescaler
    // Set for 16 MHz clock
    CLKPR = 0x80;
    CLKPR = 0;

    // Configure pins:
    //          DDR=1   DDR=0
    // PORT=1   high    pull-up
    // PORT=0   low     floating
    // Input and unused pins are configured as pull-up.
    DDRB  = GBA_CLK | GBA_MOSI;
    PORTB = 0b11111111;

    // Configure timer.
    // Transmission happens at 256 kHz.
    OCR0A  = F_CPU / 256000;
    TCCR0A = 0b01000011;
    TCCR0B = 0b00001001;
    TIMSK0 = 0;

    usb_init();
    while (!usb_configured()) {}
    _delay_ms(1000);

    for (;;) {
        if (usb_serial_available() < 4)
            continue;

        uint32_t send = (uint32_t)usb_serial_getchar();
        send |= (uint32_t)usb_serial_getchar() << 8;
        send |= (uint32_t)usb_serial_getchar() << 16;
        send |= (uint32_t)usb_serial_getchar() << 24;

        uint32_t recv = 0;
        
        cli();

        TCNT0 = 0;
        TIFR0 = 0b00000010;

        for (size_t i = 0; i < 32; i++) {
            uint32_t bit = 1 << (31 - i);

            if (send & bit) {
                PORTB = (PORTB & ~GBA_CLK) | GBA_MOSI;
            } else {
                PORTB = (PORTB & ~(GBA_CLK | GBA_MOSI));
            }

            while(!(TIFR0 & 0b00000010)) {}
            TIFR0 = 0b00000010;

            PORTB |= GBA_CLK;
            recv |= (PINB & GBA_MISO) ? bit : 0;

            while(!(TIFR0 & 0b00000010)) {}
            TIFR0 = 0b00000010;
        }

        sei();

        usb_serial_putchar_nowait(recv);
        usb_serial_putchar_nowait(recv >> 8);
        usb_serial_putchar_nowait(recv >> 16);
        usb_serial_putchar_nowait(recv >> 24);
    }
    return 0;
}