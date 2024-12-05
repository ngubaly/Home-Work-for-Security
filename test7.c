#include <stdio.h>
#include "pico/stdlib.h"
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>  // Thêm header này để sử dụng hàm atoi

#define UART_ID         uart0
#define BAUD_RATE       9600
#define UART_TX_PIN     0
#define UART_RX_PIN     1

#define ROM_SIZE        128
#define RAM_SIZE        128

#define STATUS_ZERO     0x01

typedef struct {
    uint8_t r0;
    uint8_t r1;
    uint8_t r2;
    uint8_t r3;
} GP_Registers;

typedef struct {
    uint8_t PC;
    GP_Registers regs;
    uint8_t status;
    uint8_t program_rom[ROM_SIZE];
    uint8_t program_ram[RAM_SIZE];
} CPU_State;

void initialize_cpu(CPU_State *cpu);
void load_factorial_program(CPU_State *cpu);
void fetch_instruction(CPU_State *cpu, uint8_t *instruction);
void execute_instruction(CPU_State *cpu, uint8_t instruction, bool *end_flag);

void instr_mem_to_r0(CPU_State *cpu, uint8_t rx);
void instr_r0_to_mem(CPU_State *cpu, uint8_t rx);
void instr_transfer_reg(CPU_State *cpu, uint8_t rx, uint8_t ry);
void instr_immediate_set(CPU_State *cpu, uint8_t value);
void instr_add(CPU_State *cpu, uint8_t rx, uint8_t ry);
void instr_sub(CPU_State *cpu, uint8_t rx, uint8_t ry);
void instr_shift_left(CPU_State *cpu, uint8_t rx);
void instr_shift_right(CPU_State *cpu, uint8_t rx);
void instr_compare(CPU_State *cpu, uint8_t rx);
void instr_output(CPU_State *cpu, uint8_t rx);
void instr_input(CPU_State *cpu, uint8_t rx);
void instr_cond_branch(CPU_State *cpu, uint8_t rx);
void instr_uncond_branch(CPU_State *cpu, uint8_t rx);

uint8_t get_register_value(CPU_State *cpu, uint8_t reg);
void set_register_value(CPU_State *cpu, uint8_t reg, uint8_t value);
void update_zero_flag(CPU_State *cpu, uint8_t result);

int main() {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    CPU_State cpu;
    initialize_cpu(&cpu);
    load_factorial_program(&cpu);

    bool end_flag = false;
    while (!end_flag) {
        uint8_t instruction;
        fetch_instruction(&cpu, &instruction);
        execute_instruction(&cpu, instruction, &end_flag);
    }

    uart_puts(UART_ID, "Program execution completed.\n");
    return 0;
}

void initialize_cpu(CPU_State *cpu) {
    cpu->PC = 0;
    cpu->regs.r0 = 0;
    cpu->regs.r1 = 0;
    cpu->regs.r2 = 0;
    cpu->regs.r3 = 0;
    cpu->status = 0;
    memset(cpu->program_rom, 0, ROM_SIZE);
    memset(cpu->program_ram, 0, RAM_SIZE);
}

void load_factorial_program(CPU_State *cpu) {
    uint8_t factorial_program[] = {
        0xD1, // Input into r1
        0x51, // Immediate set r0 = 1
        0x42, // Transfer r0 to r2
        0x43, // Transfer r1 to r3
        0xA3, // Compare r3 == 0
        0x5E, // Immediate set r0 = 14
        0xE0, // Conditional branch to r0 if zero
        0x24, // Transfer r2 to r0
        0x50, // Immediate set r0 = 0
        0x42, // Transfer r0 to r2
        0x4C, // Transfer r3 to r1
        0xA1, // Compare r1 == 0
        0x5F, // Immediate set r0 = 15
        0xE0, // Conditional branch to r0 if zero
        0x60, // Add r0 + r2 -> r0
        0x42, // Transfer r0 to r2
        0x51, // Immediate set r0 = 1
        0x71, // Subtract r1 - r0 -> r1
        0x5B, // Immediate set r0 = 11
        0xF0, // Unconditional branch to r0
        0x51, // Immediate set r0 = 1
        0x73, // Subtract r3 - r0 -> r3
        0x54, // Immediate set r0 = 4
        0xF0, // Unconditional branch to r0
        0xC2, // Output from r2
        0xFF  // Invalid opcode to stop execution
    };
    memcpy(cpu->program_rom, factorial_program, sizeof(factorial_program));
}

void fetch_instruction(CPU_State *cpu, uint8_t *instruction) {
    if (cpu->PC >= ROM_SIZE) {
        *instruction = 0xFF;
        return;
    }
    *instruction = cpu->program_rom[cpu->PC];
}

void execute_instruction(CPU_State *cpu, uint8_t instruction, bool *end_flag) {
    uint8_t opcode = instruction >> 4;
    uint8_t operand = instruction & 0x0F;

    switch (opcode) {
        case 0x0:
            instr_mem_to_r0(cpu, operand & 0x03);
            break;
        case 0x2:
            instr_r0_to_mem(cpu, operand & 0x03);
            break;
        case 0x4:
            {
                uint8_t rx = (operand & 0x0C) >> 2;
                uint8_t ry = operand & 0x03;
                instr_transfer_reg(cpu, rx, ry);
            }
            break;
        case 0x5:
            instr_immediate_set(cpu, operand);
            break;
        case 0x6:
            {
                uint8_t rx = (operand & 0x0C) >> 2;
                uint8_t ry = operand & 0x03;
                instr_add(cpu, rx, ry);
            }
            break;
        case 0x7:
            {
                uint8_t rx = (operand & 0x0C) >> 2;
                uint8_t ry = operand & 0x03;
                instr_sub(cpu, rx, ry);
            }
            break;
        case 0x8:
            instr_shift_left(cpu, operand & 0x03);
            break;
        case 0x9:
            instr_shift_right(cpu, operand & 0x03);
            break;
        case 0xA:
            instr_compare(cpu, operand & 0x03);
            break;
        case 0xC:
            instr_output(cpu, operand & 0x03);
            break;
        case 0xD:
            instr_input(cpu, operand & 0x03);
            break;
        case 0xE:
            instr_cond_branch(cpu, operand & 0x03);
            break;
        case 0xF:
            instr_uncond_branch(cpu, operand & 0x03);
            break;
        default:
            *end_flag = true;
            break;
    }
}

