#include "pcb.h"
#include "cpu.h"
#include <string.h>
#include "os_display.h"

ProgramInfo programs[10];
int num_programs = 0;

ProcessManager* init_process_manager(int quantum_size) {
    ProcessManager* pm = (ProcessManager*)malloc(sizeof(ProcessManager));
    if (!pm) {
        printf("Failed to allocate process manager\n");
        exit(1);
    }

    pm->ready_queue = (PCB**)malloc(sizeof(PCB*) * MAX_PROCESSES);
    pm->blocked_queue = (PCB**)malloc(sizeof(PCB*) * MAX_PROCESSES);
    if (!pm->ready_queue || !pm->blocked_queue) {
        printf("Failed to allocate process queues\n");
        exit(1);
    }

    pm->ready_count = 0;
    pm->blocked_count = 0;
    pm->next_pid = 0;
    pm->quantum_size = quantum_size;

    printf("Process manager initialized (quantum: %d)\n", quantum_size);
    printf("Ready queue initialized with size %d\n", MAX_PROCESSES);

    return pm;
}

PCB* all_processes[MAX_PROCESSES];
int total_processes = 0;

PCB* create_process(ProcessManager* pm) {
    if (total_processes >= MAX_PROCESSES) return NULL;

    PCB* pcb = (PCB*)malloc(sizeof(PCB));
    if (pcb == NULL) {
        printf("Failed to allocate PCB\n");
        exit(1);
    }

    // Inicialização normal...
    pcb->pid = pm->next_pid++;
    pcb->state = READY;
    pcb->priority = 0;
    pcb->PC = 0;
    pcb->core_id = -1;
    pcb->quantum = pm->quantum_size;
    pcb->has_io = false;
    pcb->total_instructions = 0;
    pcb->cycles_executed = 0;
    pcb->was_completed = false;

    pcb->using_resource = false;
    pcb->resource_address = -1;
    pcb->blocked_by_pid = -1;
    pcb->using_io = false;
    pcb->io_block_cycles = 0;

    // Inicialização dos novos campos
    pcb->recovery_count = 0;
    pcb->had_violations = false;

    // Adiciona ao array global
    all_processes[total_processes++] = pcb;

    return pcb;
}

void set_process_base_address(PCB* pcb, int base_address) {
    pcb->base_address = base_address;
    printf("Updated process %d with base address %d\n", pcb->pid, base_address);
}

void save_context(PCB* pcb, core* cur_core) {
    if (!pcb || !cur_core) {
        printf("Warning: Null pointer in save_context\n");
        return;
    }

    pcb->PC = cur_core->PC;
    if (pcb->registers && cur_core->registers) {
        memcpy(pcb->registers, cur_core->registers, NUM_REGISTERS * sizeof(unsigned short int));
    }
}

void restore_context(PCB* pcb, core* cur_core) {
    // Restaura PC relativo
    cur_core->PC = pcb->PC;
    memcpy(cur_core->registers, pcb->registers, NUM_REGISTERS * sizeof(unsigned short int));
}

bool check_program_running(cpu* cpu) {
    // Verifica se há processos na fila de prontos
    if (cpu->process_manager->ready_count > 0) {
        return true;
    }

    // Verifica se há processos na fila de bloqueados
    if (cpu->process_manager->blocked_count > 0) {
        return true;
    }

    // Verifica se há processos executando nos cores
    for (int i = 0; i < NUM_CORES; i++) {
        if (!cpu->core[i].is_available && cpu->core[i].current_process != NULL) {
            return true;
        }
    }

    return false;
}

void remove_from_ready_queue(ProcessManager* pm, int idx) {
    for (int i = idx; i < pm->ready_count - 1; i++) {
        pm->ready_queue[i] = pm->ready_queue[i + 1];
    }
    pm->ready_queue[pm->ready_count - 1] = NULL;
    pm->ready_count--;
}

