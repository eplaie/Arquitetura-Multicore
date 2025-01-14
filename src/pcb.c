#include "pcb.h"
#include "cpu.h"
#include "os_display.h"

PCB* all_processes[MAX_PROCESSES];
int total_processes = 0;

PCB* create_pcb(void) {
    if (total_processes >= MAX_PROCESSES) {
        printf("[PCB] ERRO: Número máximo de processos atingido\n");
        return NULL;
    }

    PCB* pcb = malloc(sizeof(PCB));
    if (!pcb) {
        printf("[PCB] Erro: Falha na alocação do PCB\n");
        return NULL;
    }

    pcb->pid = total_processes;
    pcb->state = NEW;
    pcb->core_id = -1;
    pcb->PC = 0;
    pcb->cycles_executed = 0;
    pcb->quantum_remaining = DEFAULT_QUANTUM;
    pcb->base_address = 0;  // Deve ser calculado baseado no total de processos
    pcb->memory_limit = NUM_MEMORY;  // Ajustar conforme necessário
    pcb->was_completed = false;

    pcb->registers = calloc(NUM_REGISTERS, sizeof(unsigned short int));
    if (!pcb->registers) {
        printf("[PCB] Erro: Falha na alocação dos registradores\n");
        free(pcb);
        return NULL;
    }

    pcb->using_io = false;
    pcb->io_block_cycles = 0;
    pcb->waiting_resource = false;
    pcb->resource_name = NULL;

    pcb->total_instructions = 0;
    pcb->waiting_time = 0;
    pcb->turnaround_time = 0;

    all_processes[total_processes++] = pcb;
    printf("[PCB] Processo %d criado com sucesso\n", pcb->pid);
    show_process_state(pcb->pid, "CREATED", "NEW");

    return pcb;
}

void save_context(PCB* pcb, core* current_core) {
    if (!pcb || !current_core) return;

    pcb->PC = current_core->PC;
    memcpy(pcb->registers, current_core->registers, NUM_REGISTERS * sizeof(unsigned short int));
    pcb->quantum_remaining = current_core->quantum_remaining;
}

void restore_context(PCB* pcb, core* current_core) {
    if (!pcb || !current_core) return;

    current_core->PC = pcb->PC;
    memcpy(current_core->registers, pcb->registers, NUM_REGISTERS * sizeof(unsigned short int));
    current_core->quantum_remaining = pcb->quantum_remaining;
}

void free_pcb(PCB* pcb) {
    if (!pcb) return;
    
    if (pcb->registers) {
        free(pcb->registers);
    }
    if (pcb->resource_name) {
        free(pcb->resource_name);
    }
    free(pcb);
}

const char* state_to_string(process_state state) {
    switch(state) {
        case NEW: return "NEW";
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case BLOCKED: return "BLOCKED";
        case FINISHED: return "FINISHED";
        default: return "UNKNOWN";
    }
}

ProcessManager* init_process_manager(int quantum_size) {
    printf("\n[PM Init] Iniciando gerenciador de processos");
    
    ProcessManager* pm = malloc(sizeof(ProcessManager));
    if (!pm) {
        printf("\nFalha ao alocar Process Manager\n");
        exit(1);
    }

    // Inicializa filas
    pm->ready_queue = malloc(sizeof(PCB*) * MAX_PROCESSES);
    pm->blocked_queue = malloc(sizeof(PCB*) * MAX_PROCESSES);
    
    if (!pm->ready_queue || !pm->blocked_queue) {
        printf("\nFalha ao alocar filas\n");
        exit(1);
    }

    pm->ready_count = 0;
    pm->blocked_count = 0;
    pm->quantum_size = quantum_size;

    // Inicializa mutexes
    pthread_mutex_init(&pm->queue_mutex, NULL);
    pthread_mutex_init(&pm->resource_mutex, NULL);
    pthread_cond_init(&pm->resource_condition, NULL);

    printf("\n[PM Init] Gerenciador de processos inicializado");
    return pm;
}

void schedule_next_process(cpu* cpu, int core_id) {
    printf("\n[Scheduler] Tentando escalonar processo para Core %d", core_id);
    
    if (!cpu || core_id < 0 || core_id >= NUM_CORES) {
        printf("\n[Scheduler] ERRO: Parâmetros inválidos");
        return;
    }

    ProcessManager* pm = cpu->process_manager;
    lock_process_manager(pm);

    if (pm->ready_count == 0) {
        printf("\n[Scheduler] Nenhum processo pronto para escalonar");
        unlock_process_manager(pm);
        return;
    }

    printf("\n[Scheduler] Processos prontos: %d", pm->ready_count);

    // Busca por um processo que não esteja rodando em outro core
    for (int i = 0; i < pm->ready_count; i++) {
        PCB* process = pm->ready_queue[i];
        if (!process) {
            printf("\n[Scheduler] ERRO: Processo NULL na fila");
            continue;
        }

        bool already_running = false;
        for (int j = 0; j < NUM_CORES; j++) {
            if (j != core_id && 
                !cpu->core[j].is_available && 
                cpu->core[j].current_process == process) {
                already_running = true;
                break;
            }
        }

        if (!already_running) {
            // Remove da fila de prontos
            for (int j = i; j < pm->ready_count - 1; j++) {
                pm->ready_queue[j] = pm->ready_queue[j + 1];
            }
            pm->ready_count--;

            // Atribui ao core
            process->state = RUNNING;
            process->core_id = core_id;
            cpu->core[core_id].current_process = process;
            cpu->core[core_id].quantum_remaining = pm->quantum_size;
            cpu->core[core_id].is_available = false;

            printf("\n[Scheduler] Processo %d escalado para Core %d", process->pid, core_id);
            printf("\n - Quantum: %d", cpu->core[core_id].quantum_remaining);
            printf("\n - Base: %d", process->base_address);
            printf("\n - PC: %d", process->PC);
            
            show_process_state(process->pid, "READY", "RUNNING");
            break;
        }
    }

    unlock_process_manager(pm);
}

void check_blocked_processes(cpu* cpu) {
    ProcessManager* pm = cpu->process_manager;
    
    lock_process_manager(pm);

    for (int i = 0; i < pm->blocked_count; i++) {
        PCB* process = pm->blocked_queue[i];
        if (!process) continue;

        if (process->io_block_cycles > 0) {
            process->io_block_cycles--;
            if (process->io_block_cycles == 0) {
                process->state = READY;
                pm->ready_queue[pm->ready_count++] = process;
                
                for (int j = i; j < pm->blocked_count - 1; j++) {
                    pm->blocked_queue[j] = pm->blocked_queue[j + 1];
                }
                pm->blocked_count--;
                i--;

                show_process_state(process->pid, "BLOCKED", "READY");
            }
        }
    }

    unlock_process_manager(pm);
}

void lock_process_manager(ProcessManager* pm) {
    pthread_mutex_lock(&pm->queue_mutex);
}

void unlock_process_manager(ProcessManager* pm) {
    pthread_mutex_unlock(&pm->queue_mutex);
}

int count_ready_processes(ProcessManager* pm) {
    if (!pm) return 0;
    return pm->ready_count;
}