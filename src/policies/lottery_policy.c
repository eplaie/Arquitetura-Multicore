// #include "policy.h"
// #include "../libs.h"

// static void init_lottery() {
//     static bool initialized = false;
//     if (!initialized) {
//         srand(time(NULL));
//         initialized = true;
//     }
// }

// static int count_total_tickets(ProcessManager* pm) {
//     int total = 0;
//     for (int i = 0; i < pm->ready_count; i++) {
//         total += pm->ready_queue[i]->tickets;
//     }
//     return total;
// }

// PCB* lottery_select_next(ProcessManager* pm) {
//     if (!pm || pm->ready_count == 0) return NULL;
    
//     // Conta total de tickets
//     int total_tickets = count_total_tickets(pm);
//     if (total_tickets == 0) return NULL;
    
//     // Sorteia um ticket
//     int winning_ticket = rand() % total_tickets;
    
//     // Encontra processo vencedor
//     int current_sum = 0;
//     for (int i = 0; i < pm->ready_count; i++) {
//         current_sum += pm->ready_queue[i]->tickets;
//         if (current_sum > winning_ticket) {
//             // Processo vencedor encontrado
//             PCB* winner = pm->ready_queue[i];
            
//             // Remove da fila
//             for (int j = i; j < pm->ready_count - 1; j++) {
//                 pm->ready_queue[j] = pm->ready_queue[j + 1];
//             }
//             pm->ready_count--;
            
//             return winner;
//         }
//     }
    
//     return NULL;
// }

// void lottery_on_quantum_expired(ProcessManager* pm __attribute__((unused)), 
//                               PCB* process __attribute__((unused))) {
//     if (process && pm) {
//         // Processo volta para fila de prontos mantendo seus tickets
//         process->state = READY;
//         pm->ready_queue[pm->ready_count++] = process;
//     }
// }

// void lottery_on_process_complete(ProcessManager* pm __attribute__((unused)), PCB* process) {
//     if (process) {
//         process->completion_time = process->cycles_executed;
//         process->state = FINISHED;
//         process->was_completed = true;
//     }
// }

// Policy* create_lottery_policy() {
//     Policy* policy = malloc(sizeof(Policy));
//     policy->type = POLICY_LOTTERY;
//     policy->name = "Lottery Scheduling";
//     policy->is_preemptive = true;
//     policy->select_next = lottery_select_next;
//     policy->on_quantum_expired = lottery_on_quantum_expired;
//     policy->on_process_complete = lottery_on_process_complete;
    
//     init_lottery();
//     return policy;
// }