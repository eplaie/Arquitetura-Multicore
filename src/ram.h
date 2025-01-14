#ifndef RAM_H
#define RAM_H

#define NUM_MEMORY 1024

#include "libs.h"

typedef struct ram {
    char* vector;
} ram;


ram* allocate_ram(size_t memory_size);
void init_ram(ram* memory);
void print_ram(ram* memory);
void print_ram_segment(ram* memory, int start, int length);
void load_program_on_ram(ram* memory_ram, char* program_content);
// Adicione esta declaração no ram.h
void write_ram(ram* memory_ram, unsigned short int address, const char* data);
#endif 