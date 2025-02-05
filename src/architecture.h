#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#include "common_types.h"
#include "disc.h"
#include "peripherals.h"
#include "cache.h"
#include "pipeline.h"
#include "cpu.h"
#include "pcb.h"
#include "ram.h"  
#include "architecture_state.h"

#define DEFAULT_QUANTUM 4
#define MAX_CYCLES 7

// Funções principais 
void init_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state);

void free_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state,
                      int cycle_count);

static inline void init_mutex(pthread_mutex_t* mutex) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

// Funções auxiliares
void update_system_metrics(architecture_state* state);
void check_system_state(architecture_state* state, cpu* cpu);

#endif