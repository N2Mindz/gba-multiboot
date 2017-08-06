#include <avr/interrupt.h>
#include <stdio.h>
#include <usb_serial.h>
#include <util/delay.h>

#define GBA_CLK  0b00000001
#define GBA_MOSI 0b00000010
#define GBA_MISO 0b00000100

#define MODE_NOWAIT 1

int main() {
    // CPU Prescaler
    // Set for 16 MHz clock
    cli();
    CLKPR = 0x80;
    CLKPR = 0;
    sei();

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
        if (usb_serial_available() < 5)
            continue;

        uint8_t mode = usb_serial_getchar();

        uint32_t send = (uint32_t)usb_serial_getchar();
        send |= (uint32_t)usb_serial_getchar() << 8;
        send |= (uint32_t)usb_serial_getchar() << 16;
        send |= (uint32_t)usb_serial_getchar() << 24;

        uint32_t recv = 0;

        // See http://www.akkit.org/info/gba_comms.html for details.

        if (!(mode & MODE_NOWAIT)) {
            while (PINB & GBA_MISO) {} // Spin until slave is ready.
        }

        cli();

        TCNT0 = 0;
        TIFR0 = 0b00000010;
        for (size_t i = 0; i < 32; i++) {
            PORTB &= ~(GBA_CLK | GBA_MOSI);
            PORTB |= (send >> 31) ? GBA_MOSI : 0;
            send <<= 1;

            while (!(TIFR0 & 0b00000010)) {}
            TIFR0 = 0b00000010;

            PORTB |= GBA_CLK;
            recv <<= 1;
            recv |= (PINB & GBA_MISO) ? 1 : 0;

            while (!(TIFR0 & 0b00000010)) {}
            TIFR0 = 0b00000010;
        }

        PORTB |= GBA_MOSI | GBA_CLK;

        sei();

        usb_serial_putchar(recv);
        usb_serial_putchar(recv >> 8);
        usb_serial_putchar(recv >> 16);
        usb_serial_putchar(recv >> 24);
        usb_serial_flush_output();
    }
    return 0;
}