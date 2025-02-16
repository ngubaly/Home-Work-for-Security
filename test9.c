// Function Definitions

// Initialize CPU State: initialize all registers and memories to 0 
void initialize_cpu() {
    cpu->PC = 0;
    cpu->r0 = 0;
    cpu->r1 = 0;
    cpu->r2 = 0;
    cpu->r3 = 0;
    cpu->status = 0;
    memset(cpu->program_rom, 0, ROM_SIZE); // initialize ROM
    memset(cpu->program_ram, 0, RAM_SIZE); // initialize RAM
}

// Load Factorial Program into ROM
void load_factorial_program() {
    // Program for factorial
    // [Mô tả chương trình như bạn đã cung cấp]
    
    // Instruction code for factorial program
    uint8_t factorial_program[] = {
        // 1. Address 0: Input N into r1   
        0xD1, // 1101 0001 (external input into r1)
        // 2. Address 1: Set r0 = 1  
        0x51, // 0101 0001 (immediate set r0 = 1)
        // 3. Address 2: Transfer r0 into r2
        0x42, // 0100 0010 (r0 -> r2)
        // 4. Address 3: Transfer r1 into r3
        0x43, // 0100 0011 (r1 -> r3)
        // 5. Address 4: Compare r3 == 0                
        0xA0, // 1010 0000 (compare r3 with 0)
        
        // 6. Address 5: Set r0 = 12 (Address end)
        0x5C, // 0101 1100 (immediate set r0 = 12)
        // 7. Address 6: Conditional Branch to r0 if Zero
        0xE0, // 1110 0000 (conditional branch to address in r0 if Zero)
        // 8. Address 7: Transfer r2 into r0 (Not used in this program)
        0x20, // 0010 0000 (r0 -> RAM[r2])
        
        // 9. Address 8: Set r0 = 0
        0x50, // 0101 0000 (immediate set r0 = 0)
        // 10. Address 9: Transfer r3 into r1
        0x41, // 0100 0001 (r3 -> r1)
        // 11. Address 10: Compare r1 == 0
        0xA1, // 1010 0001 (compare r1 with 0)
        // 12. Address 11: Set r0 = 12 (Address end)
        0x5C, // 0101 1100 (immediate set r0 = 12)
        // 13. Address 12: Conditional Branch to r1 if Zero
        0xE1, // 1110 0001 (conditional branch to address in r1 if Zero)
        // 14. Address 13: Add (r0 + r2 → r0)
        0x62, // 0110 0010 (add r0 + r2 -> r0)
        // 15. Address 14: Transfer r0 into r2
        0x42, // 0100 0010 (r0 -> r2)
        // 16. Address 15: Set r0 = 1
        0x51, // 0101 0001 (immediate set r0 = 1)
        // 17. Address 16: Subtract (r1 - r0 → r1)
        0x71, // 0111 0001 (sub r1 - r0 -> r1)
        // 18. Address 17: Unconditional Branch to r1
        0xF1, // 1111 0001 (unconditional branch to address in r1)
        // 19. Address 18: Output r2
        0xC2, // 1100 0010 (output r2)
        // 20. Address 19: Set r0 = 12 (Address end)
        0x5C, // 0101 1100 (immediate set r0 = 12)
        // 21. Address 20: Unconditional Branch to r2 (Infinite loop to end program)
        0xF2  // 1111 0010 (unconditional branch to address in r2)
    };
    
    // Load the factorial program into program_rom
    memcpy(cpu->program_rom, factorial_program, sizeof(factorial_program));
    
    // Fill the rest of ROM with zero
    for (int i = sizeof(factorial_program); i < ROM_SIZE; i++) {
        cpu->program_rom[i] = 0x00; 
    }
    
    // Uncomment the line below for debugging
    // uart_puts(UART_ID, "Factorial program loaded into ROM.\n\r");
}

