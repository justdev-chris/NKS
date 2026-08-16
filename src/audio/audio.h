// src/audio/audio.h
#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stddef.h>

int audio_init(void);
void audio_shutdown(void);
void audio_play_beep(void);
void audio_play_sample(const uint8_t* sample, size_t size);
void audio_play_sine(int freq, int duration_ms);
void audio_update(void);

#endif
