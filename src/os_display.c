#include "os_display.h"

// Função para mostrar o banner do sistema
void show_os_banner(void) {
    printf("\n%s╔════════════════════════════════════╗%s\n", STYLE_HEADER, STYLE_RESET);
    printf("%s║     Sistema Operacional Simulado     ║%s\n", STYLE_HEADER, STYLE_RESET);
    printf("%s╚════════════════════════════════════╝%s\n\n", STYLE_HEADER, STYLE_RESET);
}

// Função para mostrar início de ciclo
void show_cycle_start(int cycle) {
    printf("\n%s==== Ciclo de Execução %d ====%s\n", STYLE_HEADER, cycle, STYLE_RESET);
}

// Função para mostrar estado do processo
void show_process_state(int pid, const char* old_state, const char* new_state) {
    printf("%s[Processo %d]%s Transição: %s → %s%s%s\n",
           STYLE_PROCESS, pid, STYLE_RESET,
           old_state, STYLE_HIGHLIGHT, new_state, STYLE_RESET);
}

// Função para mostrar execução na CPU
void show_cpu_execution(int core_id, int pid, const char* instruction, int pc) {
    printf("%s[CPU Core %d]%s Executando Processo %d\n",
           STYLE_CPU, core_id, STYLE_RESET, pid);
    printf("  Instrução: %s (PC: %d)\n", instruction, pc);
}

// Função para mostrar estado da memória
void show_memory_operation(int pid, const char* operation, int address) {
    printf("%s[Memória]%s Processo %d: %s em 0x%04X\n",
           STYLE_MEMORY, STYLE_RESET, pid, operation, address);
}

// Função para mostrar estado do escalonador
void show_scheduler_state(int ready_count, int blocked_count) {
    printf("\n%s=== Estado do Escalonador ===%s\n", STYLE_HEADER, STYLE_RESET);
    printf("Processos Prontos: %d\n", ready_count);
    printf("Processos Bloqueados: %d\n", blocked_count);
}

// Função para mostrar estado dos cores
void show_cores_state(int num_cores, const int* core_pids) {
    printf("\n%s=== Estado dos Cores ===%s\n", STYLE_HEADER, STYLE_RESET);
    for (int i = 0; i < num_cores; i++) {
        if (core_pids[i] >= 0) {
            printf("Core %d: Executando Processo %d\n", i, core_pids[i]);
        } else {
            printf("Core %d: Ocioso\n", i);
        }
    }
}

// Função para mostrar resumo do sistema
void show_system_summary(int cycle, int total_processes, int completed,
                        int instructions_executed) {
    printf("\n%s╔════ Resumo do Sistema ════╗%s\n", STYLE_HEADER, STYLE_RESET);
    printf("  Ciclo: %d\n", cycle);
    printf("  Processos Totais: %d\n", total_processes);
    printf("  Processos Concluídos: %d\n", completed);
    printf("  Instruções Executadas: %d\n", instructions_executed);
    printf("%s╚═══════════════════════════╝%s\n\n", STYLE_HEADER, STYLE_RESET);
}