// Fetch instruction from ROM
void fetch_instruction() {
    if (cpu->PC >= ROM_SIZE) {
        uart_puts(UART_ID, "Program Counter out of ROM bounds.\n\r");
        *instruction = 0xFF; // using undefined opcode to stop program
        return;
    }
    *instruction = cpu->program_rom[cpu->PC];
}

// Decode and Execute Instruction
void decode_execute_instruction(bool *end_flag) {
    // Separate opcode and operand
    uint8_t opcode = (*instruction & 0xF0) >> 4; // Upper 4 bits
    uint8_t operand = (*instruction & 0x0F);    // Lower 4 bits

    switch (opcode) {
        case 0x0:
            // 0000 ddxx: Address indicated by rx ⇒ r0
            instr_mem_to_r0(operand & 0x03); 
            break;
        case 0x2:
            // 0010 ddxx: r0 ⇒ Address indicated by rx
            instr_r0_to_mem(operand & 0x03); 
            break;
        case 0x4:
            // 0100 xxyy: Transfer between 2 registers (rx → ry)
            {
                uint8_t rx = (operand & 0xC) >> 2; 
                uint8_t ry = operand & 0x3;        
                instr_transfer_reg(rx, ry);
            }
            break;
        case 0x5:
            // 0101 xxxx: set immediate value into r0
            {
                uint8_t value = operand; 
                instr_immediate_set(value);
            }
            break;
        case 0x6:
            // 0110 xxyy: addition (rx + ry → rx)
            {
                uint8_t rx = (operand & 0xC) >> 2;
                uint8_t ry = operand & 0x3;
                instr_add(rx, ry);
            }
            break;
        case 0x7:
            // 0111 xxyy: subtract (rx - ry → rx)
            {
                uint8_t rx = (operand & 0xC) >> 2;
                uint8_t ry = operand & 0x3;
                instr_sub(rx, ry);
            }
            break;
        case 0x8:
            // 1000 ddxx: shift left (rx << 4)
            {
                uint8_t rx = operand & 0x3;
                instr_shift_left(rx);
            }
            break;
        case 0x9:
            // 1001 ddxx: shift right (rx >> 4)
            {
                uint8_t rx = operand & 0x3;
                instr_shift_right(rx);
            }
            break;
        case 0xA:
            // 1010 ddxx: compare with zero (rx == 0)
            {
                uint8_t rx = operand & 0x3;
                instr_compare(rx);
            }
            break;
        case 0xC:
            // 1100 ddxx: output external from rx
            {
                uint8_t rx = operand & 0x3;
                instr_output(rx);
            }
            break;
        case 0xD:
            // 1101 ddxx: input external into rx
            {
                uint8_t rx = operand & 0x3;
                instr_input(rx);
            }
            break;
        case 0xE:
            // 1110 ddxx: conditional branch (if ZF = 1, PC ← rx)
            {
                uint8_t rx = operand & 0x3;
                instr_cond_branch(rx, end_flag);
            }
            break;
        case 0xF:
            // 1111 ddxx: unconditional branch (PC ← rx)
            {
                uint8_t rx = operand & 0x3;
                instr_uncond_branch(rx, end_flag);
            }
            break;
        default:
            uart_puts(UART_ID, "Undefined opcode encountered.\n\r");
            *end_flag = true;  
            break;
    }
}

// Instruction Implementations

// 1. 0000 ddxx: Address indicated by rx ⇒ r0
void instr_mem_to_r0(uint8_t rx) {
    if (rx > 0x3) {
        uart_puts(UART_ID, "Invalid register index in mem_to_r0.\n\r");
        cpu->PC += 1;
        return;
    }
    uint8_t address = get_register_value(rx);
    if (address < 0x80 || address >= 0x100) {  // the address is not in the RAM memory space (0x80 to 0xFF)
        uart_puts(UART_ID, "Memory address out of bounds in mem_to_r0.\n\r");
        cpu->PC += 1;
        return;
    }
    uint8_t value = cpu->program_ram[address - 0x80];   
    set_register_value(0, value); // r0 = RAM[address]
    update_zero_flag(value);
    cpu->PC += 1;
}

