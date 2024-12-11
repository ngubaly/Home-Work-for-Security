#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

// Định nghĩa các thanh ghi
typedef struct {
    uint8_t PC;               // Program Counter
    uint8_t r[4];             // General-purpose registers r0 to r3
    struct {
        bool Zero;            // Zero flag
        bool Overflow;        // Overflow flag
    } status;
} CPU;

// Định nghĩa ROM và RAM
#define ROM_SIZE 128
#define RAM_SIZE 128

uint8_t ROM[ROM_SIZE];
uint8_t RAM[RAM_SIZE];

// Khởi tạo CPU
CPU cpu = {0};

// Hàm khởi tạo CPU và bộ nhớ
void initialize_cpu() {
    cpu.PC = 0; // Program Counter bắt đầu từ 0
    for(int i = 0; i < 4; i++) {
        cpu.r[i] = 0;
    }
    cpu.status.Zero = false;
    cpu.status.Overflow = false;

    // Khởi tạo ROM và RAM bằng 0
    for(int i = 0; i < ROM_SIZE; i++) {
        ROM[i] = 0;
    }
    for(int i = 0; i < RAM_SIZE; i++) {
        RAM[i] = 0;
    }
}

// Hàm khởi tạo UART
void initialize_uart() {
    const uint uart_id = uart0;
    const uint baud_rate = 9600;
    const uint tx_pin = 0; // GPIO0
    const uint rx_pin = 1; // GPIO1

    // Khởi tạo UART
    uart_init(uart_id, baud_rate);
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);

    // Cấu hình UART
    uart_set_format(uart_id, 8, 1, UART_PARITY_NONE);
}

// Hàm nhập từ UART
uint8_t uart_input() {
    while(!uart_is_readable(uart0));
    return uart_getc(uart0);
}

// Hàm xuất qua UART
void uart_output(uint8_t value) {
    uart_putc(uart0, value);
}

// Hàm Fetch: Lấy lệnh từ ROM
uint8_t fetch() {
    if(cpu.PC >= ROM_SIZE) {
        uart_puts(uart0, "PC out of ROM bounds!\n");
        return 0xFF; // Mã lỗi
    }
    uint8_t instruction = ROM[cpu.PC];
    cpu.PC += 1;
    return instruction;
}

// Hàm Decode và Execute
void decode_execute(uint8_t instruction) {
    uint8_t opcode = instruction >> 4; // 4 bit đầu là opcode
    uint8_t operand = instruction & 0x0F; // 4 bit sau là operand

    uint8_t rx, ry;
    uint8_t result;
    switch(opcode) {
        case 0x0: { // 0000 ddxx: Transfer từ memory tới r0
            rx = (instruction & 0x0C) >> 2; // dd
            uint8_t addr = cpu.r[rx];
            if(addr >= RAM_SIZE) {
                uart_puts(uart0, "RAM address out of bounds!\n");
                break;
            }
            cpu.r[0] = RAM[addr];
            // Cập nhật STATUS Zero
            cpu.status.Zero = (cpu.r[0] == 0);
            break;
        }
        case 0x2: { // 0010 ddxx: Transfer từ r0 tới memory
            rx = (instruction & 0x0C) >> 2; // dd
            uint8_t addr = cpu.r[rx];
            if(addr >= RAM_SIZE) {
                uart_puts(uart0, "RAM address out of bounds!\n");
                break;
            }
            RAM[addr] = cpu.r[0];
            // Cập nhật STATUS Zero
            cpu.status.Zero = (RAM[addr] == 0);
            break;
        }
        case 0x4: { // 0100 xxyy: Chuyển dữ liệu giữa các thanh ghi
            rx = (instruction & 0x0C) >> 2; // xx
            ry = (instruction & 0x03);      // yy
            cpu.r[ry] = cpu.r[rx];
            // Cập nhật STATUS Zero
            cpu.status.Zero = (cpu.r[ry] == 0);
            break;
        }
        case 0x5: { // 0101 xxxx: Đặt giá trị lập tức vào r0
            uint8_t immediate = instruction & 0x0F;
            cpu.r[0] = immediate;
            // Cập nhật STATUS Zero
            cpu.status.Zero = (cpu.r[0] == 0);
            break;
        }
        case 0x6: { // 0110 xxyy: Cộng giá trị thanh ghi
            rx = (instruction & 0x0C) >> 2; // xx
            ry = (instruction & 0x03);      // yy
            uint16_t temp = cpu.r[rx] + cpu.r[ry];
            cpu.status.Overflow = (temp > 0xFF);
            cpu.r[rx] = temp & 0xFF;
            // Cập nhật STATUS Zero
            cpu.status.Zero = (cpu.r[rx] == 0);
            break;
        }
        case 0x7: { // 0111 xxyy: Trừ giá trị thanh ghi
            rx = (instruction & 0x0C) >> 2; // xx
            ry = (instruction & 0x03);      // yy
            int16_t temp = (int8_t)cpu.r[rx] - (int8_t)cpu.r[ry];
            cpu.status.Overflow = (temp < -128 || temp > 127);
            cpu.r[rx] = temp & 0xFF;
            // Cập nhật STATUS Zero
            cpu.status.Zero = (cpu.r[rx] == 0);
            break;
        }
        case 0x8: { // 1000 ddxx: Dịch trái thanh ghi
            rx = (instruction & 0x0C) >> 2; // dd
            cpu.r[rx] = cpu.r[rx] << 4;
            // Không cập nhật STATUS
            break;
        }
        case 0x9: { // 1001 ddxx: Dịch phải thanh ghi
            rx = (instruction & 0x0C) >> 2; // dd
            cpu.r[rx] = cpu.r[rx] >> 4;
            // Không cập nhật STATUS
            break;
        }
        case 0xA: { // 1010 ddxx: So sánh thanh ghi với 0
            rx = (instruction & 0x0C) >> 2; // dd
            cpu.status.Zero = (cpu.r[rx] == 0);
            break;
        }
        case 0xC: { // 1100 ddxx: Xuất dữ liệu từ rx ra ngoài (Output)
            rx = (instruction & 0x0C) >> 2; // dd
            // Xuất qua UART
            uart_output(cpu.r[rx]);
            break;
        }
        case 0xD: { // 1101 ddxx: Nhập dữ liệu từ ngoài vào rx (Input)
            rx = (instruction & 0x0C) >> 2; // dd
            // Nhập từ UART
            uart_puts(uart0, "Input value: ");
            cpu.r[rx] = uart_input();
            break;
        }
        case 0xE: { // 1110 ddxx: Nhảy có điều kiện (Conditional Branch)
            rx = (instruction & 0x0C) >> 2; // dd
            if(cpu.status.Zero) {
                cpu.PC = cpu.r[rx];
            }
            break;
        }
        case 0xF: { // 1111 ddxx: Nhảy không điều kiện (Unconditional Branch)
            rx = (instruction & 0x0C) >> 2; // dd
            cpu.PC = cpu.r[rx];
            break;
        }
        default:
            uart_puts(uart0, "Unknown opcode: ");
            char buffer[4];
            snprintf(buffer, sizeof(buffer), "%02X\n", opcode);
            uart_puts(uart0, buffer);
            break;
    }
}

