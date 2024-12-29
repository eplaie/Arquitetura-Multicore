#include "architecture.h"

void init_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, peripherals* peripherals, architecture_state* state) {
    if (!cpu || !memory_ram || !memory_disc || !peripherals || !state) {
        printf("Error: NULL pointer in init_architecture\n");
        exit(1);
    }

    // Inicializa memória RAM primeiro
    memory_ram->vector = (char*)calloc(NUM_MEMORY, sizeof(char));
    if (memory_ram->vector == NULL) {
        printf("Failed to allocate RAM memory\n");
        exit(1);
    }

    // Inicializa CPU
    init_cpu(cpu);
    printf("CPU initialized with %d cores\n", NUM_CORES);

    // Inicializa disco
    init_disc(memory_disc);
    printf("Disc initialized\n");

    // Inicializa periféricos
    init_peripherals(peripherals);
    printf("Peripherals initialized\n");

    // Inicializa estado da arquitetura
    state->pipeline = malloc(sizeof(pipeline));
    if (state->pipeline == NULL) {
        printf("Failed to allocate pipeline\n");
        exit(1);
    }
    init_pipeline(state->pipeline);
    printf("Pipeline initialized\n");

    // Inicializa gerenciador de processos
    state->process_manager = init_process_manager(DEFAULT_QUANTUM);
    state->program_running = true;
    printf("Process manager initialized\n");

    // Configura CPU para trabalhar com o process manager
    cpu->process_manager = state->process_manager;
    printf("Architecture initialization complete\n");
}

void init_pipeline_multicore(architecture_state* state, cpu* cpu __attribute__((unused)), 
                           ram* memory_ram __attribute__((unused))) {
    state->pipeline->current_core = 0;
    printf("Pipeline initialized with multicore support\n");
}

void execute_pipeline_cycle(architecture_state* state __attribute__((unused)), 
                          cpu* cpu, ram* memory_ram, int core_id, int cycle_count) {
    printf("\n=== Pipeline Execution Debug (Cycle %d) ===\n", cycle_count);
    
    core* current_core = &cpu->core[core_id];
    if (!current_core) {
        printf("Error: Invalid core\n");
        return;
    }

    PCB* current_process = current_core->current_process;
    if (!current_process) {
        printf("Debug - Core %d has no process\n", core_id);
        return;
    }

    printf("Process %d on Core %d:\n", current_process->pid, core_id);
    printf("  PC: %d\n", current_process->PC);
    printf("  Base Address: %d\n", current_process->base_address);
    printf("  Memory Limit: %d\n", current_process->memory_limit);
    printf("  Quantum: %d\n", current_core->quantum_remaining);
    printf("  Cycles Executed: %d\n", cycle_count);

    // Verifica se o processo terminou
    if (current_process->PC >= current_process->memory_limit) {
        printf("Process %d completed after %d instructions (at cycle %d)\n", 
               current_process->pid, current_process->total_instructions, cycle_count);
        lock_scheduler(cpu);
        current_process->state = FINISHED;
        current_process->was_completed = true;
        current_process->total_instructions = current_process->PC;
        current_process->cycles_executed = cycle_count;
        release_core(cpu, core_id);
        unlock_scheduler(cpu);
        return;
    }

    char* instruction = NULL;
    printf("\n- Fetch Stage -\n");
    
    instruction = get_line_of_program(
        memory_ram->vector + current_process->base_address,
        current_process->PC
    );
    
    if (!instruction || strlen(instruction) == 0) {
        printf("Process %d reached end of instructions at cycle %d\n", 
               current_process->pid, cycle_count);
        lock_scheduler(cpu);
        current_process->state = FINISHED;
        current_process->was_completed = true;
        current_process->cycles_executed = cycle_count;
        release_core(cpu, core_id);
        unlock_scheduler(cpu);
        if (instruction) {
            free(instruction);
        }
        return;
    }

    printf("Fetched: '%s'\n", instruction);
    current_process->total_instructions++;
    
    // Executa a instrução
    printf("Executing instruction: %s\n", instruction);
    
    if (strncmp(instruction, "LOAD", 4) == 0) {
        load(cpu, instruction);
    } else if (strncmp(instruction, "STORE", 5) == 0) {
        store(cpu, memory_ram, instruction);
    } else if (strncmp(instruction, "ADD", 3) == 0) {
        add(cpu, instruction);
    }

    // Atualiza PC e quantum
    current_process->PC++;
    current_core->quantum_remaining--;
    current_process->cycles_executed = cycle_count;

    printf("Updated - PC: %d, Quantum: %d, Cycle: %d\n", 
           current_process->PC, current_core->quantum_remaining, cycle_count);

    // Verifica quantum
    if (current_core->quantum_remaining <= 0) {
        printf("Quantum expired for Process %d at cycle %d\n", 
               current_process->pid, cycle_count);
        lock_scheduler(cpu);
        current_process->state = READY;
        cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = current_process;
        release_core(cpu, core_id);
        unlock_scheduler(cpu);
    }

    if (instruction) {
        free(instruction);
    }
}

