// src/cpu/cpu.h
#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stddef.h>

void cpu_init(void);
void cpu_reset(void);
void cpu_load_rom(const uint8_t* rom, size_t size);
void cpu_step(void);
int cpu_is_halted(void);
uint32_t cpu_get_pc(void);
uint32_t cpu_get_reg(int idx);
void cpu_dump_state(void);

#endif
