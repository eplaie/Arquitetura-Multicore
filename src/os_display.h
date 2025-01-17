#ifndef OS_DISPLAY_H
#define OS_DISPLAY_H

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "common_types.h"  
#include "pcb.h"          

// Cores para diferentes partes do display
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"

// Funções básicas do sistema
void show_os_banner(void);
void show_system_start(void);
void show_cycle_start(int cycle);

// Funções de visualização do pipeline
void show_pipeline_start(int cycle, int core_id, int pid);
void show_pipeline_fetch(const char* instruction);
void show_pipeline_decode(const char* decoded_type);
void show_pipeline_execute(const char* operation);
void show_pipeline_memory(const char* mem_operation);
void show_pipeline_writeback(const char* wb_operation);
void show_pipeline_end(void);

// Funções de estado do sistema
void show_core_state(int core_id, int pid, const char* status);
void show_thread_status(int core_id, const char* status);
void show_process_state(int pid, const char* old_state, const char* new_state);
void show_scheduler_state(int ready_count, int blocked_count);
void show_system_metrics(int cycle, int instructions);

// Funções de estatísticas
void show_stage_statistics(int if_stalls, int mem_stalls, int data_hazards);
void show_final_summary(int total_cycles, int total_processes, int completed);


void show_pipeline_status(int cycle, int core_id, PCB* process);

void show_policy_menu(void);
void show_policy_selected(const char* policy_name);
void display_final_statistics(architecture_state* state, Policy* policy);

#endif