#include "architecture.h"
#include "os_display.h"
#include "pcb.h"
#include <unistd.h>
#include <stdbool.h>
#include "libs.h"
#include "architecture.h"

#define NUM_PROGRAMS 10


void init_system(void) {
    show_os_banner();
    printf("\n[Sistema] Iniciando com %d cores...\n", NUM_CORES);
}

int main(void) {
    init_system();

    // Alocação dos componentes principais
    cpu* cpu = malloc(sizeof(cpu));
    ram* memory_ram = malloc(sizeof(ram));
    disc* memory_disc = malloc(sizeof(disc));
    peripherals* p = malloc(sizeof(peripherals));
    architecture_state* arch_state = malloc(sizeof(architecture_state));

    if (!cpu || !memory_ram || !memory_disc || !p || !arch_state) {
        printf("Erro: Falha na alocação de memória\n");
        return 1;
    }

    // Inicialização da arquitetura
    init_architecture(cpu, memory_ram, memory_disc, p, arch_state);
    
    // Inicializa threads dos cores
    for (int i = 0; i < NUM_CORES; i++) {
        core_thread_args* args = malloc(sizeof(core_thread_args));
        if (!args) {
            printf("Erro: Falha na alocação dos argumentos da thread %d\n", i);
            return 1;
        }
        args->cpu = cpu;
        args->memory_ram = memory_ram;
        args->core_id = i;
        args->state = arch_state;
        cpu->core[i].arch_state = arch_state;

        if (pthread_create(&cpu->core[i].thread, NULL, core_execution_thread, args) != 0) {
            printf("Erro: Falha na criação da thread do core %d\n", i);
            return 1;
        }
        show_thread_status(i, "Iniciada");
    }

    // Carrega programas
printf("\n[Sistema] Carregando programas...\n");
char* program_files[] = {"program.txt", "program2.txt", "program3.txt"};
int num_programs = sizeof(program_files) / sizeof(program_files[0]);

for (int i = 0; i < num_programs; i++) {
    char filename[100];
    snprintf(filename, sizeof(filename), "dataset/%s", program_files[i]);
    
    printf("Tentando carregar arquivo: %s\n", filename);
    
    PCB* process = create_pcb();
    if (process != NULL) {
        char* program = read_program(filename);
        
        // Debug: adicionar verificações
        if (program == NULL) {
            printf("ERRO CRÍTICO: Falha ao ler programa %s\n", filename);
            printf("Verificando detalhes do arquivo:\n");
            
            FILE* test = fopen(filename, "r");
            if (test == NULL) {
                printf("Arquivo não encontrado ou sem permissão de leitura\n");
                perror("Erro:");
            } else {
                printf("Arquivo existe e é legível\n");
                fclose(test);
            }
            
            continue;  // Pule para o próximo programa
        }

        printf("Programa %s carregado com sucesso\n", filename);
        
        load_program_on_ram(memory_ram, program);
        process->state = READY;
        
        if (cpu->process_manager->ready_count < MAX_PROCESSES) {
            cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = process;
        } else {
            printf("ERRO: Número máximo de processos atingido\n");
            break;
        }
        
        show_process_state(process->pid, "INITIALIZED", "READY");
        free(program);
    } else {
        printf("Falha ao criar PCB para o programa %s\n", filename);
    }
}

// Após carregar o programa
printf("Detalhes do primeiro processo:\n");
if (cpu->process_manager->ready_queue[0] != NULL) {
    PCB* first_process = cpu->process_manager->ready_queue[0];
    printf("  PID: %d\n", first_process->pid);
    printf("  Estado: %s\n", state_to_string(first_process->state));
    printf("  Endereço base: %d\n", first_process->base_address);
    printf("  Limite de memória: %d\n", first_process->memory_limit);
} else {
    printf("ERRO: Primeiro processo na fila é NULL\n");
}

// Verificar estado do process manager
printf("Process Manager Status:\n");
printf("  Contagem de processos prontos: %d\n", cpu->process_manager->ready_count);
printf("  Endereço do gerenciador: %p\n", (void*)cpu->process_manager);

// Verificar estado da CPU
printf("CPU Status:\n");
printf("  Endereço da CPU: %p\n", (void*)cpu);
for (int i = 0; i < NUM_CORES; i++) {
    printf("  Core %d Status: %s\n", i, 
           cpu->core[i].is_available ? "Disponível" : "Ocupado");
}

// Adicione uma verificação após o loop
printf("Total de processos carregados: %d\n", cpu->process_manager->ready_count);

    // Loop principal - monitoramento do sistema
    printf("\n[Sistema] Iniciando execução...\n");
    arch_state->program_running = true;
    int cycle_count = 0;
    
    while (arch_state->program_running && cycle_count < MAX_CYCLES) {
        cycle_count++;
        show_cycle_start(cycle_count);

        // Verifica processos bloqueados
        check_blocked_processes(cpu);

        // Verifica se há processos para escalonar
        if (cpu->process_manager->ready_count > 0) {
            for (int core_id = 0; core_id < NUM_CORES; core_id++) {
                if (cpu->core[core_id].is_available) {
                    schedule_next_process(cpu, core_id);
                }
            }
        }

        // Verifica estado do sistema
        bool all_idle = true;
        for (int i = 0; i < NUM_CORES; i++) {
            if (!cpu->core[i].is_available) {
                all_idle = false;
                break;
            }
        }

        // Verifica término
        if (all_idle && cpu->process_manager->ready_count == 0 && 
            cpu->process_manager->blocked_count == 0) {
            arch_state->program_running = false;
            printf("\n[Sistema] Todos os processos concluídos\n");
        }

        // Mostra estatísticas do ciclo
        printf("\n=== Estatísticas do Ciclo %d ===\n", cycle_count);
        printf("Processos Prontos: %d\n", cpu->process_manager->ready_count);
        printf("Processos Bloqueados: %d\n", cpu->process_manager->blocked_count);
        printf("Instruções Executadas: %d\n", arch_state->total_instructions);
        printf("Trocas de Contexto: %d\n", arch_state->context_switches);

        
    }

    // Sinaliza término para as threads
    for (int i = 0; i < NUM_CORES; i++) {
        cpu->core[i].running = false;
    }

    // Espera as threads terminarem
    for (int i = 0; i < NUM_CORES; i++) {
        pthread_join(cpu->core[i].thread, NULL);
        show_thread_status(i, "Finalizada");
    }

    // Estatísticas finais
    printf("\n=== Estatísticas Finais ===\n");
    printf("Total de Ciclos: %d\n", cycle_count);
    printf("Processos Completados: %d/%d\n", arch_state->completed_processes, total_processes);
    printf("Total de Instruções: %d\n", arch_state->total_instructions);
    printf("Média de Turnaround: %.2f ciclos\n", 
           arch_state->completed_processes > 0 ? 
           (float)cycle_count / arch_state->completed_processes : 0);
    printf("Trocas de Contexto: %d\n", arch_state->context_switches);

    // Cleanup
    cleanup_cpu_threads(cpu);
    free_architecture(cpu, memory_ram, memory_disc, p, arch_state);

    return 0;
}