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
    printf("[Sistema] Iniciando com %d cores\n", NUM_CORES);
}

int main(void) {
    init_system();

    // Alocação dos componentes principais
    cpu* cpu = malloc(sizeof(cpu));
    ram* memory_ram = allocate_ram(NUM_MEMORY);

    printf("[Sistema] Estado inicial:\n");
    printf("- CPU: %p\n", (void*)cpu);
    printf("- RAM: %p\n", (void*)memory_ram);
    printf("- RAM vector: %p\n", memory_ram ? (void*)memory_ram->vector : NULL);

    if (!memory_ram || !memory_ram->vector) {
        printf("[Sistema] Erro: Falha na alocação da RAM\n");
        return 1;
    }

    disc* memory_disc = malloc(sizeof(disc));
    peripherals* p = malloc(sizeof(peripherals));
    architecture_state* arch_state = malloc(sizeof(architecture_state));

    if (!memory_ram || !memory_ram->vector) {
        printf("[Sistema] Erro: Falha na alocação da RAM\n");
        return 1;
    }

    printf("[Sistema] RAM inicial: %p\n", (void*)memory_ram->vector);

    // Inicialização da arquitetura
    init_architecture(cpu, memory_ram, memory_disc, p, arch_state);

    printf("[Sistema] RAM após init: %p\n", (void*)memory_ram->vector);

    if (!memory_ram->vector) {
        printf("[Sistema] Erro: Falha na alocação da RAM\n");
    }

    // Inicializa threads dos cores
    for (int i = 0; i < NUM_CORES; i++) {
        if (!memory_ram || !memory_ram->vector) {
            printf("[Sistema] Erro: RAM inválida - Core %d\n", i);
            break;
        }
        core_thread_args* args = malloc(sizeof(core_thread_args));
        if (!args) {
            printf("[Sistema] Erro: Falha na alocação - Core %d\n", i);
            return 1;
        }
        args->cpu = cpu;
        args->memory_ram = memory_ram;
        args->core_id = i;
        args->state = arch_state;
        cpu->core[i].arch_state = arch_state;

        if (pthread_create(&cpu->core[i].thread, NULL, core_execution_thread, args) != 0) {
            printf("[Sistema] Erro: Falha ao criar thread - Core %d\n", i);
            return 1;
        }
        show_thread_status(i, "Iniciada");
    }

    printf("[Sistema] Carregando programas\n");
    char* program_files[] = {"program.txt", "program2.txt", "program3.txt"};
    int num_programs = sizeof(program_files) / sizeof(program_files[0]);

    for (int i = 0; i < num_programs; i++) {
        char filename[100];
        snprintf(filename, sizeof(filename), "dataset/%s", program_files[i]);

        PCB* process = create_pcb();
        if (process != NULL) {
            char* program = read_program(filename);
            if (program) {
                unsigned int base_address = i * (NUM_MEMORY / MAX_PROCESSES);
                process->base_address = base_address;
                process->memory_limit = base_address + (NUM_MEMORY / MAX_PROCESSES) - 1;

                printf("[Sistema] Programa %d:\n", i);
                printf("- Base: %u\n", base_address);
                printf("- Limite: %u\n", process->memory_limit);

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

    printf("[Sistema] Primeiro processo:\n");
    if (cpu->process_manager->ready_queue[0] != NULL) {
        PCB* first_process = cpu->process_manager->ready_queue[0];
        printf("- PID: %d\n", first_process->pid);
        printf("- Estado: %s\n", state_to_string(first_process->state));
        printf("- Base: %d\n", first_process->base_address);
        printf("- Limite: %d\n", first_process->memory_limit);
    } else {
        printf("[Sistema] Erro: Processo inicial não encontrado\n");
    }

    printf("[Status] Process Manager:\n");
    printf("- Processos prontos: %d\n", cpu->process_manager->ready_count);
    printf("- Endereço: %p\n", (void*)cpu->process_manager);

    printf("[Status] CPU:\n");
    printf("- Endereço: %p\n", (void*)cpu);
    for (int i = 0; i < NUM_CORES; i++) {
        printf("- Core %d: %s\n", i,
               cpu->core[i].is_available ? "Disponível" : "Ocupado");
    }

    printf("[Status] Total de processos: %d\n", cpu->process_manager->ready_count);

    printf("[Sistema] Iniciando execução\n");
    arch_state->program_running = true;
    int cycle_count = 0;

    while (arch_state->program_running && cycle_count < MAX_CYCLES) {
        cycle_count++;
        show_cycle_start(cycle_count);

        check_blocked_processes(cpu);

        for (int core_id = 0; core_id < NUM_CORES; core_id++) {
            if (cpu->core[core_id].is_available) {
                schedule_next_process(cpu, core_id);
            }
        }

        bool all_idle = true;
        int ready_count = cpu->process_manager->ready_count;
        int running_count = 0;

        for (int i = 0; i < NUM_CORES; i++) {
            if (!cpu->core[i].is_available && cpu->core[i].current_process) {
                all_idle = false;
                running_count++;
            }
        }

        if (all_idle && ready_count == 0 && cpu->process_manager->blocked_count == 0) {
            arch_state->program_running = false;
            printf("[Sistema] Todos os processos concluídos\n");
        }

        printf("[Ciclo %d] Status:\n", cycle_count);
        printf("- Prontos: %d\n", ready_count);
        printf("- Executando: %d\n", running_count);
        printf("- Bloqueados: %d\n", cpu->process_manager->blocked_count);
        printf("- Instruções: %d\n", arch_state->total_instructions);
        printf("- Trocas de contexto: %d\n", arch_state->context_switches);

        printf("[Ciclo %d] Cores:\n", cycle_count);
        for (int i = 0; i < NUM_CORES; i++) {
            printf("- Core %d: ", i);
            if (!cpu->core[i].is_available && cpu->core[i].current_process) {
                printf("P%d (PC=%d, Q=%d)\n",
                    cpu->core[i].current_process->pid,
                    cpu->core[i].current_process->PC,
                    cpu->core[i].quantum_remaining);
            } else {
                printf("Ocioso\n");
            }
        }

        usleep(10000); // 10ms entre ciclos
    }

    printf("[Sistema] Finalizando processos ativos\n");

    for (int i = 0; i < NUM_CORES; i++) {
        if (!cpu->core[i].is_available && cpu->core[i].current_process) {
            printf("[Sistema] Aguardando P%d - Core %d\n",
                cpu->core[i].current_process->pid, i);
        }
        cpu->core[i].running = false;
    }

    for (int i = 0; i < NUM_CORES; i++) {
        pthread_join(cpu->core[i].thread, NULL);
        show_thread_status(i, "Finalizada");
    }

    printf("[Sistema] Estatísticas finais:\n");
    printf("- Ciclos: %d\n", cycle_count);
    printf("- Processos: %d/%d\n", arch_state->completed_processes, total_processes);
    printf("- Instruções: %d\n", arch_state->total_instructions);
    printf("- Média instruções/ciclo: %.2f\n",
           (float)arch_state->total_instructions / cycle_count);
    printf("- Média turnaround: %.2f ciclos\n",
           arch_state->completed_processes > 0 ?
           (float)cycle_count / arch_state->completed_processes : 0);
    printf("- Trocas de contexto: %d\n", arch_state->context_switches);
    printf("- Utilização dos cores: %.2f%%\n",
           (float)(arch_state->total_instructions * 100) / (cycle_count * NUM_CORES));

    printf("[Sistema] Liberando recursos\n");
    cleanup_cpu_threads(cpu);
    free_architecture(cpu, memory_ram, memory_disc, p, arch_state);

    printf("[Sistema] Execução finalizada\n");
    return 0;
}