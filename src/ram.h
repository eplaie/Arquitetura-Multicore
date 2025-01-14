#ifndef RAM_H
#define RAM_H

#define NUM_MEMORY 1024

#include "libs.h"

// Definir a estrutura RAM antes de incluir cpu.h
typedef struct ram {
    char *vector;
} ram;

// Forward declaration da estrutura cpu
struct cpu;

// Funções da RAM
ram* allocate_ram(size_t memory_size);
void load_program_on_ram(struct cpu* cpu, char* program_content, unsigned int base_address);
void write_ram(ram* memory_ram, unsigned short int address, const char* data);

#endif