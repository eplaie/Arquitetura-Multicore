#ifndef PCB_H
#define PCB_H

#include "libs.h"
#include "architecture.h"
#include "common_types.h"
#include "cpu.h" 

typedef enum {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    FINISHED
} process_state;

typedef struct PCB {
    // Identificação
    int pid;
    process_state state;
    int core_id;
    
    // Controle de execução
    int PC;
    int cycles_executed;
    int quantum_remaining;
    int base_address;
    int memory_limit;
    bool was_completed;
    
    // Registradores
    unsigned short int* registers;
    
    // Recursos e I/O
    bool using_io;
    int io_block_cycles;
    bool waiting_resource;
    char* resource_name;
    
    // Estatísticas
    int total_instructions;
    int waiting_time;
    int turnaround_time;
} PCB;

typedef struct ProcessManager {
    PCB** ready_queue;
    PCB** blocked_queue;
    int ready_count;
    int blocked_count;
    int quantum_size;
    int next_pid;
    pthread_mutex_t queue_mutex;
    pthread_mutex_t resource_mutex;
    pthread_cond_t resource_condition;
} ProcessManager;

// Funções do PCB
PCB* create_pcb(void);
void save_context(PCB* pcb, core* current_core);
void restore_context(PCB* pcb, core* current_core);
void free_pcb(PCB* pcb);

// Funções auxiliares
const char* state_to_string(process_state state);

// Variáveis globais
extern PCB* all_processes[];
extern int total_processes;

// Funções do ProcessManager
ProcessManager* init_process_manager(int quantum_size);
void schedule_next_process(cpu* cpu, int core_id);
void check_blocked_processes(cpu* cpu);
int count_ready_processes(ProcessManager* pm);
void lock_process_manager(ProcessManager* pm);
void unlock_process_manager(ProcessManager* pm);

#endif