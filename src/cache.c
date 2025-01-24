#include "cache.h"
#include <math.h>

#define CACHE_SIZE 128
#define MAX_PATTERNS 10


CacheEntry cache[CACHE_SIZE];
static InstructionPattern known_patterns[MAX_PATTERNS] = {
   {"LOAD ADD MUL", 0},
   {"LOOP ADD SUB", 0}, 
   {"LOOP MUL DIV", 0},
   {"IF DIV STORE", 0},
   {"STORE MUL", 0}
};

float analyze_temporal_locality(CacheEntry* entry) {
   float time_since_access = difftime(time(NULL), entry->last_access);
   return exp(-time_since_access / 1000.0);
}

float analyze_spatial_locality(PCB* process, ProcessManager* pm) {
   float score = 0.0;
   for(int i = 0; i < NUM_CORES; i++) {
       if(!pm->cpu->core[i].is_available) {
           unsigned int addr_diff = process->base_address > 
               pm->cpu->core[i].current_process->base_address ?
               process->base_address - pm->cpu->core[i].current_process->base_address :
               pm->cpu->core[i].current_process->base_address - process->base_address;
           score += (1000.0 - fmin(addr_diff, 1000.0)) / 1000.0;
       }
   }
   return score / NUM_CORES;
}

float analyze_instruction_pattern(char* content) {
   float score = 0.0;
   for(int i = 0; i < MAX_PATTERNS; i++) {
       if(strstr(content, known_patterns[i].pattern)) {
           score += 0.2;
       }
   }
   return fmin(score, 1.0);
}

void update_metrics(CacheEntry* entry, bool is_hit) {
   if(is_hit) {
       entry->hits++;
   } else {
       entry->misses++;
   }
   
   entry->hit_ratio = (float)entry->hits / (entry->hits + entry->misses);
   entry->last_access = time(NULL);
   
   printf("\n[Cache Metrics] Hit ratio: %.2f%% (%d hits, %d misses)",
          entry->hit_ratio * 100, entry->hits, entry->misses);
}

float calculate_cache_score(PCB* process, ProcessManager* pm) {
    CacheEntry* entry = &cache[process->base_address % CACHE_SIZE];
    
    // Calcular métricas
    float temporal = analyze_temporal_locality(entry);
    float spatial = analyze_spatial_locality(process, pm);
    float pattern = analyze_instruction_pattern(get_program_content(process, pm->cpu->memory_ram));
    float score = temporal * 0.35 + spatial * 0.35 + pattern * 0.20 + 
                  (process->waiting_time / 1000.0) * 0.10;

    // Exibir métricas detalhadas
    printf("\n[Cache] Analisando P%d", process->pid);
    printf("\n - Temporal: %.2f (Último acesso: %lds atrás)", temporal, 
           time(NULL) - entry->last_access);
    printf("\n - Espacial: %.2f", spatial);
    printf("\n - Pattern: %.2f", pattern);
    printf("\n - Hits/Misses: %d/%d (%.1f%%)", entry->hits, entry->misses,
           (entry->hits + entry->misses > 0) ? 
           (entry->hits * 100.0f) / (entry->hits + entry->misses) : 0.0f);
    printf("\n - Score Final: %.2f\n", score);

    return score;
}

void init_cache() {
   for(int i = 0; i < CACHE_SIZE; i++) {
       cache[i].tag = 0;
       cache[i].data = NULL;
       cache[i].access_count = 0;
       cache[i].last_access = time(NULL);
       cache[i].hits = 0;
       cache[i].misses = 0;
       cache[i].hit_ratio = 0.0f;
       cache[i].sequence_index = 0;
       cache[i].temporal_score = 0.0f;
       cache[i].spatial_score = 0.0f;
       cache[i].pattern_score = 0.0f;
   }
}

char* get_program_content(PCB* pcb, ram* memory_ram) {
   if (!pcb || !memory_ram) return NULL;
   return memory_ram->vector + pcb->base_address;
}

bool check_cache(unsigned int address) {
    int index = address % CACHE_SIZE;
    bool is_hit = cache[index].tag == address;
    
    if(is_hit) {
        cache[index].hits++;
    } else {
        cache[index].misses++;
    }
    
    cache[index].last_access = time(NULL);
    cache[index].hit_ratio = (float)cache[index].hits / 
                            (cache[index].hits + cache[index].misses);
    
    return is_hit;
}

void update_cache(unsigned int address, char* data) {
   int index = address % CACHE_SIZE;
   cache[index].tag = address;
   cache[index].data = data;
   cache[index].access_count++;
   cache[index].last_access = time(NULL);
   
   if(cache[index].sequence_index < MAX_PATTERN_LENGTH) {
       cache[index].access_sequence[cache[index].sequence_index++] = address;
   }
}

float get_cache_benefit(PCB* process) {
   if(!process) return 0.0f;
   
   int hits = 0, total = 0;
   for(unsigned int i = process->base_address; i < process->memory_limit; i++) {
       if(check_cache(i)) hits++;
       total++;
   }
   
   return total > 0 ? (float)hits / total : 0.0f;
}