// 2. 0010 ddxx: r0 ⇒ Address indicated by rx
void instr_r0_to_mem(uint8_t rx) {
    if (rx > 0x3) {
        uart_puts(UART_ID, "Invalid register index in r0_to_mem.\n\r");
        cpu->PC += 1;
        return;
    }
    uint8_t address = get_register_value(rx);   // get address from rx
    if (address < 0x80 || address >= 0x100) {
        uart_puts(UART_ID, "Memory address out of bounds in r0_to_mem.\n\r");
        cpu->PC += 1;
        return;
    }
    uint8_t value = get_register_value(0); // r0 value
    cpu->program_ram[address - 0x80] = value;
    update_zero_flag(value);
    cpu->PC += 1;
}

// 3. 0100 xxyy: Transfer between 2 registers (rx → ry)
void instr_transfer_reg(uint8_t rx, uint8_t ry) {
    if (rx > 0x3 || ry > 0x3) {
        uart_puts(UART_ID, "Invalid register index in transfer_reg.\n\r");
        cpu->PC += 1;
        return;
    }
    uint8_t value = get_register_value(rx);
    set_register_value(ry, value);
    update_zero_flag(value);
    cpu->PC += 1;
}

// 4. 0101 xxxx: Set immediate value into r0
void instr_immediate_set(uint8_t value) {
    set_register_value(0, value); // Set r0
    update_zero_flag(value);
    cpu->PC += 1;
}

// 5. 0110 xxyy: Add (rx + ry → rx)
void instr_add(uint8_t rx, uint8_t ry) {
    if (rx > 0x3 || ry > 0x3) {
        uart_puts(UART_ID, "Invalid register index in add.\n\r");
        cpu->PC += 1;
        return;
    }
    uint8_t a = get_register_value(rx);
    uint8_t b = get_register_value(ry);
    uint16_t result = a + b;
    set_register_value(rx, result & 0xFF);
    update_zero_flag(result & 0xFF);
    // Update Overflow Flag
    if (result > 0xFF) {
        cpu->status |= STATUS_OF;
    }
    else {
        cpu->status &= ~STATUS_OF;
    }
    cpu->PC += 1;
}

// 6. 0111 xxyy: Subtract (rx - ry → rx)
void instr_sub(uint8_t rx, uint8_t ry) {
    if (rx > 0x3 || ry > 0x3) {
        uart_puts(UART_ID, "Invalid register index in sub.\n\r");
        cpu->PC += 1;
        return;
    }
    int16_t a = get_register_value(rx);
    int16_t b = get_register_value(ry);
    int16_t result = a - b;
    set_register_value(rx, result & 0xFF);
    update_zero_flag(result & 0xFF);
    // Update Overflow Flag
    if (result < 0) {
        cpu->status |= STATUS_OF;
    }
    else {
        cpu->status &= ~STATUS_OF;
    }
    cpu->PC += 1;
}

// 7. 1000 ddxx: Shift left (rx << 4 → rx)
void instr_shift_left(uint8_t rx) {
    if (rx > 0x3) {
        uart_puts(UART_ID, "Invalid register index in shift_left.\n\r");
        cpu->PC += 1;
        return;
    }
    uint8_t value = get_register_value(rx);
    value <<= 4;
    set_register_value(rx, value);
    // No status update
    cpu->PC += 1;
}

// 8. 1001 ddxx: Shift right (rx >> 4 → rx)
void instr_shift_right(uint8_t rx) {
    if (rx > 0x3) {
        uart_puts(UART_ID, "Invalid register index in shift_right.\n\r");
        cpu->PC += 1;
        return;
    }
    uint8_t value = get_register_value(rx);
    value >>= 4;
    set_register_value(rx, value);
    // No status update
    cpu->PC += 1;
}

