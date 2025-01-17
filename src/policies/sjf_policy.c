#include "policy.h"
#include "../libs.h"
#include "../os_display.h"

int get_program_length(PCB* process) {
    if (!process) return 0;
    
    // Calcular com base no tamanho em bytes do programa
    int program_size = process->memory_limit - process->base_address;
    
    // Considerando que cada instrução ocupa aproximadamente 25 bytes (em média)
    return (program_size / 25) + 1; // +1 para garantir no mínimo 1 instrução
}

PCB* sjf_select_next(ProcessManager* pm) {
    if (!pm || pm->ready_count == 0) return NULL;
    
    // Encontra processo com menor tamanho
    int shortest_idx = 0;
    int shortest_size = pm->ready_queue[0]->memory_limit - pm->ready_queue[0]->base_address;
    
    printf("\n[SJF] Analisando processos:");
    for (int i = 0; i < pm->ready_count; i++) {
        int size = pm->ready_queue[i]->memory_limit - pm->ready_queue[i]->base_address;
        printf("\n - P%d: %d bytes", pm->ready_queue[i]->pid, size);
        
        if (size < shortest_size) {
            shortest_size = size;
            shortest_idx = i;
        }
    }
    
    PCB* selected = pm->ready_queue[shortest_idx];
    
    // Remove da fila
    for (int i = shortest_idx; i < pm->ready_count - 1; i++) {
        pm->ready_queue[i] = pm->ready_queue[i + 1];
    }
    pm->ready_count--;
    
    // Resetar quantum ao selecionar
    selected->quantum_remaining = pm->quantum_size;
    
    printf("\n[SJF] Selecionado P%d (tamanho: %d bytes, quantum: %d)\n", 
           selected->pid, shortest_size, selected->quantum_remaining);
    
    return selected;
}


void sjf_on_quantum_expired(ProcessManager* pm, PCB* process) {
    if (!pm || !process) return;
    
    // Resetar quantum antes de retornar à fila
    process->quantum_remaining = pm->quantum_size;
    process->state = READY;
    pm->ready_queue[pm->ready_count++] = process;
}


void sjf_on_process_complete(ProcessManager* pm, PCB* process) {
    if (!pm || !process) return;
    
    process->state = FINISHED;
    process->was_completed = true;
    process->completion_time = process->cycles_executed;
}

Policy* create_sjf_policy(void) {
    Policy* policy = malloc(sizeof(Policy));
    if (!policy) return NULL;
    
    policy->type = POLICY_SJF;
    policy->name = "Shortest Job First";
    policy->is_preemptive = false;
    policy->select_next = sjf_select_next;
    policy->on_quantum_expired = sjf_on_quantum_expired;
    policy->on_process_complete = sjf_on_process_complete;
    
    return policy;
}