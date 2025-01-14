#include "architecture.h"
#include "os_display.h"
#include "cpu.h"
#include "ram.h"
#include "disc.h"
#include "peripherals.h"
#include "pcb.h"
#include "pipeline.h"

void init_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state) {
    printf("\n[Init] Iniciando arquitetura");
    printf("\n[Init] Verificação inicial:");
    printf("\n - CPU: %p", (void*)cpu);
    printf("\n - RAM: %p", (void*)memory_ram);
    printf("\n - RAM vector: %p", (void*)memory_ram->vector);

    if (!cpu || !memory_ram || !memory_disc || !peripherals || !state) {
        printf("\n[Init] ERRO: Ponteiro NULL detectado");
        exit(1);
    }

    // Inicializa CPU
    init_cpu(cpu, memory_ram);
    
    printf("\n[Init] Verificação após init_cpu:");
    printf("\n - CPU memory_ram: %p", (void*)cpu->memory_ram);
    printf("\n - CPU memory_ram vector: %p", (void*)cpu->memory_ram->vector);

    // Inicializa disco e periféricos
    init_disc(memory_disc);
    printf("\n[Init] Disco inicializado");

    init_peripherals(peripherals);
    printf("\n[Init] Periféricos inicializados");

    // Inicializa pipeline
    state->pipeline = malloc(sizeof(pipeline));
    if (!state->pipeline) {
        printf("\n[Init] ERRO: Falha ao alocar pipeline");
        exit(1);
    }
    init_pipeline(state->pipeline);
    printf("\n[Init] Pipeline inicializado");

    // Inicializa gerenciador de processos
    state->process_manager = init_process_manager(DEFAULT_QUANTUM);
    printf("\n[Init] Process Manager inicializado");
    
    // Configura o gerenciador de processos na CPU
    cpu->process_manager = state->process_manager;

    // Cria as threads dos cores
    printf("\n[Init] Criando threads dos cores");
    for (int i = 0; i < NUM_CORES; i++) {
        core_thread_args* args = malloc(sizeof(core_thread_args));
        if (!args) {
            printf("\n[Init] ERRO: Falha na alocação dos argumentos da thread %d", i);
            exit(1);
        }

        args->cpu = cpu;
        args->memory_ram = cpu->memory_ram;
        args->core_id = i;
        args->state = state;
        cpu->core[i].arch_state = state;

        printf("\n[Init] Thread %d - argumentos:", i);
        printf("\n - CPU: %p", (void*)args->cpu);
        printf("\n - RAM: %p", (void*)args->memory_ram);
        printf("\n - RAM vector: %p", (void*)args->memory_ram->vector);

        if (pthread_create(&cpu->core[i].thread, NULL, core_execution_thread, args) != 0) {
            printf("\n[Init] ERRO: Falha na criação da thread do core %d", i);
            exit(1);
        }
        
        printf("\n[Init] Thread do core %d criada com sucesso", i);
        show_thread_status(i, "Iniciada");
    }

    // Inicializa métricas e estado global
    state->program_running = true;
    state->cycle_count = 0;
    state->total_instructions = 0;
    state->completed_processes = 0;
    state->blocked_processes = 0;
    state->context_switches = 0;
    state->avg_turnaround = 0;

    pthread_mutex_init(&state->global_mutex, NULL);

    printf("\n[Init] Arquitetura inicializada com sucesso");
    printf("\n - Cores ativos: %d", NUM_CORES);
    printf("\n - Quantum: %d", DEFAULT_QUANTUM);
    printf("\n - RAM final: %p", (void*)cpu->memory_ram->vector);
}

void update_system_metrics(architecture_state* state) {
    pthread_mutex_lock(&state->global_mutex);
    
    float total_turnaround = 0;
    int completed = 0;
    
    for (int i = 0; i < total_processes; i++) {
        if (all_processes[i] && all_processes[i]->was_completed) {
            total_turnaround += all_processes[i]->cycles_executed;
            completed++;
        }
    }
    
    if (completed > 0) {
        state->avg_turnaround = total_turnaround / completed;
    }
    
    pthread_mutex_unlock(&state->global_mutex);
}

void check_system_state(architecture_state* state, cpu* cpu) {
    pthread_mutex_lock(&state->global_mutex);
    
    int running = 0, ready = 0, blocked = 0;
    
    ready = cpu->process_manager->ready_count;
    blocked = cpu->process_manager->blocked_count;
    
    for (int i = 0; i < NUM_CORES; i++) {
        if (!cpu->core[i].is_available && 
            cpu->core[i].current_process && 
            cpu->core[i].current_process->state == RUNNING) {
            running++;
        }
    }
    
    state->blocked_processes = blocked;
    state->program_running = (running + ready + blocked) > 0;
    
    pthread_mutex_unlock(&state->global_mutex);
    show_scheduler_state(ready, blocked);
}

void free_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state) {
    cleanup_cpu_threads(cpu);
    
    if (cpu && cpu->memory_ram) {
        free(cpu->memory_ram);
    }

    if (memory_ram) {
        free(memory_ram->vector);
        free(memory_ram);
    }

    if (state->process_manager) {
        free(state->process_manager->ready_queue);
        free(state->process_manager->blocked_queue);
        free(state->process_manager);
    }

    if (state->pipeline) {
        free(state->pipeline);
    }

    if (memory_disc) {
        free(memory_disc);
    }
    if (peripherals) {
        free(peripherals);
    }

    pthread_mutex_destroy(&state->global_mutex);

    free(state);

    for (int i = 0; i < total_processes; i++) {
        if (all_processes[i]) {
            free_pcb(all_processes[i]);
        }
    }
}