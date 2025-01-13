#ifndef PIPELINE_H
#define PIPELINE_H

#include <pthread.h>
#include "common_types.h"

// Forward declarations
struct cpu;
struct ram;
struct architecture_state;
struct core;
struct PCB;

typedef struct pipeline_stage {
    const char* instruction;
    type_of_instruction type;
    int core_id;
    bool is_stalled;
    bool is_busy;
    pthread_mutex_t stage_mutex;
} pipeline_stage;

typedef struct pipeline {
    pipeline_stage IF;
    pipeline_stage ID;
    pipeline_stage EX;
    pipeline_stage MEM;
    pipeline_stage WB;
    int current_core;
    pthread_mutex_t pipeline_mutex;
} pipeline;

// Funções principais do pipeline
void init_pipeline(pipeline* p);
void reset_pipeline_stage(pipeline_stage* stage);
void execute_pipeline_cycle(struct architecture_state* state, struct cpu* cpu, 
                          struct ram* memory_ram, int core_id, int cycle_count);
void cleanup_pipeline(pipeline* p);

// Funções de estágio do pipeline
void execute_instruction(struct cpu* cpu, struct ram* memory_ram, const char* instruction,
                       type_of_instruction type, int core_id,
                       instruction_processor* instr_processor, const char* program);
void handle_memory_stage(type_of_instruction type);
void handle_writeback_stage(type_of_instruction type);
void update_process_state(struct architecture_state* state, struct PCB* current_process,
                         struct core* current_core, struct cpu* cpu, 
                         int core_id, int cycle_count);

// Funções auxiliares
type_of_instruction decode_instruction(const char* instruction);
const char* get_instruction_name(type_of_instruction type);

// Funções de sincronização
void lock_pipeline_stage(pipeline_stage* stage);
void unlock_pipeline_stage(pipeline_stage* stage);
void lock_pipeline(pipeline* p);
void unlock_pipeline(pipeline* p);

#endif