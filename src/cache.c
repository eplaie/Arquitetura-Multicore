#include "cache.h"
#include <string.h>

unsigned long long current_cycle = 0;
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
    current_cycle = 0;
    for(int i = 0; i < CACHE_SIZE; i++) {
        cache[i].tag = 0;
        cache[i].data = NULL;
        cache[i].last_used = time(NULL);
        cache[i].valid = false;
        cache[i].dirty = false;
        cache[i].hits = 0;
        cache[i].misses = 0;
        cache[i].last_used_cycle = 0;
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
    current_cycle++;
    if (!cache_enabled) {
        return false;
    }

    unsigned idx = address % CACHE_SIZE;
    bool is_hit = cache[idx].valid && cache[idx].tag == address;
    bool was_prefetched = cache[idx].prefetched;

    cache[idx].last_used_cycle = current_cycle;

    printf("\n═══════════ Acesso à Cache ═══════════");
    printf("\n┌── Endereço: %u", address);
    printf("\n├── Instrução: %s", current_instruction);

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

    // Atualizar estatísticas e mostrar resultado
    if(is_hit) {
        cache[idx].last_used_cycle = current_cycle;
        cache[idx].hits++;
        if(was_prefetched) {
            cache[idx].prefetch_hits++;
            printf("\n├── Resultado: ✓ HIT (prefetched)");
            printf("\n├── Bloco: %d", idx);
            printf("\n└── Ganho: %d ciclos", MISS_PENALTY);
        } else {
            printf("\n├── Resultado: ✓ HIT");
            printf("\n├── Bloco: %d", idx);
            printf("\n└── Ganho: %d ciclos", MISS_PENALTY);
        }
    } else {
        cache[idx].misses++;
        printf("\n├── Resultado: ✗ MISS");
        printf("\n├── Bloco: %d", idx);
        printf("\n└── Penalidade: %d ciclos", MISS_PENALTY);

        // Análise de padrão e prefetch em caso de miss
        if(current_instruction) {
            printf("\n\n[Análise de Padrão]");
            if(strstr(current_instruction, "LOAD") != NULL) {
                printf("\n├── Padrão: LOAD detectado");
                printf("\n├── Ação: Prefetch do próximo bloco");
                printf("\n└── Bloco alvo: %d", (address / BLOCK_SIZE + 1) * BLOCK_SIZE);
                prefetch_block(address + 1, 1);
            }
            else if(strstr(current_instruction, "LOOP") != NULL) {
                int loop_size = estimate_loop_size(NULL, current_instruction);
                printf("\n├── Padrão: LOOP detectado");
                printf("\n├── Ação: Prefetch de %d blocos", loop_size);
                printf("\n└── Início: bloco %d", address);
                prefetch_block(address, loop_size);
            }
        }
    }
    printf("\n═════════════════════════════════════\n");

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
    //printf("\n[Prefetch] Iniciando prefetch para endereço base %u", base_address);

    // Calcular próximos endereços sequencialmente
    for(int i = 1; i <= distance; i++) {
        // Agora vamos incrementar apenas em 1 unidade, não em BLOCK_SIZE
        unsigned int next_address = base_address + i;
        int idx = next_address % CACHE_SIZE;
        
        // Se o bloco já está na cache, pula
        if(cache[idx].valid && cache[idx].tag == next_address) {
            // printf("\n[Prefetch] Endereço %u já está na cache", next_address);
            continue;
        }

        // printf("\n[Prefetch] Pré-carregando endereço %u em bloco %d", next_address, idx);
        
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
        
        // printf("\n[Prefetch] Bloco %d marcado como prefetched", idx);
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

    print_block_details();

    printf("\n\n╔═══════════ Resumo de Cache ═══════════╗");

    // Estatísticas globais
    int total_hits = 0, total_misses = 0, total_prefetch_hits = 0;
    for(int i = 0; i < CACHE_SIZE; i++) {
        total_hits += cache[i].hits;
        total_misses += cache[i].misses;
        total_prefetch_hits += cache[i].prefetch_hits;
    }

    printf("\n║ Desempenho Global                      ║");
    printf("\n╠═══════════════════════════════════════╣");
    printf("\n║ ├── Total de Acessos: %-6d           ║",
           total_hits + total_misses);
    printf("\n║ ├── Hits: %-6d                       ║", total_hits);
    printf("\n║ ├── Misses: %-6d                     ║", total_misses);
    printf("\n║ ├── Hit Ratio: %.1f%%                 ║",
           (float)total_hits / (total_hits + total_misses) * 100);
    printf("\n║ └── Prefetch Hits: %-6d              ║",
           total_prefetch_hits);

    // Análise de ciclos
    int ciclos_salvos = total_hits * (MISS_PENALTY - 1);

    printf("\n║                                       ║");
    printf("\n║ Impacto no Desempenho                 ║");
    printf("\n╠═══════════════════════════════════════╣");
    printf("\n║ ├── Pontuação Economizada: %-6d       ║", ciclos_salvos);
    printf("\n║ └── Speedup: %.2fx                    ║",
           get_speedup_ratio());

    printf("\n╚═══════════════════════════════════════╝\n");
}


unsigned int find_lru_entry(void) {
    unsigned int lru_index = 0;
    unsigned long long oldest_cycle = cache[0].last_used_cycle;
    
    for(unsigned int i = 1; i < CACHE_SIZE; i++) {
        if(!cache[i].valid) {
            return i;
        }
        if(cache[i].last_used_cycle < oldest_cycle) {
            oldest_cycle = cache[i].last_used_cycle;
            lru_index = i;
        }
    }

    unsigned long long age = current_cycle - cache[lru_index].last_used_cycle;
    
    printf("\n║ │   └── LRU: BLOCO %u substituído (idade: %llu intruções)║",
           lru_index, age);
    
    return lru_index;
}

void update_cache(unsigned int address, char* data) {
    if (!cache_enabled) return;
    
    unsigned int idx = address % CACHE_SIZE;
    
    // Incrementar idade de todos os blocos antes da atualização
    for(unsigned int i = 0; i < CACHE_SIZE; i++) {
        if(cache[i].valid && i != idx) {
            cache[i].age++;
        }
    }

    printf("\n╔═══════════ Atualização de Bloco ═══════════╗");
    printf("\n║ Endereço: 0x%04X → Bloco %u              ║",
           address, idx);

    if(cache[idx].valid && cache[idx].tag != address) {
        cache[idx].misses++;
        unsigned int lru_idx = find_lru_entry();
        
        printf("\n║ ├── Conflito detectado                     ║");
        printf("\n║ │   ├── Tag antiga: 0x%04X                ║",
               cache[idx].tag);
        printf("\n║ │   └── Nova tag: 0x%04X                  ║",
               address);
        cache[lru_idx].last_used_cycle = current_cycle;
    }

    // Reset da idade do bloco atualizado
    cache[idx].age = 0;
    cache[idx].last_used = time(NULL);
    cache[idx].tag = address;
    cache[idx].data = data;
    cache[idx].valid = true;
    cache[idx].access_count++;

    printf("\n║ └── Estado após atualização                ║");
    printf("\n║     ├── Acessos: %-4u                     ║",
           cache[idx].access_count);
    printf("\n║     ├── Hits: %-4u                        ║",
           cache[idx].hits);
    printf("\n║     └── Hit Ratio: %.1f%%                 ║",
           cache[idx].hit_ratio * 100);

    printf("\n╚═════════════════════════════════════════════╝\n");

    if (cache[idx].hits + cache[idx].misses > 0) {
        cache[idx].hit_ratio = (float)cache[idx].hits /
                            (cache[idx].hits + cache[idx].misses);
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

void print_block_details(void) {
    printf("\n╔═══════════ Análise de Blocos de Cache ═══════════╗");

    for(int i = 0; i < CACHE_SIZE; i++) {
        if(cache[i].access_count > 0) {
            printf("\n║                                               ║");
            printf("\n║ Bloco %2d                                     ║", i);
            printf("\n╠═══════════════════════════════════════════════╣");
            printf("\n║ ├── Estado                                    ║");
            printf("\n║ │   ├── Válido: %s                        ║",
                   cache[i].valid ? "Sim" : "Não");
            printf("\n║ │   ├── Tag: 0x%04X                          ║",
                   cache[i].tag);
            printf("\n║ │   └── Dirty: %s                         ║",
                   cache[i].dirty ? "Sim" : "Não");

            printf("\n║ ├── Estatísticas                             ║");
            printf("\n║ │   ├── Acessos: %-4d                        ║",
                   cache[i].access_count);
            printf("\n║ │   ├── Hits: %-4d                           ║",
                   cache[i].hits);
            printf("\n║ │   ├── Misses: %-4d                         ║",
                   cache[i].misses);
            printf("\n║ │   └── Hit Ratio: %.1f%%                    ║",
                   cache[i].hit_ratio * 100);

            printf("\n║ ├── Prefetching                              ║");
            printf("\n║ │   ├── Foi prefetched: %s                ║",
                   cache[i].prefetched ? "Sim" : "Não");
            printf("\n║ │   ├── Prefetch hits: %-4d                  ║",
                   cache[i].prefetch_hits);
            printf("\n║ │   └── Precisão: %.1f%%                     ║",
                   cache[i].prefetch_accuracy * 100);

            printf("\n║ ├── Temporalidade                            ║");
            printf("\n║ │   └── Último acesso: %lds atrás            ║",
                   time(NULL) - cache[i].last_access);

            if(cache[i].current_instruction) {
                printf("\n║ └── Última Instrução                         ║");
                printf("\n║     └── %s                      ║",
                       cache[i].current_instruction);
            }

            printf("\n╠═══════════════════════════════════════════════╣");

            // Resumo de eficiência do bloco
            float efficiency = (cache[i].hits * 100.0f) /
                             (cache[i].hits + cache[i].misses * MISS_PENALTY);
            printf("\n║ Eficiência do Bloco: %.1f%%                    ║",
                   efficiency);
        }
    }
    printf("\n╚═══════════════════════════════════════════════╝");
}