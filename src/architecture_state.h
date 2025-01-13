#ifndef ARCHITECTURE_STATE_H
#define ARCHITECTURE_STATE_H

#include <pthread.h>
#include <stdbool.h>
#include "common_types.h"

// Forward declarations
struct pipeline;
struct ProcessManager;

typedef struct architecture_state {
    struct pipeline* pipeline;
    struct ProcessManager* process_manager;
    bool program_running;
    pthread_mutex_t global_mutex;
    
    // Métricas de desempenho adicionadas
    int cycle_count;
    int total_instructions;
    int completed_processes;
    int blocked_processes;
    int context_switches;
    float avg_turnaround;
} architecture_state;

#endif