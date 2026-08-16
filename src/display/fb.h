// src/display/fb.h
#ifndef FB_H
#define FB_H

#include <stdint.h>

int fb_init(void);
void fb_shutdown(void);
void fb_clear(uint8_t index);
void fb_set_pixel(int x, int y, uint8_t index);
uint8_t fb_get_pixel(int x, int y);
void fb_draw_rect(int x, int y, int w, int h, uint8_t index);
void fb_render(void);
void fb_draw_text(const char* text, int x, int y, uint8_t color);
int fb_get_width(void);
int fb_get_height(void);
void fb_set_palette_entry(int idx, uint8_t r, uint8_t g, uint8_t b);

#endif
