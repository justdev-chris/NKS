// src/audio/audio.c
// NKS Audio - OSS (/dev/dsp0) PCM playback

#include "audio.h"
#include "../panic/panic.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static int audio_fd = -1;
static int audio_enabled = 0;

// Audio parameters
#define SAMPLE_RATE 22050
#define CHANNELS 1       // Mono
#define BITS 8           // 8-bit unsigned PCM

// Simple square wave buffer (440Hz, 0.1s)
#define BEEP_DURATION  (SAMPLE_RATE / 10)  // 0.1 seconds
static uint8_t beep_buffer[BEEP_DURATION];

// Generate a square wave
static void generate_square_wave(uint8_t* buffer, int samples, int freq) {
    int period = SAMPLE_RATE / freq;
    for (int i = 0; i < samples; i++) {
        int phase = i % period;
        // 8-bit unsigned: 0x80 is silence, 0xFF is max, 0x00 is min
        buffer[i] = (phase < period / 2) ? 0xFF : 0x00;
    }
}

// Generate a sine wave (smoother)
static void generate_sine_wave(uint8_t* buffer, int samples, int freq) {
    for (int i = 0; i < samples; i++) {
        double angle = 2.0 * M_PI * freq * i / SAMPLE_RATE;
        uint8_t sample = 0x80 + (uint8_t)(0x7F * sin(angle));
        buffer[i] = sample;
    }
}

int audio_init(void) {
    // Open OSS device
    audio_fd = open("/dev/dsp0", O_WRONLY);
    if (audio_fd < 0) {
        // Try /dev/dsp (legacy)
        audio_fd = open("/dev/dsp", O_WRONLY);
        if (audio_fd < 0) {
            kitty_panic_simple("No audio device! (/dev/dsp0)");
            audio_enabled = 0;
            return -1;
        }
    }
    
    // Set audio parameters
    int format = AFMT_U8;  // 8-bit unsigned
    if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format) < 0) {
        kitty_panic_simple("Failed to set audio format");
        close(audio_fd);
        audio_enabled = 0;
        return -1;
    }
    
    int channels = CHANNELS;
    if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels) < 0) {
        kitty_panic_simple("Failed to set audio channels");
        close(audio_fd);
        audio_enabled = 0;
        return -1;
    }
    
    int speed = SAMPLE_RATE;
    if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed) < 0) {
        kitty_panic_simple("Failed to set audio speed");
        close(audio_fd);
        audio_enabled = 0;
        return -1;
    }
    
    // Generate beep buffer (square wave at 440Hz)
    generate_square_wave(beep_buffer, BEEP_DURATION, 440);
    
    audio_enabled = 1;
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
    if (!audio_enabled || audio_fd < 0) return;
    
    // Write beep buffer to audio device
    ssize_t written = write(audio_fd, beep_buffer, BEEP_DURATION);
    if (written < 0) {
        // Audio device may be busy - ignore
    }
}

void audio_play_sample(const uint8_t* sample, size_t size) {
    if (!audio_enabled || audio_fd < 0 || !sample || size == 0) return;
    
    ssize_t written = write(audio_fd, sample, size);
    if (written < 0) {
        // Audio device may be busy - ignore
    }
}

void audio_play_sine(int freq, int duration_ms) {
    if (!audio_enabled || audio_fd < 0) return;
    
    int samples = (SAMPLE_RATE * duration_ms) / 1000;
    uint8_t* buffer = malloc(samples);
    if (!buffer) return;
    
    generate_sine_wave(buffer, samples, freq);
    write(audio_fd, buffer, samples);
    free(buffer);
}

void audio_update(void) {
    // Nothing to do - audio is event-driven
    // This function is called every frame from main loop
}
