#include "cpu.h"
#include "pipeline.h"
#include "architecture.h"
#include <unistd.h>
#include "os_display.h"

void* core_execution_thread(void* arg) {
    core_thread_args* args = (core_thread_args*)arg;
    
    if (!args || !args->cpu || !args->memory_ram || !args->state) {
        printf("\n[Thread] ERRO: Argumentos inválidos");
        return NULL;
    }
    
    cpu* cpu = args->cpu;
    ram* memory_ram = args->memory_ram;
    int core_id = args->core_id;
    core* current_core = &cpu->core[core_id];
    
    printf("\n[Thread %d] Thread iniciando", core_id);

    while (current_core->running) {
        if (!current_core->is_available && current_core->current_process) {
            PCB* process = current_core->current_process;
            if (process->state == RUNNING) {
                // Executa ciclo
                execute_pipeline_cycle(args->state, cpu, memory_ram, core_id, process->cycles_executed);
                
                // Verifica quantum após execução
                if (current_core->quantum_remaining <= 0) {
                    lock_process_manager(cpu->process_manager);
                    process->state = READY;
                    cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = process;
                    current_core->is_available = true;
                    current_core->current_process = NULL;
                    unlock_process_manager(cpu->process_manager);
                }
            }
        }
        usleep(1000); // Reduz para 1ms para maior responsividade
    }

    printf("\n[Thread %d] Finalizando", core_id);
    free(args);
    return NULL;
}

void init_cpu(cpu* cpu, ram* memory_ram) {
    printf("\n[CPU Init] Iniciando CPU");
    printf("\n[CPU Init] Estado inicial:");
    printf("\n - CPU: %p", (void*)cpu);
    printf("\n - RAM recebida: %p", (void*)memory_ram);
    printf("\n - RAM vector: %p", (void*)memory_ram->vector);
    
    if (!cpu || !memory_ram || !memory_ram->vector) {
        printf("\n[CPU Init] ERRO: Parâmetros inválidos");
        exit(1);
    }

    // Aloca e inicializa os cores
    cpu->core = malloc(NUM_CORES * sizeof(core));
    if (!cpu->core) {
        printf("\nErro: Falha na alocação da CPU\n");
        exit(1);
    }

    // Armazena o ponteiro para a RAM original
    cpu->memory_ram = malloc(sizeof(ram));
    if (!cpu->memory_ram) {
        printf("\n[CPU Init] ERRO: Falha ao alocar estrutura RAM");
        free(cpu->core);
        exit(1);
    }

    // Copia a estrutura RAM
    cpu->memory_ram->vector = memory_ram->vector;
    
    // Verifica a cópia
    printf("\n[CPU Init] Verificação após atribuição:");
    printf("\n - CPU memory_ram: %p", (void*)cpu->memory_ram);
    printf("\n - CPU memory_ram vector: %p", (void*)cpu->memory_ram->vector);

    // Inicializa mutexes...
    pthread_mutex_init(&cpu->scheduler_mutex, NULL);
    pthread_mutex_init(&cpu->memory_mutex, NULL);
    pthread_mutex_init(&cpu->resource_mutex, NULL);
    pthread_cond_init(&cpu->resource_condition, NULL);

    printf("\n[CPU Init] Iniciando cores");
    for (int i = 0; i < NUM_CORES; i++) {
        printf("\n[CPU Init] Inicializando core %d", i);
        
        // Inicialização de registradores
        cpu->core[i].registers = malloc(NUM_REGISTERS * sizeof(unsigned short int));
        if (!cpu->core[i].registers) {
            printf("\nErro: Falha na alocação dos registradores do core %d\n", i);
            exit(1);
        }

        // Inicialização de estados
        cpu->core[i].PC = 0;
        cpu->core[i].current_process = NULL;
        cpu->core[i].is_available = true;
        cpu->core[i].quantum_remaining = 0;
        cpu->core[i].running = true;
        
        pthread_mutex_init(&cpu->core[i].mutex, NULL);
        memset(cpu->core[i].registers, 0, NUM_REGISTERS * sizeof(unsigned short int));

        printf("\n[CPU Init] Core %d inicializado", i);
        printf("\n - Registradores: %p", (void*)cpu->core[i].registers);
        printf("\n - Running: %d", cpu->core[i].running);
        printf("\n - Disponível: %d", cpu->core[i].is_available);
    }
    
    printf("\n[CPU Init] CPU inicializada com sucesso");
    show_thread_status(-1, "CPU initialized");
}

