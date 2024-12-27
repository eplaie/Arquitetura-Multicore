#ifndef INSTRUCTION_UTILS_H
#define INSTRUCTION_UTILS_H

#include "libs.h"
#include "cpu.h"
#include "ram.h"
#include "interpreter.h"

// Forward declarations para evitar dependência circular
struct cpu;
struct instruction_pipe;

// Funções auxiliares para instruções
unsigned short int get_register_index(const char* reg_name);
unsigned short int ula(unsigned short int operating_a, unsigned short int operating_b, type_of_instruction operation);
unsigned short int verify_address(ram* memory_ram, char* address, unsigned short int num_positions);

// Funções de controle de fluxo
void if_end(struct instruction_pipe* pipe);
void else_i(struct cpu* cpu, struct instruction_pipe* pipe);
void else_end(struct instruction_pipe* pipe);

#endif