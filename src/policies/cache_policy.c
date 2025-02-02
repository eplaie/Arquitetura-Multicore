#include "policy.h"
#include "../ram.h"
#include "../libs.h"
#include "../os_display.h"
#include <string.h>

static int last_group_id = -1;

#define MAX_GROUPS 10
#define SIMILARITY_THRESHOLD 0.7

float calculate_instruction_similarity(const char* instr1, const char* instr2) {
    if (!instr1 || !instr2) return 0.0f;
    
    // Buffer para armazenar até 5 instruções próximas
    char instr1_buffer[5][50];
    char instr2_buffer[5][50];
    int count = 0;
    float total_similarity = 0.0f;
    
    // Pegar próximas instruções
    for(int i = 0; i < 5; i++) {
        // Pegar próxima instrução do primeiro processo
        const char* next1 = get_next_instruction(instr1, i);
        if(next1) {
            strncpy(instr1_buffer[count], next1, 49);
            
            // Pegar próxima instrução do segundo processo
            const char* next2 = get_next_instruction(instr2, i);
            if(next2) {
                strncpy(instr2_buffer[count], next2, 49);
                count++;
            }
        }
    }
    
    // Comparar cada par de instruções
    for(int i = 0; i < count; i++) {
        // Extrair tipo da instrução
        char type1[10], type2[10];
        sscanf(instr1_buffer[i], "%s", type1);
        sscanf(instr2_buffer[i], "%s", type2);
        
        // Instruções idênticas
        if (strcmp(instr1_buffer[i], instr2_buffer[i]) == 0) {
            total_similarity += 1.0f;
        }
        // Mesmo tipo de instrução
        else if (strcmp(type1, type2) == 0) {
            total_similarity += 0.8f;
        }
        // Operações similares (LOAD/STORE ou operações aritméticas)
        else if (is_similar_operation(type1, type2)) {
            total_similarity += 0.5f;
        }
    }
    
    return count > 0 ? total_similarity / count : 0.0f;
}

char* get_program_content(PCB* pcb, ram* memory_ram) {
    if (!pcb || !memory_ram || !memory_ram->vector) return NULL;

    // Verificar se o endereço base é válido
    if (pcb->base_address >= memory_ram->size) return NULL;

    return memory_ram->vector + pcb->base_address;
}

