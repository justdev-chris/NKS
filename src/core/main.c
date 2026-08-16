// src/core/main.c
// NKS Entry Point - Glues everything together

#include "../cpu/cpu.h"
#include "../memory/mem.h"
#include "../display/fb.h"
#include "../input/kbd.h"
#include "../audio/audio.h"
#include "../panic/panic.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>  // For usleep alternative

// ROM path (hardcoded for now)
#define ROM_PATH "/boot/game.nks"

static uint8_t rom_buffer[64 * 1024];  // 64KB max
static volatile int running = 1;

// Signal handler for clean shutdown
static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

// Load ROM from file
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

// Draw boot logo
static void draw_boot_logo(void) {
    fb_clear(0x04);  // Blue background
    
    // Big NKS ASCII art
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
    int center_x = (width - 40 * 8) / 2;  // Approximate 40 chars wide
    
    for (int i = 0; i < 13; i++) {
        if (i < 8) {
            // NKS logo - yellow
            fb_draw_text(logo[i], center_x, 10 + i * 14, 0x05);  // Yellow
        } else if (i < 11) {
            // Tagline - white
            fb_draw_text(logo[i], center_x, 10 + i * 14, 0x01);  // White
        } else {
            // Boot message - magenta
            fb_draw_text(logo[i], center_x, 10 + i * 14, 0x06);  // Magenta
        }
    }
    
    fb_render();
}

// Draw ROM loading error
static void draw_rom_error(void) {
    fb_clear(0x04);  // Blue
    fb_draw_text("ROM NOT FOUND", 20, 80, 0x02);  // Red
    fb_draw_text("Insert game.nks and reset", 20, 100, 0x01);  // White
    fb_render();
}

// Handle input mapping (NKS controls)
static void handle_input(void) {
    // Poll keyboard
    kbd_poll();
    
    // Map keyboard to NKS controls
    // These will be read by the CPU via memory-mapped I/O later
    
    // For now, just check for quit
    if (kbd_is_pressed('Q') || kbd_is_pressed('q')) {
        // Wait for key release to avoid accidental quit
        usleep(100000);
        if (kbd_is_pressed('Q') || kbd_is_pressed('q')) {
            running = 0;
        }
    }
    
    // Example: Map arrow keys to something
    if (kbd_is_pressed(NKS_KEY_UP)) {
        // Handle up
    }
    if (kbd_is_pressed(NKS_KEY_DOWN)) {
        // Handle down
    }
    if (kbd_is_pressed(NKS_KEY_LEFT)) {
        // Handle left
    }
    if (kbd_is_pressed(NKS_KEY_RIGHT)) {
        // Handle right
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    // Set up signal handlers for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 1. Initialize framebuffer
    if (fb_init() < 0) {
        kitty_panic_simple("Framebuffer init failed");
        return 1;
    }
    
    // 2. Initialize memory
    mem_init();
    
    // 3. Initialize keyboard
    if (kbd_init() < 0) {
        // Keyboard init failed - warn but continue (some games might not need it)
        kitty_panic_simple("Keyboard init failed - continuing anyway");
    }
    
    // 4. Initialize audio
    audio_init();
    
    // 5. Draw boot logo
    draw_boot_logo();
    
    // 6. Wait for any key press to continue
    kbd_wait_key();
    
    // 7. Load ROM
    const char* rom_path = (argc > 1) ? argv[1] : ROM_PATH;
    if (load_rom_file(rom_path) < 0) {
        draw_rom_error();
        // Wait for key press to continue (if keyboard works)
        kbd_wait_key();
        kitty_panic("Insert ROM and reset");
    }
    
    // 8. Load ROM into memory
    if (mem_load_rom(rom_buffer, sizeof(rom_buffer)) < 0) {
        kitty_panic("Failed to load ROM into memory");
    }
    
    // 9. Initialize CPU
    cpu_init();
    cpu_load_rom(rom_buffer, sizeof(rom_buffer));
    
    // 10. Clear screen and start
    fb_clear(0x00);  // Black
    fb_draw_text("NKS Ready", 10, 10, 0x01);  // White
    fb_render();
    usleep(200000);  // Show for 0.2s
    
    // 11. Main emulation loop
    int frame_counter = 0;
    struct timeval frame_start, frame_end;
    gettimeofday(&frame_start, NULL);
    
    while (running && !cpu_is_halted()) {
        // Execute one instruction
        cpu_step();
        
        // Render at ~60 FPS
        frame_counter++;
        if (frame_counter >= 5000) {  // Adjust for performance
            fb_render();
            frame_counter = 0;
            
            // Update input
            handle_input();
            
            // Frame timing (target 60 FPS)
            gettimeofday(&frame_end, NULL);
            long us = (frame_end.tv_sec - frame_start.tv_sec) * 1000000L +
                      (frame_end.tv_usec - frame_start.tv_usec);
            if (us < 16666L) {  // 16.67ms = 60 FPS
                usleep(16666L - us);
            }
            gettimeofday(&frame_start, NULL);
        }
        
        // Check for audio events
        audio_update();
    }
    
    // 12. Clean shutdown
    audio_shutdown();
    kbd_shutdown();
    fb_shutdown();
    
    // 13. Print goodbye message
    printf("\n🐾 NKS shutdown complete. Nya~!\n");
    
    return 0;
}