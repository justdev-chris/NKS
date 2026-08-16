// src/input/kbd.h
#ifndef KBD_H
#define KBD_H

#include <stdint.h>

int kbd_init(void);
void kbd_shutdown(void);
void kbd_poll(void);
int kbd_is_pressed(uint8_t key);
void kbd_clear_state(void);
uint8_t kbd_wait_key(void);

// NKS key codes
#define NKS_KEY_UP    128
#define NKS_KEY_DOWN  129
#define NKS_KEY_LEFT  130
#define NKS_KEY_RIGHT 131
#define NKS_KEY_A     132
#define NKS_KEY_B     133
#define NKS_KEY_START 134
#define NKS_KEY_SELECT 135

#endif
