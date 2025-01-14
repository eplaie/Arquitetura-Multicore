#ifndef CPU_H
#define CPU_H

#include "libs.h"
#include "common_types.h"
#include "ram.h"  // Agora que ram.h tem a estrutura definida primeiro
#include "pcb.h"
#include "architecture_state.h"

#define NUM_CORES 4
#define NUM_REGISTERS 32

// Estrutura de core com suporte a threads
typedef struct core {
    unsigned short int *registers;
    unsigned short int PC;
    PCB* current_process;
    bool is_available;
    int quantum_remaining;
    pthread_t thread;
    pthread_mutex_t mutex;
    bool running;
    architecture_state* arch_state;  
} core;

// CPU com mutex global de recursos e RAM
typedef struct cpu {
    ram* memory_ram;        // Agora a estrutura ram já está definida
    core *core;
    ProcessManager* process_manager;
    pthread_mutex_t scheduler_mutex;
    pthread_mutex_t memory_mutex;     
    pthread_mutex_t resource_mutex;   
    pthread_cond_t resource_condition; 
} cpu;

// Argumentos para thread de core
typedef struct core_thread_args {
    cpu* cpu;
    ram* memory_ram;
    int core_id;
    architecture_state* state;
} core_thread_args;

// Funções de inicialização e cleanup - Atualizada para incluir RAM
void init_cpu(cpu* cpu, ram* memory_ram);
void cleanup_cpu_threads(cpu* cpu);

// Thread principal dos cores
void* core_execution_thread(void* arg);

// Gerenciamento de cores e recursos
void release_core(cpu* cpu, int core_id);
core* get_current_core(cpu* cpu);

// Funções de sincronização
void lock_core(core* c);
void unlock_core(core* c);
void lock_scheduler(cpu* cpu);
void unlock_scheduler(cpu* cpu);
void lock_memory(cpu* cpu);
void unlock_memory(cpu* cpu);
void lock_resources(cpu* cpu);
void unlock_resources(cpu* cpu);

#endif