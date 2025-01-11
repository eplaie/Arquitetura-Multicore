#ifndef POLICY_H
#define POLICY_H

#include "../pcb.h"

// Tipos de políticas disponíveis
typedef enum {
    POLICY_RR,      
    POLICY_SJF,     
    POLICY_MULTI,   
    POLICY_LOTTERY  
} PolicyType;

// Interface da política de escalonamento
typedef struct Policy {
    PolicyType type;
    const char* name;
    bool is_preemptive;
    PCB* (*select_next)(ProcessManager* pm);
    void (*on_quantum_expired)(ProcessManager* pm, PCB* process);
    void (*on_process_complete)(ProcessManager* pm, PCB* process);
} Policy;

// Funções de criação de políticas
Policy* create_rr_policy(void);
Policy* create_sjf_policy(void);
Policy* create_multi_policy(void);
Policy* create_lottery_policy(void);

#endif