#include "policy.h"
#include "../libs.h"
#include <time.h>
#include "../os_display.h"  

// Inicializa o gerador de números aleatórios
static void init_lottery() {
    static bool initialized = false;
    if (!initialized) {
        srand(time(NULL));
        initialized = true;
    }
}

// Conta total de tickets disponíveis
static int count_total_tickets(ProcessManager* pm) {
    int total = 0;
    for (int i = 0; i < pm->ready_count; i++) {
        // Cada processo começa com tickets = pid + 1 para garantir pelo menos 1 ticket
        total += (pm->ready_queue[i]->pid + 1);
    }
    return total;
}

        PCB* lottery_select_next(ProcessManager* pm) {
    if (!pm || pm->ready_count == 0) return NULL;
    
    int total_tickets = count_total_tickets(pm);
    if (total_tickets == 0) return NULL;
    
    int winning_ticket = rand() % total_tickets;
    
    // Encontra processo vencedor
    int current_sum = 0;
    for (int i = 0; i < pm->ready_count; i++) {
        current_sum += (pm->ready_queue[i]->pid + 1);  // tickets do processo
        if (current_sum > winning_ticket) {
            PCB* winner = pm->ready_queue[i];
                winner->lottery_selections++;
            // Remove da fila
            for (int j = i; j < pm->ready_count - 1; j++) {
                pm->ready_queue[j] = pm->ready_queue[j + 1];
            }
            pm->ready_count--;
            
            // Resetar quantum ao selecionar
            winner->quantum_remaining = pm->quantum_size;
            
            printf("%s[Lottery] P%d ganhou o sorteio (ticket: %d, quantum: %d)%s\n", 
                   COLOR_YELLOW, winner->pid, winning_ticket, winner->quantum_remaining, 
                   COLOR_RESET);
            
            return winner;
        }
    }
    
    return NULL;
}

void lottery_on_quantum_expired(ProcessManager* pm, PCB* process) {
    if (!pm || !process) return;
    
    // Resetar quantum antes de retornar à fila
    process->quantum_remaining = pm->quantum_size;
    process->state = READY;
    pm->ready_queue[pm->ready_count++] = process;
}


void lottery_on_process_complete(ProcessManager* pm, PCB* process) {
    if (!pm || !process) return;
    
    process->state = FINISHED;
    process->was_completed = true;
    process->completion_time = process->cycles_executed;
}

Policy* create_lottery_policy() {
    Policy* policy = malloc(sizeof(Policy));
    if (!policy) return NULL;
    
    policy->type = POLICY_LOTTERY;
    policy->name = "Lottery Scheduling";
    policy->is_preemptive = true;
    policy->select_next = lottery_select_next;
    policy->on_quantum_expired = lottery_on_quantum_expired;
    policy->on_process_complete = lottery_on_process_complete;
    
    // Inicializa o gerador de números aleatórios
    init_lottery();
    
    return policy;
}