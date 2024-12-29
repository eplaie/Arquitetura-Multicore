#include "pcb.h"
#include "cpu.h"
#include <string.h>

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
    
    // Inicializa novos campos
    pcb->total_instructions = 0;
    pcb->cycles_executed = 0;
    pcb->was_completed = false;
    
    // Adiciona ao array global
    all_processes[total_processes++] = pcb;
    
    // Resto da inicialização...
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

void schedule_next_process(cpu* cpu, int core_id) {
    printf("Attempting to schedule next process (ready queue size: %d)\n", 
           cpu->process_manager->ready_count);
           
    if (cpu->process_manager->ready_count <= 0) {
        printf("No processes in ready queue\n");
        return;
    }
    
    lock_scheduler(cpu);
    PCB* next_process = cpu->process_manager->ready_queue[0];
    
    printf("Selected process %d for core %d\n", next_process->pid, core_id);
    
    // Reorganiza a fila de prontos
    for (int i = 0; i < cpu->process_manager->ready_count - 1; i++) {
        cpu->process_manager->ready_queue[i] = cpu->process_manager->ready_queue[i + 1];
    }
    cpu->process_manager->ready_count--;
    
    // Atribui ao core
    if (cpu->core[core_id].is_available) {
        assign_process_to_core(cpu, next_process, core_id);
        printf("Scheduled process %d to core %d\n", next_process->pid, core_id);
    }
    unlock_scheduler(cpu);
}

void check_blocked_processes(cpu* cpu) {
    if (!cpu || !cpu->process_manager) {
        return;
    }

    ProcessManager* pm = cpu->process_manager;
    
    for (int i = 0; i < pm->blocked_count; i++) {
        PCB* process = pm->blocked_queue[i];
        
        if (!process) continue;

        // Simula término da operação de I/O após alguns ciclos
        if (process->has_io && process->cycles_executed > 2) {
            printf("I/O completed for Process %d\n", process->pid);
            process->has_io = false;
            process->state = READY;
            
            lock_scheduler(cpu);
            
            // Move para fila de prontos
            pm->ready_queue[pm->ready_count++] = process;
            
            // Remove da fila de bloqueados
            for (int j = i; j < pm->blocked_count - 1; j++) {
                pm->blocked_queue[j] = pm->blocked_queue[j + 1];
            }
            pm->blocked_count--;
            i--; // Ajusta índice após remoção
            
            unlock_scheduler(cpu);
            
            printf("Process %d moved from BLOCKED to READY\n", process->pid);
        }
    }
}