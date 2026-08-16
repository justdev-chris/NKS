// src/memory/mem.h
#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

void mem_init(void);
void mem_reset(void);
int mem_load_rom(const uint8_t* data, size_t size);

uint8_t mem_read_byte(uint32_t addr);
uint16_t mem_read_half(uint32_t addr);
uint32_t mem_read_word(uint32_t addr);

void mem_write_byte(uint32_t addr, uint8_t val);
void mem_write_half(uint32_t addr, uint16_t val);
void mem_write_word(uint32_t addr, uint32_t val);

void mem_dump(uint32_t start, uint32_t end);

#endif
