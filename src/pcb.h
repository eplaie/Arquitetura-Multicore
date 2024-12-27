#ifndef PCB_H
#define PCB_H

#include "common_types.h"

typedef enum {
    READY,
    RUNNING,
    BLOCKED
} ProcessState;

struct PCB {
    int pid;
    ProcessState state;
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

// Function declarations
ProcessManager* init_process_manager(int quantum_size);
PCB* create_process(ProcessManager* pm);
void schedule_process(ProcessManager* pm, cpu* cpu);
void schedule_next_process(cpu* cpu, int core_id);
void save_context(PCB* pcb, core* core);
void restore_context(PCB* pcb, core* core);
bool check_program_running(cpu* cpu);

#endif