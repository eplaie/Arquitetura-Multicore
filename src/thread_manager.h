#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "cpu.h"
#include "ram.h"
#include "architecture.h"

void init_thread_system(architecture_state* state);
void cleanup_thread_system(architecture_state* state);
void start_core_threads(cpu* cpu, ram* memory_ram, architecture_state* state);
void stop_core_threads(cpu* cpu);
bool is_system_running(architecture_state* state);

#endif