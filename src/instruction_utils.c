#include "instruction_utils.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

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
    
    printf("Error: Invalid register name %s\n", reg_name);
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

unsigned short int verify_address(ram* memory_ram, char* address, unsigned short int num_positions) {
    printf("\n[Verify] Verificando endereço: %s (tamanho: %d)", address, num_positions);
    
    if (!memory_ram || !address || address[0] != 'A') {
        printf("\n[Verify] Erro: Parâmetros inválidos ou formato incorreto");
        return 0;
    }

    unsigned short int pos = atoi(&address[1]);
    printf("\n[Verify] Posição calculada: %d", pos);
    
    if (pos + num_positions >= NUM_MEMORY) {
        printf("\n[Verify] Erro: Posição fora dos limites");
        return 0;
    }

    return pos;
}

void load(cpu* cpu, const char* instruction, unsigned short int index_core) {
    char *instruction_copy, *token, *register_name;
    unsigned short int value, register_index;

    instruction_copy = strdup(instruction);

    token = strtok(instruction_copy, " "); 
    trim(token);

    if (strcmp(token, "LOAD") != 0) {
        printf("Error: Invalid instruction - LOAD\n");
        free(instruction_copy);
        return;
    }

    token = strtok(NULL, " ");
    trim(token);

    register_name = token;

    token = strtok(NULL, " ");
    trim(token);

    value = atoi(token);

    trim(register_name);
    register_index = get_register_index(register_name);

    cpu->core[index_core].registers[register_index] = value;

    free(instruction_copy);
}

void store(cpu* cpu, ram* memory_ram, const char* instruction, unsigned short int index_core) {
    printf("\n[Store] Iniciando instrução: '%s'", instruction);

    char *instruction_copy, *token, *register_name, *memory_address;
    char buffer[10]; 
    unsigned short int register_index, register_value;

    instruction_copy = strdup(instruction);
    if (!instruction_copy) {
        printf("\n[Store] Erro: Falha ao copiar instrução");
        return;
    }

    // Verifica STORE
    token = strtok(instruction_copy, " "); 
    if (strcmp(token, "STORE") != 0) {
        printf("\n[Store] Erro: Instrução inválida - STORE\n");
        free(instruction_copy);
        return;
    }

    // Pega registro fonte
    token = strtok(NULL, " ");
    register_name = token;
    printf("\n[Store] Registro fonte: %s", register_name);

    // Pega endereço destino
    token = strtok(NULL, " ");
    memory_address = token;
    printf("\n[Store] Endereço destino: %s", memory_address);

    // Obtém índice do registro
    register_index = get_register_index(register_name);
    printf("\n[Store] Índice do registro: %d", register_index);

    // Obtém valor do registro
    register_value = cpu->core[index_core].registers[register_index];
    printf("\n[Store] Valor do registro: %d", register_value);

    // Converte para string
    sprintf(buffer, "%d", register_value);  
    printf("\n[Store] Valor convertido: %s", buffer);

    // Verifica endereço
    unsigned short int address = verify_address(memory_ram, memory_address, strlen(buffer));
    printf("\n[Store] Endereço verificado: %d", address);
    
    // Escreve na RAM
    write_ram(memory_ram, address, buffer);
    printf("\n[Store] Escrita na RAM concluída");

    free(instruction_copy);
    printf("\n[Store] Instrução concluída\n");
}


unsigned short int add(cpu* cpu, const char* instruction, unsigned short int index_core) {
    char *instruction_copy, *token, *register_name1, *register_name2;
    unsigned short int value, register_index1, register_index2, result;

    instruction_copy = strdup(instruction);

    token = strtok(instruction_copy, " "); 

    if (strcmp(token, "ADD") != 0) {
        printf("Error: Invalid instruction - ADD\n");
        free(instruction_copy);
        return 0;
    }

    token = strtok(NULL, " ");
    register_name1 = token;

    token = strtok(NULL, " ");

    if (isdigit(token[0])) {
        value = atoi(token);

        register_index1 = get_register_index(register_name1);

        result = ula(cpu->core[index_core].registers[register_index1], value, ADD);
    } else {
        register_name2 = token;

        register_index1 = get_register_index(register_name1);
        register_index2 = get_register_index(register_name2);

        result = ula(cpu->core[index_core].registers[register_index1], 
                     cpu->core[index_core].registers[register_index2], 
                     ADD);
    }

    free(instruction_copy);
    return result; 
}

