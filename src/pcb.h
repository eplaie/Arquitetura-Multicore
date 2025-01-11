#ifndef PCB_H
#define PCB_H

#include "common_types.h"
#include "policies/policy.h"

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
    int recovery_count;    // Contador de recuperações
    bool had_violations;   // Flag para indicar violações de memória

    bool using_resource;    // Indica se está usando algum recurso
    int resource_address;   // Endereço do recurso sendo usado
    int blocked_by_pid;     
    bool using_io;
    int io_block_cycles; 

    int arrival_time;             // Tempo de chegada
    int start_time;              // Tempo do primeiro uso da CPU
    int completion_time;         // Tempo de término
    int estimated_execution_time; // Para SJF

    int queue_level;            // Para Multilevel
    int tickets;                // Para Lottery
} PCB;

// Gerenciador de Processos
struct ProcessManager {
    PCB **ready_queue;
    PCB **blocked_queue;
    int ready_count;
    int blocked_count;
    int next_pid;
    int quantum_size;

    struct Policy* current_policy;
    
    // Métricas para comparação
    int total_context_switches;
    int total_preemptions;
    float avg_waiting_time;
    float avg_turnaround_time;
    float throughput;

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
void remove_from_ready_queue(ProcessManager* pm, int idx);
void assign_to_core(cpu* cpu, PCB* process, int core_id);
int count_ready_processes(ProcessManager* pm);

#endif