// Vòng lặp chính của CPU
void run_cpu() {
    while(true) {
        uint8_t instruction = fetch();
        if(instruction == 0xFF) {
            uart_puts(uart0, "Halting CPU.\n");
            break;
        }
        decode_execute(instruction);
        // Đặt một khoảng dừng để dễ theo dõi
        sleep_ms(100);
    }
}

// Hàm nạp chương trình tính giai thừa vào ROM
void load_program_factorial() {
    // Reset ROM
    for(int i = 0; i < ROM_SIZE; i++) {
        ROM[i] = 0;
    }

    // Các lệnh tính giai thừa
    // Địa chỉ 0x00 đến 0x15
    ROM[0] = 0xDC; // 0x00: Nhập n vào r3
    ROM[1] = 0x51; // 0x01: Đặt 1 vào r0
    ROM[2] = 0x42; // 0x02: Chuyển r0 vào r2 (result = 1)
    ROM[3] = 0x4D; // 0x03: Chuyển r3 vào r1 (counter = n)
    ROM[4] = 0xA4; // 0x04: So sánh r1 với 0
    ROM[5] = 0x5C; // 0x05: Đặt immediate 12 vào r0 (END = 0x0C)
    ROM[6] = 0xE3; // 0x06: Conditional Branch nếu Zero = 1 → PC = r3 (END)
    ROM[7] = 0x50; // 0x07: Đặt 0 vào r0 (temp = 0)
    ROM[8] = 0xA4; // 0x08: So sánh r1 với 0 (loop phụ)
    ROM[9] = 0x5E; // 0x09: Đặt immediate 14 vào r0 (END_MULTIPLY = 0x0E)
    ROM[10] = 0xE3; // 0x0A: Conditional Branch nếu Zero = 1 → PC = r3 (END_MULTIPLY)
    ROM[11] = 0x68; // 0x0B: Cộng r2 vào r0 (temp = temp + result)
    ROM[12] = 0x51; // 0x0C: Đặt 1 vào r0
    ROM[13] = 0x71; // 0x0D: Trừ r0 từ r1 (loop_counter--)
    ROM[14] = 0xF0; // 0x0E: Unconditional Branch đến r0 (0x0A)
    ROM[15] = 0x42; // 0x0F: Chuyển r0 vào r2 (result = temp)
    ROM[16] = 0x71; // 0x10: Trừ r0 từ r1 (counter--)
    ROM[17] = 0x54; // 0x11: Đặt immediate 4 vào r0 (loop chính = 0x04)
    ROM[18] = 0xF0; // 0x12: Unconditional Branch đến r0 (0x04)
    ROM[19] = 0x48; // 0x13: Chuyển r2 vào r0 (result vào r0)
    ROM[20] = 0xC0; // 0x14: Xuất r0 qua UART
    ROM[21] = 0xF0; // 0x15: Unconditional Branch vô hạn
}

// Hàm main
int main() {
    // Khởi tạo hệ thống
    stdio_init_all();
    initialize_uart();
    initialize_cpu();
    load_program_factorial(); // Sử dụng chương trình tính factorial

    // In thông báo bắt đầu
    uart_puts(uart0, "CPU started. Please input a number for factorial:\n");

    // Chạy CPU
    run_cpu();

    return 0;
}
