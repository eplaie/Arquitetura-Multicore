#include "cache.h"
#include <string.h>

CacheEntry cache[CACHE_SIZE];
CacheAccess access_history[MAX_ACCESS_HISTORY];
int access_count = 0;
bool cache_enabled = true;

static InstructionPattern known_patterns[MAX_PATTERNS] = { 
    {"LOAD C0", 0, true, 1},
    {"LOAD B0", 0, true, 1}, 
    {"LOOP", 0, true, 2},
    {"MUL", 0, false, 0},
    {"STORE", 0, true, 1}
};


void set_cache_enabled(bool enabled) {
    cache_enabled = enabled;
    if (!enabled) {
        // Limpa completamente todas as estatísticas
        init_cache();  // Reinicializa a cache completamente
        access_count = 0;
    }
}

void init_cache(void) {
    for(int i = 0; i < CACHE_SIZE; i++) {
        cache[i].tag = 0;
        cache[i].data = NULL;
        cache[i].last_used = 0;
        cache[i].valid = false;
        cache[i].dirty = false;
        cache[i].hits = 0;
        cache[i].misses = 0;
        cache[i].hit_ratio = 0.0f;
        cache[i].history_index = 0;
        cache[i].temporal_locality_score = 0.0f;
        cache[i].spatial_locality_score = 0.0f;
        cache[i].prefetched = false;
        cache[i].prefetch_hits = 0;
        cache[i].prefetch_accuracy = 0.0f;
        cache[i].current_instruction = NULL;
        
        memset(cache[i].access_history, 0, sizeof(cache[i].access_history));
    }
    access_count = 0;
}

bool check_cache(unsigned int address, char* current_instruction) {
    if (!cache_enabled) {
        // Se cache está desabilitada, sempre conta como miss
        // unsigned idx = address % CACHE_SIZE;
        // cache[idx].misses++;
        // printf("\n[Cache] Cache desabilitada - Miss forçado no endereço %u", address);
        return false;
    }

    unsigned idx = address % CACHE_SIZE;
    bool is_hit = cache[idx].valid && cache[idx].tag == address;
    bool was_prefetched = cache[idx].prefetched;
    
    // Registrar instrução atual e atualizar histórico
    if(current_instruction) {
        if(cache[idx].current_instruction) {
            free(cache[idx].current_instruction);
        }
        cache[idx].current_instruction = strdup(current_instruction);
        
        if(access_count < MAX_ACCESS_HISTORY) {
            strncpy(access_history[access_count].instruction, current_instruction, 49);
            access_history[access_count].was_hit = is_hit;
            access_history[access_count].access_time = time(NULL);
            access_count++;
        }
    }
    
    // Atualizar estatísticas
    if(is_hit) {
        cache[idx].hits++;
        if(was_prefetched) {
            cache[idx].prefetch_hits++;
            printf("\n[Cache] Hit no endereço %u (prefetched)", address);
        } else {
            printf("\n[Cache] Hit no endereço %u", address);
        }
    } else {
        cache[idx].misses++;
        printf("\n[Cache] Miss no endereço %u", address);
        
        // Prefetch em caso de miss
        if(current_instruction) {
            if(strstr(current_instruction, "LOAD") != NULL) {
                printf("\n[Prefetch] LOAD detectado - prefetch próximo bloco");
                int next_block = (address / BLOCK_SIZE + 1) * BLOCK_SIZE;
                prefetch_block(next_block, 1);
            } 
            else if(strstr(current_instruction, "LOOP") != NULL) {
                int loop_size = estimate_loop_size(NULL, current_instruction);
                printf("\n[Prefetch] LOOP detectado - prefetch %d blocos", loop_size);
                prefetch_block(address, loop_size);
            }
        }
    }

    // Atualizar hit ratio e outras métricas
    if(cache[idx].hits + cache[idx].misses > 0) {
        cache[idx].hit_ratio = (float)cache[idx].hits / 
                              (cache[idx].hits + cache[idx].misses);
        if(was_prefetched) {
            cache[idx].prefetch_accuracy = (float)cache[idx].prefetch_hits / 
                                         cache[idx].hits;
        }
    }
    
    cache[idx].last_access = time(NULL);
    cache[idx].access_count++;
    
    return is_hit;
}

