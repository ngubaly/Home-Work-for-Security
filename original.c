#include <stdio.h>
#include "pico/stdlib.h"
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define ROM_SIZE        (128)
#define RAM_SIZE        (128)
#define MEM_SIZE        (256)

#define STATUS_ZERO     (0x01)   // ZF specified by LSB bit
#define STATUS_OF       (0x02)   // OF specified by the second LSB bit 

// Define opcodes
#define mem_to_r0_1                 (0x0)
#define mem_to_r0_2                 (0x1)
#define r0_to_ram_1                 (0x2)
#define r0_to_ram_2                 (0x3)    
#define rx_to_ry                    (0x4)
#define set_value_r0                (0x5) 
#define add_ry_to_rx                (0x6) 
#define substract_ry_by_rx          (0x7)
#define shift_left                  (0x8)
#define shift_right                 (0x9)
#define compare_rx_with_zero_1      (0xA)
#define compare_rx_with_zero_2      (0xB)
#define output_external             (0xC)
#define input_external              (0xD)
#define condition_jump              (0xE)
#define uncondition_jump            (0xF)



typedef enum {
    REG_0,
    REG_1,
    REG_2,
    REG_3
} Register;

// Structure of CPU
typedef struct cpu_state{
    uint8_t PC;
    uint8_t r0;
    uint8_t r1;
    uint8_t r2;
    uint8_t r3;
    uint8_t status;
    uint8_t instruction; 
    uint8_t memory[MEM_SIZE];
} stCpu_state;

// Components of an instruction
typedef struct inst{
    uint8_t opcode;
    uint8_t operand;
} stInstruction;

//----------------------------------------------
// Global variables declare
stCpu_state gCpu_instance;  
stCpu_state *gCpu_p = NULL;
bool gEnd_flag_value = false;
bool *gEnd_flag_p = NULL;


//----------------------------------
// -------Function prototype--------

// CPU implementer function
void initialize_cpu();                            
void load_addition_program();                     
void fetch_instruction();
stInstruction decode_instruction();  
void execute_instruction(stInstruction instr, bool *end_flag);

// Instructor function
void instr_mem_to_r0(Register rx);
void instr_r0_to_ram(Register rx);
void instr_transfer_reg(Register rx, Register ry);
void instr_immediate_set(uint8_t value);
void instr_add(Register rx, Register ry);
void instr_sub(Register rx, Register ry);
void instr_shift_left(Register rx);
void instr_shift_right(Register rx);
void instr_compare(Register rx);
void instr_output(Register rx);
void instr_input(Register rx);
void instr_cond_branch(Register rx, bool *end_flag);
void instr_uncond_branch(Register rx, bool *end_flag);

uint8_t get_register_value(Register reg);
void set_register_value(Register reg, uint8_t value);

void update_zero_flag(uint8_t value);


//---------------------------
// Define functions

// Initilization of CPU family
void initialize_cpu(){
    // initilization for registers
    gCpu_p->PC = 0;
    gCpu_p->r0 = 0;
    gCpu_p->r1 = 0;
    gCpu_p->r2 = 0;
    gCpu_p->r3 = 0;
    gCpu_p->status = 0;
    gCpu_p->instruction = 0;
    // initialization for memory
    memset(&(gCpu_p->memory), 0, MEM_SIZE);     
    printf("CPU is initialized!\n");
}
// Store the machine code of factorial program into ROM
void load_addition_program() {
    uint8_t addition_program[] = {
        0xD2,
        0x4B,
        0x51,
        0x7C,

        0x5D,
        0x60,
        0xA3,
        0xE0,

        0x4D,

        0x5A,
        0x60,
        0xA1,
        0xE0,
        0x03,
        0x62,
        0x23,
        0x51,
        0x74,

        0x59,
        0xF0,
        0x03,
        0x42,
        0x51,
        0x7C,

        0x54,
        0xF0,
        0xC2
    };
    // write the load program to memory
    memcpy(gCpu_p->memory, addition_program, sizeof(addition_program));
    // set the rest of ROM equal 0
    size_t t = ROM_SIZE - sizeof(addition_program);
    memset(&(gCpu_p->memory[sizeof(addition_program)]), 0, t);
    printf("Program is loaded into ROM!!\n");
}

