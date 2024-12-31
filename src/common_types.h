#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include "libs.h"

// Forward declarations
struct PCB;
struct ProcessManager;
struct core;
struct cpu;
struct pipeline;
struct pipeline_stage;
struct peripherals;

typedef struct PCB PCB;
typedef struct ProcessManager ProcessManager;
typedef struct core core;
typedef struct cpu cpu;
typedef struct pipeline pipeline;
typedef struct pipeline_stage pipeline_stage;
typedef struct peripherals peripherals;

#define DEFAULT_QUANTUM 6
#define MAX_CYCLES 13
#define SLEEP_INTERVAL 10000
#define MAX_PROCESSES 10

#endif