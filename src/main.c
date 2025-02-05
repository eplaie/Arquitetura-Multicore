#include "architecture.h"
#include "os_display.h"
#include "pcb.h"
#include "reader.h"
#include "libs.h"
#include "ram.h"
#include "policies/policy.h"
#include "policies/policy_selector.h"


void clean_ready_queue(ProcessManager* pm) {
    if (!pm) return;
    
    // Remove processos finalizados
    for (int i = 0; i < pm->ready_count; i++) {
        if (pm->ready_queue[i]->state == FINISHED) {
            // Move os processos restantes
            for (int j = i; j < pm->ready_count - 1; j++) {
                pm->ready_queue[j] = pm->ready_queue[j + 1];
            }
            pm->ready_count--;
            i--; 
        }
    }
}

void init_system(void) {
    show_os_banner();
    printf("[Sistema] Iniciando com %d cores\n", NUM_CORES);
}

int main(void) {
    init_system();

    // Inicialização dos componentes
    cpu* cpu = malloc(sizeof(cpu));
    ram* memory_ram = allocate_ram(NUM_MEMORY);

    if (!memory_ram || !memory_ram->vector) {
        printf("[Sistema] Erro: Falha na alocação de memória\n");
        printf(" - RAM: %p\n", (void*)memory_ram);
        printf(" - Vector: %p\n", (void*)(memory_ram ? memory_ram->vector : NULL));
        return 1;
    }

    disc* memory_disc = malloc(sizeof(disc));
    peripherals* p = malloc(sizeof(peripherals));
    architecture_state* arch_state = malloc(sizeof(architecture_state));

    // 1. Primeiro inicializar o Process Manager
    ProcessManager* pm = init_process_manager(DEFAULT_QUANTUM);
    if (!pm) {
        printf("\n[Sistema] ERRO: Falha ao inicializar Process Manager");
        exit(1);
    }

    // 2. Atribuir o Process Manager 
    pm->cpu = cpu;
    cpu->process_manager = pm;
    arch_state->process_manager = pm;

    init_cache();

        printf("\n╔════════ Configuração de Cache ════════╗");
        printf("\n║  Deseja utilizar cache? (s/n):        ║");
        printf("\n╚═══════════════════════════════════════╝");
        printf("\nEscolha: ");
        char use_cache;
        scanf(" %c", &use_cache);
        set_cache_enabled(use_cache == 's' || use_cache == 'S');

        if (cache_enabled) {
            printf("\n[Cache] Cache habilitada - Executando com otimizações");
        } else {
            printf("\n[Cache] Cache desabilitada - Executando sem otimizações");
        }

    // 3. Inicialização da arquitetura
    init_architecture(cpu, memory_ram, memory_disc, p, arch_state);

    // 4. Seleção da política de escalonamento
    printf("\n[Sistema] Selecionando política de escalonamento\n");
    Policy* selected_policy = select_scheduling_policy();
    if (!selected_policy) {
        printf("\n[Sistema] ERRO: Política nula");
        exit(1);
    }

    if (!cpu->process_manager) {
        printf("\n[Sistema] ERRO: Process manager nulo após inicialização");
        exit(1);
    }

    cpu->process_manager->policy = selected_policy;

    // Carregamento dos programas
    printf("\n[Sistema] Carregando programas\n");
    char* program_files[] = {"program.txt", "program2.txt", "program3.txt"};
    int num_programs = sizeof(program_files) / sizeof(program_files[0]);
    unsigned int base_address;
    char* program;

    for (int i = 0; i < num_programs; i++) {
        char filename[100];
        snprintf(filename, sizeof(filename), "dataset/%s", program_files[i]);

        PCB* process = create_pcb();
        program = read_program(filename);
        if (process && program) {
            base_address = i * (NUM_MEMORY / MAX_PROCESSES);
            process->base_address = base_address;
            process->memory_limit = base_address + (NUM_MEMORY / MAX_PROCESSES) - 1;
            
            load_program_on_ram(cpu, program, base_address, process);
            process->state = READY;
            cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = process;
            
            show_process_state(process->pid, "CREATED", "READY");
            free(program);
        }
    }

    show_scheduler_state(cpu->process_manager->ready_count, 0);

    // Loop principal de execução
    printf("\n[Sistema] Iniciando execução\n");
    int cycle_count = 0;
    arch_state->program_running = true;

    // Verificações de segurança antes do loop
    if (!arch_state->pipeline) {
        printf("\n[Sistema] ERRO: Pipeline não inicializado");
        exit(1);
    }

    if (!cpu->memory_ram || !cpu->memory_ram->vector) {
        printf("\n[Sistema] ERRO: RAM inválida antes da execução");
        exit(1);
    }

        while (cycle_count < MAX_CYCLES) {
    cycle_count++;
    cpu->process_manager->current_time = cycle_count;
    show_cycle_start(cycle_count);
    
    // Escalonar processos para cores disponíveis
for (int core_id = 0; core_id < NUM_CORES; core_id++) {
    if (cpu->core[core_id].is_available && 
        cpu->process_manager->ready_count > 0) {
        
        // Garantir que o quantum seja resetado
        cpu->core[core_id].quantum_remaining = cpu->process_manager->quantum_size;
        schedule_next_process(cpu, core_id);
    }
}

    // Executar processos nos cores
    int running_count = 0;
    for (int core_id = 0; core_id < NUM_CORES; core_id++) {
        if (!cpu->core[core_id].is_available && 
            cpu->core[core_id].current_process && 
            cpu->core[core_id].current_process->state == RUNNING) {
            
            running_count++;
            execute_pipeline_cycle(arch_state, cpu, cpu->memory_ram, core_id, cycle_count);
            
            // Verificar quantum após execução
            if (cpu->core[core_id].quantum_remaining <= 0) {
                PCB* current_process = cpu->core[core_id].current_process;
                
                // Usar a política para tratar o quantum expirado
                cpu->process_manager->policy->on_quantum_expired(
                    cpu->process_manager, 
                    current_process
                );
                
                // Liberar o core
                release_core(cpu, core_id);
            }
        }
    }

    // Verificar término
    if (cpu->process_manager->ready_count == 0 && running_count == 0) {
        bool all_done = true;
        for (int i = 0; i < total_processes; i++) {
            if (all_processes[i] && all_processes[i]->state != FINISHED) {
                all_done = false;
                break;
            }
        }
        
        if (all_done) {
            printf("\n[Sistema] Todos os processos finalizaram execução");
            break;
        }
    }



    usleep(50000);
}

        if (cache_enabled) {
            printf("\n\n═══════════ Métricas de Cache ═══════════");
            printf("\nSpeedup com cache: %.2fx", get_speedup_ratio());
            printf("\nModo de execução: Otimizado");
            printf("\n═════════════════════════════════════\n");
            print_cache_statistics();
        } else {
            printf("\n\n═══════════ Métricas de Cache ═══════════");
            printf("\nModo de execução: Sem otimizações");
            printf("\n═════════════════════════════════════\n");
        }

    printf("\n[Sistema] Limpando recursos...");
    
    // Parar threads
    if (cpu && cpu->core) {
        for (int i = 0; i < NUM_CORES; i++) {
            cpu->core[i].running = false;
        }
        usleep(100000); // Dar tempo para threads finalizarem
    }

    // Limpeza final
    // printf("[Sistema] Liberando recursos\n");
    free_architecture(cpu, memory_ram, memory_disc, p, arch_state, cycle_count);

    printf("[Sistema] Execução finalizada\n");
    return 0;
}