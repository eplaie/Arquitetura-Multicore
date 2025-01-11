#include "policy.h"
#include "../libs.h"

PCB* sjf_select_next(ProcessManager* pm) {
    if (!pm || pm->ready_count == 0) return NULL;
    
    // Encontra processo com menor tempo estimado
    int shortest_idx = 0;
    PCB* shortest = pm->ready_queue[0];
    
    for (int i = 1; i < pm->ready_count; i++) {
        if (pm->ready_queue[i]->estimated_execution_time < shortest->estimated_execution_time) {
            shortest = pm->ready_queue[i];
            shortest_idx = i;
        }
    }
    
    // Remove da fila
    for (int i = shortest_idx; i < pm->ready_count - 1; i++) {
        pm->ready_queue[i] = pm->ready_queue[i + 1];
    }
    pm->ready_count--;
    
    return shortest;
}

// void sjf_on_quantum_expired(ProcessManager* pm __attribute__((unused)), 
//                           PCB* process __attribute__((unused))) {
//     // SJF não preemptivo ignora quantum
// }

// void sjf_on_quantum_expired(ProcessManager* pm __attribute__((unused)), 
//                           PCB* process __attribute__((unused))) {
//     if (process) {
//         process->completion_time = process->cycles_executed;
//         process->state = FINISHED;
//         process->was_completed = true;
//     }
// }

Policy* create_sjf_policy() {
    Policy* policy = malloc(sizeof(Policy));
    policy->type = POLICY_SJF;
    policy->name = "Shortest Job First";
    policy->is_preemptive = false;
    policy->select_next = sjf_select_next;
    // policy->on_quantum_expired = sjf_on_quantum_expired;
    // policy->on_process_complete = sjf_on_process_complete;
    return policy;
}