int count_ready_processes(ProcessManager* pm) {
    if (!pm) return 0;

    int ready_count = 0;
    for (int i = 0; i < pm->ready_count; i++) {
        PCB* process = pm->ready_queue[i];
        if (process && process->state != BLOCKED && process->io_block_cycles == 0) {
            ready_count++;
        }
    }
    return ready_count;
}

void assign_to_core(cpu* cpu, PCB* process, int core_id) {
    lock_core(&cpu->core[core_id]);
    
    cpu->core[core_id].current_process = process;
    cpu->core[core_id].quantum_remaining = cpu->process_manager->quantum_size;
    cpu->core[core_id].PC = process->PC;
    cpu->core[core_id].is_available = false;

    process->state = RUNNING;
    process->core_id = core_id;
    
    show_process_state(process->pid, "READY", "RUNNING");
    
    unlock_core(&cpu->core[core_id]);
}

void schedule_next_process(cpu* cpu, int core_id) {
    if (!cpu || !cpu->process_manager) return;

    lock_scheduler(cpu);

    // Procura primeiro processo que pode ser executado
    int selected_idx = -1;
    PCB* next_process = NULL;

    for (int i = 0; i < cpu->process_manager->ready_count; i++) {
        PCB* process = cpu->process_manager->ready_queue[i];
        if (!process) continue;

        // Pula processo se estiver bloqueado
        if (process->state == BLOCKED || process->io_block_cycles > 0) {
            continue;
        }

        // Verifica se já está em execução em algum core
        bool already_running = false;
        for (int j = 0; j < NUM_CORES; j++) {
            if (j != core_id && !cpu->core[j].is_available && 
                cpu->core[j].current_process == process) {
                already_running = true;
                break;
            }
        }

        if (!already_running) {
            next_process = process;
            selected_idx = i;
            break;
        }
    }

    // Se não encontrou processo elegível
    if (!next_process || selected_idx == -1) {
        unlock_scheduler(cpu);
        return;
    }

    // Remove da fila de prontos e configura
    remove_from_ready_queue(cpu->process_manager, selected_idx);
    assign_to_core(cpu, next_process, core_id);

    unlock_scheduler(cpu);
}



void check_blocked_processes(cpu* cpu) {
    if (!cpu || !cpu->process_manager) return;

    lock_scheduler(cpu);
    ProcessManager* pm = cpu->process_manager;

    for (int i = 0; i < pm->blocked_count; i++) {
        PCB* blocked_process = pm->blocked_queue[i];
        if (!blocked_process) continue;

        if (blocked_process->io_block_cycles > 0) {
            blocked_process->io_block_cycles--;
            printf("\nProcess %d I/O block cycles remaining: %d\n", 
                   blocked_process->pid, blocked_process->io_block_cycles);
            
            if (blocked_process->io_block_cycles == 0) {
                printf("\nProcess %d I/O block completed\n", blocked_process->pid);
                
                // Libera o recurso de I/O
                blocked_process->using_io = false;
                blocked_process->resource_address = -1;
                blocked_process->state = READY;
                
                // Move para fila de prontos
                pm->ready_queue[pm->ready_count++] = blocked_process;
                
                // Remove da fila de bloqueados
                for (int j = i; j < pm->blocked_count - 1; j++) {
                    pm->blocked_queue[j] = pm->blocked_queue[j + 1];
                }
                pm->blocked_queue[pm->blocked_count - 1] = NULL;
                pm->blocked_count--;
                i--;
                
                printf("Process %d moved from BLOCKED to READY\n", 
                       blocked_process->pid);
            }
        }
    }

    unlock_scheduler(cpu);
}

bool check_resource_available(cpu* cpu __attribute__((unused)), 
                            int address, 
                            int requesting_pid __attribute__((unused))) {
    // Verifica se algum processo está usando o recurso
    for (int i = 0; i < total_processes; i++) {
        PCB* process = all_processes[i];
        if (process && process->using_resource && 
            process->resource_address == address &&
            process->state != FINISHED) {
            return false;
        }
    }
    return true;
}