void print_execution_summary(architecture_state* state, cpu* cpu, ram* memory_ram, int cycle_count) {
    printf("\n================ Execution Summary ================\n");
    
    // Informação sobre ciclos
    printf("Status: %s\n", 
           cycle_count >= MAX_CYCLES ? "Stopped at maximum cycle count" : "Completed all processes");
    printf("Total cycles executed: %d\n\n", cycle_count);
    
    // Contadores
    int total_instructions = 0;
    int processes_completed = 0;
    int processes_preempted = 0;
    int processes_blocked = 0;
    
    for (int i = 0; i < total_processes; i++) {
        PCB* process = all_processes[i];
        if (process) {
            total_instructions += process->total_instructions;
            
            if (process->was_completed) {
                processes_completed++;
                printf("  Process %d - Completed - Instructions: %d, Cycles: %d\n",
                       process->pid, process->total_instructions, process->cycles_executed);
            } else if (process->state == BLOCKED) {
                processes_blocked++;
            } else if (process->state == READY) {
                processes_preempted++;
            }
        }
    }

    // Blocked Queue Statistics
    printf("\nBlocked Queue Statistics:\n");
    for (int i = 0; i < state->process_manager->blocked_count; i++) {
        PCB* process = state->process_manager->blocked_queue[i];
        if (process) {
            processes_blocked++;
            total_instructions += process->PC;
            printf("  Process %d - PC: %d, Instructions Executed: %d\n",
                   process->pid, process->PC, process->PC);
        }
    }

    // Core Status
    printf("\nCore Status:\n");
    for (int i = 0; i < NUM_CORES; i++) {
        PCB* process = cpu->core[i].current_process;
        if (process) {
            total_instructions += process->PC;
            if (process->state == FINISHED) {
                processes_completed++;
            }
            printf("  Core %d - Process %d (%s, Instructions: %d)\n",
                   i, process->pid, state_to_string(process->state), process->PC);
        } else {
            printf("  Core %d - Idle\n", i);
        }
    }

    printf("\nProcess Statistics:\n");
    printf("  Completed: %d\n", processes_completed);
    printf("  Preempted: %d\n", processes_preempted);
    printf("  Blocked: %d\n", processes_blocked);
    printf("  Total Processes: %d\n", processes_completed + processes_preempted + processes_blocked);
    
    printf("\nExecution Statistics:\n");
    printf("  Total Instructions: %d\n", total_instructions);
    printf("  Instructions per Cycle (IPC): %.2f\n", 
           cycle_count > 0 ? (float)total_instructions / cycle_count : 0);
    if (processes_completed > 0) {
        printf("  Average Instructions per Process: %.2f\n", 
               (float)total_instructions / processes_completed);
    }

    printf("\nFinal Memory State:\n");
    print_ram(memory_ram);
    printf("\n================================================\n");
}


void load_program_on_ram(ram* memory_ram, char* program, int base_address) {
    if (!program || !memory_ram->vector) return;
    
    size_t program_len = strlen(program);
    printf("Debug - Loading program of length %zu at base %d\n", program_len, base_address);
    
    // Verifica limites de memória
    if (base_address + program_len >= NUM_MEMORY) {
        printf("Error: Program would exceed RAM capacity\n");
        exit(1);
    }
    
    // Limpa área de destino
    size_t clear_size = program_len + 1;
    memset(memory_ram->vector + base_address, 0, clear_size);
    
    // Copia programa
    memcpy(memory_ram->vector + base_address, program, program_len);
    memory_ram->vector[base_address + program_len] = '\0';
    
    // Verifica conteúdo carregado
    printf("Debug - Verifying loaded content at base %d:\n", base_address);
    char* first_line = get_line_of_program(memory_ram->vector + base_address, 0);
    if (first_line) {
        printf("First instruction: '%s'\n", first_line);
        free(first_line);
    }
    
    // Conta número de linhas
    int num_lines = 0;
    char* p = memory_ram->vector + base_address;
    while (*p) {
        if (*p == '\n') num_lines++;
        p++;
    }
    printf("Program loaded with %d instructions\n", num_lines + 1);
}

void check_instructions_on_ram(ram* memory_ram) {
    char* line;
    unsigned short int num_line = 0;
    unsigned short int num = count_lines(memory_ram->vector);

    while (num_line < num) {
        line = get_line_of_program(memory_ram->vector, num_line);
        verify_instruction(line, num_line);
        num_line++;
    }
}

void free_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, peripherals* peripherals, architecture_state* state) {
    // Libera ProcessManager
    if (state->process_manager) {
        // Libera PCBs na fila de prontos
        for (int i = 0; i < state->process_manager->ready_count; i++) {
            if (state->process_manager->ready_queue[i]) {
                if (state->process_manager->ready_queue[i]->registers) {
                    free(state->process_manager->ready_queue[i]->registers);
                }
                free(state->process_manager->ready_queue[i]);
            }
        }
        free(state->process_manager->ready_queue);

        // Libera PCBs na fila de bloqueados
        for (int i = 0; i < state->process_manager->blocked_count; i++) {
            if (state->process_manager->blocked_queue[i]) {
                if (state->process_manager->blocked_queue[i]->registers) {
                    free(state->process_manager->blocked_queue[i]->registers);
                }
                free(state->process_manager->blocked_queue[i]);
            }
        }
        free(state->process_manager->blocked_queue);
        free(state->process_manager);
    }

    // Libera Pipeline
    if (state->pipeline) {
        free(state->pipeline);
    }

    // Libera as estruturas básicas
    if (memory_ram) {
        free(memory_ram);
    }
    if (memory_disc) {
        free(memory_disc);
    }
    if (peripherals) {
        free(peripherals);
    }
    if (cpu) {
        for (int i = 0; i < NUM_CORES; i++) {
            if (cpu->core[i].registers) {
                free(cpu->core[i].registers);
            }
        }
        if (cpu->core) {
            free(cpu->core);
        }
        free(cpu);
    }

    // Libera o estado
    free(state);
}