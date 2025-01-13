#include "architecture.h"
#include "os_display.h"

void init_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state) {
    if (!cpu || !memory_ram || !memory_disc || !peripherals || !state) {
        printf("Error: NULL pointer in init_architecture\n");
        exit(1);
    }

    // Inicializa memória RAM primeiro
    memory_ram->vector = (char*)calloc(NUM_MEMORY, sizeof(char));
    if (!memory_ram->vector) {
        printf("Failed to allocate RAM memory\n");
        exit(1);
    }

    // Inicializa CPU e threads
    init_cpu(cpu);
    show_thread_status(-1, "CPU initialized");

    // Inicializa disco e periféricos
    init_disc(memory_disc);
    init_peripherals(peripherals);

    // Inicializa estado da arquitetura
    state->pipeline = malloc(sizeof(pipeline));
    if (!state->pipeline) {
        printf("Failed to allocate pipeline\n");
        exit(1);
    }
    init_pipeline(state->pipeline);

    // Inicializa gerenciador de processos
    state->process_manager = init_process_manager(DEFAULT_QUANTUM);
    state->program_running = true;
    state->cycle_count = 0;
    
    // Inicializa métricas
    state->total_instructions = 0;
    state->completed_processes = 0;
    state->blocked_processes = 0;
    state->context_switches = 0;
    state->avg_turnaround = 0;

    // Inicializa mutex global
    pthread_mutex_init(&state->global_mutex, NULL);

    // Configura CPU para trabalhar com o process manager
    cpu->process_manager = state->process_manager;

    // Inicializa threads dos cores
    for (int i = 0; i < NUM_CORES; i++) {
        core_thread_args* args = malloc(sizeof(core_thread_args));
        if (!args) {
            printf("Failed to allocate thread arguments for core %d\n", i);
            exit(1);
        }

        args->cpu = cpu;
        args->memory_ram = memory_ram;
        args->core_id = i;
        args->state = state;
        cpu->core[i].arch_state = state;

        // Cria a thread do core
        if (pthread_create(&cpu->core[i].thread, NULL, core_execution_thread, args) != 0) {
            printf("Failed to create thread for core %d\n", i);
            exit(1);
        }
        show_thread_status(i, "Created");
    }

    show_system_start();
}

void update_system_metrics(architecture_state* state) {
    pthread_mutex_lock(&state->global_mutex);
    
    // Calcula tempo médio de turnaround
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
    
    // Conta processos em cada estado
    ready = cpu->process_manager->ready_count;
    blocked = cpu->process_manager->blocked_count;
    
    for (int i = 0; i < NUM_CORES; i++) {
        if (!cpu->core[i].is_available && 
            cpu->core[i].current_process && 
            cpu->core[i].current_process->state == RUNNING) {
            running++;
        }
    }
    
    // Atualiza estado do sistema
    state->blocked_processes = blocked;
    state->program_running = (running + ready + blocked) > 0;
    
    pthread_mutex_unlock(&state->global_mutex);
    
    // Mostra estado atual
    show_scheduler_state(ready, blocked);
}

void free_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state) {
    // Cleanup das threads primeiro
    cleanup_cpu_threads(cpu);
    
    // Libera ProcessManager
    if (state->process_manager) {
        // Limpa filas
        free(state->process_manager->ready_queue);
        free(state->process_manager->blocked_queue);
        free(state->process_manager);
    }

    // Libera Pipeline
    if (state->pipeline) {
        free(state->pipeline);
    }

    // Libera memória e outros componentes
    if (memory_ram) {
        free(memory_ram->vector);
        free(memory_ram);
    }
    if (memory_disc) {
        free(memory_disc);
    }
    if (peripherals) {
        free(peripherals);
    }

    // Libera mutex global
    pthread_mutex_destroy(&state->global_mutex);

    // Libera estado
    free(state);

    // Libera PCBs
    for (int i = 0; i < total_processes; i++) {
        if (all_processes[i]) {
            free_pcb(all_processes[i]);
        }
    }
}