unsigned short int sub(cpu* cpu, const char* instruction, unsigned short int index_core) {
    char *instruction_copy, *token, *register_name1, *register_name2;
    unsigned short int value, register_index1, register_index2, result;

    instruction_copy = strdup(instruction);

    token = strtok(instruction_copy, " "); 
    trim(token);

    if (strcmp(token, "SUB") != 0) {
        printf("Error: Invalid instruction - SUB\n");
        exit(1);
    }

    token = strtok(NULL, " ");
    trim(token);

    register_name1 = token;

    token = strtok(NULL, " ");
    trim(token);

    if (isdigit(token[0])) {
        value = atoi(token);

        trim(register_name1);
        register_index1 = get_register_index(register_name1);

        result = ula(cpu->core[index_core].registers[register_index1], value, SUB);
    } else {
        register_name2 = token;

        trim(register_name1);
        trim(register_name2);
        register_index1 = get_register_index(register_name1);
        register_index2 = get_register_index(register_name2);

        result = ula(cpu->core[index_core].registers[register_index1], 
                     cpu->core[index_core].registers[register_index2], 
                     SUB);
    }

    return result; 
}

unsigned short int mul(cpu* cpu, const char* instruction, unsigned short int index_core) {
    char *instruction_copy, *token, *register_name1, *register_name2;
    unsigned short int value, register_index1, register_index2, result;

    instruction_copy = strdup(instruction);

    token = strtok(instruction_copy, " ");
    trim(token); 

    if (strcmp(token, "MUL") != 0) {
        printf("Error: Invalid instruction - MUL\n");
        exit(1);
    }

    token = strtok(NULL, " ");
    trim(token);

    register_name1 = token;

    token = strtok(NULL, " ");
    trim(token);

    if (isdigit(token[0])) {
        value = atoi(token);

        trim(register_name1);
        register_index1 = get_register_index(register_name1);

        result = ula(cpu->core[index_core].registers[register_index1], value, MUL);
    } else {
        register_name2 = token;

        trim(register_name1);
        trim(register_name2);
        register_index1 = get_register_index(register_name1);
        register_index2 = get_register_index(register_name2);

        result = ula(cpu->core[index_core].registers[register_index1], 
                     cpu->core[index_core].registers[register_index2], 
                     MUL);
    }

    return result; 
}

unsigned short int div_c(cpu* cpu, const char* instruction, unsigned short int index_core) {
    char *instruction_copy, *token, *register_name1, *register_name2;
    unsigned short int value, register_index1, register_index2, result;

    instruction_copy = strdup(instruction);

    token = strtok(instruction_copy, " "); 
    trim(token);

    if (strcmp(token, "DIV") != 0) {
        printf("Error: Invalid instruction - DIV\n");
        exit(1);
    }

    token = strtok(NULL, " ");
    trim(token);

    register_name1 = token;

    token = strtok(NULL, " ");
    trim(token);

    if (isdigit(token[0])) {
        value = atoi(token);

        trim(register_name1);
        register_index1 = get_register_index(register_name1);

        result = ula(cpu->core[index_core].registers[register_index1], value, DIV);
    } else {
        register_name2 = token;

        trim(register_name1);
        trim(register_name2);
        register_index1 = get_register_index(register_name1);
        register_index2 = get_register_index(register_name2);

        result = ula(cpu->core[index_core].registers[register_index1], 
                     cpu->core[index_core].registers[register_index2], 
                     DIV);
    }

    return result;  
}

