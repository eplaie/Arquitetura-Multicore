#include "architecture.h"
#include "os_display.h"

void init_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, peripherals* peripherals, architecture_state* state) {
    if (!cpu || !memory_ram || !memory_disc || !peripherals || !state) {
        show_memory_operation(-1, "ERROR: Null pointer in initialization", 0);
        exit(1);
    }



    // Inicializa memória RAM
    memory_ram->vector = (char*)calloc(NUM_MEMORY, sizeof(char));
    if (memory_ram->vector == NULL) {
        show_memory_operation(-1, "ERROR: Failed to allocate RAM", NUM_MEMORY);
        exit(1);
    }
    show_memory_operation(-1, "RAM Initialized", NUM_MEMORY);

    // Inicializa componentes
    init_cpu(cpu);
    init_disc(memory_disc);
    init_peripherals(peripherals);

    // Inicializa pipeline
    state->pipeline = malloc(sizeof(pipeline));
    if (state->pipeline == NULL) {
        show_memory_operation(-1, "ERROR: Failed to allocate pipeline", 0);
        exit(1);
    }
    init_pipeline(state->pipeline);

    // Inicializa gerenciador de processos
    state->process_manager = init_process_manager(DEFAULT_QUANTUM);
    state->program_running = true;
    cpu->process_manager = state->process_manager;

    // show_scheduler_state(0, 0);
}

void init_pipeline_multicore(architecture_state* state, cpu* cpu __attribute__((unused)), 
                           ram* memory_ram __attribute__((unused))) {
    if (!state || !state->pipeline) return;
    
    state->pipeline->current_core = 0;
    show_cores_state(NUM_CORES, NULL);
}

void execute_pipeline_cycle(architecture_state* state __attribute__((unused)), 
                          cpu* cpu, ram* memory_ram, int core_id, int cycle_count) {
    show_cycle_start(cycle_count);
    
    core* current_core = &cpu->core[core_id];
    if (!current_core) return;

    PCB* current_process = current_core->current_process;
    if (!current_process) {
        show_cores_state(NUM_CORES, NULL);
        return;
    }

    // Mostra estado atual
    show_process_state(current_process->pid, "RUNNING", 
                      state_to_string(current_process->state));
    
    // Verifica término do processo
    if (current_process->PC >= current_process->memory_limit) {
        show_process_state(current_process->pid, "RUNNING", "FINISHED");
        lock_scheduler(cpu);
        current_process->state = FINISHED;
        current_process->was_completed = true;
        current_process->total_instructions = current_process->PC;
        current_process->cycles_executed = cycle_count;
        release_core(cpu, core_id);
        unlock_scheduler(cpu);
        return;
    }

    // Fetch Stage
    char* instruction = get_line_of_program(
        memory_ram->vector + current_process->base_address,
        current_process->PC
    );
    
    if (!instruction || strlen(instruction) == 0) {
        show_process_state(current_process->pid, "RUNNING", "FINISHED");
        lock_scheduler(cpu);
        current_process->state = FINISHED;
        current_process->was_completed = true;
        current_process->cycles_executed = cycle_count;
        release_core(cpu, core_id);
        unlock_scheduler(cpu);
        free(instruction);
        return;
    }

    // Execute Stage
    show_cpu_execution(core_id, current_process->pid, instruction, current_process->PC);
    current_process->total_instructions++;
    
    if (strncmp(instruction, "LOAD", 4) == 0) {
        load(cpu, instruction);
        show_memory_operation(current_process->pid, "LOAD", current_process->PC);
    } else if (strncmp(instruction, "STORE", 5) == 0) {
        store(cpu, memory_ram, instruction);
        show_memory_operation(current_process->pid, "STORE", current_process->PC);
    } else if (strncmp(instruction, "ADD", 3) == 0) {
        add(cpu, instruction);
    }

    // Atualiza estado
    current_process->PC++;
    current_core->quantum_remaining--;
    current_process->cycles_executed = cycle_count;

    // Verifica quantum
    if (current_core->quantum_remaining <= 0) {
        show_process_state(current_process->pid, "RUNNING", "READY");
        lock_scheduler(cpu);
        current_process->state = READY;
        cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = current_process;
        release_core(cpu, core_id);
        unlock_scheduler(cpu);
    }

    free(instruction);
    // show_scheduler_state(cpu->process_manager->ready_count,
    //                     cpu->process_manager->blocked_count);
}

