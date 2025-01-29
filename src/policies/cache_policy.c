#include "policy.h"
#include "../libs.h"
#include <time.h>
#include "../os_display.h"  


float calculate_process_cache_score(PCB* process) {
    int idx = process->base_address % CACHE_SIZE;
    
    // Aumentar o peso do hit ratio
    float hit_ratio_factor = cache[idx].hit_ratio * 0.7f;
    
    // Considerar a idade do bloco na cache
    float age_factor = 1.0f / (cache[idx].age + 1) * 0.2f;
    
    // Bônus para processos já carregados
    float presence_bonus = check_cache(process->base_address, NULL) ? 0.1f : 0.0f;
    
    return (hit_ratio_factor + age_factor + presence_bonus) * 100.0f;
}

char* get_program_content(PCB* pcb, ram* memory_ram) {
    if (!pcb || !memory_ram) return NULL;
    return memory_ram->vector + pcb->base_address;
}

PCB* cache_aware_select_next(ProcessManager* pm) {
    if (!pm || pm->ready_count == 0 || !pm->cpu || !pm->cpu->memory_ram) return NULL;
    
    int core_id = -1;
    for(int i = 0; i < NUM_CORES; i++) {
        if(pm->cpu->core[i].is_available) {
            core_id = i;
            break;
        }
    }
    if(core_id == -1) return NULL;

    PCB* selected = NULL;
    float best_score = -1;
    int best_idx = 0;

    // Usar o novo cálculo de score
    for(int i = 0; i < pm->ready_count; i++) {
        float score = calculate_process_cache_score(pm->ready_queue[i]);
        
        if(score > best_score) {
            best_score = score;
            best_idx = i;
            selected = pm->ready_queue[i];
        }
    }

    if(selected) {
        selected->quantum_remaining = pm->quantum_size;
        selected->state = RUNNING;
        selected->core_id = core_id;
        pm->cpu->core[core_id].is_available = false;
        pm->cpu->core[core_id].current_process = selected;

        update_cache(selected->base_address, 
                    get_program_content(selected, pm->cpu->memory_ram));

        for(int i = best_idx; i < pm->ready_count - 1; i++) {
            pm->ready_queue[i] = pm->ready_queue[i + 1];
        }
        pm->ready_count--;
        
        printf("\n[Cache] P%d atribuído ao core %d (score: %.2f)", 
               selected->pid, core_id, best_score);
    }

    return selected;
}

Policy* create_cache_aware_policy() {
    Policy* policy = malloc(sizeof(Policy));
    if(!policy) return NULL;
    
    printf("\n[Cache Policy] Creating policy"); 

    policy->type = POLICY_CACHE_AWARE;
    policy->name = "Cache-Aware Scheduling";
    policy->is_preemptive = true;
    policy->select_next = cache_aware_select_next;
    policy->on_quantum_expired = rr_on_quantum_expired;  
    policy->on_process_complete = rr_on_process_complete;
    
    init_cache();
    return policy;
}