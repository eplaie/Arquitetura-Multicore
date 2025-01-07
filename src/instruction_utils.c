// instruction_utils.c
#include "instruction_utils.h"

unsigned short int get_register_index(const char* reg_name) {
    static const char* register_names[] = {
        "A0", "B0", "C0", "D0", "E0", "F0", "G0", "H0",
        "I0", "J0", "K0", "L0", "M0", "N0", "O0", "P0",
        "A1", "B1", "C1", "D1", "E1", "F1", "G1", "H1",
        "I1", "J1", "K1", "L1", "M1", "N1", "O1", "P1"
    };
    
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (strcmp(reg_name, register_names[i]) == 0) {
            return i;
        }
    }
    
    printf("Error: Invalid register name.\n");
    return 0;
}

unsigned short int ula(unsigned short int operating_a, unsigned short int operating_b, type_of_instruction operation) {
    switch(operation) {
        case ADD:
            return operating_a + operating_b;
        case SUB:
            return operating_a - operating_b;
        case MUL:
            return operating_a * operating_b;
        case DIV:
            if (operating_b == 0) {
                printf("Error: Division by zero.\n");
                return 0;
            }
            return operating_a / operating_b;
        default:
            printf("Error: Invalid operation.\n");
            return 0;
    }
}

unsigned short int verify_address(ram* memory_ram, 
                                char* address, 
                                unsigned short int num_positions) {
    if (!memory_ram || !address || address[0] != 'A') {
        printf("Error: Invalid address format or memory\n");
        exit(1);
    }

    unsigned short int pos = atoi(&address[1]);
    
    // Verifica se o endereço está dentro dos limites da memória
    if (pos + num_positions >= NUM_MEMORY) {
        printf("Error: Memory address out of bounds (address %d, size %d)\n", 
               pos, num_positions);
        exit(1);
    }

    // Verifica se há espaço suficiente na memória
    if (memory_ram->vector[pos] != '\0' && memory_ram->vector[pos] != ' ') {
        // Em vez de erro, apenas avisa que vai sobrescrever
        printf("Warning: Overwriting existing data at address %d\n", pos);
    }

    return pos;
}

void if_end(instruction_pipe* pipe) {
    char* instruction_copy = strdup(pipe->instruction);
    char* token = strtok(instruction_copy, " ");

    if (strcmp(token, "I_END") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        exit(1);
    }

    pipe->running_if = false;
    free(instruction_copy);
}

void else_i(cpu* cpu __attribute__((unused)), instruction_pipe* pipe) {
    char* instruction_copy = strdup(pipe->instruction);
    char* token = strtok(instruction_copy, " ");

    if (strcmp(token, "ELSE") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        exit(1);
    }

    if (pipe->has_if && !pipe->valid_if) {
        pipe->has_if = false;
    } else if (pipe->running_if) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        exit(1);
    } else if (!pipe->has_if) {
        printf("Error: Invalid instruction. No IF after ELSE.\n");
        free(instruction_copy);
        exit(1);
    }

    free(instruction_copy);
}

void else_end(instruction_pipe* pipe) {
    char* instruction_copy = strdup(pipe->instruction);
    char* token = strtok(instruction_copy, " ");

    if (strcmp(token, "ELS_END") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        exit(1);
    }

    pipe->has_if = false;
    free(instruction_copy);
}