float get_speedup_ratio(void) {
    int total_hits = 0, total_misses = 0;
    
    for(int i = 0; i < CACHE_SIZE; i++) {
        total_hits += cache[i].hits;
        total_misses += cache[i].misses;
    }
    
    if (total_hits + total_misses == 0) return 1.0f;
    
    // Calcular ciclos economizados
    int cycles_with_cache = total_hits + (total_misses * MISS_PENALTY);
    int cycles_without_cache = (total_hits + total_misses) * MISS_PENALTY;
    
    return cycles_without_cache / (float)cycles_with_cache;
}

void prefetch_block(unsigned int base_address, int distance) {
    if (!cache_enabled) return;
    printf("\n[Prefetch] Iniciando prefetch para endereço base %u", base_address);

    // Calcular próximos endereços sequencialmente
    for(int i = 1; i <= distance; i++) {
        // Agora vamos incrementar apenas em 1 unidade, não em BLOCK_SIZE
        unsigned int next_address = base_address + i;
        int idx = next_address % CACHE_SIZE;
        
        // Se o bloco já está na cache, pula
        if(cache[idx].valid && cache[idx].tag == next_address) {
            printf("\n[Prefetch] Endereço %u já está na cache", next_address);
            continue;
        }

        printf("\n[Prefetch] Pré-carregando endereço %u em bloco %d", next_address, idx);
        
        // Limpar bloco antigo se necessário
        if(cache[idx].current_instruction) {
            free(cache[idx].current_instruction);
            cache[idx].current_instruction = NULL;
        }

        // Atualizar bloco
        cache[idx].tag = next_address;
        cache[idx].valid = true;
        cache[idx].prefetched = true;
        cache[idx].prefetch_hits = 0;
        cache[idx].last_access = time(NULL);
        
        printf("\n[Prefetch] Bloco %d marcado como prefetched", idx);
    }
}

int estimate_loop_size(char* content __attribute__((unused)), char* loop_start) {
    if(!loop_start) return 0;
    
    int count = 0;
    char* end = strstr(loop_start, "L_END");
    
    if(end) {
        char* curr = loop_start;
        while(curr < end) {
            if(*curr == '\n') count++;
            curr++;
        }
    }
    return count > 0 ? count : 2;  // Mínimo de 2 blocos para LOOP
}

void print_cache_statistics(void) {
    if (!cache_enabled) return;
    printf("\n═══════════ Análise de Prefetching ═══════════\n");
    
    // Análise de LOAD sequenciais
    printf("\n[Padrões de LOAD Sequencial]");
    for(int i = 0; i < access_count; i++) {
        if(strstr(access_history[i].instruction, "LOAD") != NULL) {
            printf("\n%s → %s%s%s", 
                   access_history[i].instruction,
                   access_history[i].was_hit ? "\033[32mHIT\033[0m" : "\033[31mMISS\033[0m",
                   access_history[i].was_hit ? " - Benefício do prefetch" : "",
                   !access_history[i].was_hit ? " → Prefetch iniciado" : "");
        }
    }

    // Análise de LOOP
    printf("\n\n[Detecção de LOOPs]");
    for(int i = 0; i < CACHE_SIZE; i++) {
        if(cache[i].current_instruction && strstr(cache[i].current_instruction, "LOOP") != NULL) {
            printf("\nLOOP em bloco %d → Prefetch de %d blocos", 
                   i, estimate_loop_size(NULL, cache[i].current_instruction));
        }
    }

    // Métricas de Prefetch
    printf("\n\n[Eficiência do Prefetching]");
    int total_prefetched = 0;
    int successful_prefetch = 0;
    
    for(int i = 0; i < CACHE_SIZE; i++) {
        if(cache[i].prefetched) {
            total_prefetched++;
            if(cache[i].prefetch_hits > 0) {
                successful_prefetch++;
            }
            printf("\nBloco %d:", i);
            printf("\n  - Hits após prefetch: %d", cache[i].prefetch_hits);
            printf("\n  - Eficiência: %.1f%%", 
                   (cache[i].prefetch_hits > 0) ? 100.0f : 0.0f);
        }
    }

    printf("\n\n[Análise de Misses Evitados]");
    int total_hits = 0;
    int prefetch_hits = 0;
    for(int i = 0; i < CACHE_SIZE; i++) {
        total_hits += cache[i].hits;
        if(cache[i].prefetched) {
            prefetch_hits += cache[i].prefetch_hits;
        }
    }
    
    printf("\n- Total de hits: %d", total_hits);
    printf("\n- Hits por prefetch: %d", prefetch_hits);
    if(total_hits > 0) {
        float prefetch_ratio = (float)prefetch_hits / total_hits * 100;
        printf("\n- Porcentagem de hits devido ao prefetch: %.1f%%", prefetch_ratio);
    }
    if(total_prefetched > 0) {
        printf("\n- Precisão do prefetch: %.1f%%", 
               (float)successful_prefetch / total_prefetched * 100);
    }

    printf("\n═════════════════════════════════════\n");
}