void print_execution_summary(architecture_state* state, cpu* cpu, int cycle_count) {
    show_system_summary(cycle_count, total_processes, 0, 0);
    
    int total_instructions = 0;
    int processes_completed = 0;
    int processes_preempted = 0;
    int processes_blocked = 0;
    
    // Conta estatísticas dos processos
    for (int i = 0; i < total_processes; i++) {
        PCB* process = all_processes[i];
        if (process) {
            total_instructions += process->total_instructions;
            
            if (process->was_completed) {
                processes_completed++;
                show_process_state(process->pid, "COMPLETED", 
                                 state_to_string(process->state));
            } else if (process->state == BLOCKED) {
                processes_blocked++;
            } else if (process->state == READY) {
                processes_preempted++;
            }
        }
    }

    // Estatísticas da fila de bloqueados
    for (int i = 0; i < state->process_manager->blocked_count; i++) {
        PCB* process = state->process_manager->blocked_queue[i];
        if (process) {
            processes_blocked++;
            total_instructions += process->PC;
            show_process_state(process->pid, "BLOCKED", 
                             state_to_string(process->state));
        }
    }

    // Estado dos cores
    int core_pids[NUM_CORES] = {-1};
    for (int i = 0; i < NUM_CORES; i++) {
        PCB* process = cpu->core[i].current_process;
        if (process) {
            core_pids[i] = process->pid;
            total_instructions += process->PC;
            if (process->state == FINISHED) {
                processes_completed++;
            }
        }
    }
    show_cores_state(NUM_CORES, core_pids);

    // Mostra resumo final
    show_system_summary(cycle_count, total_processes, processes_completed, 
                       total_instructions);
    show_scheduler_state(processes_preempted, processes_blocked);
}

void load_program_on_ram(ram* memory_ram, char* program, int base_address) {
    if (!program || !memory_ram->vector) return;
    
    size_t program_len = strlen(program);
    show_memory_operation(-1, "Loading Program", base_address);
    
    if (base_address + program_len >= NUM_MEMORY) {
        show_memory_operation(-1, "ERROR: Memory Overflow", NUM_MEMORY);
        exit(1);
    }
    
    // Copia programa para memória
    memset(memory_ram->vector + base_address, 0, program_len + 1);
    memcpy(memory_ram->vector + base_address, program, program_len);
    memory_ram->vector[base_address + program_len] = '\0';
    
    // Conta instruções
    int num_lines = 0;
    for (char* p = memory_ram->vector + base_address; *p; p++) {
        if (*p == '\n') num_lines++;
    }
    
    show_memory_operation(-1, "Program Loaded", base_address);
}

void check_instructions_on_ram(ram* memory_ram) {
    if (!memory_ram || !memory_ram->vector) return;
    
    unsigned short int num = count_lines(memory_ram->vector);
    for (unsigned short int i = 0; i < num; i++) {
        char* line = get_line_of_program(memory_ram->vector, i);
        if (line) {
            verify_instruction(line, i);
            free(line);
        }
    }
}

void free_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                      peripherals* peripherals, architecture_state* state) {
    show_system_summary(0, total_processes, 0, 0);

    // Libera process manager
    if (state && state->process_manager) {
        for (int i = 0; i < state->process_manager->ready_count; i++) {
            if (state->process_manager->ready_queue[i]) {
                free(state->process_manager->ready_queue[i]->registers);
                free(state->process_manager->ready_queue[i]);
            }
        }
        free(state->process_manager->ready_queue);

        for (int i = 0; i < state->process_manager->blocked_count; i++) {
            if (state->process_manager->blocked_queue[i]) {
                free(state->process_manager->blocked_queue[i]->registers);
                free(state->process_manager->blocked_queue[i]);
            }
        }
        free(state->process_manager->blocked_queue);
        free(state->process_manager);
    }

    // Libera componentes
    if (state && state->pipeline) free(state->pipeline);
    if (memory_ram) free(memory_ram);
    if (memory_disc) free(memory_disc);
    if (peripherals) free(peripherals);
    
    if (cpu) {
        for (int i = 0; i < NUM_CORES; i++) {
            free(cpu->core[i].registers);
        }
        free(cpu->core);
        free(cpu);
    }

    free(state);
    show_system_summary(0, 0, 0, 0);
}