// capture the machine code from RAM to instruction register through by PC
void fetch_instruction() {
    // stop the program when PC is over the MEM_SIZE
    if (gCpu_p->PC >= MEM_SIZE) {
        printf("Program Counter out of memory's bound.\n\r");
        *gEnd_flag_p = true;        // instruction code for stop program     
    }
    else{
        gCpu_p->instruction = gCpu_p->memory[gCpu_p->PC];  // start from 0
        gCpu_p->PC += 1;
    }
}

// Decode the instruction to obtain the opcode part and operand part 
stInstruction decode_instruction(){
    stInstruction instr;
    // extract the opcode part and operand part from instruction code
    instr.opcode = (gCpu_p->instruction) >> 4;
    instr.operand = (gCpu_p->instruction) & 0x0F;
    return instr;
}

// From the result of decoding process, execute the program accordingly 
void execute_instruction(stInstruction instr, bool *end_flag) {
    uint8_t opcode_decoded = instr.opcode;
    uint8_t operand_decoded = instr.operand;
    uint8_t select_bit = (instr.operand & 0x04) >> 2;


    Register reg_H = (Register)((operand_decoded & 0x0C) >> 2);          // Take the first two bits ("higher") of the 4-bit LSB
    Register reg_L = (Register)(operand_decoded & 0x03);                 // Take the last two bits ("lower") of the 4-bit LSB

    switch (opcode_decoded) {

        case mem_to_r0_1: 
        case mem_to_r0_2:                           // transfer data from RAM to register
            instr_mem_to_r0(reg_L, select_bit);
            break;

        case r0_to_ram_1: 
        case r0_to_ram_2:                           // transfer data from register to RAM
            instr_r0_to_ram(reg_L);
            break;

        case rx_to_ry:                              // transfer data between 2 register
            instr_transfer_reg(reg_H, reg_L);
            break;
        case set_value_r0:                          // set value for register
            instr_immediate_set(operand_decoded);
            break;
        case add_ry_to_rx:                          // add 2 register
            instr_add(reg_H, reg_L);
            break;

        case substract_ry_by_rx:                        // substract 2 register
            instr_sub(reg_H, reg_L);
            break;
        case shift_left:                                // shift left
            instr_shift_left(reg_H);
            break;

        case shift_right:                               // shift right
            instr_shift_right(reg_H);
            break;

        case compare_rx_with_zero_1: 
        case compare_rx_with_zero_2:                    // compare rx with zero
            instr_compare(reg_H);
            break;

        case output_external:                           // output external
            instr_output(reg_H);
            break;

        case input_external:                            // input external
            instr_input(reg_H);
            break;

        case condition_jump:                             // jump with condition
            instr_cond_branch(reg_H, end_flag);
            break;

        case uncondition_jump:                          // jump with uncondition
            instr_uncond_branch(reg_H, end_flag);
            break;
    }
}

// transfer data from memory[value_rx] to r0 (read data from memory)
// The memory space stretches from memory[0] belong to memory[255]

void instr_mem_to_r0(Register rx, uint8_t select_bit) {
    uint8_t address_read_data = get_register_value(rx);      // region for reading: from memory index: 0x00 to 0xFF (all of area memory)
    uint8_t read_data;

    // check if the address is within valid memory bounds
    // if select bit = 0, access the ROM part of memory
    if(select_bit == 0){
        if(address_read_data < ROM_SIZE){
            read_data = gCpu_p->memory[address_read_data];
        }
        else{
            printf("ROM access out of bounds.\n");
            *gEnd_flag_p = true;
            return;
        }
    }
    // if select bit = 1, access the RAM part of memory
    else{
        if (address_read_data >= RAM_SIZE && address_read_data < MEM_SIZE){
            read_data = gCpu_p->memory[address_read_data];
        }
        else{
            printf("RAM access out of bounds.\n");
            *gEnd_flag_p = true;
            return;
        }
    }
    set_register_value(REG_0, read_data);                     // read data to register 0 
    update_zero_flag(read_data);
} 

