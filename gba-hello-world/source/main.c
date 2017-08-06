#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_sio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void serial_init() {
    REG_RCNT = 0;
    REG_SIOCNT = SIO_32BIT | SIO_SO_HIGH;
}

void serial_rawsend(u32 value) {
    REG_SIODATA32 = value;
    REG_SIOCNT = SIO_32BIT;
    REG_SIOCNT = SIO_32BIT | SIO_SO_HIGH | SIO_START;
    while (REG_SIOCNT & SIO_START) {}
}

int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);

    consoleDemoInit();

    iprintf("\x1b[10;8HTesting...\n");
    iprintf("\x1b[11;8HHello world!\n");

    const char* serial_str = "Hello world!\n";

    for (;;) {
        VBlankIntrWait();
        for (size_t i = 0; i < strlen(serial_str); i++) {
            serial_rawsend(serial_str[i]);
        }
    }
}


