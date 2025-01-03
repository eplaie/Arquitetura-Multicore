#include "pcb.h"
#include "cpu.h"
#include <string.h>
#include "os_display.h"

ProgramInfo programs[10];
int num_programs = 0;

ProcessManager* init_process_manager(int quantum_size) {
    ProcessManager* pm = (ProcessManager*)malloc(sizeof(ProcessManager));
    if (!pm) {
        show_system_summary(0, 0, 0, 0);  // Mostra estado inicial do sistema
        exit(1);
    }

    pm->ready_queue = (PCB**)malloc(sizeof(PCB*) * MAX_PROCESSES);
    pm->blocked_queue = (PCB**)malloc(sizeof(PCB*) * MAX_PROCESSES);
    if (!pm->ready_queue || !pm->blocked_queue) {
        show_system_summary(0, 0, 0, 0);
        exit(1);
    }

    pm->ready_count = 0;
    pm->blocked_count = 0;
    pm->next_pid = 0;
    pm->quantum_size = quantum_size;

    show_scheduler_state(0, 0);  // Mostra estado inicial do escalonador
    return pm;
}

PCB* all_processes[MAX_PROCESSES];
int total_processes = 0;

PCB* create_process(ProcessManager* pm) {
    if (total_processes >= MAX_PROCESSES) return NULL;

    PCB* pcb = (PCB*)malloc(sizeof(PCB));
    if (pcb == NULL) {
        show_system_summary(0, total_processes, 0, 0);
        exit(1);
    }

    // Inicialização
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

    // Adiciona ao array global
    all_processes[total_processes++] = pcb;

    show_process_state(pcb->pid, "NEW", "READY");
    return pcb;
}
void set_process_base_address(PCB* pcb, int base_address) {
    pcb->base_address = base_address;
    show_memory_operation(pcb->pid, "Base Address Set", base_address);
}

void save_context(PCB* pcb, core* cur_core) {
    if (!pcb || !cur_core) return;

    pcb->PC = cur_core->PC;
    if (pcb->registers && cur_core->registers) {
        memcpy(pcb->registers, cur_core->registers, NUM_REGISTERS * sizeof(unsigned short int));
    }
}

void restore_context(PCB* pcb, core* cur_core) {
    if (!pcb || !cur_core || !pcb->registers || !cur_core->registers) {
        printf("Warning: Invalid pointers in restore_context\n");
        return;
    }

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
    if (cpu->process_manager->ready_count <= 0) {
        // show_scheduler_state(0, cpu->process_manager->blocked_count);
        return;
    }

    lock_scheduler(cpu);
    PCB* next_process = cpu->process_manager->ready_queue[0];

    // Reorganiza fila de prontos
    for (int i = 0; i < cpu->process_manager->ready_count - 1; i++) {
        cpu->process_manager->ready_queue[i] = cpu->process_manager->ready_queue[i + 1];
    }
    cpu->process_manager->ready_count--;

    // Atribui ao core
    if (cpu->core[core_id].is_available) {
        assign_process_to_core(cpu, next_process, core_id);
        // show_process_state(next_process->pid, "READY", "RUNNING");

        // Mostra estado atualizado do escalonador
        // show_scheduler_state(cpu->process_manager->ready_count,
        //                    cpu->process_manager->blocked_count);
    }
    unlock_scheduler(cpu);
}

void check_blocked_processes(cpu* cpu) {
    if (!cpu || !cpu->process_manager) return;

    ProcessManager* pm = cpu->process_manager;

    for (int i = 0; i < pm->blocked_count; i++) {
        PCB* process = pm->blocked_queue[i];
        if (!process) continue;

        // Simula término da operação de I/O
        if (process->has_io && process->cycles_executed > 2) {
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
            i--;

            show_process_state(process->pid, "BLOCKED", "READY");
            unlock_scheduler(cpu);
        }
    }

    // Mostra estado atualizado do escalonador após verificações
    // show_scheduler_state(pm->ready_count, pm->blocked_count);
}