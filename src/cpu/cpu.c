// src/cpu/cpu.c
// NKS RISC-V RV32I Emulator

#include "cpu.h"
#include "../memory/mem.h"
#include "../panic/panic.h"
#include <string.h>
#include <stdio.h>

// CPU state
typedef struct {
    uint32_t regs[32];
    uint32_t pc;
    uint32_t cycles;
    int halted;
} rv32i_cpu;

static rv32i_cpu cpu;

// Helper: sign extend
static inline uint32_t sign_extend(uint32_t val, int bits) {
    if (val & (1 << (bits - 1)))
        val |= (0xFFFFFFFF << bits);
    return val;
}

// Helper: read instruction (little-endian)
static inline uint32_t fetch_insn(void) {
    uint32_t insn = 0;
    for (int i = 0; i < 4; i++) {
        insn |= (uint32_t)mem_read_byte(cpu.pc + i) << (i * 8);
    }
    cpu.pc += 4;
    return insn;
}

// Execute one instruction
static void execute_insn(uint32_t insn) {
    uint32_t opcode = insn & 0x7F;
    uint32_t rd = (insn >> 7) & 0x1F;
    uint32_t rs1 = (insn >> 15) & 0x1F;
    uint32_t rs2 = (insn >> 20) & 0x1F;
    uint32_t funct3 = (insn >> 12) & 0x7;
    uint32_t funct7 = (insn >> 25) & 0x7F;
    uint32_t imm;

    switch (opcode) {
        // ----- R-Type -----
        case 0x33: // ADD, SUB, AND, OR, XOR, SLT, SLL, SRL, SRA
            imm = 0;
            switch (funct3) {
                case 0x0: // ADD / SUB
                    if (funct7 == 0x00)
                        cpu.regs[rd] = cpu.regs[rs1] + cpu.regs[rs2];
                    else if (funct7 == 0x20)
                        cpu.regs[rd] = cpu.regs[rs1] - cpu.regs[rs2];
                    else goto invalid;
                    break;
                case 0x1: // SLL
                    cpu.regs[rd] = cpu.regs[rs1] << (cpu.regs[rs2] & 0x1F);
                    break;
                case 0x2: // SLT
                    cpu.regs[rd] = (int32_t)cpu.regs[rs1] < (int32_t)cpu.regs[rs2];
                    break;
                case 0x3: // SLTU
                    cpu.regs[rd] = cpu.regs[rs1] < cpu.regs[rs2];
                    break;
                case 0x4: // XOR
                    cpu.regs[rd] = cpu.regs[rs1] ^ cpu.regs[rs2];
                    break;
                case 0x5: // SRL / SRA
                    if (funct7 == 0x00)
                        cpu.regs[rd] = cpu.regs[rs1] >> (cpu.regs[rs2] & 0x1F);
                    else if (funct7 == 0x20)
                        cpu.regs[rd] = (int32_t)cpu.regs[rs1] >> (cpu.regs[rs2] & 0x1F);
                    else goto invalid;
                    break;
                case 0x6: // OR
                    cpu.regs[rd] = cpu.regs[rs1] | cpu.regs[rs2];
                    break;
                case 0x7: // AND
                    cpu.regs[rd] = cpu.regs[rs1] & cpu.regs[rs2];
                    break;
                default: goto invalid;
            }
            break;

        // ----- I-Type -----
        case 0x13: // ADDI, ORI, XORI, SLTI, SLLI, SRLI, SRAI
            imm = sign_extend((insn >> 20) & 0xFFF, 12);
            switch (funct3) {
                case 0x0: // ADDI
                    cpu.regs[rd] = cpu.regs[rs1] + imm;
                    break;
                case 0x1: // SLLI
                    cpu.regs[rd] = cpu.regs[rs1] << (imm & 0x1F);
                    break;
                case 0x2: // SLTI
                    cpu.regs[rd] = (int32_t)cpu.regs[rs1] < (int32_t)imm;
                    break;
                case 0x3: // SLTIU
                    cpu.regs[rd] = cpu.regs[rs1] < (uint32_t)imm;
                    break;
                case 0x4: // XORI
                    cpu.regs[rd] = cpu.regs[rs1] ^ imm;
                    break;
                case 0x5: // SRLI / SRAI
                    if (funct7 == 0x00)
                        cpu.regs[rd] = cpu.regs[rs1] >> (imm & 0x1F);
                    else if (funct7 == 0x20)
                        cpu.regs[rd] = (int32_t)cpu.regs[rs1] >> (imm & 0x1F);
                    else goto invalid;
                    break;
                case 0x6: // ORI
                    cpu.regs[rd] = cpu.regs[rs1] | imm;
                    break;
                case 0x7: // ANDI
                    cpu.regs[rd] = cpu.regs[rs1] & imm;
                    break;
                default: goto invalid;
            }
            break;

        case 0x03: // LB, LH, LW, LBU, LHU
            imm = sign_extend((insn >> 20) & 0xFFF, 12);
            uint32_t addr = cpu.regs[rs1] + imm;
            switch (funct3) {
                case 0x0: // LB
                    cpu.regs[rd] = sign_extend(mem_read_byte(addr), 8);
                    break;
                case 0x1: // LH
                    cpu.regs[rd] = sign_extend(mem_read_half(addr), 16);
                    break;
                case 0x2: // LW
                    cpu.regs[rd] = mem_read_word(addr);
                    break;
                case 0x4: // LBU
                    cpu.regs[rd] = mem_read_byte(addr);
                    break;
                case 0x5: // LHU
                    cpu.regs[rd] = mem_read_half(addr);
                    break;
                default: goto invalid;
            }
            break;

        case 0x67: // JALR
            imm = sign_extend((insn >> 20) & 0xFFF, 12);
            uint32_t target = (cpu.regs[rs1] + imm) & ~1;
            cpu.regs[rd] = cpu.pc;
            cpu.pc = target;
            break;

        // ----- S-Type -----
        case 0x23: // SB, SH, SW
            imm = (insn >> 7) & 0x1F;
            imm |= ((insn >> 25) & 0x7F) << 5;
            imm = sign_extend(imm, 12);
            addr = cpu.regs[rs1] + imm;
            switch (funct3) {
                case 0x0: // SB
                    mem_write_byte(addr, cpu.regs[rs2] & 0xFF);
                    break;
                case 0x1: // SH
                    mem_write_half(addr, cpu.regs[rs2] & 0xFFFF);
                    break;
                case 0x2: // SW
                    mem_write_word(addr, cpu.regs[rs2]);
                    break;
                default: goto invalid;
            }
            break;

        // ----- B-Type -----
        case 0x63:
            imm = (insn >> 7) & 0x1E;
            imm |= ((insn >> 25) & 0x7F) << 5;
            imm |= ((insn >> 8) & 0xF) << 1;  // Fix: correct B-type bit extraction
            imm = sign_extend(imm, 12);
            int branch = 0;
            switch (funct3) {
                case 0x0: // BEQ
                    branch = (cpu.regs[rs1] == cpu.regs[rs2]);
                    break;
                case 0x1: // BNE
                    branch = (cpu.regs[rs1] != cpu.regs[rs2]);
                    break;
                case 0x4: // BLT
                    branch = ((int32_t)cpu.regs[rs1] < (int32_t)cpu.regs[rs2]);
                    break;
                case 0x5: // BGE
                    branch = ((int32_t)cpu.regs[rs1] >= (int32_t)cpu.regs[rs2]);
                    break;
                case 0x6: // BLTU
                    branch = (cpu.regs[rs1] < cpu.regs[rs2]);
                    break;
                case 0x7: // BGEU
                    branch = (cpu.regs[rs1] >= cpu.regs[rs2]);
                    break;
                default: goto invalid;
            }
            if (branch) cpu.pc += imm;
            break;

        // ----- U-Type -----
        case 0x37: // LUI
            imm = (insn >> 12) << 12;
            cpu.regs[rd] = imm;
            break;

        case 0x17: // AUIPC
            imm = (insn >> 12) << 12;
            cpu.regs[rd] = cpu.pc + imm;
            break;

        // ----- J-Type -----
        case 0x6F: // JAL
            imm = (insn >> 21) & 0x3FF;
            imm |= ((insn >> 20) & 0x1) << 10;
            imm |= ((insn >> 12) & 0xFF) << 11;
            imm |= ((insn >> 31) & 0x1) << 20;
            imm = sign_extend(imm, 20);
            cpu.regs[rd] = cpu.pc;
            cpu.pc += imm;
            break;

        // ----- System (ECALL/EBREAK) -----
        case 0x73:
            if (funct3 == 0x0) {
                // EBREAK or ECALL
                kitty_panic("RISC-V ECALL/EBREAK - System call triggered!");
                cpu.halted = 1;
            }
            break;

        default:
            invalid:
            char msg[128];
            snprintf(msg, sizeof(msg), "Invalid opcode: 0x%08X at PC=0x%08X", insn, cpu.pc - 4);
            kitty_panic(msg);
            cpu.halted = 1;
            break;
    }
}