void if_i(cpu* cpu, char* program, instruction_processor* instr_processor, unsigned short int index_core) {
    if (!program || !instr_processor || !cpu) return;

    char* program_copy = NULL;
    char* instruction_copy = NULL;
    char* token = NULL;
    char* operator = NULL;
    unsigned short int register_value = 0;
    unsigned short int operand_value = 0;

    program_copy = strdup(program);
    if (!program_copy) return;
    
    instruction_copy = strdup(instr_processor->instruction);
    if (!instruction_copy) {
        free(program_copy);
        return;
    }

    token = strtok(instruction_copy, " ");
    trim(token);

    instr_processor->has_if = true;

    if (strcmp(token, "IF") != 0) {
        printf("Error: Invalid instruction - IF\n");
        free(instruction_copy);
        free(program_copy);
        return;
    }

    token = strtok(NULL, " ");
    trim(token);

    register_value = get_register_index(token);
    register_value = cpu->core[index_core].registers[register_value];

    token = strtok(NULL, " ");
    trim(token);
    operator = token;

    token = strtok(NULL, " ");
    trim(token);

    if (isdigit(token[0])) {
        operand_value = atoi(token);
    } else {
        operand_value = get_register_index(token);
    }

    int result = 0;
    if (strcmp(operator, "==") == 0) {
        result = register_value == operand_value;
    } else if (strcmp(operator, "!=") == 0) {
        result = register_value != operand_value;
    } else if (strcmp(operator, "<=") == 0) {
        result = register_value <= operand_value;
    } else if (strcmp(operator, ">=") == 0) {
        result = register_value >= operand_value;
    } else if (strcmp(operator, ">") == 0) {
        result = register_value > operand_value;
    } else if (strcmp(operator, "<") == 0) {
        result = register_value < operand_value;
    } else {
        printf("Error: Invalid operator. Line %hd.\n", instr_processor->num_instruction + 1);
        free(instruction_copy);
        free(program_copy);
        return;
    }

    if (result == 0) {
        instr_processor->valid_if = false;
        while (1) {
            instr_processor->num_instruction++;
            char* temp_inst = instruction_fetch(cpu, program, index_core);
            if (!temp_inst) break;

            instr_processor->instruction = temp_inst;
            instr_processor->type = instruction_decode(instr_processor->instruction);

            free(instruction_copy);
            instruction_copy = strdup(instr_processor->instruction);
            if (!instruction_copy) break;

            token = strtok(instruction_copy, " "); 
            trim(token);

            if (strcmp(token, "I_END") == 0)
                break;
        }
    } else {
        instr_processor->valid_if = true;
        instr_processor->running_if = true;
    }

    free(instruction_copy);
    free(program_copy);
}

void if_end(instruction_processor* instr_processor) {
    if (!instr_processor || !instr_processor->instruction) return;

    char* instruction_copy = strdup(instr_processor->instruction);
    if (!instruction_copy) return;

    char* token = strtok(instruction_copy, " "); 
    trim(token);

    if (strcmp(token, "I_END") != 0) {
        printf("Error: Invalid instruction - I_END\n");
        free(instruction_copy);
        return;
    }

    instr_processor->running_if = false;
    free(instruction_copy);
}

void else_i(cpu* cpu, char* program, instruction_processor* instr_processor, unsigned short int index_core) {
    if (!program || !instr_processor || !cpu) return;
    
    char* program_copy = NULL;
    char* instruction_copy = NULL;
    char* token = NULL;

    program_copy = strdup(program);
    if (!program_copy) return;
    
    instruction_copy = strdup(instr_processor->instruction);
    if (!instruction_copy) {
        free(program_copy);
        return;
    }

    token = strtok(instruction_copy, " ");
    trim(token);

    if (strcmp(token, "ELSE") != 0) {
        printf("Error: Invalid instruction - ELSE\n");
        free(instruction_copy);
        free(program_copy);
        return;
    }

    if (instr_processor->has_if && !instr_processor->valid_if) {
        instr_processor->has_if = false;
    }
    else if (instr_processor->running_if) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        free(program_copy);
        return;
    }
    else if (!instr_processor->has_if) {
        printf("Error: Invalid instruction. No IF before ELSE. Line %hd.\n",
               instr_processor->num_instruction + 1);
        free(instruction_copy);
        free(program_copy);
        return;
    }
    else if (instr_processor->has_if && instr_processor->valid_if) {
        while (1) {
            instr_processor->num_instruction++;
            char* temp_inst = instruction_fetch(cpu, program, index_core);
            if (!temp_inst) break;

            instr_processor->instruction = temp_inst;
            instr_processor->type = instruction_decode(instr_processor->instruction);

            free(instruction_copy);
            instruction_copy = strdup(instr_processor->instruction);
            if (!instruction_copy) break;

            token = strtok(instruction_copy, " "); 
            if (strcmp(token, "ELS_END") == 0)
                break;
        }
    }

    free(instruction_copy);
    free(program_copy);
}

