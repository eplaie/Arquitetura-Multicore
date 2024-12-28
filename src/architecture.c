#include "architecture.h"

void init_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, peripherals* peripherals, architecture_state* state) {
    // Inicializa componentes básicos
    init_cpu(cpu);
    init_ram(memory_ram);
    init_disc(memory_disc);
    init_peripherals(peripherals);

    // Inicializa estado da arquitetura
    state->pipeline = malloc(sizeof(pipeline));
    if (state->pipeline == NULL) {
        printf("Failed to allocate pipeline\n");
        exit(1);
    }
    init_pipeline(state->pipeline);

    // Inicializa gerenciador de processos
    state->process_manager = init_process_manager(DEFAULT_QUANTUM);
    state->program_running = true;

    // Configura CPU para trabalhar com o process manager
    cpu->process_manager = state->process_manager;
}

void init_pipeline_multicore(architecture_state* state, cpu* cpu __attribute__((unused)), 
                           ram* memory_ram __attribute__((unused))) {
    state->pipeline->current_core = 0;
    printf("Pipeline initialized with multicore support\n");
}

void execute_pipeline_cycle(architecture_state* state __attribute__((unused)), 
                          cpu* cpu, ram* memory_ram, int core_id) {
    printf("\n=== Pipeline Execution Debug ===\n");
    
    core* current_core = &cpu->core[core_id];
    PCB* current_process = current_core->current_process;
    
    if (current_process == NULL) {
        printf("Debug - Core %d has no process\n", core_id);
        return;
    }

    printf("Process %d on Core %d:\n", current_process->pid, core_id);
    printf("  PC: %d\n", current_process->PC);
    printf("  Base Address: %d\n", current_process->base_address);
    printf("  Memory Limit: %d\n", current_process->memory_limit);
    printf("  Quantum: %d\n", current_core->quantum_remaining);

    // Verifica se o processo terminou
    if (current_process->PC >= current_process->memory_limit) {
        printf("Process %d completed all instructions\n", current_process->pid);
        current_process->state = FINISHED;
        release_core(cpu, core_id);
        return;
    }

    // Fetch Stage
    printf("\n- Fetch Stage -\n");
    char* instruction = get_line_of_program(
        memory_ram->vector + current_process->base_address,
        current_process->PC
    );
    
    if (!instruction || strlen(instruction) == 0) {
        printf("Process %d reached end of instructions\n", current_process->pid);
        current_process->state = FINISHED;
        release_core(cpu, core_id);
        if (instruction) free(instruction);
        return;
    }

    printf("Fetched: '%s'\n", instruction);

    // Execute e atualiza contadores
    printf("Executing instruction: %s\n", instruction);
    if (strncmp(instruction, "LOAD", 4) == 0) {
        load(cpu, instruction);
    } else if (strncmp(instruction, "STORE", 5) == 0) {
        store(cpu, memory_ram, instruction);
    } else if (strncmp(instruction, "ADD", 3) == 0) {
        add(cpu, instruction);
    }

    // Atualiza PC e quantum
    current_core->quantum_remaining--;
    current_process->PC++;
    printf("Updated - PC: %d, Quantum: %d\n", 
           current_process->PC, current_core->quantum_remaining);

    // Verifica quantum
    if (current_core->quantum_remaining <= 0) {
        printf("Quantum expired for Process %d\n", current_process->pid);
        current_process->state = READY;
        lock_scheduler(cpu);
        cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = current_process;
        release_core(cpu, core_id);
        unlock_scheduler(cpu);
    }

    free(instruction);

    // Adicione verificação de bloqueio por I/O em STORE
if (strncmp(instruction, "STORE", 5) == 0) {
    // Simula chance de bloqueio por I/O
    if (rand() % 10 == 0) {  // 10% de chance de bloqueio
        printf("Process %d blocked waiting for I/O\n", current_process->pid);
        current_process->state = BLOCKED;
        lock_scheduler(cpu);
        cpu->process_manager->blocked_queue[cpu->process_manager->blocked_count++] = current_process;
        release_core(cpu, core_id);
        unlock_scheduler(cpu);
        free(instruction);
        return;
    }
    store(cpu, memory_ram, instruction);
}
}

void print_execution_summary(architecture_state* state, cpu* cpu, ram* memory_ram, int cycle_count) {
    printf("\n====== Execution Summary ======\n");
    
    // Estatísticas básicas
    printf("Total cycles executed: %d\n", cycle_count);
    
    // Contadores de processos por estado
    int processes_completed = 0;
    int processes_preempted = 0;
    int processes_blocked = 0;
    int total_instructions = 0;

    // Verifica processos na fila de prontos
    for (int i = 0; i < state->process_manager->ready_count; i++) {
        PCB* process = state->process_manager->ready_queue[i];
        if (process->state == READY) processes_preempted++;
        total_instructions += process->PC;  // Conta instruções já executadas
    }

    // Verifica processos na fila de bloqueados
    for (int i = 0; i < state->process_manager->blocked_count; i++) {
        PCB* process = state->process_manager->blocked_queue[i];
        if (process->state == BLOCKED) processes_blocked++;
        total_instructions += process->PC;
    }

    // Verifica processos nos cores
    for (int i = 0; i < NUM_CORES; i++) {
        PCB* process = cpu->core[i].current_process;
        if (process) {
            if (process->state == FINISHED) processes_completed++;
            total_instructions += process->PC;
        }
    }

    printf("Process Statistics:\n");
    printf("  Completed: %d\n", processes_completed);
    printf("  Preempted: %d\n", processes_preempted);
    printf("  Blocked: %d\n", processes_blocked);
    printf("\nExecution Statistics:\n");
    printf("  Total Instructions: %d\n", total_instructions);
    printf("  Average Instructions per Cycle: %.2f\n", 
           (float)total_instructions / cycle_count);
    
    if (processes_completed > 0) {
        printf("  Average Cycles per Process: %.2f\n", 
               (float)cycle_count / processes_completed);
    }

    // Estado final da RAM
    printf("\nFinal Memory State:\n");
    print_ram(memory_ram);
    printf("\n==========================\n");
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