// src/core/main.c
// NKS Entry Point - Glues everything together

#include "../cpu/cpu.h"
#include "../memory/mem.h"
#include "../display/fb.h"
#include "../panic/panic.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// ROM path (hardcoded for now)
#define ROM_PATH "/boot/game.nks"

static uint8_t rom_buffer[64 * 1024];  // 64KB max

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
    
    if (st.st_size > sizeof(rom_buffer)) {
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
    
    // Simple "NKS" text art
    const char* logo[] = {
        "   _   _   _   _   _   _   _   _   _   _",
        "  / \\ / \\ / \\ / \\ / \\ / \\ / \\ / \\ / \\ / \\",
        " ( N | K | S )",
        "  \\_/ \\_/ \\_/",
        "",
        "  NyaaKitStation",
        "  Purrformance meets chaos."
    };
    
    int width = fb_get_width();
    int center_x = (width - 20 * 8) / 2;  // Approximate
    
    for (int i = 0; i < 7; i++) {
        fb_draw_text(logo[i], center_x, 20 + i * 16, 0x01);  // White
    }
    
    fb_draw_text("Loading ROM...", center_x, 140, 0x06);  // Magenta
    
    fb_render();
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    // 1. Initialize subsystems
    if (fb_init() < 0) {
        kitty_panic_simple("Framebuffer init failed");
        return 1;
    }
    
    // 2. Initialize memory
    mem_init();
    
    // 3. Draw boot logo
    draw_boot_logo();
    usleep(500000);  // Show logo for 0.5s
    
    // 4. Load ROM
    const char* rom_path = (argc > 1) ? argv[1] : ROM_PATH;
    if (load_rom_file(rom_path) < 0) {
        // Show error on screen
        fb_clear(0x04);  // Blue
        fb_draw_text("No ROM found! Insert game.nks", 20, 100, 0x02);  // Red
        fb_render();
        kitty_panic("Insert ROM and reset");
    }
    
    // 5. Load ROM into memory
    if (mem_load_rom(rom_buffer, sizeof(rom_buffer)) < 0) {
        kitty_panic("Failed to load ROM into memory");
    }
    
    // 6. Initialize CPU
    cpu_init();
    cpu_load_rom(rom_buffer, sizeof(rom_buffer));
    
    // 7. Clear screen
    fb_clear(0x00);  // Black
    fb_render();
    
    // 8. Main emulation loop
    int frame_counter = 0;
    while (!cpu_is_halted()) {
        // Execute one instruction
        cpu_step();
        
        // Render at ~60 FPS (every ~16666 cycles at 1MHz)
        // For RV32I, we'll render every N instructions
        frame_counter++;
        if (frame_counter >= 1000) {  // Adjust for performance
            fb_render();
            frame_counter = 0;
            
            // Simple keyboard check (placeholder)
            // Real input will go here later
        }
        
        // Small delay to prevent 100% CPU usage
        // usleep(1);  // Uncomment if needed
    }
    
    // Should never reach here
    kitty_panic("CPU halted - game over?");
    return 0;
}
