# Home-Work-for-Security
Nơi lưu giữ bài tập theo tuần của học phần An toàn thông tin 


Week 2: Thuật toán bình phương giữa

Week 3: Mã hóa Sosemanuk
#include <stdio.h>
#include "pico/stdlib.h"
#include <stdint.h>
#include <string.h>
#include <ctype.h>

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
        // 0: Input N into r1
        0xD1, // 1101 0001: Input into r1
        // 1: Set r0 = 1
        0x51, // 0101 0001: Immediate set r0 = 1
        // 2: Transfer r0 to r2
        0x42, // 0100 0010: Transfer r0 to r2
        // 3: Transfer r1 to r3
        0x43, // 0100 0011: Transfer r1 to r3
    // Loop_Start:
        // 4: Compare r3 == 0
        0xA3, // 1010 0011: Compare r3 == 0
        // 5: If zero, jump to End (address 14)
        0x5E, // 0101 1110: Immediate set r0 = 14
        0xE0, // 1110 0000: Conditional branch to r0 if zero
        // 7: Transfer r2 to r0
        0x42, // 0100 0010: Transfer r0 to r2 (should be r2 to r0)
        // Corrected instruction:
        0x24, // 0010 0100: Transfer r2 to r0 (0100 xxyy with rx=2, ry=0)
        // 8: Set r2 = 0
        0x50, // 0101 0000: Immediate set r0 = 0
        0x42, // 0100 0010: Transfer r0 to r2
        // 10: Transfer r3 to r1
        0x43, // 0100 0011: Transfer r1 to r3 (should be r3 to r1)
        // Corrected instruction:
        0x4C, // 0100 1100: Transfer r3 to r1 (rx=3, ry=1)
    // Inner_Loop_Start:
        // 11: Compare r1 == 0
        0xA1, // 1010 0001: Compare r1 == 0
        // 12: If zero, jump to After_Inner_Loop (address 17)
        0x5F, // 0101 1111: Immediate set r0 = 15
        0xE0, // 1110 0000: Conditional branch to r0 if zero
        // 14: Add r0 + r2 -> r0
        0x60, // 0110 0000: Add r0 + r2 -> r0
        // 15: Transfer r0 to r2
        0x42, // 0100 0010: Transfer r0 to r2
        // 16: Subtract 1 from r1
        0x51, // 0101 0001: Immediate set r0 = 1
        0x71, // 0111 0001: Subtract r1 - r0 -> r1
        // 18: Unconditional branch to Inner_Loop_Start (address 11)
        0x5B, // 0101 1011: Immediate set r0 = 11
        0xF0, // 1111 0000: Unconditional branch to r0
    // After_Inner_Loop:
        // 20: Subtract 1 from r3
        0x51, // 0101 0001: Immediate set r0 = 1
        0x73, // 0111 0011: Subtract r3 - r0 -> r3
        // 22: Unconditional branch to Loop_Start (address 4)
        0x54, // 0101 0100: Immediate set r0 = 4
        0xF0, // 1111 0000: Unconditional branch to r0
    // End:
        // 24: Output r2
        0xC2, // 1100 0010: Output from r2
        // 25: Infinite loop to end program
        0xFF  // 1111 1111: Invalid opcode to stop execution
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
    update_zero_flag(cpu, value);
    cpu->PC += 1;
}

void instr_immediate_set(CPU_State *cpu, uint8_t value) {
    cpu->regs.r0 = value;
    update_zero_flag(cpu, value);
    cpu->PC += 1;
}

void instr_add(CPU_State *cpu, uint8_t rx, uint8_t ry) {
    uint8_t a = get_register_value(cpu, rx);
    uint8_t b = get_register_value(cpu, ry);
    uint8_t result = a + b;
    set_register_value(cpu, rx, result);
    update_zero_flag(cpu, result);
    cpu->PC += 1;
}

void instr_sub(CPU_State *cpu, uint8_t rx, uint8_t ry) {
    uint8_t a = get_register_value(cpu, rx);
    uint8_t b = get_register_value(cpu, ry);
    uint8_t result = a - b;
    set_register_value(cpu, rx, result);
    update_zero_flag(cpu, result);
    cpu->PC += 1;
}

void instr_shift_left(CPU_State *cpu, uint8_t rx) {
    uint8_t value = get_register_value(cpu, rx);
    value <<= 4;
    set_register_value(cpu, rx, value);
    cpu->PC += 1;
}

void instr_shift_right(CPU_State *cpu, uint8_t rx) {
    uint8_t value = get_register_value(cpu, rx);
    value >>= 4;
    set_register_value(cpu, rx, value);
    cpu->PC += 1;
}

void instr_compare(CPU_State *cpu, uint8_t rx) {
    uint8_t value = get_register_value(cpu, rx);
    if (value == 0)
        cpu->status |= STATUS_ZERO;
    else
        cpu->status &= ~STATUS_ZERO;
    cpu->PC += 1;
}

void instr_output(CPU_State *cpu, uint8_t rx) {
    uint8_t value = get_register_value(cpu, rx);
    char buffer[16];
    sprintf(buffer, "%d\n", value);
    uart_puts(UART_ID, buffer);
    cpu->PC += 1;
}

void instr_input(CPU_State *cpu, uint8_t rx) {
    uart_puts(UART_ID, "Enter a number (0-255): ");
    char input_buffer[4] = {0};
    int idx = 0;
    while (1) {
        int c = getchar_timeout_us(1000000); // Timeout 1 second
        if (c == PICO_ERROR_TIMEOUT) {
            continue;
        }
        if (c == '\n' || c == '\r') {
            break;
        }
        if (isdigit(c)) {
            if (idx < 3) {
                uart_putc(UART_ID, c);
                input_buffer[idx++] = (char)c;
            }
        }
    }
    input_buffer[idx] = '\0';
    uint8_t value = (uint8_t)atoi(input_buffer);
    set_register_value(cpu, rx, value);
    update_zero_flag(cpu, value);
    uart_puts(UART_ID, "\n");
    cpu->PC += 1;
}

void instr_cond_branch(CPU_State *cpu, uint8_t rx) {
    if (cpu->status & STATUS_ZERO) {
        uint8_t address = get_register_value(cpu, rx);
        if (address < ROM_SIZE) {
            cpu->PC = address;
        } else {
            cpu->PC += 1;
        }
    } else {
        cpu->PC += 1;
    }
}

void instr_uncond_branch(CPU_State *cpu, uint8_t rx) {
    uint8_t address = get_register_value(cpu, rx);
    if (address < ROM_SIZE) {
        cpu->PC = address;
    } else {
        cpu->PC += 1;
    }
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
    if (result == 0)
        cpu->status |= STATUS_ZERO;
    else
        cpu->status &= ~STATUS_ZERO;
}