void group_similar_processes(ProcessManager* pm, PCB** ready_queue, int ready_count, ProcessGroup* groups, int* group_count) {
    printf("\n[Similarity Analysis] Iniciando agrupamento de processos");
    *group_count = 0;
    
    for (int i = 0; i < ready_count; i++) {
        if (!ready_queue[i]) continue;
        
        char* curr_instr = get_program_content(ready_queue[i], pm->cpu->memory_ram);
        if (!curr_instr) continue;
        
        printf("\n\n[Process P%d] Analisando instruções:", ready_queue[i]->pid);
        printf("\n - Instrução atual: %s", curr_instr);
        
        bool added_to_group = false;
        
        // Tentar adicionar a um grupo existente
        for (int g = 0; g < *group_count; g++) {
            float total_similarity = 0.0f;
            
            printf("\n[Group %d] Comparando com grupo existente:", g);
            
            for (int p = 0; p < groups[g].count; p++) {
                char* group_instr = get_program_content(groups[g].processes[p], pm->cpu->memory_ram);
                float sim_score = calculate_instruction_similarity(curr_instr, group_instr);
                printf("\n - Similaridade com P%d: %.2f", groups[g].processes[p]->pid, sim_score);
                total_similarity += sim_score;
            }
            
            float avg_similarity = groups[g].count > 0 ? 
                                 total_similarity / groups[g].count : 0.0f;
            
            printf("\n - Similaridade média: %.2f", avg_similarity);
            
            if (avg_similarity >= SIMILARITY_THRESHOLD) {
                groups[g].processes[groups[g].count++] = ready_queue[i];
                groups[g].similarity_score = avg_similarity;
                added_to_group = true;
                printf("\n → Adicionado ao grupo %d", g);
                break;
            }
        }
        
        // Criar novo grupo se necessário
        if (!added_to_group && *group_count < MAX_GROUPS) {
            int new_group = *group_count;
            groups[new_group].processes[0] = ready_queue[i];
            groups[new_group].count = 1;
            groups[new_group].similarity_score = 1.0f;
            (*group_count)++;
            printf("\n → Novo grupo %d criado", new_group);
        }
    }
    
    printf("\n\n[Similarity Analysis] Resultado final:");
    for (int g = 0; g < *group_count; g++) {
        printf("\nGrupo %d:", g);
        printf("\n - Processos: ");
        for (int p = 0; p < groups[g].count; p++) {
            printf("P%d ", groups[g].processes[p]->pid);
        }
        printf("\n - Score de similaridade: %.2f", groups[g].similarity_score);
    }
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

    // Se cache estiver desabilitada, usar política Round Robin
    if (!cache_enabled) {
        if (pm->ready_count == 0) return NULL;
        
        // Seleciona o primeiro processo da fila
        PCB* selected = pm->ready_queue[0];
        
        // Configura o processo selecionado
        selected->quantum_remaining = pm->quantum_size;
        selected->state = RUNNING;
        selected->core_id = core_id;
        pm->cpu->core[core_id].is_available = false;
        pm->cpu->core[core_id].current_process = selected;
        
        // Remove da fila de prontos
        for(int i = 0; i < pm->ready_count - 1; i++) {
            pm->ready_queue[i] = pm->ready_queue[i + 1];
        }
        pm->ready_count--;
        
        return selected;
    }

    // Agrupar processos similares
    ProcessGroup groups[MAX_GROUPS];
    int group_count;
    group_similar_processes(pm, pm->ready_queue, pm->ready_count, groups, &group_count);

    PCB* selected = NULL;
    float best_score = -1.0f;
    int selected_idx = -1;
    int selected_group = -1;

    // Primeiro, tentar selecionar do mesmo grupo anterior
    if (last_group_id >= 0 && last_group_id < group_count) {
        ProcessGroup* current_group = &groups[last_group_id];
        for (int p = 0; p < current_group->count; p++) {
            PCB* process = current_group->processes[p];
            float cache_score = calculate_process_cache_score(process, groups, last_group_id);
            float similarity_bonus = current_group->similarity_score * 0.3f;
            float combined_score = cache_score * 0.7f + similarity_bonus * 100.0f;
            
            if (combined_score > best_score) {
                best_score = combined_score;
                selected = process;
                selected_group = last_group_id;
                
                for (int i = 0; i < pm->ready_count; i++) {
                    if (pm->ready_queue[i] == process) {
                        selected_idx = i;
                        break;
                    }
                }
            }
        }
    }

    // Se não encontrou no mesmo grupo, procurar no melhor grupo disponível
    if (!selected) {
        for (int g = 0; g < group_count; g++) {
            for (int p = 0; p < groups[g].count; p++) {
                PCB* process = groups[g].processes[p];
                float cache_score = calculate_process_cache_score(process, groups, g);
                float similarity_bonus = groups[g].similarity_score * 0.3f;
                float combined_score = cache_score * 0.7f + similarity_bonus * 100.0f;
                
                if (combined_score > best_score) {
                    best_score = combined_score;
                    selected = process;
                    selected_group = g;
                    
                    for (int i = 0; i < pm->ready_count; i++) {
                        if (pm->ready_queue[i] == process) {
                            selected_idx = i;
                            break;
                        }
                    }
                }
            }
        }
    }

    if(selected && selected_idx >= 0) {
        printf("\n[Cache Scheduler] Análise de seleção:");
        printf("\n - Processo selecionado: P%d", selected->pid);
        printf("\n - Grupo atual: %d", selected_group);
        printf("\n - Score final: %.2f", best_score);
        
        // Atualizar grupo atual
        last_group_id = selected_group;
        
        // Configurar processo selecionado
        selected->quantum_remaining = pm->quantum_size;
        selected->state = RUNNING;
        selected->core_id = core_id;
        pm->cpu->core[core_id].is_available = false;
        pm->cpu->core[core_id].current_process = selected;

        // Atualizar cache com o conteúdo correto
        char* program_content = get_program_content(selected, pm->cpu->memory_ram);
        if (program_content) {
            update_cache(selected->base_address, program_content);
        } else {
            printf("\n[Cache] Aviso: Não foi possível obter conteúdo do programa");
        }

        // Remover da fila de prontos
        for(int i = selected_idx; i < pm->ready_count - 1; i++) {
            pm->ready_queue[i] = pm->ready_queue[i + 1];
        }
        pm->ready_count--;
        
        printf("\n[Cache] P%d atribuído ao core %d (score: %.2f, grupo: %d)", 
               selected->pid, core_id, best_score, selected_group);
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