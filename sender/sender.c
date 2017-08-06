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

void serial_send(int fd, uint32_t value) {
    // printf("send: %08x\n", value);
    uint8_t send[5];
    send[0] = 1;
    memcpy(&send[1], &value, 4);
    write(fd, send, 5);
}

uint32_t serial_recv(int fd) {
    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN,
        .revents = 0
    };
    poll(&pfd, 1, 10000);

    uint32_t ret = 0;
    if (read(fd, &ret, 4) != 4) {
        printf("short read\n");
        exit(2);
    }
    // printf("recv: %08x\n", ret);
    return ret;
}

struct crc_state {
    uint32_t final_a;
    uint32_t final_b;
    uint32_t crc;
};

struct crc_state crc_init(uint32_t final_a, uint32_t final_b) {
    struct crc_state ret = {
        .final_a = final_a,
        .final_b = final_b,
        .crc = 0xc387
    };
    return ret;
}

void crc_step(struct crc_state* state, uint32_t value) {
    for (size_t i = 0; i < 32; i++) {
        uint32_t bit = (state->crc ^ value) & 1;
        state->crc = (state->crc >> 1) ^ (bit ? 0xc37b : 0);
        value >>= 1;
    }
}

uint32_t crc_final(struct crc_state* state) {
    crc_step(state, 0xFFFF0000 | (state->final_b << 8) | state->final_a);
    return state->crc;
}

struct encrypt_state {
    uint32_t seed;
};

struct encrypt_state encrypt_init(uint32_t seed) {
    struct encrypt_state ret = {
        .seed = seed
    };
    return ret;
}

uint32_t encrypt_step(struct encrypt_state* state, uint32_t value, uint32_t i) {
    state->seed = state->seed * 0x6F646573 + 1;
    return state->seed ^ value ^ (0xFE000000 - i) ^ 0x43202F2F;
}

void do_multiboot(int fd, char* data, long length) {
    // Wait for GBA to be ready
    printf("Waiting for GBA...\n");
    for (;;) {
        serial_send(fd, 0x6202);
        uint32_t token_recv = serial_recv(fd);

        if ((token_recv >> 16) == 0x7202)
            break;
    }

    // Send first 0xC0 bytes of data

    printf("Sending header.\n");
    serial_send(fd, 0x6102);
    serial_recv(fd);
    uint16_t* data16 = (uint16_t*)data;
    for (size_t i = 0; i < 0xC0; i += 2) {
        serial_send(fd, data16[i / 2]);
        serial_recv(fd);
    }
    serial_send(fd, 0x6200);
    serial_recv(fd);

    // Get encryption and crc seeds

    printf("Getting seeds.\n");

    serial_send(fd, 0x6202);
    serial_recv(fd);
    serial_send(fd, 0x63D1);
    serial_recv(fd);
    serial_send(fd, 0x63D1);
    uint32_t token = serial_recv(fd);
    if ((token >> 24) != 0x73) {
        printf("Failed handshake!\n");
        return;
    }

    uint32_t crc_final_a = (token >> 16) & 0xFF;
    uint32_t seed = 0xFFFF00D1 | (crc_final_a << 8);
    crc_final_a = (crc_final_a + 0xF) & 0xFF;

    serial_send(fd, 0x6400 | crc_final_a);
    serial_recv(fd);

    length += 0xF;
    length &= ~0xF;

    serial_send(fd, (length - 0x190) / 4);
    token = serial_recv(fd);

    uint32_t crc_final_b = (token >> 16) & 0xFF;

    struct crc_state crc = crc_init(crc_final_a, crc_final_b);
    struct encrypt_state enc = encrypt_init(seed);

    printf("Seeds: %02x, %02x, %08x\n", crc_final_a, crc_final_b, seed);

    // Send 

    printf("Sending...\n");

    uint32_t* data32 = (uint32_t*) data;
    for (size_t i = 0xC0; i < length; i += 4) {
        uint32_t datum = data32[i / 4];

        crc_step(&crc, datum);
        datum = encrypt_step(&enc, datum, i);

        serial_send(fd, datum);
        uint32_t check = serial_recv(fd) >> 16;
        if (check != (i & 0xFFFF)) {
            printf("\nTransmission error at byte %zu: check == %08x\n", i, check);
            return;
        }

        printf("\r%lu%%", i * 100 / length);
    }
    printf("\n");
    crc_final(&crc);

    // Checksum

    printf("Waiting for checksum...\n");
    serial_send(fd, 0x0065);
    serial_recv(fd);
    for (;;) {
        serial_send(fd, 0x0065);
        if (serial_recv(fd) >> 16 == 0x0075)
            break;
    }

    serial_send(fd, 0x0066);
    serial_recv(fd);

    serial_send(fd, crc.crc & 0xFFFF);
    uint32_t their_crc = serial_recv(fd) >> 16;
    printf("their crc: %x, our crc: %x\n", their_crc, crc.crc);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("filename required\n");
        return 0;
    }

    int fd = serial_open("/dev/tty.usbmodem12341");
    if (fd < 0) {
        printf("Couldn't open serial\n");
        return 1;
    }

    FILE* gba_file = fopen(argv[1], "rb");
    if (!gba_file) {
        printf("Couldn't open file\n");
        return 1;
    }

    fseek(gba_file, 0, SEEK_END);
    long gba_file_size = ftell(gba_file);
    rewind(gba_file);

    printf("To send file of length %lu\n", gba_file_size);

    char* data = (char*)calloc(gba_file_size + 0x10, 1);
    size_t read_size = fread(data, 1, gba_file_size, gba_file);
    if (read_size != gba_file_size) {
        printf("Failed to read file: (%zu/%zu)", read_size, gba_file_size);
        return 1;
    }

    fclose(gba_file);

    do_multiboot(fd, data, gba_file_size);

    free(data);

    close(fd);

    return 0;
}