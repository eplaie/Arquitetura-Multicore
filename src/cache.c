#include "cache.h"

#define CACHE_SIZE 128
#define MAX_PATTERNS 10

typedef struct {
    unsigned int tag;
    char* data;
    int access_count;
    int last_access;
} CacheEntry;

typedef struct {
    char* pattern;
    int frequency;
} InstructionPattern;

static CacheEntry cache[CACHE_SIZE];
static InstructionPattern known_patterns[MAX_PATTERNS] = {
    {"LOAD ADD STORE", 0},        // Padrão básico de load-compute-store
    {"LOOP ADD SUB", 0},          // Loop com operações matemáticas
    {"IF ADD STORE", 0},          // Condicionais com operações
    {"LOAD MUL STORE", 0},        // Multiplicação e armazenamento
    {"SUB DIV STORE", 0},         // Divisão e armazenamento
    {"LOAD LOAD ADD", 0},         // Carregamento múltiplo e soma
    {"LOAD LOAD MUL", 0},         // Carregamento múltiplo e multiplicação
    {"LOOP MUL DIV", 0},          // Loop com operações complexas
    {"IF MUL STORE", 0},          // Condicional com multiplicação
    {"LOOP STORE", 0}             // Loop com armazenamento
};

void init_cache() {
    for(int i = 0; i < CACHE_SIZE; i++) {
        cache[i].tag = 0;
        cache[i].data = NULL;
        cache[i].access_count = 0;
        cache[i].last_access = 0;
    }
}

char* get_program_content(PCB* pcb, ram* memory_ram) {
    if (!pcb || !memory_ram) return NULL;
    return memory_ram->vector + pcb->base_address;
}

float calculate_similarity(PCB* p1, PCB* p2, ram* memory_ram) {
    if (!p1 || !p2) return 0.0f;
    
    int similar_patterns = 0;
    int total_patterns = 0;
    
    char* prog1 = get_program_content(p1, memory_ram);
    char* prog2 = get_program_content(p2, memory_ram);
    
    for(int i = 0; i < MAX_PATTERNS; i++) {
        bool in_p1 = strstr(prog1, known_patterns[i].pattern) != NULL;
        bool in_p2 = strstr(prog2, known_patterns[i].pattern) != NULL;
        
        if(in_p1 && in_p2) similar_patterns++;
        if(in_p1 || in_p2) total_patterns++;
    }
    
    return total_patterns > 0 ? (float)similar_patterns / total_patterns : 0.0f;
}

bool check_cache(unsigned int address) {
    int index = address % CACHE_SIZE;
    return cache[index].tag == address;
}

void update_cache(unsigned int address, char* data) {
    static int access_time = 0;
    int index = address % CACHE_SIZE;
    
    cache[index].tag = address;
    cache[index].data = data;
    cache[index].access_count++;
    cache[index].last_access = access_time++;
}

float get_cache_benefit(PCB* process) {
    if(!process) return 0.0f;
    
    int cache_hits = 0;
    int total_accesses = 0;
    
    // Simulate memory accesses
    for(unsigned int i = process->base_address; i < process->memory_limit; i++){
        if(check_cache(i)) cache_hits++;
        total_accesses++;
    }
    
    return total_accesses > 0 ? (float)cache_hits / total_accesses : 0.0f;
}