void cleanup_cpu_threads(cpu* cpu) {
    for (int i = 0; i < NUM_CORES; i++) {
        cpu->core[i].running = false;
        pthread_join(cpu->core[i].thread, NULL);
        pthread_mutex_destroy(&cpu->core[i].mutex);
        free(cpu->core[i].registers);
    }

    pthread_mutex_destroy(&cpu->scheduler_mutex);
    pthread_mutex_destroy(&cpu->memory_mutex);
    pthread_mutex_destroy(&cpu->resource_mutex);
    pthread_cond_destroy(&cpu->resource_condition);
    free(cpu->core);
}

// Funções de sincronização
void lock_core(core* c) {
    pthread_mutex_lock(&c->mutex);
}

void unlock_core(core* c) {
    pthread_mutex_unlock(&c->mutex);
}

void lock_scheduler(cpu* cpu) {
    pthread_mutex_lock(&cpu->scheduler_mutex);
}

void unlock_scheduler(cpu* cpu) {
    pthread_mutex_unlock(&cpu->scheduler_mutex);
}

void lock_memory(cpu* cpu) {
    pthread_mutex_lock(&cpu->memory_mutex);
}

void unlock_memory(cpu* cpu) {
    pthread_mutex_unlock(&cpu->memory_mutex);
}

void lock_resources(cpu* cpu) {
    pthread_mutex_lock(&cpu->resource_mutex);
}

void unlock_resources(cpu* cpu) {
    pthread_mutex_unlock(&cpu->resource_mutex);
}


// Implementação de store e add se mantém igual ao seu código original...

void release_core(cpu* cpu, int core_id) {
    if (!cpu || core_id < 0 || core_id >= NUM_CORES) {
        printf("\n[Release] ERRO: parâmetros inválidos");
        return;
    }

    printf("\n[Release] Iniciando liberação do core %d", core_id);

    lock_core(&cpu->core[core_id]);
    lock_scheduler(cpu);

    core* current_core = &cpu->core[core_id];
    if (current_core->current_process) {
        PCB* process = current_core->current_process;
        process_state prev_state = process->state;

        printf("\n[Release] Liberando processo %d do core %d", process->pid, core_id);
        printf("\n - Estado anterior: %s", state_to_string(prev_state));

        if (process->state != BLOCKED) {
            if (prev_state != FINISHED) {
                save_context(process, current_core);
            }
            process->state = READY;
            cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = process;
            printf("\n - Processo movido para fila de prontos");
        }

        current_core->is_available = true;
        current_core->quantum_remaining = 0;
        current_core->current_process = NULL;
        memset(current_core->registers, 0, NUM_REGISTERS * sizeof(unsigned short int));

        printf("\n[Release] Core %d liberado com sucesso", core_id);
        show_core_state(core_id, -1, "Disponível");
    }

    unlock_scheduler(cpu);
    unlock_core(current_core);
}

core* get_current_core(cpu* cpu) {
    if (!cpu) return NULL;
    
    for (int i = 0; i < NUM_CORES; i++) {
        lock_core(&cpu->core[i]);
        if (!cpu->core[i].is_available && cpu->core[i].current_process != NULL) {
            unlock_core(&cpu->core[i]);
            return &cpu->core[i];
        }
        unlock_core(&cpu->core[i]);
    }
    return NULL;
}