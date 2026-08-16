// src/input/kbd.c
// NKS Keyboard Input - USB HID + syscons

#include "kbd.h"
#include "../panic/panic.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dev/atkbdc/atkbdcreg.h>   // For KBD_* ioctls
#include <string.h>
#include <stdio.h>

static int kbd_fd = -1;
static uint8_t key_state[256];  // Key pressed/released

// USB HID keycodes (simplified)
static const uint8_t hid_to_nks[256] = {
    // Map USB HID keycodes to our internal keymap
    // This is a simplified mapping - expand as needed
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
    [0x28] = '\n',  // Enter
    [0x29] = 27,    // Escape
    [0x2A] = 127,   // Backspace
    [0x2B] = '\t',  // Tab
    [0x2C] = ' ',   // Space
    [0x2D] = '-',   [0x2E] = '=',   [0x2F] = '[',   [0x30] = ']',
    [0x31] = '\\',  [0x32] = '#',   [0x33] = ';',   [0x34] = '\'',
    [0x35] = '`',   [0x36] = ',',   [0x37] = '.',   [0x38] = '/',
    // Arrow keys
    [0x52] = 128,   // Up
    [0x51] = 129,   // Down
    [0x50] = 130,   // Left
    [0x4F] = 131,   // Right
};

// FreeBSD keymap (for /dev/kbd0)
static const uint8_t kbd_to_nks[128] = {
    // Map scancodes to our keymap
    [0x01] = 27,    // Escape
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
    [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
    [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=', [0x0E] = 127, // Backspace
    [0x0F] = '\t', // Tab
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R',
    [0x14] = 'T', [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I',
    [0x18] = 'O', [0x19] = 'P', [0x1A] = '[', [0x1B] = ']',
    [0x1C] = '\n', // Enter
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F',
    [0x22] = 'G', [0x23] = 'H', [0x24] = 'J', [0x25] = 'K',
    [0x26] = 'L', [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\', [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C',
    [0x2F] = 'V', [0x30] = 'B', [0x31] = 'N', [0x32] = 'M',
    [0x33] = ',', [0x34] = '.', [0x35] = '/',
    [0x39] = ' ', // Space
    // Arrow keys (extended)
    [0x48] = 128, // Up
    [0x50] = 129, // Down
    [0x4B] = 130, // Left
    [0x4D] = 131, // Right
};

int kbd_init(void) {
    // Try /dev/uhid0 first (USB keyboards)
    kbd_fd = open("/dev/uhid0", O_RDWR);
    if (kbd_fd < 0) {
        // Fall back to /dev/kbd0 (syscons)
        kbd_fd = open("/dev/kbd0", O_RDWR);
        if (kbd_fd < 0) {
            // Fall back to /dev/kbdmux0 (keyboard multiplexer)
            kbd_fd = open("/dev/kbdmux0", O_RDWR);
            if (kbd_fd < 0) {
                // No keyboard found - warn but don't panic
                kitty_panic_simple("No keyboard found! Using empty input.");
                return -1;
            }
        }
    }
    
    // For /dev/kbd, set raw mode (don't translate)
    // Only if we're using syscons
    if (kbd_fd >= 0) {
        // Try to set raw mode
        int mode = K_RAW;
        ioctl(kbd_fd, KDSKBMODE, &mode);
    }
    
    memset(key_state, 0, sizeof(key_state));
    return 0;
}

void kbd_shutdown(void) {
    if (kbd_fd >= 0) {
        close(kbd_fd);
        kbd_fd = -1;
    }
}

void kbd_poll(void) {
    if (kbd_fd < 0) return;
    
    // For /dev/uhid0 (USB HID)
    // Read raw HID reports
    // This is simplified - real HID reports are more complex
    unsigned char buf[64];
    ssize_t bytes = read(kbd_fd, buf, sizeof(buf));
    if (bytes > 0) {
        // Parse HID report
        // Report format: modifier byte + reserved + keycodes (6 keys)
        // We'll just check for key presses/releases
        for (int i = 0; i < bytes; i++) {
            // This is a simplified HID parser - expand as needed
            if (i >= 2 && i < 8) {  // Keycode bytes
                int key = buf[i];
                if (key > 0 && key < 256) {
                    uint8_t nks_key = hid_to_nks[key];
                    if (nks_key) {
                        key_state[nks_key] = 1;
                        // Also add release detection (not implemented here)
                    }
                }
            }
        }
    }
    
    // For /dev/kbd0 (syscons)
    // Use ioctl to get key state
    // Not implemented - use USB HID instead
}

int kbd_is_pressed(uint8_t key) {
    if (key >= 256) return 0;
    return key_state[key];
}

void kbd_clear_state(void) {
    memset(key_state, 0, sizeof(key_state));
}

// Wait for a key press (blocking)
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
        usleep(10000);  // 10ms
    }
}
