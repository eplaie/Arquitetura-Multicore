#include "pipeline.h"
#include "os_display.h"

void init_pipeline(pipeline* p) {
    if (!p) return;

    reset_pipeline_stage(&p->IF);
    reset_pipeline_stage(&p->ID);
    reset_pipeline_stage(&p->EX);
    reset_pipeline_stage(&p->MEM);
    reset_pipeline_stage(&p->WB);
    p->current_core = 0;

    show_system_summary(0, 0, 0, 0);  // Estado inicial do pipeline
}

void reset_pipeline_stage(pipeline_stage* stage) {
    if (!stage) return;

    stage->instruction = NULL;
    stage->type = INVALID;
    stage->core_id = -1;
    stage->is_stalled = false;
    stage->is_busy = false;
}

char* instruction_fetch(cpu* cpu, ram* memory, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) return NULL;

    core* current_core = &cpu->core[core_id];
    if (!current_core->is_available || !current_core->current_process) return NULL;

    PCB* current_process = current_core->current_process;

    if (check_quantum_expired(cpu, core_id)) {
        show_process_state(current_process->pid, "RUNNING", "READY");
        handle_preemption(cpu, core_id);
        return NULL;
    }

    char* instruction = get_line_of_program(
        memory->vector + current_process->base_address,
        current_process->PC
    );

    if (instruction) {
        show_cpu_execution(core_id, current_process->pid, instruction, current_process->PC);
    }

    return instruction;
}

void execute(cpu* cpu, instruction_pipe* p, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) return;

    core* current_core = &cpu->core[core_id];
    if (!current_core->is_available || !current_core->current_process) return;

    PCB* current_process = current_core->current_process;

    if (check_quantum_expired(cpu, core_id)) {
        show_process_state(current_process->pid, "RUNNING", "READY");
        handle_preemption(cpu, core_id);
        return;
    }

    // Executa a instrução
    show_cpu_execution(core_id, current_process->pid, p->instruction, current_process->PC);
    control_unit(cpu, p);

    // Atualiza quantum
    if (current_process) {
        current_core->quantum_remaining--;
        show_scheduler_state(cpu->process_manager->ready_count,
                           cpu->process_manager->blocked_count);
    }
}

void memory_access(cpu* cpu, ram* memory_ram, type_of_instruction type, char* instruction, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES || !instruction) return;

    core* current_core = &cpu->core[core_id];
    if (!current_core->is_available || !current_core->current_process) return;

    PCB* current_process = current_core->current_process;

    // Verifica limites de memória
    if (type == STORE || type == LOAD) {
        char* token = strtok(strdup(instruction), " ");
        token = strtok(NULL, " "); // Skip instruction name
        token = strtok(NULL, " "); // Get address

        unsigned short int address = atoi(token + 1); // Remove 'A' prefix

        if (address < current_process->base_address ||
            address >= (current_process->base_address + current_process->memory_limit)) {
            show_memory_operation(current_process->pid, "ACCESS VIOLATION", address);
            return;
        }

        show_memory_operation(current_process->pid,
                            type == STORE ? "STORE" : "LOAD",
                            address);
    }

    // Executa acesso
    if (type == STORE) {
        store(cpu, memory_ram, instruction);
    } else if (type == LOAD) {
        load(cpu, instruction);
    }
}

void write_back(cpu* cpu, type_of_instruction type, char* instruction, unsigned short int result, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) return;

    core* current_core = &cpu->core[core_id];
    if (!current_core->is_available || !current_core->current_process) return;

    if (type == ADD || type == SUB || type == MUL || type == DIV) {
        char* instr_copy = strdup(instruction);
        char* token = strtok(instr_copy, " ");
        token = strtok(NULL, " "); // Get destination register

        unsigned short int reg_index = get_register_index(token);
        current_core->registers[reg_index] = result;

        show_cpu_execution(core_id, current_core->current_process->pid,
                          "WRITE_BACK", current_core->PC);

        free(instr_copy);
    }
}

void check_pipeline_preemption(pipeline* p, cpu* cpu) {
    if (!p || !cpu) return;

    int core_id = p->current_core;
    core* current_core = &cpu->core[core_id];

    if (!current_core->is_available || !current_core->current_process) return;

    if (current_core->quantum_remaining <= 0) {
        PCB* current_process = current_core->current_process;

        show_process_state(current_process->pid, "RUNNING", "READY");

        lock_scheduler(cpu);

        current_process->state = READY;
        cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = current_process;
        release_core(cpu, core_id);

        unlock_scheduler(cpu);

        // Limpa pipeline
        reset_pipeline_stage(&p->IF);
        reset_pipeline_stage(&p->ID);
        reset_pipeline_stage(&p->EX);
        reset_pipeline_stage(&p->MEM);
        reset_pipeline_stage(&p->WB);

        // show_scheduler_state(cpu->process_manager->ready_count,
        //                    cpu->process_manager->blocked_count);
    }
}

void switch_pipeline_context(pipeline* p, cpu* cpu, int new_core_id) {
    if (!p || !cpu || new_core_id < 0 || new_core_id >= NUM_CORES) return;

    // Salva contexto atual
    if (p->current_core >= 0 && p->current_core < NUM_CORES) {
        core* current_core = &cpu->core[p->current_core];
        if (current_core->current_process) {
            show_process_state(current_core->current_process->pid,
                             "RUNNING", "SWITCHING");
            save_context(current_core->current_process, current_core);
        }
    }

    show_cores_state(NUM_CORES, NULL);
    p->current_core = new_core_id;

    // Limpa pipeline para novo contexto
    reset_pipeline_stage(&p->IF);
    reset_pipeline_stage(&p->ID);
    reset_pipeline_stage(&p->EX);
    reset_pipeline_stage(&p->MEM);
    reset_pipeline_stage(&p->WB);
}