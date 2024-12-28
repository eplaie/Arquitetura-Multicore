#include "pcb.h"
#include "cpu.h"
#include <string.h>

ProcessManager* init_process_manager(int quantum_size) {
    ProcessManager* pm = (ProcessManager*)malloc(sizeof(ProcessManager));
    if (!pm) {
        printf("Failed to allocate process manager\n");
        exit(1);
    }
    
    pm->ready_queue = (PCB**)malloc(sizeof(PCB*) * 100);
    pm->blocked_queue = (PCB**)malloc(sizeof(PCB*) * 100);
    if (!pm->ready_queue || !pm->blocked_queue) {
        printf("Failed to allocate process queues\n");
        exit(1);
    }

    pm->ready_count = 0;
    pm->blocked_count = 0;
    pm->next_pid = 0;  // Começa com 0 e incrementa ao criar processos
    pm->quantum_size = quantum_size;

    printf("Process manager initialized (quantum: %d)\n", quantum_size);
    
    return pm;
}

PCB* create_process(ProcessManager* pm) {
    PCB* pcb = (PCB*)malloc(sizeof(PCB));
    if (pcb == NULL) {
        printf("Failed to allocate PCB\n");
        exit(1);
    }
    
    pcb->pid = pm->next_pid++;
    pcb->state = READY;
    pcb->priority = 0;
    pcb->PC = 0;
    pcb->core_id = -1;
    pcb->quantum = pm->quantum_size;
    pcb->has_io = false;
    pcb->base_address = 0;  // Será atualizado depois
    pcb->memory_limit = 0;  // Será atualizado depois
    
    pcb->registers = (unsigned short int*)malloc(NUM_REGISTERS * sizeof(unsigned short int));
    if (!pcb->registers) {
        printf("Failed to allocate registers for PCB\n");
        free(pcb);
        exit(1);
    }
    memset(pcb->registers, 0, NUM_REGISTERS * sizeof(unsigned short int));
    
    pm->ready_queue[pm->ready_count++] = pcb;
    
    return pcb;
}

void set_process_base_address(PCB* pcb, int base_address) {
    pcb->base_address = base_address;
    printf("Updated process %d with base address %d\n", pcb->pid, base_address);
}

void save_context(PCB* pcb, core* cur_core) {
    // Salva PC relativo (não o endereço base)
    pcb->PC = cur_core->PC;
    memcpy(pcb->registers, cur_core->registers, NUM_REGISTERS * sizeof(unsigned short int));
}

void restore_context(PCB* pcb, core* cur_core) {
    // Restaura PC relativo
    cur_core->PC = pcb->PC;
    memcpy(cur_core->registers, pcb->registers, NUM_REGISTERS * sizeof(unsigned short int));
}

bool check_program_running(cpu* cpu) {
    if (cpu->process_manager->ready_count > 0 || 
        cpu->process_manager->blocked_count > 0) {
        return true;
    }
    
    for (int i = 0; i < NUM_CORES; i++) {
        if (!cpu->core[i].is_available) {
            return true;
        }
    }
    
    return false;
}

void schedule_next_process(cpu* cpu, int core_id) {
    if (cpu->process_manager->ready_count <= 0) return;
    
    lock_scheduler(cpu);
    PCB* next_process = cpu->process_manager->ready_queue[0];
    
    // Reorganiza a fila de prontos
    for (int i = 0; i < cpu->process_manager->ready_count - 1; i++) {
        cpu->process_manager->ready_queue[i] = cpu->process_manager->ready_queue[i + 1];
    }
    cpu->process_manager->ready_count--;
    
    // Atribui ao core com quantum completo
    if (cpu->core[core_id].is_available) {
        cpu->core[core_id].quantum_remaining = DEFAULT_QUANTUM;
        assign_process_to_core(cpu, next_process, core_id);
        printf("Scheduled process %d to core %d\n", next_process->pid, core_id);
    }
    unlock_scheduler(cpu);
}