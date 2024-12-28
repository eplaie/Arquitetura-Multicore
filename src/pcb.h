#ifndef PCB_H
#define PCB_H

#include "common_types.h"

typedef enum {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    FINISHED
} process_state;

// Function prototype for state to string conversion
const char* state_to_string(process_state state);

struct PCB {
    int pid;
    process_state state; 
    int priority;
    unsigned short int PC;
    unsigned short int *registers;
    int core_id;
    int quantum;
    int base_address;
    int memory_limit;
    bool has_io;
    int execution_time;
};

struct ProcessManager {
    PCB **ready_queue;
    PCB **blocked_queue;
    int ready_count;
    int blocked_count;
    int next_pid;
    int quantum_size;
};

// Estrutura para gerenciar informações dos programas em memória
typedef struct {
    char* start;       // Início do programa na memória
    int length;        // Tamanho do programa
    int num_lines;     // Número de linhas
} ProgramInfo;

extern ProgramInfo programs[10];  // Array para armazenar informações dos programas
extern int num_programs;          // Contador de programas

// Function declarations
ProcessManager* init_process_manager(int quantum_size);
PCB* create_process(ProcessManager* pm);
void set_process_base_address(PCB* pcb, int base_address);
void schedule_process(ProcessManager* pm, cpu* cpu);
void schedule_next_process(cpu* cpu, int core_id);
void save_context(PCB* pcb, core* core);
void restore_context(PCB* pcb, core* core);
bool check_program_running(cpu* cpu);

#endif