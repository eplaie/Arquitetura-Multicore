#include "policy.h"
#include "../libs.h"
#include "../os_display.h"
#include <string.h>

#define MAX_GROUPS 10
#define SIMILARITY_THRESHOLD 0.6

bool is_similar_operation(const char* type1, const char* type2) {
    // Grupo de operações de memória
    if ((strcmp(type1, "LOAD") == 0 || strcmp(type1, "STORE") == 0) &&
        (strcmp(type2, "LOAD") == 0 || strcmp(type2, "STORE") == 0)) {
        return true;
    }
    
    // Grupo de operações aritméticas
    if ((strcmp(type1, "ADD") == 0 || strcmp(type1, "SUB") == 0 || 
         strcmp(type1, "MUL") == 0 || strcmp(type1, "DIV") == 0) &&
        (strcmp(type2, "ADD") == 0 || strcmp(type2, "SUB") == 0 || 
         strcmp(type2, "MUL") == 0 || strcmp(type2, "DIV") == 0)) {
        return true;
    }
    
    return false;
}

// Nova função auxiliar
const char* get_next_instruction(const char* current, int offset) {
    if (!current) return NULL;
    
    const char* ptr = current;
    for (int i = 0; i < offset; i++) {
        ptr = strchr(ptr, '\n');
        if (!ptr) return NULL;
        ptr++; // Skip the newline
    }
    
    return ptr;
}

float calculate_process_cache_score(PCB* process, ProcessGroup* groups, int current_group) {
    int idx = process->base_address % CACHE_SIZE;
    
    // Hit ratio tem peso menor agora (40%)
    float hit_ratio_factor = cache[idx].hit_ratio * 0.4f;
    
    // Similaridade tem peso maior (40%)
    float similarity_factor = groups[current_group].similarity_score * 0.4f;
    
    // Tempo de acesso (20%)
    float recency_factor = 1.0f / (cache[idx].age + 1) * 0.2f;
    
    return (hit_ratio_factor + similarity_factor + recency_factor) * 100.0f;
}

