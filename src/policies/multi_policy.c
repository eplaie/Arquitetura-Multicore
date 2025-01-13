// #include "policy.h"
// #include "../libs.h"

// #define NUM_LEVELS 3
// #define QUANTUM_BASE 4

// typedef struct {
//     PCB* queues[NUM_LEVELS][MAX_PROCESSES];
//     int count[NUM_LEVELS];
//     int quantum[NUM_LEVELS];
// } MultilevelQueues;

// static MultilevelQueues ml_queues = {
//     .count = {0, 0, 0},
//     .quantum = {QUANTUM_BASE, QUANTUM_BASE*2, QUANTUM_BASE*4}
// };

// PCB* multi_select_next(ProcessManager* pm __attribute__((unused))) {
//     if (!pm) return NULL;
    
//     // Verifica cada nível, começando pelo mais prioritário
//     for (int level = 0; level < NUM_LEVELS; level++) {
//         if (ml_queues.count[level] > 0) {
//             // Pega primeiro processo do nível atual
//             PCB* selected = ml_queues.queues[level][0];
            
            
//             for (int i = 0; i < ml_queues.count[level] - 1; i++) {
//                 ml_queues.queues[level][i] = ml_queues.queues[level][i + 1];
//             }
//             ml_queues.count[level]--;
            
//             selected->quantum = ml_queues.quantum[level];
//             return selected;
//         }
//     }
    
//     return NULL;
// }

// void multi_on_quantum_expired(ProcessManager* pm __attribute__((unused)), 
//                             PCB* process) {
//     if (!process) return;
    
//     // Move para o próximo nível (menor prioridade)
//     int next_level = (process->queue_level < NUM_LEVELS - 1) ? 
//                      process->queue_level + 1 : process->queue_level;
    
//     // Adiciona no final da fila do próximo nível
//     int idx = ml_queues.count[next_level];
//     ml_queues.queues[next_level][idx] = process;
//     ml_queues.count[next_level]++;
    
//     process->queue_level = next_level;
//     process->state = READY;
// }

// void multi_on_process_complete(ProcessManager* pm __attribute__((unused)), PCB* process) {
//     if (process) {
//         process->completion_time = process->cycles_executed;
//         process->state = FINISHED;
//         process->was_completed = true;
//     }
// }

// Policy* create_multi_policy() {
//     Policy* policy = malloc(sizeof(Policy));
//     policy->type = POLICY_MULTI;
//     policy->name = "Multilevel Queue";
//     policy->is_preemptive = true;
//     policy->select_next = multi_select_next;
//     policy->on_quantum_expired = multi_on_quantum_expired;
//     policy->on_process_complete = multi_on_process_complete;
//     return policy;
// }