// ----- Public API -----

void cpu_init(void) {
    memset(&cpu, 0, sizeof(cpu));
    cpu.pc = 0x00010000; // Standard RISC-V reset vector
    cpu.halted = 0;
    cpu.cycles = 0;
}

void cpu_reset(void) {
    cpu_init();
}

void cpu_load_rom(const uint8_t* rom, size_t size) {
    // Load ROM at 0x00010000 (standard RISC-V)
    for (size_t i = 0; i < size && i < 0x10000; i++) {
        mem_write_byte(0x00010000 + i, rom[i]);
    }
}

void cpu_step(void) {
    if (cpu.halted) return;
    uint32_t insn = fetch_insn();
    execute_insn(insn);
    cpu.cycles++;
}

int cpu_is_halted(void) {
    return cpu.halted;
}

uint32_t cpu_get_pc(void) {
    return cpu.pc;
}

uint32_t cpu_get_reg(int idx) {
    if (idx >= 0 && idx < 32) return cpu.regs[idx];
    return 0;
}

void cpu_dump_state(void) {
    printf("PC: 0x%08X\n", cpu.pc);
    printf("Cycles: %u\n", cpu.cycles);
    for (int i = 0; i < 32; i++) {
        printf("x%d: 0x%08X%s", i, cpu.regs[i], (i % 4 == 3) ? "\n" : " ");
    }
}
