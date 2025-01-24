#ifndef CACHE_H
#define CACHE_H

#include "pcb.h"
#include <stdbool.h>
#include <math.h>
#include <time.h>

#define CACHE_SIZE 128
#define MISS_PENALTY 10
#define MAX_ACCESS_HISTORY 32
#define MAX_PATTERNS 5
#define MAX_PATTERN_LENGTH 50

typedef struct {
    char pattern[MAX_PATTERN_LENGTH];
    int frequency;
} InstructionPattern;

typedef struct {
   unsigned int tag;
   char* data;
   int last_used;    // Para política LRU
   bool valid;       // Indica entrada válida
   bool dirty;       // Para write-back
   int hits;
   int misses;
   float hit_ratio;
   time_t last_access;
   
   // Para análise de padrões de acesso
   int access_history[MAX_ACCESS_HISTORY];
   int history_index;
   float temporal_locality_score;
   float spatial_locality_score;
} CacheEntry;

// Funções principais da cache
void init_cache(void);
bool check_cache(unsigned int address);
void update_cache(unsigned int address, char* data);
void print_cache_state(void);

// Funções de análise
char* get_program_content(PCB* pcb, ram* memory_ram);

// Funções LRU
int find_lru_entry(void);

extern CacheEntry cache[CACHE_SIZE];

void analyze_instruction_pattern(char* content, unsigned int address);
void print_instruction_patterns(void);

#endif