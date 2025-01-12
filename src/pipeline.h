#ifndef PIPELINE_H
#define PIPELINE_H

#include "common_types.h"
#include "cpu.h"
#include "ram.h"
#include "instruction_utils.h"

typedef struct pipeline_stage {
    char* instruction;
    type_of_instruction type;
    int core_id;
    bool is_stalled;
    bool is_busy;
} pipeline_stage;

typedef struct pipeline {
    pipeline_stage IF;
    pipeline_stage ID;
    pipeline_stage EX;
    pipeline_stage MEM;
    pipeline_stage WB;
    int current_core;
} pipeline;

// Funções do pipeline
void init_pipeline(pipeline* p);
void reset_pipeline_stage(pipeline_stage* stage);
// char* instruction_fetch(cpu* cpu, ram* memory, int core_id);
type_of_instruction instruction_decode(cpu* cpu, char* instruction, unsigned short int num_instruction);  // Corrigido
void execute(cpu* cpu, instruction_pipe* p, int core_id);
// void memory_access(cpu* cpu, ram* memory_ram, type_of_instruction type, char* instruction, int core_id);
// void write_back(cpu* cpu, type_of_instruction type, char* instruction, unsigned short int result, int core_id);
// void check_pipeline_preemption(pipeline* p, cpu* cpu);
// void switch_pipeline_context(pipeline* p, cpu* cpu, int new_core_id);

#endif