// transfer data from r0 to RAM[rx]
void instr_r0_to_ram(Register rx) {
    uint8_t address_write_data =  (get_register_value(rx))%128 + 0x80;      // define the location for write data to, limit the offset   
    uint8_t write_data = get_register_value(REG_0);                         // get data stored in register 0
    
    // ensure the address is within RAM bounds (0x80 to 0xFF)
    if( address_write_data >= ROM_SIZE && address_write_data < MEM_SIZE ){   // the address places on [0x80, 0xFF], i.e. in the bound of RAM 
        gCpu_p->memory[address_write_data] = write_data;                    // write data to the defined location 
        update_zero_flag(write_data);
    }
    else{
        printf("Out of RAM store!\n");
        *gEnd_flag_p = true;
    }    
}
// transfer data between 2 registers
void instr_transfer_reg(Register rx, Register ry) {
    uint8_t value_rx = get_register_value(rx);
    set_register_value(ry, value_rx);
    update_zero_flag(value_rx);
}

// set immediate value into r0
void instr_immediate_set(uint8_t value) {
    set_register_value(REG_0, value);
    update_zero_flag(value);
}

// addition rx and ry and then store the result in rx
void instr_add(Register rx, Register ry) {
    uint8_t value_rx = get_register_value(rx);
    uint8_t value_ry = get_register_value(ry);
    int16_t result = value_rx + value_ry;
    // if the result can't represented by 8 bits, i.e. overflow is occur, 
    // then the result should be truncated to 8-bit LSB of result (cast to 8-bits unsign int) 
    int16_t result_mask = result & 0xFF;
    uint8_t result_truncated = (uint8_t)result_mask;
    if(result > 0xFF){      // result exceeds 0xFF
        set_register_value(rx, result_truncated);
        gCpu_p->status |= STATUS_OF;        // update the Overflow flag
        printf("\nOverflow occurs in addition operation!\n");
    }
    else {   // result didn't exceed 0xFF
        set_register_value(rx, (uint8_t)result);
        update_zero_flag((uint8_t)result);
        gCpu_p->status &= ~STATUS_OF;
    }
}
// substract rx and ry then store the result in rx
void instr_sub(Register rx, Register ry) {
    uint8_t value_rx = get_register_value(rx);
    uint8_t value_ry = get_register_value(ry);
    int16_t result = value_rx - value_ry;    
    // if the result can't represented by 8 bits, i.e. overflow is occur, 
    // then the result should be truncated to 8-bit LSB of result (cast to 8-bits unsign int) 
    int16_t result_mask = result & 0xFF;
    uint8_t result_truncated = (uint8_t)result_mask;
    if(result < 0){
        set_register_value(rx, result_truncated);
        gCpu_p->status |= STATUS_OF;
        printf("\nOverflow occurs in subtraction operation!\n");
    }
    else{
        set_register_value(rx, (uint8_t)result);
        update_zero_flag((uint8_t)result);  
        gCpu_p->status &= ~STATUS_OF;
    }
}

// shift left rx 4 bits 
void instr_shift_left(Register rx) {
    uint8_t value_rx = get_register_value(rx);
    value_rx <<= 4;
    set_register_value(rx, value_rx);
}

// shift right rx 4 bits
void instr_shift_right(Register rx) {
    uint8_t value_rx = get_register_value(rx);
    value_rx >>= 4;
    set_register_value(rx, value_rx);
}

// compare rx with zero
void instr_compare(Register rx) {
    uint8_t value_rx = get_register_value(rx);
    update_zero_flag(value_rx);
}

// external the last ouput 
void instr_output(Register rx) {
    uint8_t value_rx = get_register_value(rx);
    printf("\n%d\n", value_rx);
    *gEnd_flag_p = true;           // exit program
}

