#ifndef RAM_H
#define RAM_H

#define NUM_MEMORY 1024

#include "libs.h"
#include "pcb.h"

typedef struct ram {
    char *vector;
    size_t size;
    pthread_mutex_t mutex;
    bool initialized;
} ram;

struct cpu;

// Funções da RAM
ram* allocate_ram(size_t memory_size);
void load_program_on_ram(struct cpu* cpu, char* program_content, unsigned int base_address, PCB* pcb);
void write_ram(ram* memory_ram, unsigned short int address, const char* data);
bool verify_ram(ram* memory_ram, const char* context);

#endif