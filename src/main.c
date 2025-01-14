#include "architecture.h"
#include "os_display.h"
#include "pcb.h"
#include "reader.h"
#include <unistd.h>
#include <stdbool.h>
#include "libs.h"
#include "ram.h"


#define NUM_PROGRAMS 10


void init_system(void) {
    show_os_banner();
    printf("\n[Sistema] Iniciando com %d cores...\n", NUM_CORES);
}

int main(void) {
    init_system();

            // Alocação dos componentes principais
        cpu* cpu = malloc(sizeof(cpu));
        ram* memory_ram = allocate_ram(NUM_MEMORY);

        printf("\n[Debug] Estado inicial:");
        printf("\n - CPU: %p", (void*)cpu);
        printf("\n - RAM: %p", (void*)memory_ram);
        printf("\n - RAM vector: %p", memory_ram ? (void*)memory_ram->vector : NULL);

        if (!memory_ram || !memory_ram->vector) {
            printf("Erro: Falha na alocação da RAM\n");
            return 1;
        }

        disc* memory_disc = malloc(sizeof(disc));
        peripherals* p = malloc(sizeof(peripherals));
        architecture_state* arch_state = malloc(sizeof(architecture_state));

        if (!memory_ram || !memory_ram->vector) {
            printf("Erro: Falha na alocação da RAM\n");
            return 1;
        }

        printf("RAM inicial: %p\n", (void*)memory_ram->vector); //debug

        // Inicialização da arquitetura
    init_architecture(cpu, memory_ram, memory_disc, p, arch_state);

      printf("RAM após init: %p\n", (void*)memory_ram->vector);  // Debug

    if (!memory_ram->vector) {
        printf("Falha na allocação_2");
    }
    // Inicializa threads dos cores
    for (int i = 0; i < NUM_CORES; i++) {
        if (!memory_ram || !memory_ram->vector) {
            printf("ERRO: RAM inválida antes de carregar programa %d\n", i);
            break;
        }
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
    
    PCB* process = create_pcb();
    if (process != NULL) {
        char* program = read_program(filename);
        if (program) {
            // Calcula endereço base único para este processo
            unsigned int base_address = i * (NUM_MEMORY / MAX_PROCESSES);
            process->base_address = base_address;
            process->memory_limit = base_address + (NUM_MEMORY / MAX_PROCESSES) - 1;

            printf("\n[Sistema] Carregando programa %d na RAM:", i);
            printf("\n - Endereço base: %u", base_address);
            printf("\n - Limite: %u\n", process->memory_limit);

            load_program_on_ram(cpu, program, base_address);
            process->state = READY;
            
            if (cpu->process_manager->ready_count < MAX_PROCESSES) {
                cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = process;
            }
            
            show_process_state(process->pid, "INITIALIZED", "READY");
            free(program);
        }
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

        // Tenta escalonar para cores disponíveis
        for (int core_id = 0; core_id < NUM_CORES; core_id++) {
            if (cpu->core[core_id].is_available) {
                schedule_next_process(cpu, core_id);
            }
        }

        // Verifica estado do sistema 
        bool all_idle = true;
        int ready_count = cpu->process_manager->ready_count;
        int running_count = 0;

        for (int i = 0; i < NUM_CORES; i++) {
            if (!cpu->core[i].is_available && cpu->core[i].current_process) {
                all_idle = false;
                running_count++;
            }
        }

        // Verifica término apenas se não houver mais processos
        if (all_idle && ready_count == 0 && cpu->process_manager->blocked_count == 0) {
            arch_state->program_running = false;
            printf("\n[Sistema] Todos os processos concluídos\n");
        }

        // Mostra estatísticas detalhadas do ciclo
        printf("\n=== Estatísticas do Ciclo %d ===\n", cycle_count);
        printf("Processos Prontos: %d\n", ready_count);
        printf("Processos em Execução: %d\n", running_count); 
        printf("Processos Bloqueados: %d\n", cpu->process_manager->blocked_count);
        printf("Instruções Executadas: %d\n", arch_state->total_instructions);
        printf("Trocas de Contexto: %d\n", arch_state->context_switches);

        // Estado dos cores
        printf("\nEstado dos Cores:\n");
        for (int i = 0; i < NUM_CORES; i++) {
            printf("Core %d: ", i);
            if (!cpu->core[i].is_available && cpu->core[i].current_process) {
                printf("Executando P%d (PC=%d, Quantum=%d)\n", 
                    cpu->core[i].current_process->pid,
                    cpu->core[i].current_process->PC,
                    cpu->core[i].quantum_remaining);
            } else {
                printf("Ocioso\n");
            }
        }

        usleep(10000); // 10ms entre ciclos
        }

        // Aguarda conclusão de todos os processos ativos
        printf("\n[Sistema] Aguardando conclusão dos processos ativos...\n");

        // Sinaliza término para as threads
        for (int i = 0; i < NUM_CORES; i++) {
        if (!cpu->core[i].is_available && cpu->core[i].current_process) {
            printf("Aguardando conclusão do processo %d no core %d...\n",
                cpu->core[i].current_process->pid, i);
        }
        cpu->core[i].running = false;
        }

        // Espera as threads terminarem
        for (int i = 0; i < NUM_CORES; i++) {
        pthread_join(cpu->core[i].thread, NULL);
        show_thread_status(i, "Finalizada");
        }

        // Estatísticas finais detalhadas
        printf("\n=== Estatísticas Finais ===\n");
        printf("Total de Ciclos: %d\n", cycle_count);
        printf("Processos Completados: %d/%d\n", arch_state->completed_processes, total_processes);
        printf("Total de Instruções: %d\n", arch_state->total_instructions);
        printf("Média de Instruções por Ciclo: %.2f\n", 
        (float)arch_state->total_instructions / cycle_count);
        printf("Média de Turnaround: %.2f ciclos\n", 
        arch_state->completed_processes > 0 ? 
        (float)cycle_count / arch_state->completed_processes : 0);
        printf("Trocas de Contexto: %d\n", arch_state->context_switches);
        printf("Taxa de Utilização dos Cores: %.2f%%\n",
        (float)(arch_state->total_instructions * 100) / (cycle_count * NUM_CORES));

        // Cleanup com verificações
        printf("\n[Sistema] Iniciando cleanup...\n");
        cleanup_cpu_threads(cpu);

        printf("[Sistema] Liberando recursos...\n");
        free_architecture(cpu, memory_ram, memory_disc, p, arch_state);

        printf("[Sistema] Execução finalizada com sucesso\n");
        return 0;
}