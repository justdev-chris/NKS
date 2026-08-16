// src/display/fb.c
// NKS Framebuffer - 256x240 internal, scaled to modern display

#include "fb.h"
#include "../panic/panic.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/fbio.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Internal framebuffer (256x240, 8-bit indexed color)
#define NKS_WIDTH  256
#define NKS_HEIGHT 240
#define NKS_BPP    8
#define NKS_PALETTE_SIZE 256

static uint8_t nks_fb[NKS_WIDTH * NKS_HEIGHT];
static uint32_t nks_palette[NKS_PALETTE_SIZE];

// Host framebuffer
static int fb_fd = -1;
static uint32_t* fb_ptr = NULL;
static int fb_width = 0;
static int fb_height = 0;
static int fb_pitch = 0;
static int fb_bpp = 0;

// Scaling
static int scale_factor = 1;

// ----- Internal helpers -----

// Default NKS palette (NES-inspired)
static void init_palette(void) {
    // Basic 8-bit RGB332-like palette
    for (int i = 0; i < 256; i++) {
        uint8_t r = (i >> 5) & 0x7;
        uint8_t g = (i >> 2) & 0x7;
        uint8_t b = i & 0x3;
        // Expand to 8-bit
        r = (r << 5) | (r << 2) | (r >> 1);
        g = (g << 5) | (g << 2) | (g >> 1);
        b = (b << 6) | (b << 4) | (b << 2) | b;
        nks_palette[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
    
    // Override some with NES colors
    nks_palette[0] = 0xFF000000;  // Black
    nks_palette[1] = 0xFFFFFFFF;  // White
    nks_palette[2] = 0xFFFF0000;  // Red
    nks_palette[3] = 0xFF00FF00;  // Green
    nks_palette[4] = 0xFF0000FF;  // Blue
    nks_palette[5] = 0xFFFFFF00;  // Yellow
    nks_palette[6] = 0xFFFF00FF;  // Magenta
    nks_palette[7] = 0xFF00FFFF;  // Cyan
}

// Draw a scaled pixel (nearest neighbor)
static void draw_scaled_pixel(int x, int y, uint32_t color) {
    for (int dy = 0; dy < scale_factor; dy++) {
        for (int dx = 0; dx < scale_factor; dx++) {
            int px = x * scale_factor + dx;
            int py = y * scale_factor + dy;
            if (px < fb_width && py < fb_height) {
                fb_ptr[py * fb_pitch / 4 + px] = color;
            }
        }
    }
}

// ----- Public API -----

int fb_init(void) {
    // Open framebuffer
    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0) {
        kitty_panic("No framebuffer device! (/dev/fb0)");
        return -1;
    }
    
    // Get display info
    struct fbtype info;
    if (ioctl(fb_fd, FBIOGTYPE, &info) < 0) {
        kitty_panic("Failed to get framebuffer info");
        close(fb_fd);
        return -1;
    }
    
    fb_width = info.fb_width;
    fb_height = info.fb_height;
    fb_bpp = info.fb_depth;
    fb_pitch = info.fb_width * (fb_bpp / 8);
    
    // Memory map framebuffer
    size_t fb_size = fb_pitch * fb_height;
    fb_ptr = mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_ptr == MAP_FAILED) {
        kitty_panic("Failed to map framebuffer");
        close(fb_fd);
        return -1;
    }
    
    // Calculate scale factor (preserve aspect ratio)
    int scale_x = fb_width / NKS_WIDTH;
    int scale_y = fb_height / NKS_HEIGHT;
    scale_factor = (scale_x < scale_y) ? scale_x : scale_y;
    if (scale_factor < 1) scale_factor = 1;
    
    // Init palette
    init_palette();
    
    // Clear to black
    fb_clear(0);
    
    return 0;
}

void fb_shutdown(void) {
    if (fb_ptr) {
        munmap(fb_ptr, fb_pitch * fb_height);
        fb_ptr = NULL;
    }
    if (fb_fd >= 0) {
        close(fb_fd);
        fb_fd = -1;
    }
}

void fb_clear(uint8_t index) {
    memset(nks_fb, index, NKS_WIDTH * NKS_HEIGHT);
}

void fb_set_pixel(int x, int y, uint8_t index) {
    if (x >= 0 && x < NKS_WIDTH && y >= 0 && y < NKS_HEIGHT) {
        nks_fb[y * NKS_WIDTH + x] = index;
    }
}

uint8_t fb_get_pixel(int x, int y) {
    if (x >= 0 && x < NKS_WIDTH && y >= 0 && y < NKS_HEIGHT) {
        return nks_fb[y * NKS_WIDTH + x];
    }
    return 0;
}

void fb_draw_rect(int x, int y, int w, int h, uint8_t index) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            fb_set_pixel(x + dx, y + dy, index);
        }
    }
}

void fb_render(void) {
    if (!fb_ptr) return;
    
    // Clear host framebuffer
    memset(fb_ptr, 0, fb_pitch * fb_height);
    
    // Center the image
    int offset_x = (fb_width - (NKS_WIDTH * scale_factor)) / 2;
    int offset_y = (fb_height - (NKS_HEIGHT * scale_factor)) / 2;
    
    // Draw scaled framebuffer
    for (int y = 0; y < NKS_HEIGHT; y++) {
        for (int x = 0; x < NKS_WIDTH; x++) {
            uint8_t idx = nks_fb[y * NKS_WIDTH + x];
            uint32_t color = nks_palette[idx];
            
            for (int dy = 0; dy < scale_factor; dy++) {
                for (int dx = 0; dx < scale_factor; dx++) {
                    int px = offset_x + x * scale_factor + dx;
                    int py = offset_y + y * scale_factor + dy;
                    if (px < fb_width && py < fb_height) {
                        fb_ptr[py * (fb_pitch / 4) + px] = color;
                    }
                }
            }
        }
    }
}

void fb_draw_text(const char* text, int x, int y, uint8_t color) {
    // Simple 8x8 pixel font (basic ASCII)
    // This is a placeholder - implement a real font later
    (void)text; (void)x; (void)y; (void)color;
    
    // For now, just draw a colored box to indicate text
    int len = strlen(text);
    fb_draw_rect(x, y, len * 8, 8, color);
}

int fb_get_width(void) {
    return NKS_WIDTH;
}

int fb_get_height(void) {
    return NKS_HEIGHT;
}

void fb_set_palette_entry(int idx, uint8_t r, uint8_t g, uint8_t b) {
    if (idx >= 0 && idx < NKS_PALETTE_SIZE) {
        nks_palette[idx] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
}
