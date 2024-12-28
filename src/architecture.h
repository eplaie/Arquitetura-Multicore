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

void init_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, peripherals* peripherals, architecture_state* state);
void load_program_on_ram(ram* memory, char* program, int base_address);
void check_instructions_on_ram(ram* memory_ram);
void init_pipeline_multicore(architecture_state* state, cpu* cpu, ram* memory_ram);
void execute_pipeline_cycle(architecture_state* state, cpu* cpu, ram* memory_ram, int core_id);
void free_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, peripherals* peripherals, architecture_state* state);

#endif