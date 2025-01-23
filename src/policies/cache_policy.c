#include "policy.h"
#include "../libs.h"
#include <time.h>
#include "../os_display.h"  


PCB* cache_aware_select_next(ProcessManager* pm) {
    printf("\n[Cache] Iniciando seleção de processo");
    
    if (!pm || pm->ready_count == 0 || !pm->cpu || !pm->cpu->memory_ram) return NULL;

    int core_id = -1;
    for(int i = 0; i < NUM_CORES; i++) {
        if(pm->cpu->core[i].is_available) {
            core_id = i;
            printf("\n[Cache] Core %d disponível", i);
            break;
        }
    }
    
    if(core_id == -1) return NULL;

    // Selecionar processo baseado em similaridade
    PCB* selected = NULL;
    float best_score = -1;
    int best_idx = 0;

    for(int i = 0; i < pm->ready_count && pm->ready_queue[i]; i++) {
        float score = 0.0f;
        
        // Verificar instruções similares em cache
        for(int j = 0; j < NUM_CORES; j++) {
            if(!pm->cpu->core[j].is_available && 
               pm->cpu->core[j].current_process) {
                score += 0.3f;  // Bônus por similaridade
            }
        }
        
        // Adicionar bônus de cache se processo já usado antes
        if(check_cache(pm->ready_queue[i]->base_address)) {
            score += 0.7f;
        }

        printf("\n[Cache] P%d score: %.2f", pm->ready_queue[i]->pid, score);
        
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

        // Atualizar cache
        update_cache(selected->base_address, 
                    get_program_content(selected, pm->cpu->memory_ram));

        // Remover da fila
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
    policy->on_quantum_expired = rr_on_quantum_expired;  // Reuse RR quantum handling
    policy->on_process_complete = rr_on_process_complete;
    
    init_cache();
    return policy;
}