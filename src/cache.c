#include "cache.h"
#include <string.h>

CacheEntry cache[CACHE_SIZE];

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
        
        memset(cache[i].access_history, 0, sizeof(cache[i].access_history));
    }
}

char* get_program_content(PCB* pcb, ram* memory_ram) {
    if (!pcb || !memory_ram) return NULL;
    return memory_ram->vector + pcb->base_address;
}

bool check_cache(unsigned int address) {
    unsigned idx = address % CACHE_SIZE;
    bool is_hit = cache[idx].valid && cache[idx].tag == address;
    
    // Atualizar métricas
    if (is_hit) {
        cache[idx].hits++;
    } else {
        cache[idx].misses++;
    }
    
    // Atualizar hit ratio
    cache[idx].hit_ratio = (float)cache[idx].hits / 
                          (cache[idx].hits + cache[idx].misses);
    
    cache[idx].last_access = time(NULL);
    
    return is_hit;
}

int find_lru_entry(void) {
    int lru_index = 0;
    int min_timestamp = cache[0].last_used;
    
    for(int i = 1; i < CACHE_SIZE; i++) {
        if(cache[i].last_used < min_timestamp) {
            min_timestamp = cache[i].last_used;
            lru_index = i;
        }
    }
    return lru_index;
}

void update_cache(unsigned int address, char* data) {
    static int timestamp = 0;
    int idx = address % CACHE_SIZE;
    
    printf("\n[Cache] Tentando acessar endereço %u", address);
    
    // Análise de padrões de instrução
    if(data) {
        analyze_instruction_pattern(data, address);
    }
    
    if(cache[idx].valid && cache[idx].tag != address) {
        int lru_idx = find_lru_entry();
        printf("\n[Cache] Miss! Substituindo bloco %d (último acesso: %d) pelo novo endereço %u", 
               lru_idx, cache[lru_idx].last_used, address);
        idx = lru_idx;
    }
    
    cache[idx].tag = address;
    cache[idx].data = data;
    cache[idx].last_used = timestamp++;
    cache[idx].valid = true;
    
    // printf("\n[Cache] Estado após substituição:");
    print_cache_state();
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
    static InstructionPattern known_patterns[MAX_PATTERNS] = { 
        {"LOAD C0", 0}, 
        {"LOAD B0", 0}, 
        {"LOOP", 0}, 
        {"MUL", 0}, 
        {"STORE", 0} 
    };
    
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