#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

// Định nghĩa các hằng số
#define ROM_SIZE        128
#define RAM_SIZE        128
#define STATUS_ZERO     0x01
#define STATUS_OF       0x02

// Cấu trúc thanh ghi chung
typedef struct {
    uint8_t r0;
    uint8_t r1;
    uint8_t r2;
    uint8_t r3;
} GP_Registers;

// Cấu trúc CPU
typedef struct {
    uint8_t PC;  // Program Counter
    GP_Registers regs;  // Các thanh ghi r0 đến r3
    uint8_t status;  // Thanh ghi trạng thái
    uint8_t program_rom[ROM_SIZE];  // Bộ nhớ chương trình ROM
    uint8_t program_ram[RAM_SIZE];  // Bộ nhớ dữ liệu RAM
} CPU_State;

// Khởi tạo CPU
void initialize_cpu(CPU_State *cpu) {
    cpu->PC = 0;  // Đặt PC bắt đầu từ 0
    cpu->regs.r0 = 0;
    cpu->regs.r1 = 0;
    cpu->regs.r2 = 0;
    cpu->regs.r3 = 0;
    cpu->status = 0;
}

// Hàm tải chương trình giai thừa vào ROM
void load_factorial_program(CPU_State *cpu) {
    cpu->program_rom[0] = 0x10; // Lệnh: Immediate Set r0 (gán giá trị 5 vào r0)
    cpu->program_rom[1] = 5;    // Giá trị 5 cho r0
    cpu->program_rom[2] = 0x11; // Lệnh: Immediate Set r1 (gán giá trị 1 vào r1)
    cpu->program_rom[3] = 1;    // Giá trị 1 cho r1
    cpu->program_rom[4] = 0x32; // Lệnh: Multiply r0 * r1 → r1
    cpu->program_rom[5] = 0x00; // r0 là operand 1
    cpu->program_rom[6] = 0x01; // r1 là operand 2
    cpu->program_rom[7] = 0xFF; // Lệnh dừng (ví dụ: END)
}

// Hàm Fetch: Lấy lệnh từ ROM
void fetch_instruction(CPU_State *cpu, uint8_t *instruction) {
    *instruction = cpu->program_rom[cpu->PC];
    cpu->PC++;  // Tăng Program Counter
}

// Hàm Execute: Thực thi lệnh
void execute_instruction(CPU_State *cpu, uint8_t instruction) {
    switch (instruction) {
        case 0x10:  // Immediate Set r0
            cpu->regs.r0 = cpu->program_rom[cpu->PC++];
            break;
        case 0x11:  // Immediate Set r1
            cpu->regs.r1 = cpu->program_rom[cpu->PC++];
            break;
        case 0x32:  // Multiply r0 * r1 → r1
            cpu->regs.r1 = cpu->regs.r0 * cpu->regs.r1;
            if (cpu->regs.r1 == 0) cpu->status |= STATUS_ZERO;  // Kiểm tra Zero
            break;
        case 0xFF:  // Dừng chương trình
            break;
        default:
            printf("Unknown instruction: %02X\n", instruction);
            break;
    }
}

// Chương trình chính
int main() {
    CPU_State cpu;
    initialize_cpu(&cpu);
    load_factorial_program(&cpu);

    uint8_t instruction;
    bool end_flag = false;

    while (!end_flag) {
        fetch_instruction(&cpu, &instruction);
        execute_instruction(&cpu, instruction);
        if (instruction == 0xFF) {
            end_flag = true;
        }
    }

    printf("Factorial result in r1: %d\n", cpu.regs.r1);
    return 0;
}
