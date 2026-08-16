// src/audio/audio.c
// NKS Audio - OSS on FreeBSD, stubs elsewhere

#include "audio.h"
#include "../panic/panic.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int audio_fd = -1;
static int audio_enabled = 0;

#ifdef __FreeBSD__
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#endif

int audio_init(void) {
#ifdef __FreeBSD__
    audio_fd = open("/dev/dsp0", O_WRONLY);
    if (audio_fd < 0) {
        audio_fd = open("/dev/dsp", O_WRONLY);
        if (audio_fd < 0) {
            kitty_panic_simple("No audio device!");
            audio_enabled = 0;
            return -1;
        }
    }
    
    int format = AFMT_U8;
    ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format);
    int channels = 1;
    ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels);
    int speed = 22050;
    ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed);
    
    audio_enabled = 1;
#else
    audio_enabled = 0;
#endif
    return 0;
}

void audio_shutdown(void) {
    if (audio_fd >= 0) {
        close(audio_fd);
        audio_fd = -1;
    }
    audio_enabled = 0;
}

void audio_play_beep(void) {
#ifdef __FreeBSD__
    if (!audio_enabled || audio_fd < 0) return;
    const int sample_rate = 22050;
    const int duration = sample_rate / 10;
    unsigned char* beep = malloc(duration);
    if (!beep) return;
    for (int i = 0; i < duration; i++) {
        beep[i] = (i % (sample_rate / 440) < (sample_rate / 880)) ? 0xFF : 0x00;
    }
    write(audio_fd, beep, duration);
    free(beep);
#endif
}

void audio_play_sample(const uint8_t* sample, size_t size) {
#ifdef __FreeBSD__
    if (!audio_enabled || audio_fd < 0 || !sample || size == 0) return;
    write(audio_fd, sample, size);
#endif
}

void audio_play_sine(int freq, int duration_ms) {
#ifdef __FreeBSD__
    if (!audio_enabled || audio_fd < 0) return;
    const int sample_rate = 22050;
    int samples = (sample_rate * duration_ms) / 1000;
    unsigned char* buffer = malloc(samples);
    if (!buffer) return;
    for (int i = 0; i < samples; i++) {
        double angle = 2.0 * 3.14159 * freq * i / sample_rate;
        buffer[i] = 0x80 + (unsigned char)(0x7F * sin(angle));
    }
    write(audio_fd, buffer, samples);
    free(buffer);
#endif
}

void audio_update(void) {
    // Stub
}