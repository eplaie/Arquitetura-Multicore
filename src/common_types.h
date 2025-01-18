#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

// Definições globais
#define MAX_PROCESSES 5
#define NUM_CORES 4
#define NUM_REGISTERS 32
#define NUM_MEMORY 1024

#include "libs.h"

//instruções
typedef enum type_of_instruction {
    LOAD,
    STORE,
    ADD,
    SUB,
    MUL,
    DIV,
    IF,
    ELSE,
    LOOP,
    L_END,
    I_END,
    ELS_END,
    INVALID,
} type_of_instruction;


typedef struct PCB PCB;
typedef struct ProcessManager ProcessManager;
typedef struct core core;
typedef struct cpu cpu;
typedef struct pipeline pipeline;
typedef struct pipeline_stage pipeline_stage;
typedef struct peripherals peripherals;
typedef struct ram ram;

// Estruturas de instruções
typedef struct instruction_processor {
    char *instruction;
    type_of_instruction type;
    unsigned short int num_instruction;
    unsigned short int result;
    unsigned short int loop_start;
    unsigned short int loop_value;
    bool loop;
    bool has_if;
    bool valid_if;
    bool running_if;
} instruction_processor;

typedef struct instruction_pipe {
    char *instruction;
    unsigned short int num_instruction;
    type_of_instruction type;
    unsigned short int result;
    bool loop;
    unsigned short int loop_start;
    unsigned short int loop_value;
    bool has_if;
    bool valid_if;
    bool running_if;
    ram* mem_ram;
} instruction_pipe;

typedef struct program {
    char* name;          
    char* instructions;  
    int num_instructions;
} program;

#endif