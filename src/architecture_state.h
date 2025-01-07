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
} architecture_state;

#endif