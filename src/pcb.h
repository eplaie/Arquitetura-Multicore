#ifndef PCB_H
#define PCB_H

#include "libs.h"
#include "architecture.h"
#include "common_types.h"
#include "cpu.h" 
#include "policies/policy.h"

typedef enum {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    FINISHED
} process_state;

struct Policy;

typedef struct ProcessManager {
    PCB** ready_queue;
    PCB** blocked_queue;
    int ready_count;
    int blocked_count;
    int quantum_size;
    int current_time;
    pthread_mutex_t queue_mutex;
    pthread_mutex_t resource_mutex;
    pthread_cond_t resource_condition;
    struct Policy* policy;  // Mudando para usar a forward declaration
} ProcessManager;

typedef struct PCB {
    int pid;
    process_state state;
    int core_id;
    unsigned short int PC;
    unsigned short int* registers;
    int cycles_executed;
    int quantum_remaining;
    unsigned int base_address;
    unsigned int memory_limit;
    bool was_completed;
    bool using_io;
    int io_block_cycles;
    bool waiting_resource;
    char* resource_name;
    int total_instructions;
    int waiting_time;
    int turnaround_time;
    // Adicionando completion_time
    int completion_time;
    int start_time; 
    bool already_freed;
} PCB;

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