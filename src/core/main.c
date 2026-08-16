// src/core/main.c
// NKS Entry Point

#include "../cpu/cpu.h"
#include "../memory/mem.h"
#include "../display/fb.h"
#include "../input/kbd.h"
#include "../audio/audio.h"
#include "../panic/panic.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

// POSIX sleep wrapper (works everywhere)
static inline void msleep(unsigned int ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
}

#define ROM_PATH "/boot/game.nks"
static uint8_t rom_buffer[64 * 1024];
static volatile int running = 1;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

static int load_rom_file(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        kitty_panic_simple("No ROM found! Insert game.nks");
        return -1;
    }
    
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        kitty_panic_simple("Failed to stat ROM");
        return -1;
    }
    
    if (st.st_size > (off_t)sizeof(rom_buffer)) {
        close(fd);
        kitty_panic_simple("ROM too large! Max 64KB");
        return -1;
    }
    
    ssize_t bytes = read(fd, rom_buffer, st.st_size);
    close(fd);
    
    if (bytes != st.st_size) {
        kitty_panic_simple("Failed to read ROM");
        return -1;
    }
    
    return 0;
}

static void draw_boot_logo(void) {
    fb_clear(0x04);
    
    const char* logo[] = {
        "/$$   /$$ /$$   /$$  /$$$$$$ ",
        "| $$$ | $$| $$  /$$/ /$$__  $$",
        "| $$$$| $$| $$ /$$/ | $$  \\__/",
        "| $$ $$ $$| $$$$$/  |  $$$$$$ ",
        "| $$  $$$$| $$  $$   \\____  $$",
        "| $$\\  $$$| $$\\  $$  /$$  \\ $$",
        "| $$ \\  $$| $$ \\  $$|  $$$$$$/",
        "|__/  \\__/|__/  \\__/ \\______/ ",
        "",
        "  NyaaKitStation v1.0",
        "  Purrformance meets chaos.",
        "",
        "  Press any key to boot..."
    };
    
    int width = fb_get_width();
    int center_x = (width - 40 * 8) / 2;
    if (center_x < 0) center_x = 0;
    
    for (int i = 0; i < 13; i++) {
        if (i < 8) {
            fb_draw_text(logo[i], center_x, 10 + i * 14, 0x05);
        } else if (i < 11) {
            fb_draw_text(logo[i], center_x, 10 + i * 14, 0x01);
        } else {
            fb_draw_text(logo[i], center_x, 10 + i * 14, 0x06);
        }
    }
    
    fb_render();
}

static void draw_rom_error(void) {
    fb_clear(0x04);
    fb_draw_text("ROM NOT FOUND", 20, 80, 0x02);
    fb_draw_text("Insert game.nks and reset", 20, 100, 0x01);
    fb_render();
}

static void handle_input(void) {
    kbd_poll();
    
    if (kbd_is_pressed('Q') || kbd_is_pressed('q')) {
        msleep(100);
        if (kbd_is_pressed('Q') || kbd_is_pressed('q')) {
            running = 0;
        }
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (fb_init() < 0) {
        kitty_panic_simple("Framebuffer init failed");
        return 1;
    }
    
    mem_init();
    kbd_init();
    audio_init();
    
    draw_boot_logo();
    kbd_wait_key();
    
    const char* rom_path = (argc > 1) ? argv[1] : ROM_PATH;
    if (load_rom_file(rom_path) < 0) {
        draw_rom_error();
        kbd_wait_key();
        kitty_panic("Insert ROM and reset");
    }
    
    if (mem_load_rom(rom_buffer, sizeof(rom_buffer)) < 0) {
        kitty_panic("Failed to load ROM into memory");
    }
    
    cpu_init();
    cpu_load_rom(rom_buffer, sizeof(rom_buffer));
    
    fb_clear(0x00);
    fb_draw_text("NKS Ready", 10, 10, 0x01);
    fb_render();
    msleep(200);
    
    int frame_counter = 0;
    struct timespec frame_start, frame_end;
    clock_gettime(CLOCK_MONOTONIC, &frame_start);
    
    while (running && !cpu_is_halted()) {
        cpu_step();
        
        frame_counter++;
        if (frame_counter >= 5000) {
            fb_render();
            frame_counter = 0;
            handle_input();
            
            clock_gettime(CLOCK_MONOTONIC, &frame_end);
            long ns = (frame_end.tv_sec - frame_start.tv_sec) * 1000000000L +
                      (frame_end.tv_nsec - frame_start.tv_nsec);
            if (ns < 16666666L) {
                struct timespec sleep_ts = {0, (16666666L - ns) * 1000};
                nanosleep(&sleep_ts, NULL);
            }
            clock_gettime(CLOCK_MONOTONIC, &frame_start);
        }
        
        audio_update();
    }
    
    audio_shutdown();
    kbd_shutdown();
    fb_shutdown();
    
    printf("\n🐾 NKS shutdown complete. Nya~!\n");
    return 0;
}