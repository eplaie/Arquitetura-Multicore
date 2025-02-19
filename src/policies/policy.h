#ifndef POLICY_H
#define POLICY_H

#include "../pcb.h"

#define MAX_GROUPS 10

typedef struct {
    PCB* processes[MAX_PROCESSES];  // MAX_PROCESSES já definido em common_types.h
    int count;
    float similarity_score;
} ProcessGroup;

// Tipos de políticas disponíveis
typedef enum {
    POLICY_RR,      
    POLICY_SJF,       
    POLICY_LOTTERY, 
    POLICY_CACHE_AWARE
} PolicyType;

// Interface da política de escalonamento
typedef struct Policy {
    PolicyType type;
    const char* name;
    bool is_preemptive;
    PCB* (*select_next)(struct ProcessManager* pm);
    void (*on_quantum_expired)(struct ProcessManager* pm, PCB* process);
    void (*on_process_complete)(struct ProcessManager* pm, PCB* process);
} Policy;

typedef struct {
    char* binary_pid;
    PCB* process;
} BinaryPIDMapping;



// Funções de criação de políticas
Policy* create_rr_policy(void);
Policy* create_sjf_policy(void);
Policy* create_lottery_policy(void);
Policy* create_cache_aware_policy(void);
int get_program_length(PCB* process);
void rr_on_quantum_expired(ProcessManager* pm, PCB* process);
void rr_on_process_complete(ProcessManager* pm, PCB* process);
float calculate_process_cache_score(PCB* process, ProcessGroup* groups, int current_group);

// Funções para cache-aware policy
void group_similar_processes(ProcessManager* pm, PCB** ready_queue, int ready_count, ProcessGroup* groups, int* group_count);

float calculate_instruction_similarity(const char* instr1, const char* instr2);

bool is_similar_operation(const char* type1, const char* type2);
const char* get_next_instruction(const char* current, int offset);

#endif