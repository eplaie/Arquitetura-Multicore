#include "policy.h"
#include "../libs.h"
#include "../os_display.h"

PCB* rr_select_next(ProcessManager* pm) {
    if (!pm || pm->ready_count == 0) return NULL;
    
    PCB* next = pm->ready_queue[0];
    
    // Remove da fila
    for (int i = 0; i < pm->ready_count - 1; i++) {
        pm->ready_queue[i] = pm->ready_queue[i + 1];
    }
    pm->ready_count--;
    
    // Garantir que o quantum seja resetado
    next->quantum_remaining = pm->quantum_size;
    
    printf("%s[RR] Executando P%d (quantum: %d)%s\n", 
           COLOR_BLUE, next->pid, pm->quantum_size, COLOR_RESET);
    
    return next;
}

void rr_on_quantum_expired(ProcessManager* pm, PCB* process) {
    if (!pm || !process) return;
    
    // Resetar o quantum antes de colocar na fila
    process->quantum_remaining = pm->quantum_size;
    process->state = READY;
    pm->ready_queue[pm->ready_count++] = process;
    
    // printf("\n[Debug] RR: Quantum expirado para processo %d", process->pid);
    // printf("\n[Debug] RR: Processo %d movido para o fim da fila", process->pid);
    //printf("\n - Fila atual: %d processos", pm->ready_count);
}

void rr_on_process_complete(ProcessManager* pm, PCB* process) {
    if (!pm || !process) return;
    
    // Marca processo como finalizado
    process->state = FINISHED;
    process->was_completed = true;
    process->completion_time = process->cycles_executed;
}

Policy* create_rr_policy() {
    Policy* policy = malloc(sizeof(Policy));
    if (!policy) return NULL;
    
    policy->type = POLICY_RR;
    policy->name = "Round Robin";
    policy->is_preemptive = true;
    policy->select_next = rr_select_next;
    policy->on_quantum_expired = rr_on_quantum_expired;
    policy->on_process_complete = rr_on_process_complete;
    
    return policy;
}