#include "cpu.h"
#include "pipeline.h"
#include "architecture.h"
#include <unistd.h>
#include "os_display.h"


void* core_execution_thread(void* arg) {
    core_thread_args* args = (core_thread_args*)arg;
    
    // Verificações mais detalhadas
    if (!args) {
        printf("\n[Thread] ERRO: Argumentos nulos");
        return NULL;
    }

        if (!verify_ram(args->memory_ram, "thread start")) {
        printf("\n[Thread] ERRO: RAM inválida no início da thread");
        return NULL;
    }
    
    if (!args->cpu) {
        printf("\n[Thread] ERRO: CPU inválida");
        return NULL;
    }
    
    if (!args->memory_ram) {
        printf("\n[Thread] ERRO: RAM inválida");
        return NULL;
    }
    
    if (!args->memory_ram->vector) {
        printf("\n[Thread] ERRO: RAM vector inválido");
        return NULL;
    }
    
    cpu* cpu = args->cpu;
    int core_id = args->core_id;
    core* current_core = &cpu->core[core_id];
    
    printf("\n[Thread %d] Thread iniciando (CPU: %p, RAM: %p)", 
           core_id, (void*)cpu, (void*)args->memory_ram);

    while (current_core->running) {
        if (!current_core->is_available && current_core->current_process) {
            PCB* process = current_core->current_process;
            if (process && process->state == RUNNING) {
                execute_pipeline_cycle(args->state, cpu, args->memory_ram, core_id, process->cycles_executed);
            }
        }
        usleep(1000);
    }

    printf("\n[Thread %d] Finalizando", core_id);
    free(args);
    return NULL;
}

void init_cpu(cpu* cpu, ram* memory_ram) {
    printf("\n[CPU Init] Iniciando CPU");
    
    if (!verify_ram(memory_ram, "init_cpu start")) {
        printf("\n[CPU Init] ERRO: RAM inválida");
        exit(1);
    }

    // Primeiro inicializar os cores
    cpu->core = malloc(NUM_CORES * sizeof(core));
    if (!cpu->core) {
        printf("\nErro: Falha na alocação da CPU\n");
        exit(1);
    }

    // Inicializar cores primeiro
    for (int i = 0; i < NUM_CORES; i++) {
        cpu->core[i].registers = calloc(NUM_REGISTERS, sizeof(unsigned short int));
        if (!cpu->core[i].registers) {
            printf("\nErro: Falha na alocação dos registradores do core %d\n", i);
            exit(1);
        }

        cpu->core[i].PC = 0;
        cpu->core[i].current_process = NULL;
        cpu->core[i].is_available = true;
        cpu->core[i].quantum_remaining = 0;
        cpu->core[i].running = true;
        pthread_mutex_init(&cpu->core[i].mutex, NULL);
        printf("\n[CPU Init] Core %d inicializado", i);

        if (!verify_ram(memory_ram, "after core init")) {
            printf("\n[CPU Init] ERRO: RAM inválida após inicialização do core %d", i);
            exit(1);
        }
    }

    // Por último, atribuir a RAM
    cpu->memory_ram = memory_ram;
    
    if (!verify_ram(cpu->memory_ram, "after final assignment")) {
        printf("\n[CPU Init] ERRO: RAM inválida após atribuição final");
        exit(1);
    }

    printf("\n[CPU Init] CPU inicializada com sucesso");
}


void cleanup_cpu_threads(cpu* cpu) {
    if (!cpu || !cpu->core) {
        return;
    }
    
    // printf("\n[Cleanup] Iniciando limpeza dos cores");
    
    // Primeiro, sinalize para todas as threads pararem
    for (int i = 0; i < NUM_CORES; i++) {
        if (cpu->core[i].running) {
            cpu->core[i].running = false;
        }
    }

    // Aguarde um pouco para as threads terminarem
    usleep(100000);

    // Agora limpe os registradores
    for (int i = 0; i < NUM_CORES; i++) {
        if (cpu->core[i].registers) {
            free(cpu->core[i].registers);
            cpu->core[i].registers = NULL;
        }
    }

    // Por fim, limpe o array de cores
    if (cpu->core) {
        free(cpu->core);
        cpu->core = NULL;
    }
}
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
    if (!cpu) return;
    
    core* current_core = &cpu->core[core_id];
    
    // Salvar estado final do processo
    if (current_core->current_process) {
        save_context(current_core->current_process, current_core);
    }
    
    // Resetar o core
    current_core->is_available = true;
    current_core->current_process = NULL;
    current_core->quantum_remaining = 0;  // Será resetado na próxima escalonação
    current_core->PC = 0;
    
    //printf("\n[Core %d] Core liberado", core_id);
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