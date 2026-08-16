// src/panic/panic.c
// NKS Kitty Panic - Blue screen of death, catboy edition

#include "panic.h"
#include "../display/fb.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// ASCII cat for the panic screen
static const char* kitty_ascii =
"  /\\_/\\  \n"
" ( o.o ) \n"
"  > ^ <  \n"
" /_/ \\_\\ \n";

static void draw_centered_text(const char* text, int y, uint8_t color) {
    int len = strlen(text);
    int x = (fb_get_width() - len * 8) / 2;  // Approximate 8px per char
    if (x < 0) x = 0;
    fb_draw_text(text, x, y, color);
}

void kitty_panic(const char* msg) {
    // Blue background (NES blue: 0x0000FF)
    fb_clear(0x04);  // Index 4 = blue in our default palette
    
    int width = fb_get_width();
    int height = fb_get_height();
    int center_y = height / 2 - 40;
    int line_height = 16;
    
    // Draw the cat
    // (Simple placeholder - draw a colored box as cat)
    fb_draw_rect(width/2 - 20, center_y - 20, 40, 40, 0x07);  // Cyan box as cat placeholder
    
    // Draw "KITTY PANIC" in big letters (red)
    draw_centered_text("=== KITTY PANIC ===", center_y + 10, 0x02);  // Red
    
    // Draw the error message
    char error_line[128];
    snprintf(error_line, sizeof(error_line), "> %s", msg);
    draw_centered_text(error_line, center_y + 40, 0x01);  // White
    
    // Draw a cute message
    draw_centered_text("*paws on keyboard*", center_y + 60, 0x06);  // Magenta
    draw_centered_text("Press reset to reboot. Or give treats.", center_y + 80, 0x01);
    
    // Update display
    fb_render();
    
    // Halt forever
    while (1) {
        usleep(1000000);  // Sleep 1s (avoid busy loop)
    }
}

void kitty_panic_simple(const char* msg) {
    // Simplified panic for early init (before framebuffer is ready)
    fprintf(stderr, "\n💥 KITTY PANIC 💥\n");
    fprintf(stderr, "%s\n", msg);
    fprintf(stderr, "Press reset to reboot.\n");
    while (1) {
        usleep(1000000);
    }
}
