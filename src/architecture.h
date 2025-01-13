#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#include "common_types.h"    
#include "disc.h"
#include "peripherals.h"
#include "cache.h"
#include "pipeline.h"
#include "cpu.h"
#include "pcb.h"
#include "architecture_state.h"
#include "instruction_utils.h"

#define DEFAULT_QUANTUM 4
#define MAX_CYCLES 100

// Funções principais
void init_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state);

void execute_pipeline_cycle(architecture_state* state, cpu* cpu, 
                          ram* memory_ram, int core_id, int cycle_count);

void free_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state);

// Funções auxiliares
void update_system_metrics(architecture_state* state);
void check_system_state(architecture_state* state, cpu* cpu);

#endif