void else_end(instruction_processor* instr_processor) {
    if (!instr_processor || !instr_processor->instruction) return;

    char* instruction_copy = strdup(instr_processor->instruction);
    if (!instruction_copy) return;

    char* token = strtok(instruction_copy, " ");
    trim(token);

    if (strcmp(token, "ELS_END") != 0) {
        printf("Error: Invalid instruction - ELS_END\n");
        free(instruction_copy);
        return;
    }

    instr_processor->has_if = false;
    free(instruction_copy);
}

void loop(cpu* cpu, instruction_processor* instr_processor, unsigned short int index_core) {
    if (!cpu || !instr_processor) return;

    char* instruction_copy = NULL;
    char* token = NULL;
    char* register_name = NULL;
    unsigned short int value = 0;
    unsigned short int register_index = 0;

    instruction_copy = strdup(instr_processor->instruction);
    if (!instruction_copy) return;

    token = strtok(instruction_copy, " "); 
    trim(token);

    if (strcmp(token, "LOOP") != 0) {
        printf("Error: Invalid instruction - LOOP\n");
        free(instruction_copy);
        return;
    }

    if (!instr_processor->loop) {
        token = strtok(NULL, " ");
        trim(token);
        if (isdigit(token[0])) {
            value = atoi(token);
            if (value == 0) {
                printf("Error: Loop value can't be 0. Line %hd.\n",
                       instr_processor->num_instruction + 1);
                free(instruction_copy);
                return;
            }
        } else {
            register_name = token;
            register_index = get_register_index(register_name);
            value = cpu->core[index_core].registers[register_index];
            
            if (value == 0) {
                printf("Error: Loop value can't be 0. Line %hd.\n",
                       instr_processor->num_instruction + 1);
                free(instruction_copy);
                return;
            }
        }
        instr_processor->loop_value = value;
        instr_processor->loop_start = instr_processor->num_instruction;
        instr_processor->loop = true;
    }

    free(instruction_copy);
}

void loop_end(cpu* cpu, instruction_processor* instr_processor, unsigned short int index_core) {
    if (!cpu || !instr_processor) return;

    char* instruction_copy = strdup(instr_processor->instruction);
    if (!instruction_copy) return;

    char* token = strtok(instruction_copy, " "); 
    trim(token);

    if (strcmp(token, "L_END") != 0) {
        printf("Error: Invalid instruction - L_END\n");
        free(instruction_copy);
        return;
    }

    int decrease = instr_processor->num_instruction - instr_processor->loop_start + 1;
    instr_processor->loop_value--;

    if (instr_processor->loop_value == 0) {
        instr_processor->loop = false;
        instr_processor->loop_start = 0;
        instr_processor->num_instruction++;
    }
    else {
        for (int i = 0; i < decrease; i++) {
            cpu->core[index_core].PC--;
        }
        instr_processor->num_instruction = instr_processor->loop_start;
    }

    free(instruction_copy);
}

void remove_indentation(char* str) {
    if (!str) return;
    
    // Remove espaços e tabs no início
    char* start = str;
    while (isspace(*start)) start++;
    
    // Se houver indentação, move o resto da string
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}


void trim(char* str) {
    if (!str) return;
    
    // Preserva indentação
    int spaces = 0;
    while (str[spaces] == ' ') spaces++;
    
    // Remove espaços extras no início
    if (spaces > 2) {
        memmove(str, str + (spaces - 2), strlen(str) - spaces + 3);
    }
    
    // Remove espaços no final
    int end = strlen(str) - 1;
    while (end > 0 && isspace(str[end])) {
        str[end] = '\0';
        end--;
    }
}

