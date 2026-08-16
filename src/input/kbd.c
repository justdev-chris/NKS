// src/input/kbd.c
// NKS Keyboard Input

#include "kbd.h"
#include "../panic/panic.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

static int kbd_fd = -1;
static uint8_t key_state[256];

#ifdef __FreeBSD__
#include <sys/ioctl.h>
// #include <dev/atkbdc/atkbdcreg.h>
#endif

static const uint8_t hid_to_nks[256] = {
    [0x04] = 'A', [0x05] = 'B', [0x06] = 'C', [0x07] = 'D',
    [0x08] = 'E', [0x09] = 'F', [0x0A] = 'G', [0x0B] = 'H',
    [0x0C] = 'I', [0x0D] = 'J', [0x0E] = 'K', [0x0F] = 'L',
    [0x10] = 'M', [0x11] = 'N', [0x12] = 'O', [0x13] = 'P',
    [0x14] = 'Q', [0x15] = 'R', [0x16] = 'S', [0x17] = 'T',
    [0x18] = 'U', [0x19] = 'V', [0x1A] = 'W', [0x1B] = 'X',
    [0x1C] = 'Y', [0x1D] = 'Z',
    [0x1E] = '1', [0x1F] = '2', [0x20] = '3', [0x21] = '4',
    [0x22] = '5', [0x23] = '6', [0x24] = '7', [0x25] = '8',
    [0x26] = '9', [0x27] = '0',
    [0x28] = '\n', [0x29] = 27, [0x2A] = 127, [0x2B] = '\t',
    [0x2C] = ' ', [0x2D] = '-', [0x2E] = '=', [0x2F] = '[',
    [0x30] = ']', [0x31] = '\\', [0x32] = '#', [0x33] = ';',
    [0x34] = '\'', [0x35] = '`', [0x36] = ',', [0x37] = '.',
    [0x38] = '/',
    [0x52] = 128, [0x51] = 129, [0x50] = 130, [0x4F] = 131,
};

int kbd_init(void) {
    memset(key_state, 0, sizeof(key_state));
    
#ifdef __FreeBSD__
    kbd_fd = open("/dev/uhid0", O_RDWR);
    if (kbd_fd < 0) {
        kbd_fd = open("/dev/kbd0", O_RDWR);
        if (kbd_fd < 0) {
            kbd_fd = open("/dev/kbdmux0", O_RDWR);
            if (kbd_fd < 0) {
                kitty_panic_simple("No keyboard found!");
                return -1;
            }
        }
    }
#else
    // Linux/dummy
    kbd_fd = -1;
#endif
    return 0;
}

void kbd_shutdown(void) {
    if (kbd_fd >= 0) {
        close(kbd_fd);
        kbd_fd = -1;
    }
}

void kbd_poll(void) {
#ifdef __FreeBSD__
    if (kbd_fd < 0) return;
    unsigned char buf[64];
    ssize_t bytes = read(kbd_fd, buf, sizeof(buf));
    if (bytes > 0) {
        for (int i = 2; i < bytes && i < 8; i++) {
            int key = buf[i];
            if (key > 0 && key < 256 && hid_to_nks[key]) {
                key_state[hid_to_nks[key]] = 1;
            }
        }
    }
#endif
}

int kbd_is_pressed(uint8_t key) {
    if (key >= 256) return 0;
    return key_state[key];
}

void kbd_clear_state(void) {
    memset(key_state, 0, sizeof(key_state));
}

uint8_t kbd_wait_key(void) {
    while (1) {
        kbd_poll();
        for (int i = 0; i < 256; i++) {
            if (key_state[i]) {
                uint8_t key = i;
                kbd_clear_state();
                return key;
            }
        }
#ifdef __FreeBSD__
        psleep(10);;
#else
        sleep(0.01);
#endif
    }
}
