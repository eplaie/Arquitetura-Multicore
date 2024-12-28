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
    core* current_core = &cpu->core[core_id];
    PCB* current_process = current_core->current_process;
    
    if (!current_core->is_available || !current_process) {
        return;
    }

    printf("\nDebug - Pipeline cycle for process %d on core %d\n", 
           current_process->pid, core_id);
    
    if (current_process->PC >= current_process->memory_limit) {
        printf("Process %d completed\n", current_process->pid);
        current_process->state = FINISHED;
        release_core(cpu, core_id);
        return;
    }

    // Busca instrução usando PC relativo (não o endereço absoluto)
    char* instruction = get_line_of_program(
        memory_ram->vector + current_process->base_address, // Endereço base do programa
        current_process->PC  // PC relativo (0, 1, 2, etc.)
    );
    
    if (!instruction) {
        printf("Error: Could not fetch instruction for process %d (PC=%d, Base=%d)\n", 
               current_process->pid, current_process->PC, current_process->base_address);
        release_core(cpu, core_id);
        return;
    }

    // Executa instrução
    type_of_instruction type = verify_instruction(instruction, current_process->PC);
    
    printf("Core %d executing process %d:\n", core_id, current_process->pid);
    printf("  PC: %d\n", current_process->PC);
    printf("  Base Address: %d\n", current_process->base_address);
    printf("  Memory Limit: %d\n", current_process->memory_limit);
    printf("  Quantum Remaining: %d\n", current_core->quantum_remaining);
    printf("  Executing: %s\n", instruction);

    instruction_pipe pipe_data = {0};
    pipe_data.instruction = strdup(instruction);
    pipe_data.type = type;
    pipe_data.mem_ram = memory_ram;
    pipe_data.num_instruction = current_process->PC;

    // Executa e atualiza contadores
    control_unit(cpu, &pipe_data);
    current_core->quantum_remaining--;
    current_process->PC++;

    // Verifica quantum
    if (current_core->quantum_remaining <= 0) {
        printf("Process %d quantum expired on Core %d\n", current_process->pid, core_id);
        current_process->state = READY;
        lock_scheduler(cpu);
        cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = current_process;
        release_core(cpu, core_id);
        unlock_scheduler(cpu);
    }

    free(pipe_data.instruction);
    free(instruction);
}

void print_execution_summary(architecture_state* state, cpu* cpu) {
    printf("\n=== Execution Summary ===\n");
    
    // Contadores de estados
    int total_processes = 0;
    int completed = 0;
    int ready = 0;
    int blocked = 0;
    int running = 0;
    
    // Verifica processos na fila de prontos
    for (int i = 0; i < state->process_manager->ready_count; i++) {
        PCB* process = state->process_manager->ready_queue[i];
        total_processes++;
        if (process->state == READY) ready++;
    }
    
    // Verifica processos bloqueados
    for (int i = 0; i < state->process_manager->blocked_count; i++) {
        PCB* process = state->process_manager->blocked_queue[i];
        total_processes++;
        if (process->state == BLOCKED) blocked++;
    }
    
    // Verifica processos nos cores
    for (int i = 0; i < NUM_CORES; i++) {
        PCB* process = cpu->core[i].current_process;
        if (process) {
            total_processes++;
            if (process->state == FINISHED) completed++;
            else if (process->state == RUNNING) running++;
        }
    }
    
    printf("Total Processes: %d\n", total_processes);
    printf("Completed Processes: %d\n", completed);
    printf("Ready Processes: %d\n", ready);
    printf("Running Processes: %d\n", running);
    printf("Blocked Processes: %d\n", blocked);
    
    // Estatísticas por processo
    printf("\nPer-Process Statistics:\n");
    for (int i = 0; i < NUM_CORES; i++) {
        PCB* process = cpu->core[i].current_process;
        if (process) {
            printf("Process %d:\n", process->pid);
            printf("  State: %s\n", state_to_string(process->state));
            printf("  Core: %d\n", process->core_id);
            printf("  PC: %d\n", process->PC);
            printf("  Instructions Executed: %d\n", process->PC);
        }
    }
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