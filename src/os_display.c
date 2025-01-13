#include "os_display.h"

void show_os_banner(void) {
    printf("\n%s╔════════════════════════════════════════╗%s", COLOR_BLUE, COLOR_RESET);
    printf("\n%s║       Sistema Multicore Pipeline       ║%s", COLOR_BLUE, COLOR_RESET);
    printf("\n%s╚════════════════════════════════════════╝%s\n", COLOR_BLUE, COLOR_RESET);
}

void show_system_start(void) {
    printf("\n%s[Sistema] Iniciando...%s\n", COLOR_GREEN, COLOR_RESET);
}

void show_cycle_start(int cycle) {
    printf("\n%s═══════════ Ciclo %d ═══════════%s\n", COLOR_YELLOW, cycle, COLOR_RESET);
}

void show_pipeline_start(int cycle, int core_id, int pid) {
    printf("\n%s┌────── Pipeline (Core %d, P%d, Ciclo %d) ──────┐%s\n", 
           COLOR_CYAN, core_id, pid, cycle, COLOR_RESET);
}

void show_pipeline_fetch(const char* instruction) {
    printf("%s│ IF  │ ", COLOR_CYAN);
    if (instruction) {
        printf("Buscando: %s", instruction);
    } else {
        printf("Stall");
    }
    printf("%s\n", COLOR_RESET);
}

void show_pipeline_decode(const char* decoded_type) {
    printf("%s│ ID  │ ", COLOR_CYAN);
    if (decoded_type) {
        printf("Decodificando: %s", decoded_type);
    } else {
        printf("Stall");
    }
    printf("%s\n", COLOR_RESET);
}

void show_pipeline_execute(const char* operation) {
    printf("%s│ EX  │ ", COLOR_CYAN);
    if (operation) {
        printf("Executando: %s", operation);
    } else {
        printf("Stall");
    }
    printf("%s\n", COLOR_RESET);
}

void show_pipeline_memory(const char* mem_operation) {
    printf("%s│ MEM │ ", COLOR_CYAN);
    if (mem_operation) {
        printf("%s", mem_operation);
    } else {
        printf("Sem acesso à memória");
    }
    printf("%s\n", COLOR_RESET);
}

void show_pipeline_writeback(const char* wb_operation) {
    printf("%s│ WB  │ ", COLOR_CYAN);
    if (wb_operation) {
        printf("%s", wb_operation);
    } else {
        printf("Sem writeback");
    }
    printf("%s\n", COLOR_RESET);
}

void show_pipeline_end(void) {
    printf("%s└──────────────────────────────────────────┘%s\n", 
           COLOR_CYAN, COLOR_RESET);
}

void show_core_state(int core_id, int pid, const char* status) {
    printf("%s[Core %d]%s ", COLOR_MAGENTA, core_id, COLOR_RESET);
    if (pid >= 0) {
        printf("P%d: %s\n", pid, status);
    } else {
        printf("%s\n", status);
    }
}

void show_thread_status(int core_id, const char* status) {
    printf("%s[Thread %d] %s%s\n", COLOR_BLUE, core_id, status, COLOR_RESET);
}

void show_process_state(int pid, const char* old_state, const char* new_state) {
    printf("%s[P%d] %s → %s%s\n", 
           COLOR_GREEN, pid, old_state, new_state, COLOR_RESET);
}

void show_scheduler_state(int ready_count, int blocked_count) {
    printf("\n%s=== Estado do Escalonador ===%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("Prontos: %d | Bloqueados: %d\n", ready_count, blocked_count);
}

void show_system_metrics(int cycle, int total, int completed, int instructions) {
    printf("\n%s=== Métricas do Sistema ===%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("Ciclo: %d\n", cycle);
    printf("Processos: %d/%d\n", completed, total);
    printf("Instruções: %d\n", instructions);
}

void show_stage_statistics(int if_stalls, int mem_stalls, int data_hazards) {
    printf("\n%s=== Estatísticas dos Estágios ===%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("Stalls IF: %d\n", if_stalls);
    printf("Stalls MEM: %d\n", mem_stalls);
    printf("Hazards de Dados: %d\n", data_hazards);
}

void show_final_summary(int total_cycles, int total_processes, int completed) {
    printf("\n%s╔═══ Resumo Final ═══╗%s\n", COLOR_BLUE, COLOR_RESET);
    printf("Ciclos: %d\n", total_cycles);
    printf("Processos: %d/%d\n", completed, total_processes);
    printf("%s╚═══════════════════╝%s\n", COLOR_BLUE, COLOR_RESET);
}