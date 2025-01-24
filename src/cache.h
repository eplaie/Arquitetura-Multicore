#ifndef CACHE_H
#define CACHE_H

#include "pcb.h"
#include <stdbool.h>
#include <math.h>

#define CACHE_SIZE 128
#define MAX_PATTERNS 10
#define MISS_PENALTY 10

#define MAX_PATTERN_LENGTH 32

typedef struct {
    unsigned int tag;
    char* data;
    int access_count;
    time_t last_access;
    int hits;
    int misses;
    float hit_ratio;
    int access_sequence[32];
    int sequence_index;
    float temporal_score;
    float spatial_score;
    float pattern_score;
} CacheEntry;

typedef struct {
    char* pattern;
    int frequency;
} InstructionPattern;


void init_cache();
bool check_cache(unsigned int address);
void update_cache(unsigned int address, char* data);
float get_cache_benefit(PCB* process);
char* get_program_content(PCB* pcb, ram* memory_ram);
float analyze_temporal_locality(CacheEntry* entry);
float analyze_spatial_locality(PCB* process, ProcessManager* pm);
float analyze_instruction_pattern(char* content);
void update_metrics(CacheEntry* entry, bool is_hit);

#endif