int find_lru_entry(void) {
    int lru_index = 0;
    time_t min_access = cache[0].last_access;  // Usar last_access em vez de last_used
    
    for(int i = 1; i < CACHE_SIZE; i++) {
        if(!cache[i].valid) {  // Primeiro procurar entrada vazia
            return i;
        }
        if(cache[i].last_access < min_access) {
            min_access = cache[i].last_access;
            lru_index = i;
        }
    }
    return lru_index;
}

void update_cache(unsigned int address, char* data) {
    if (!cache_enabled) return;
    static int timestamp = 0;
    int idx = address % CACHE_SIZE;
    
    printf("\n[Cache] Tentando acessar endereço %u", address);
    
    // Se cache cheia e conflito
    if(cache[idx].valid && cache[idx].tag != address) {
        cache[idx].misses++;  // Adiciona miss na substituição
        int lru_idx = find_lru_entry();
        printf("\n[Cache] LRU: Bloco %d substituído (idade: %d ciclos)", 
               lru_idx, cache[lru_idx].age);
        idx = lru_idx;
    } else if (cache[idx].valid) {
        cache[idx].hits++;  // Adiciona hit quando bloco já está na cache
    }
    
    // Atualizar estatísticas
    cache[idx].tag = address;
    cache[idx].data = data;
    cache[idx].last_used = timestamp++;
    cache[idx].valid = true;
    cache[idx].access_count++;
    cache[idx].age = 0;
    
    if (cache[idx].hits + cache[idx].misses > 0) {
        cache[idx].hit_ratio = (float)cache[idx].hits / 
                            (cache[idx].hits + cache[idx].misses);
    }
    
    // Envelhecer outros blocos
    for(int i = 0; i < CACHE_SIZE; i++) {
        if(i != idx && cache[i].valid) {
            cache[i].age++;
        }
    }
    
    print_cache_state();
}

float calculate_cache_efficiency(int index) {
    if(cache[index].hits + cache[index].misses == 0) {
        return 0.0f;
    }
    
    float hit_ratio = cache[index].hit_ratio;
    float access_factor = (float)cache[index].access_count / MAX_ACCESS_HISTORY;
    float age_factor = 1.0f / (cache[index].age + 1);
    
    return (hit_ratio * 0.5f + access_factor * 0.3f + age_factor * 0.2f) * 100.0f;
}

void print_cache_state(void) {
    // printf("\n[Cache] Estado atual:");
    for(int i = 0; i < CACHE_SIZE; i++) {
        if(cache[i].valid) {
            printf("\n[%d]: tag=%u, último uso=%d", 
                   i, cache[i].tag, cache[i].last_used);
        }
    }
}
    
void analyze_instruction_pattern(char* content, unsigned int address) {
    static bool address_processed[MAX_PROCESSES] = {false};
    
    if (address_processed[address / CACHE_SIZE]) {
        return;  // Já processou este endereço
    }
    
    for(int i = 0; i < MAX_PATTERNS; i++) {
        int pattern_len = strlen(known_patterns[i].pattern);
        char* match = strstr(content, known_patterns[i].pattern);
        
        while(match) {
            bool prefix_check = (match == content) || !isalnum(*(match-1));
            bool suffix_check = !isalnum(*(match + pattern_len));
            
            if(prefix_check && suffix_check) {
                known_patterns[i].frequency++;
                break;  // Contar apenas uma vez por padrão
            }
            
            match = strstr(match + pattern_len, known_patterns[i].pattern);
        }
    }
    
    address_processed[address / CACHE_SIZE] = true;
}


void print_instruction_patterns() {
    printf("\n═════════════════════════════════════\n");
    printf("\n[Cache] Análise de Padrões de Instrução:");
    for(int i = 0; i < MAX_PATTERNS; i++) {
        if(known_patterns[i].frequency > 0) {
            printf("\nPadrão: %s, Frequência: %d", 
                   known_patterns[i].pattern, 
                   known_patterns[i].frequency);
        }
    }
}