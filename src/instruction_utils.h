#ifndef INSTRUCTION_UTILS_H
#define INSTRUCTION_UTILS_H

#include "common_types.h"
#include "cpu.h"
#include "ram.h"
#include "reader.h"

// Funções de operações básicas
unsigned short int get_register_index(const char* reg_name);
unsigned short int ula(unsigned short int operating_a, unsigned short int operating_b, type_of_instruction operation);
unsigned short int verify_address(ram* memory_ram, char* address, unsigned short int num_positions);

// Funções de instrução
void load(cpu* cpu, const char* instruction, unsigned short int index_core);
void store(cpu* cpu, ram* memory_ram, const char* instruction, unsigned short int index_core);
unsigned short int add(cpu* cpu, const char* instruction, unsigned short int index_core);
unsigned short int sub(cpu* cpu, const char* instruction, unsigned short int index_core);
unsigned short int mul(cpu* cpu, const char* instruction, unsigned short int index_core);
unsigned short int div_c(cpu* cpu, const char* instruction, unsigned short int index_core);

// Funções de controle de fluxo
void if_i(cpu* cpu, char* program, instruction_processor* pipe, unsigned short int index_core);  
void if_end(instruction_processor* pipe);
void else_i(cpu* cpu, char* program, instruction_processor* pipe, unsigned short int index_core);  
void else_end(instruction_processor* pipe);
void loop(cpu* cpu, instruction_processor* pipe, unsigned short int index_core);
void loop_end(cpu* cpu, instruction_processor* pipe, unsigned short int index_core);

// Funções auxiliares
void trim(char* str);     
void normalize_indentation(char* str);         
char* instruction_fetch(cpu* cpu, char* program, unsigned short int index_core);
type_of_instruction instruction_decode(const char* instruction);
void execute_instruction(cpu* cpu, ram* memory_ram, const char* instruction,
                       type_of_instruction type, int core_id,
                       instruction_processor* instr_processor, const char* program);

#endif