void normalize_indentation(char* str) {
    if (!str) return;
    
    // Contar espaços iniciais
    int spaces = 0;
    char* p = str;
    while (*p == ' ') {
        spaces++;
        p++;
    }

    // Se tem indentação (2 espaços = 1 nível)
    if (spaces >= 2) {
        // Preservar apenas 2 espaços para cada nível
        int levels = spaces / 2;
        int preserved = levels * 2;
        memmove(str + preserved, str + spaces, strlen(str + spaces) + 1);
        
        // Preencher com espaços preservados
        for (int i = 0; i < preserved; i++) {
            str[i] = ' ';
        }
    }
}

char* instruction_fetch(cpu* cpu, char* program, unsigned short int index_core) {
    if (!program || !cpu) return NULL;

    
    char* current_instruction = NULL;
    
    
    current_instruction = get_line_of_program(
        program + cpu->core[index_core].PC, 
        0  
    );

    return current_instruction;
}


type_of_instruction instruction_decode(const char* instruction) {
    if (!instruction) {
        printf("\n[Decode] Erro: Instrução nula");
        return INVALID;
    }

    char* cleaned = strdup(instruction);
    remove_indentation(cleaned);

    printf("\n[Decode] Decodificando: %s", instruction);

    // Remove leading whitespaces
    while (*instruction && isspace((unsigned char)*instruction)) {
        instruction++;
    }

    if (strncmp(instruction, "LOAD", 4) == 0) return LOAD;
    if (strncmp(instruction, "STORE", 5) == 0) return STORE;
    if (strncmp(instruction, "ADD", 3) == 0) return ADD;
    if (strncmp(instruction, "SUB", 3) == 0) return SUB;
    if (strncmp(instruction, "MUL", 3) == 0) return MUL;
    if (strncmp(instruction, "DIV", 3) == 0) return DIV;
    if (strncmp(instruction, "LOOP", 4) == 0) return LOOP;
    if (strncmp(instruction, "L_END", 5) == 0) return L_END;
    if (strncmp(instruction, "IF", 2) == 0) return IF;
    if (strncmp(instruction, "I_END", 5) == 0) return I_END;
    if (strncmp(instruction, "ELSE", 4) == 0) return ELSE;
    if (strncmp(instruction, "ELS_END", 7) == 0) return ELS_END;

    printf("\n[Decode] Erro: Instrução inválida: %s", instruction);
    return INVALID;
}

// Em instruction_utils.c
void execute_instruction(cpu* cpu, ram* memory_ram, const char* instruction,
                       type_of_instruction type, int core_id,
                       instruction_processor* instr_processor, const char* program) {
    if (!cpu || !memory_ram || !instruction || !instr_processor) {
        printf("\nERRO: Parâmetros inválidos em execute_instruction");
        return;
    }

    char* normalized = strdup(instruction);
    normalize_indentation(normalized);  

    switch (type) {
        case LOAD:
            load(cpu, instruction, core_id);
            break;
        case STORE:
            store(cpu, memory_ram, instruction, core_id);
            break;
        case ADD:
            add(cpu, instruction, core_id);
            break;
        case SUB:
            sub(cpu, instruction, core_id);
            break;
        case MUL:
            mul(cpu, instruction, core_id);
            break;
        case DIV:
            div_c(cpu, instruction, core_id);
            break;
        case LOOP:
            loop(cpu, instr_processor, core_id);
            break;
        case IF:
            if_i(cpu, (char*)program, instr_processor, core_id);
            break;
        case ELSE:
            else_i(cpu, (char*)program, instr_processor, core_id);
            break;
        case L_END:
            loop_end(cpu, instr_processor, core_id);
            break;
        case I_END:
            if_end(instr_processor);
            break;
        case ELS_END:
            else_end(instr_processor);
            break;
        default:
            printf("\nERRO: Instrução inválida");
            break;
    }
}

