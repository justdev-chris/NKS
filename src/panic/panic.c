// src/panic/panic.c
// NKS Kitty Panic

// Force define BSD types before any includes
#ifndef _SYS_TYPES_H_
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned long u_long;
#endif

#include "panic.h"
#include "../display/fb.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

static inline void panic_sleep_ms(unsigned int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

static void draw_centered_text(const char* text, int y, uint8_t color) {
    int len = strlen(text);
    int x = (fb_get_width() - len * 8) / 2;
    if (x < 0) x = 0;
    fb_draw_text(text, x, y, color);
}

void kitty_panic(const char* msg) {
    fb_clear(0x04);
    
    int width = fb_get_width();
    int height = fb_get_height();
    int center_y = height / 2 - 40;
    
    fb_draw_rect(width/2 - 20, center_y - 20, 40, 40, 0x07);
    
    draw_centered_text("=== KITTY PANIC ===", center_y + 10, 0x02);
    
    char error_line[128];
    snprintf(error_line, sizeof(error_line), "> %s", msg);
    draw_centered_text(error_line, center_y + 40, 0x01);
    
    draw_centered_text("*paws on keyboard*", center_y + 60, 0x06);
    draw_centered_text("Press reset to reboot. Or give treats.", center_y + 80, 0x01);
    
    fb_render();
    
    while (1) {
        panic_sleep_ms(1000);
    }
}

void kitty_panic_simple(const char* msg) {
    fprintf(stderr, "\n💥 KITTY PANIC 💥\n");
    fprintf(stderr, "%s\n", msg);
    fprintf(stderr, "Press reset to reboot.\n");
    while (1) {
        panic_sleep_ms(1000);
    }
}
