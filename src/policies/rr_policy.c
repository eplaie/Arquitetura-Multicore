// #include "policy.h"
// #include "../libs.h"

// PCB* rr_select_next(ProcessManager* pm __attribute__((unused))) {
//     //  Round Robin 
//     return NULL; 
// }

// void rr_on_quantum_expired(ProcessManager* pm __attribute__((unused)), PCB* process __attribute__((unused))) {
//     // vazio
// }

// void rr_on_process_complete(ProcessManager* pm __attribute__((unused)), PCB* process __attribute__((unused))) {
// }

// Policy* create_rr_policy() {
//     Policy* policy = malloc(sizeof(Policy));
//     policy->type = POLICY_RR;
//     policy->name = "Round Robin";
//     policy->is_preemptive = true;
//     policy->select_next = rr_select_next;
//     policy->on_quantum_expired = rr_on_quantum_expired;
//     policy->on_process_complete = rr_on_process_complete;
//     return policy;
// }