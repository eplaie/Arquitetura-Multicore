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
PCB* find_process_by_binary_pid(ProcessManager* pm, const char* binary_pid) {
    for (int i = 0; i < pm->ready_count; i++) {
        char* current_binary = lookup_pid_in_tlb(pm->ready_queue[i]->pid);
        if (current_binary && strcmp(current_binary, binary_pid) == 0) {
            return pm->ready_queue[i];
        }
    }
    return NULL;
}
PCB* sjf_select_next(ProcessManager* pm) {
    if (!pm || pm->ready_count == 0) return NULL;

    int shortest_idx = 0;
    int shortest_size = pm->ready_queue[0]->program_size;

    printf("\n[SJF] Analisando processos usando PID binário:");
    for (int i = 0; i < pm->ready_count; i++) {
        int size = pm->ready_queue[i]->program_size;
        char* binary_pid = lookup_pid_in_tlb(pm->ready_queue[i]->pid);

        printf("\n - Processo PID(bin):%s: %d bytes",
               binary_pid ? binary_pid : "N/A", size);

        if (size < shortest_size) {
            shortest_size = size;
            shortest_idx = i;
        }
    }

    PCB* selected = pm->ready_queue[shortest_idx];
    char* selected_binary = lookup_pid_in_tlb(selected->pid);

    // Remove da fila
    for (int i = shortest_idx; i < pm->ready_count - 1; i++) {
        pm->ready_queue[i] = pm->ready_queue[i + 1];
    }
    pm->ready_count--;

    // Resetar quantum ao selecionar
    selected->quantum_remaining = pm->quantum_size;
    selected->state = RUNNING;  // Atualiza estado para RUNNING

    printf("\n[SJF] Selecionado processo PID(bin):%s (tamanho: %d bytes, quantum: %d)\n",
           selected_binary ? selected_binary : "N/A",
           shortest_size, selected->quantum_remaining);

    return selected;
}

void sjf_on_quantum_expired(ProcessManager* pm, PCB* process) {
    if (!pm || !process) return;

    char* binary_pid = lookup_pid_in_tlb(process->pid);
    printf("\n[SJF] Quantum expirado para processo PID(bin):%s", binary_pid);

    // Garantir que o processo volta para a fila de prontos
    process->quantum_remaining = pm->quantum_size;
    process->state = READY;

    // Verificar se já não está na fila antes de adicionar
    int already_in_queue = 0;
    for (int i = 0; i < pm->ready_count; i++) {
        if (pm->ready_queue[i] == process) {
            already_in_queue = 1;
            break;
        }
    }

    if (!already_in_queue) {
        pm->ready_queue[pm->ready_count++] = process;
        printf("\n[SJF] Processo PID(bin):%s retornado para fila de prontos", binary_pid);
    }
}

void sjf_on_process_complete(ProcessManager* pm, PCB* process) {
    if (!pm || !process) return;

    char* binary_pid = lookup_pid_in_tlb(process->pid);
    printf("\n[SJF] Processo PID(bin):%s completado",
           binary_pid ? binary_pid : "N/A");

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