// 9. 1010 ddxx: Compare (rx == 0)
void instr_compare(uint8_t rx) {
    if (rx > 0x3) {
        uart_puts(UART_ID, "Invalid register index in compare.\n\r");
        cpu->PC += 1;
        return;
    }
    uint8_t value = get_register_value(rx);
    if (value == 0) {
        cpu->status |= STATUS_ZERO;
    }
    else {
        cpu->status &= ~STATUS_ZERO;
    }
    cpu->PC += 1;
}

// 10. 1100 ddxx: Output from rx
void instr_output(uint8_t rx) {
    if (rx > 0x3) {
        uart_puts(UART_ID, "Invalid register index in output.\n\r");
        cpu->PC += 1;
        return;
    }
    uint8_t value = get_register_value(rx);
    char buffer[6]; // Enough to hold "255\r\n"
    snprintf(buffer, sizeof(buffer), "%d\r\n", value);
    uart_puts(UART_ID, buffer);
    cpu->PC += 1;
}

// 11. 1101 ddxx: Input into rx
void instr_input(uint8_t rx) {
    if (rx > 0x3) {
        uart_puts(UART_ID, "Invalid register index in input.\n\r");
        cpu->PC += 1;
        return;
    }
    uart_puts(UART_ID, "Enter a number (0-255): ");
    char input_buffer[4] = {0};
    int idx = 0;
    while (idx < 3) { // Max 3 digits for 0-255
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            if (c == '\n' || c == '\r') {
                break;
            }
            if (isdigit(c)) {
                uart_putc(UART_ID, c); // Echo character
                input_buffer[idx++] = c;
            }
        }
    }
    input_buffer[idx] = '\0';
    uint8_t value = (uint8_t)atoi(input_buffer);
    set_register_value(rx, value);
    update_zero_flag(value);
    uart_puts(UART_ID, "\r\n");
    cpu->PC += 1;
}

// 12. 1110 ddxx: Conditional Branch (if Zero = 1, PC ← value in rx)
void instr_cond_branch(uint8_t rx, bool *end_flag) {
    if (rx > 0x3) {
        uart_puts(UART_ID, "Invalid register index in cond_branch.\n\r");
        *end_flag = true;
        return;
    }
    if (cpu->status & STATUS_ZERO) {
        uint8_t branch_address = get_register_value(rx);
        if (branch_address < ROM_SIZE) {
            cpu->PC = branch_address;
        }
        else {
            uart_puts(UART_ID, "Branch address out of bounds.\n\r");
            *end_flag = true;
        }
    }
    else {
        cpu->PC += 1;
    }
}

// 13. 1111 ddxx: Unconditional Branch (PC ← value in rx)
void instr_uncond_branch(uint8_t rx, bool *end_flag) {
    if (rx > 0x3) {
        uart_puts(UART_ID, "Invalid register index in uncond_branch.\n\r");
        *end_flag = true;
        return;
    }
    uint8_t branch_address = get_register_value(rx);
    if (branch_address < ROM_SIZE) {
        cpu->PC = branch_address;
    }
    else {
        uart_puts(UART_ID, "Branch address out of bounds.\n\r");
        *end_flag = true;
    }
}

// Utility Functions

// Get Register Value
uint8_t get_register_value(uint8_t reg) {
    switch (reg) {
        case 0: return cpu->r0;
        case 1: return cpu->r1;
        case 2: return cpu->r2;
        case 3: return cpu->r3;
        default: return 0;
    }
}

// Set Register Value
void set_register_value(uint8_t reg, uint8_t value) {
    switch (reg) {
        case 0: cpu->r0 = value; break;
        case 1: cpu->r1 = value; break;
        case 2: cpu->r2 = value; break;
        case 3: cpu->r3 = value; break;
        default: break;
    }
}

// Update Zero Flag
void update_zero_flag(uint8_t result) {
    if (result == 0) {
        cpu->status |= STATUS_ZERO;
    }
    else {
        cpu->status &= ~STATUS_ZERO;
    }
}

// Update Overflow Flag
void update_overflow_flag(int result) {
    if (result > 0xFF || result < 0) {
        cpu->status |= STATUS_OF;
    }
    else {
        cpu->status &= ~STATUS_OF;
    }
}
