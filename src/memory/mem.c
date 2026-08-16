// src/memory/mem.c
// NKS Memory Subsystem - 128KB RAM + ROM

#include "mem.h"
#include "../panic/panic.h"
#include <string.h>
#include <stdio.h>

#define RAM_SIZE  (128 * 1024)  // 128KB
#define ROM_BASE  0x00010000    // RISC-V reset vector
#define ROM_SIZE  0x00010000    // 64KB max ROM

static uint8_t ram[RAM_SIZE];
static uint8_t rom[ROM_SIZE];
static int rom_loaded = 0;

// Memory-mapped I/O regions (for future expansion)
#define MMIO_BASE  0x02000000
#define MMIO_SIZE  0x00001000

// ----- Internal helpers -----

static int is_mmio(uint32_t addr) {
    return (addr >= MMIO_BASE && addr < MMIO_BASE + MMIO_SIZE);
}

static void mmio_write(uint32_t addr, uint8_t val) {
    // Placeholder for future devices (controller, audio, etc.)
    (void)addr; (void)val;
    // Just ignore writes for now
}

static uint8_t mmio_read(uint32_t addr) {
    (void)addr;
    return 0xFF; // Return open bus
}

// ----- Public API -----

void mem_init(void) {
    memset(ram, 0, RAM_SIZE);
    memset(rom, 0, ROM_SIZE);
    rom_loaded = 0;
}

void mem_reset(void) {
    mem_init();
}

int mem_load_rom(const uint8_t* data, size_t size) {
    if (size > ROM_SIZE) {
        kitty_panic("ROM too large! Max 64KB");
        return -1;
    }
    
    memset(rom, 0, ROM_SIZE);
    memcpy(rom, data, size);
    rom_loaded = 1;
    return 0;
}

uint8_t mem_read_byte(uint32_t addr) {
    // ROM region
    if (addr >= ROM_BASE && addr < ROM_BASE + ROM_SIZE) {
        if (!rom_loaded) {
            kitty_panic("No ROM loaded! Read from empty ROM");
            return 0xFF;
        }
        return rom[addr - ROM_BASE];
    }
    
    // RAM region
    if (addr < RAM_SIZE) {
        return ram[addr];
    }
    
    // Memory-mapped I/O
    if (is_mmio(addr)) {
        return mmio_read(addr);
    }
    
    // Invalid address
    char msg[128];
    snprintf(msg, sizeof(msg), "Invalid read at 0x%08X", addr);
    kitty_panic(msg);
    return 0xFF;
}

uint16_t mem_read_half(uint32_t addr) {
    // Little-endian
    return (uint16_t)mem_read_byte(addr) |
           ((uint16_t)mem_read_byte(addr + 1) << 8);
}

uint32_t mem_read_word(uint32_t addr) {
    // Little-endian
    return (uint32_t)mem_read_byte(addr) |
           ((uint32_t)mem_read_byte(addr + 1) << 8) |
           ((uint32_t)mem_read_byte(addr + 2) << 16) |
           ((uint32_t)mem_read_byte(addr + 3) << 24);
}

void mem_write_byte(uint32_t addr, uint8_t val) {
    // ROM is read-only
    if (addr >= ROM_BASE && addr < ROM_BASE + ROM_SIZE) {
        // Silently ignore writes to ROM
        return;
    }
    
    // RAM region
    if (addr < RAM_SIZE) {
        ram[addr] = val;
        return;
    }
    
    // Memory-mapped I/O
    if (is_mmio(addr)) {
        mmio_write(addr, val);
        return;
    }
    
    // Invalid address
    char msg[128];
    snprintf(msg, sizeof(msg), "Invalid write at 0x%08X", addr);
    kitty_panic(msg);
}

void mem_write_half(uint32_t addr, uint16_t val) {
    mem_write_byte(addr, val & 0xFF);
    mem_write_byte(addr + 1, (val >> 8) & 0xFF);
}

void mem_write_word(uint32_t addr, uint32_t val) {
    mem_write_byte(addr, val & 0xFF);
    mem_write_byte(addr + 1, (val >> 8) & 0xFF);
    mem_write_byte(addr + 2, (val >> 16) & 0xFF);
    mem_write_byte(addr + 3, (val >> 24) & 0xFF);
}

void mem_dump(uint32_t start, uint32_t end) {
    printf("Memory dump 0x%08X - 0x%08X:\n", start, end);
    for (uint32_t addr = start; addr <= end; addr += 16) {
        printf("0x%08X: ", addr);
        for (int i = 0; i < 16 && addr + i <= end; i++) {
            printf("%02X ", mem_read_byte(addr + i));
        }
        printf("\n");
    }
}