void instr_mem_to_r0(CPU_State *cpu, uint8_t rx) {
    uint8_t address = get_register_value(cpu, rx);
    if (address >= RAM_SIZE) {
        cpu->PC += 1;
        return;
    }
    uint8_t value = cpu->program_ram[address];
    cpu->regs.r0 = value;
    update_zero_flag(cpu, value);
    cpu->PC += 1;
}

void instr_r0_to_mem(CPU_State *cpu, uint8_t rx) {
    uint8_t address = get_register_value(cpu, rx);
    if (address >= RAM_SIZE) {
        cpu->PC += 1;
        return;
    }
    uint8_t value = cpu->regs.r0;
    cpu->program_ram[address] = value;
    update_zero_flag(cpu, value);
    cpu->PC += 1;
}

void instr_transfer_reg(CPU_State *cpu, uint8_t rx, uint8_t ry) {
    uint8_t value = get_register_value(cpu, rx);
    set_register_value(cpu, ry, value);
    cpu->PC += 1;
}

void instr_immediate_set(CPU_State *cpu, uint8_t value) {
    cpu->regs.r0 = value;
    update_zero_flag(cpu, value);
    cpu->PC += 1;
}

void instr_add(CPU_State *cpu, uint8_t rx, uint8_t ry) {
    uint8_t value_rx = get_register_value(cpu, rx);
    uint8_t value_ry = get_register_value(cpu, ry);
    uint8_t result = value_rx + value_ry;
    cpu->regs.r0 = result;
    update_zero_flag(cpu, result);
    cpu->PC += 1;
}

void instr_sub(CPU_State *cpu, uint8_t rx, uint8_t ry) {
    uint8_t value_rx = get_register_value(cpu, rx);
    uint8_t value_ry = get_register_value(cpu, ry);
    uint8_t result = value_rx - value_ry;
    cpu->regs.r0 = result;
    update_zero_flag(cpu, result);
    cpu->PC += 1;
}

void instr_shift_left(CPU_State *cpu, uint8_t rx) {
    uint8_t value_rx = get_register_value(cpu, rx);
    uint8_t result = value_rx << 1;
    cpu->regs.r0 = result;
    update_zero_flag(cpu, result);
    cpu->PC += 1;
}

void instr_shift_right(CPU_State *cpu, uint8_t rx) {
    uint8_t value_rx = get_register_value(cpu, rx);
    uint8_t result = value_rx >> 1;
    cpu->regs.r0 = result;
    update_zero_flag(cpu, result);
    cpu->PC += 1;
}

void instr_compare(CPU_State *cpu, uint8_t rx) {
    uint8_t value_rx = get_register_value(cpu, rx);
    if (value_rx == 0) {
        cpu->status |= STATUS_ZERO;
    } else {
        cpu->status &= ~STATUS_ZERO;
    }
    cpu->PC += 1;
}

void instr_output(CPU_State *cpu, uint8_t rx) {
    uint8_t value = get_register_value(cpu, rx);
    char buffer[16];
    sprintf(buffer, "Output: %d\n", value);
    uart_puts(UART_ID, buffer);
    cpu->PC += 1;
}

void instr_input(CPU_State *cpu, uint8_t rx) {
    uart_puts(UART_ID, "Enter a value (0-255): ");
    while (!uart_is_readable(UART_ID)) {
    }
    char input = uart_getc(UART_ID);
    if (isdigit(input)) {
        int value = atoi(&input);
        set_register_value(cpu, rx, (uint8_t)value);
    }
    cpu->PC += 1;
}

void instr_cond_branch(CPU_State *cpu, uint8_t rx) {
    if (cpu->status & STATUS_ZERO) {
        cpu->PC = get_register_value(cpu, rx);
    } else {
        cpu->PC += 1;
    }
}

void instr_uncond_branch(CPU_State *cpu, uint8_t rx) {
    cpu->PC = get_register_value(cpu, rx);
}

uint8_t get_register_value(CPU_State *cpu, uint8_t reg) {
    switch (reg) {
        case 0: return cpu->regs.r0;
        case 1: return cpu->regs.r1;
        case 2: return cpu->regs.r2;
        case 3: return cpu->regs.r3;
        default: return 0;
    }
}

void set_register_value(CPU_State *cpu, uint8_t reg, uint8_t value) {
    switch (reg) {
        case 0: cpu->regs.r0 = value; break;
        case 1: cpu->regs.r1 = value; break;
        case 2: cpu->regs.r2 = value; break;
        case 3: cpu->regs.r3 = value; break;
    }
}

void update_zero_flag(CPU_State *cpu, uint8_t result) {
    if (result == 0) {
        cpu->status |= STATUS_ZERO;
    } else {
        cpu->status &= ~STATUS_ZERO;
    }
}
