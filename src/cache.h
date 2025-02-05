#ifndef CACHE_H
#define CACHE_H

#include "pcb.h"
#include <stdbool.h>
#include <math.h>
#include <time.h>

#define CACHE_SIZE 64          // Aumentar para 64 ou 128 para mais entradas
#define MISS_PENALTY 20        // Aumentar para 20 para mostrar mais impacto
#define MAX_ACCESS_HISTORY 200 // Ok para histórico maior
#define MAX_PATTERNS 8         // Aumentar para detectar mais padrões
#define MAX_PATTERN_LENGTH 100 // Aumentar para padrões maiores
#define BLOCK_SIZE 64         // Ok para o tamanho atual


extern bool cache_enabled;

// Estrutura para registrar acessos à cache
typedef struct {
    char instruction[50];
    bool was_hit;
    time_t access_time;
} CacheAccess;

typedef struct {
    char pattern[MAX_PATTERN_LENGTH];
    int frequency;
    bool needs_prefetch;
    int prefetch_distance;
} InstructionPattern;

typedef struct {
   unsigned int tag;
   char* data;
   int last_used;
   bool valid;
   bool dirty;
   int hits;
   int misses;
   float hit_ratio;
   time_t last_access;
   int access_count;
   int age;
   time_t creation_time;
   int reuse_count;
   float efficiency_score;
   
   // Para análise de padrões de acesso
   int access_history[MAX_ACCESS_HISTORY];
   int history_index;
   float temporal_locality_score;
   float spatial_locality_score;
   
   // Novos campos para prefetch
   bool prefetched;
   int prefetch_hits;
   float prefetch_accuracy;
   char* current_instruction;
} CacheEntry;

// Adicione às funções declaradas
float calculate_instruction_similarity(const char* instr1, const char* instr2);

// Funções principais
void init_cache(void);
bool check_cache(unsigned int address, char* current_instruction);
void update_cache(unsigned int address, char* data);
void print_cache_state(void);
float calculate_cache_efficiency(int index);

// Funções de análise
int find_lru_entry(void);

// Novas funções para prefetch
void prefetch_block(unsigned int base_address, int distance);
int estimate_loop_size(char* content, char* loop_start);
void analyze_instruction_pattern(char* content, unsigned int address);
void print_instruction_patterns(void);
void print_cache_statistics(void);
void print_block_details(void);

extern CacheEntry cache[CACHE_SIZE];
extern CacheAccess access_history[MAX_ACCESS_HISTORY];
extern int access_count;

// Na lista de funções
void set_cache_enabled(bool enabled);
float get_speedup_ratio(void);  // Para calcular o ganho de performance

#endif