// external input
void instr_input(Register rx) {
    char input_buffer[1];
    uint8_t value;
    input_buffer[0] = getchar();
    printf("?");
    putchar(input_buffer[0]);
    value = (uint8_t)atoi(input_buffer);    // filter number and typecast to uint_8 
    set_register_value(rx, value);
}

// condition branch (if ZF set then rx -> PC)
void instr_cond_branch(Register rx, bool *end_flag) {
    uint8_t index_branch_to = get_register_value(rx);
    if (gCpu_p->status & STATUS_ZERO) {
        if(index_branch_to < MEM_SIZE){
            gCpu_p->PC = index_branch_to;
        }
        else {
            printf("Branch location out of bounds.\\n\r");
            *gEnd_flag_p = true;
        }
    }
}

// unconditional branch (rx ->PC)
void instr_uncond_branch(Register rx, bool *end_flag) {
    uint8_t index_branch_to = get_register_value(rx);
    if(index_branch_to < MEM_SIZE){
        gCpu_p->PC = index_branch_to;
    }
    else{
        printf("Branch location out of bounds.\\n\r");
        *gEnd_flag_p = true;
    }
}

// get value stored in register
uint8_t get_register_value(Register reg) {
    uint8_t reg_value;
    switch (reg) {
        case REG_0: 
            reg_value = gCpu_p->r0;
            break;
        case REG_1: 
            reg_value = gCpu_p->r1;
            break;
        case REG_2: 
            reg_value = gCpu_p->r2;
            break;
        case REG_3: 
            reg_value = gCpu_p->r3;
            break;
        default: 
            printf("Invalid register!");
    }
    return reg_value;
}

// write a value into register
void set_register_value(Register reg, uint8_t value) {
    switch (reg) {
        case REG_0: 
            gCpu_p->r0 = value; 
            break;
        case REG_1: 
            gCpu_p->r1 = value; 
            break;
        case REG_2: 
            gCpu_p->r2 = value; 
            break;
        case REG_3: 
            gCpu_p->r3 = value; 
            break;
        default: 
            printf("Invalid register!");
    }
}

// function for update ZF bit
void update_zero_flag(uint8_t value) {
    if (value == 0) {
        gCpu_p->status |= STATUS_ZERO;
    } else {
        gCpu_p->status &= ~STATUS_ZERO;
    }
}


// ------------------------------------ 
// Main function
int main() {
    // PICO stdio initialization
    stdio_usb_init();
    while(!stdio_usb_connected());
    
    stInstruction instr_decoded;
    gEnd_flag_p = &gEnd_flag_value;
    gCpu_p = &gCpu_instance;

    sleep_ms(100);
    load_addition_program();    // load the factorial program into ROM
    initialize_cpu();           // initilization components of cpu
    while (1){
        // check the end_flag, if set, drive cpu into an infinite loop
        if(*gEnd_flag_p == true)
            continue;
    
        fetch_instruction();        // fetching
        // check the end_flag
        if(*gEnd_flag_p == true){
            continue;
        }
        instr_decoded = decode_instruction();       // decoding
        execute_instruction(instr_decoded, gEnd_flag_p);        // executing
        sleep_ms(100);
    }
    return 0;
}


void load_program_from_user() {
    char input[256];
    printf("Enter machine code in hex, separated by spaces (e.g., 0xD2 0x4B):\n");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    input[strcspn(input, "\n")] = '\0'; // Remove newline character
    char *token = strtok(input, " ");
    uint8_t *rom = &(gCpu_p->memory[0]);
    int addr = 0;
    while (token != NULL && addr < ROM_SIZE) {
        uint8_t byte;
        if (sscanf(token, "0x%hhX", &byte) == 1) {
            rom[addr] = byte;
            addr++;
        } else {
            printf("Invalid hex value: %s. Skipping.\n", token);
        }
        token = strtok(NULL, " ");
    }
    printf("Loaded %d bytes into ROM.\n", addr);
}
