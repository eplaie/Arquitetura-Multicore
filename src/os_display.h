#ifndef OS_DISPLAY_H
#define OS_DISPLAY_H

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

// Define estilos para visualização
#define STYLE_HEADER     "\033[1;37m"  // Branco brilhante para cabeçalhos
#define STYLE_PROCESS    "\033[0;36m"  // Ciano para processos
#define STYLE_CPU        "\033[0;33m"  // Amarelo para CPU
#define STYLE_MEMORY     "\033[0;35m"  // Magenta para memória
#define STYLE_HIGHLIGHT  "\033[1;32m"  // Verde brilhante para destaque
#define STYLE_RESET      "\033[0m"

// Funções de exibição do sistema
void show_os_banner(void);
void show_cycle_start(int cycle);

// Funções de exibição de processos
void show_process_state(int pid, const char* old_state, const char* new_state);
void show_cpu_execution(int core_id, int pid, const char* instruction, int pc);
void show_memory_operation(int pid, const char* operation, int address);

// Funções de exibição de estado do sistema
void show_scheduler_state(int ready_count, int blocked_count);
void show_cores_state(int num_cores, const int* core_pids);
void show_system_summary(int cycle, int total_processes, int completed, 
                        int instructions_executed);

#endif // OS_DISPLAY_H