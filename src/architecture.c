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

void init_pipeline_multicore(architecture_state* state, cpu* cpu, ram* memory_ram __attribute__((unused))) {
    state->pipeline->current_core = 0;
    
    // Cria processo inicial
    PCB* initial_process = create_process(cpu->process_manager);
    initial_process->base_address = 0;
    initial_process->memory_limit = NUM_MEMORY;
    
    // Atribui processo ao primeiro core
    assign_process_to_core(cpu, initial_process, 0);

    printf("Pipeline initialized with multicore support\n");
}

void execute_pipeline_cycle(architecture_state* state, cpu* cpu, ram* memory_ram, int core_id) {

    check_pipeline_preemption(state->pipeline, cpu);
    
    if (!cpu->core[core_id].is_available || cpu->core[core_id].current_process == NULL) {
        return;
    }

    // Estágio IF
    char* instruction = instruction_fetch(cpu, memory_ram, core_id);
    if (instruction == NULL) return;

    // Estágio ID
    type_of_instruction type = instruction_decode(cpu, instruction, cpu->core[core_id].PC);

    // Prepara dados do pipe
    instruction_pipe pipe_data = {
        .instruction = instruction,
        .type = type,
        .mem_ram = memory_ram,
        .num_instruction = cpu->core[core_id].PC,
        .loop = false,
        .has_if = false,
        .valid_if = false,
        .running_if = false
    };

    // Executa estágios
    execute(cpu, &pipe_data, core_id);
    memory_access(cpu, memory_ram, type, instruction, core_id);
    write_back(cpu, type, instruction, pipe_data.result, core_id);

    // Atualiza o quantum do processo atual
    if (cpu->core[core_id].current_process) {
        cpu->core[core_id].quantum_remaining--;
    }
}

void load_program_on_ram(ram* memory_ram, char* program) {
    unsigned short int num_caracters = strlen(program);

    if (num_caracters >= NUM_MEMORY) {
        printf("Error: Program size exceeds RAM capacity.\n");
        exit(1);
    }

    for (unsigned short int i = 0; i < num_caracters; i++) {
        memory_ram->vector[i] = program[i];
    }
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