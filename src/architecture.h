#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#include "common_types.h"
#include "disc.h"
#include "peripherals.h"
#include "cache.h"
#include "pipeline.h"
#include "cpu.h"
#include "pcb.h"
#include "ram.h"  // Adicionar este include
#include "architecture_state.h"

#define DEFAULT_QUANTUM 4
#define MAX_CYCLES 5

// Funções principais - agora com memory_ram
void init_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state);

void free_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state);

// Funções auxiliares
void update_system_metrics(architecture_state* state);
void check_system_state(architecture_state* state, cpu* cpu);

#endif