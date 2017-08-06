#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

int serial_open(const char* device_name) {
    int fd = open(device_name, O_RDWR | O_NDELAY | O_SYNC | O_NOCTTY);
    if (fd < 0) return fd;

    struct termios tio = {0};
    tcgetattr(fd, &tio);

    tio.c_cc[VMIN] = 4;
    tio.c_cc[VTIME] = 5;
    tio.c_oflag &= ~OPOST; // raw, unprocessed
    tio.c_cflag |= CLOCAL | CS8 | CREAD;

    if (tcsetattr(fd, TCSANOW, &tio)) {
        close(fd);
        return -1;
    }

    return fd;
}

uint32_t serial_recv(int fd) {
    uint8_t send[5] = {0};
    write(fd, send, 5);

    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN,
        .revents = 0
    };
    poll(&pfd, 1, 1000000);

    uint32_t ret = 0;
    if (read(fd, &ret, 4) != 4) {
        printf("short read\n");
        exit(2);
    }
    return ret;
}

int main(int argc, char* argv[]) {
    int fd = serial_open("/dev/tty.usbmodem12341");
    if (fd < 0) {
        printf("Couldn't open serial\n");
        return 1;
    }

    for (;;) {
        printf("%c", (char)serial_recv(fd));
        fflush(stdout);
    }

    return 0;
}
