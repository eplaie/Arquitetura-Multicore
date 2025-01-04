#ifndef PCB_H
#define PCB_H

#include "common_types.h"

// Enumeração de estados do processo
typedef enum {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    FINISHED
} process_state;

// Function prototype for state to string conversion
const char* state_to_string(process_state state);

// Definição do PCB
typedef struct PCB {
    int pid;
    process_state state;
    int priority;
    int PC;
    int base_address;
    int memory_limit;
    int core_id;
    int quantum;
    bool has_io;
    int blocked_time;
    int total_instructions;
    int cycles_executed;
    bool was_completed;
    unsigned short int* registers;
    // Novos campos para tratamento de erro
    int recovery_count;    // Contador de recuperações
    bool had_violations;   // Flag para indicar violações de memória

    bool using_resource;    // Indica se está usando algum recurso
    int resource_address;   // Endereço do recurso sendo usado
    int blocked_by_pid;     // PID do processo que está bloqueando este
} PCB;

// Gerenciador de Processos
struct ProcessManager {
    PCB **ready_queue;
    PCB **blocked_queue;
    int ready_count;
    int blocked_count;
    int next_pid;
    int quantum_size;
};

// Estrutura para gerenciar programas na memória
typedef struct {
    char* start;
    int length;
    int num_lines;
} ProgramInfo;

// Variáveis globais
extern PCB* all_processes[MAX_PROCESSES];
extern int total_processes;
extern ProgramInfo programs[10];
extern int num_programs;

// Function declarations
ProcessManager* init_process_manager(int quantum_size);
PCB* create_process(ProcessManager* pm);
void set_process_base_address(PCB* pcb, int base_address);
void schedule_process(ProcessManager* pm, cpu* cpu);
void schedule_next_process(cpu* cpu, int core_id);
void save_context(PCB* pcb, core* core);
void restore_context(PCB* pcb, core* core);
bool check_program_running(cpu* cpu);
void check_blocked_processes(cpu* cpu);
bool check_resource_available(cpu* cpu, int address, int requesting_pid);

#endif