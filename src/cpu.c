#include "cpu.h"
#include "pipeline.h"
#include "architecture.h"
#include <unistd.h>
#include "os_display.h"

void* core_execution_thread(void* arg) {
    core_thread_args* args = (core_thread_args*)arg;
    
    // Verificações de segurança
    if (!args || !args->cpu || !args->memory_ram || !args->state) {
        printf("ERRO: Argumentos da thread inválidos\n");
        return NULL;
    }
    
    cpu* cpu = args->cpu;
    ram* memory_ram = args->memory_ram;
    int core_id = args->core_id;
    core* current_core = &cpu->core[core_id];
    
    printf("Thread do Core %d iniciando\n", core_id);
    
    show_thread_status(core_id, "Iniciada");

    while (current_core->running) {
        // Verificações adicionais
        if (!current_core->is_available && 
            current_core->current_process == NULL) {
            printf("AVISO: Core %d marcado como ocupado, mas sem processo\n", core_id);
        }

        lock_core(current_core);

        if (!current_core->is_available && current_core->current_process != NULL) {
            PCB* process = current_core->current_process;
            show_core_state(core_id, process->pid, "Executando");
            
            // Verifica recursos antes de executar
            lock_resources(cpu);
            while (process->waiting_resource) {
                pthread_cond_wait(&cpu->resource_condition, &cpu->resource_mutex);
            }
            unlock_resources(cpu);
            
            // Executa ciclo do pipeline
            execute_pipeline_cycle(args->state, cpu, memory_ram, core_id, process->cycles_executed);
            
            // Verifica quantum
            if (current_core->quantum_remaining <= 0) {
                show_core_state(core_id, process->pid, "Quantum Expirado");
                release_core(cpu, core_id);
            }
        }

        unlock_core(current_core);
        usleep(1000); // Pequeno delay para visualização
    }

    show_thread_status(core_id, "Finalizada");
    free(args);
    return NULL;
}

void init_cpu(cpu* cpu) {
    cpu->core = malloc(NUM_CORES * sizeof(core));
    if (!cpu->core) {
        printf("Erro: Falha na alocação da CPU\n");
        exit(1);
    }

    // Inicializa mutexes globais
    pthread_mutex_init(&cpu->scheduler_mutex, NULL);
    pthread_mutex_init(&cpu->memory_mutex, NULL);
    pthread_mutex_init(&cpu->resource_mutex, NULL);
    pthread_cond_init(&cpu->resource_condition, NULL);

    for (int i = 0; i < NUM_CORES; i++) {
        // Inicialização básica do core
        cpu->core[i].registers = malloc(NUM_REGISTERS * sizeof(unsigned short int));
        if (!cpu->core[i].registers) {
            printf("Erro: Falha na alocação dos registradores do core %d\n", i);
            exit(1);
        }

        cpu->core[i].PC = 0;
        cpu->core[i].current_process = NULL;
        cpu->core[i].is_available = true;
        cpu->core[i].quantum_remaining = 0;
        cpu->core[i].running = true;
        
        pthread_mutex_init(&cpu->core[i].mutex, NULL);
        memset(cpu->core[i].registers, 0, NUM_REGISTERS * sizeof(unsigned short int));
    }
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
    if (core_id < 0 || core_id >= NUM_CORES) return;

    lock_core(&cpu->core[core_id]);
    lock_scheduler(cpu);

    core* current_core = &cpu->core[core_id];
    if (current_core->current_process) {
        PCB* process = current_core->current_process;
        process_state prev_state = process->state;

        if (process->state != BLOCKED) {
            if (prev_state != FINISHED) {
                save_context(process, current_core);
            }
            process->state = READY;
            cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = process;
        }

        current_core->is_available = true;
        current_core->quantum_remaining = 0;
        current_core->current_process = NULL;
        memset(current_core->registers, 0, NUM_REGISTERS * sizeof(unsigned short int));

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