#include "pipeline.h"
#include "os_display.h"
#include "instruction_utils.h"
#include "architecture_state.h"

void init_pipeline(pipeline* p) {
    // Inicialização do mutex global do pipeline
    pthread_mutex_init(&p->pipeline_mutex, NULL);
    
    reset_pipeline_stage(&p->IF);
    reset_pipeline_stage(&p->ID);
    reset_pipeline_stage(&p->EX);
    reset_pipeline_stage(&p->MEM);
    reset_pipeline_stage(&p->WB);
    
    p->current_core = 0;
}

void reset_pipeline_stage(pipeline_stage* stage) {
    stage->instruction = NULL;
    stage->type = 0;
    stage->core_id = -1;
    stage->is_stalled = false;
    stage->is_busy = false;
    pthread_mutex_init(&stage->stage_mutex, NULL);  // Inicializa mutex do estágio
}

void handle_memory_stage(type_of_instruction type) {
    if (type == STORE) {
        show_pipeline_memory("Escrita na memória");
    } else if (type == LOAD) {
        show_pipeline_memory("Leitura da memória");
    } else {
        show_pipeline_memory(NULL);
    }
}

void handle_writeback_stage(type_of_instruction type) {
    if (type == ADD || type == SUB || type == MUL || 
        type == DIV || type == LOAD) {
        show_pipeline_writeback("Atualizando registrador");
    } else {
        show_pipeline_writeback(NULL);
    }
}

void update_process_state(architecture_state* state, PCB* current_process, 
                         core* current_core, cpu* cpu, int core_id, int cycle_count) {
    pthread_mutex_lock(&state->global_mutex);
    current_process->PC++;
    current_process->total_instructions++;
    state->total_instructions++;
    pthread_mutex_unlock(&state->global_mutex);

    current_core->quantum_remaining--;
    current_process->cycles_executed = cycle_count;

    if (current_core->quantum_remaining <= 0) {
        show_process_state(current_process->pid, 
                         "RUNNING", 
                         "READY (Quantum expirado)");
        
        pthread_mutex_lock(&state->global_mutex);
        state->context_switches++;
        pthread_mutex_unlock(&state->global_mutex);
        
        current_process->state = READY;
        cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = current_process;
        release_core(cpu, core_id);
    }
}

void execute_instruction(cpu* cpu, ram* memory_ram, const char* instruction,
                       type_of_instruction type, int core_id,
                       instruction_processor* instr_processor, const char* program) {
    switch (type) {
        case LOAD:
            load(cpu, instruction, core_id);
            break;
        case STORE:
            store(cpu, memory_ram, instruction, core_id);
            break;
        case ADD:
            add(cpu, instruction, core_id);
            break;
        case SUB:
            sub(cpu, instruction, core_id);
            break;
        case MUL:
            mul(cpu, instruction, core_id);
            break;
        case DIV:
            div_c(cpu, instruction, core_id);
            break;
        case LOOP:
            loop(cpu, instr_processor, core_id);
            break;
        case IF:
            if_i(cpu, (char*)program, instr_processor, core_id);  
            break;
        case ELSE:
            else_i(cpu, (char*)program, instr_processor, core_id);  
            break;
        case L_END:
            loop_end(cpu, instr_processor, core_id);
            break;
        case I_END:
            if_end(instr_processor);
            break;
        case ELS_END:
            else_end(instr_processor);
            break;
        default:
            break;
    }
}

// Função auxiliar para lidar com término do processo
static void handle_process_completion(architecture_state* state, cpu* cpu, 
                                   PCB* process, int core_id, int cycle_count,
                                   char* instruction) {
    pthread_mutex_lock(&state->global_mutex);
    state->completed_processes++;
    pthread_mutex_unlock(&state->global_mutex);
    
    show_process_state(process->pid, "RUNNING", "FINISHED");
    process->state = FINISHED;
    process->was_completed = true;
    process->cycles_executed = cycle_count;
    
    release_core(cpu, core_id);
    if (instruction) free(instruction);
}

void execute_pipeline_cycle(architecture_state* state, cpu* cpu, 
                          ram* memory_ram, int core_id, int cycle_count) {
    
    // Lock global do pipeline
    pthread_mutex_lock(&state->pipeline->pipeline_mutex);
    show_pipeline_start(cycle_count, core_id, -1);

    core* current_core = &cpu->core[core_id];
    if (!current_core) {
        show_core_state(core_id, -1, "ERRO: Core inválido");
        pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
        return;
    }

    // Preparação e sincronização inicial
    lock_process_manager(cpu->process_manager);
    PCB* current_process = current_core->current_process;
    
    if (!current_process) {
        show_core_state(core_id, -1, "Ocioso");
        unlock_process_manager(cpu->process_manager);
        pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
        return;
    }

    show_pipeline_start(cycle_count, core_id, current_process->pid);

    // Verificação de recursos e término
    if (current_process->waiting_resource) {
        pthread_cond_wait(&cpu->process_manager->resource_condition, 
                         &cpu->process_manager->resource_mutex);
    }

    if (current_process->PC >= current_process->memory_limit) {
        handle_process_completion(state, cpu, current_process, core_id, cycle_count, NULL);
        unlock_process_manager(cpu->process_manager);
        pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
        return;
    }

    // 1. Instruction Fetch Stage
    pthread_mutex_lock(&state->pipeline->IF.stage_mutex);
    char* instruction = NULL;
    
    lock_memory(cpu);
    instruction = get_line_of_program(
        memory_ram->vector + current_process->base_address,
        current_process->PC
    );
    unlock_memory(cpu);

    if (!instruction || strlen(instruction) == 0) {
        handle_process_completion(state, cpu, current_process, core_id, cycle_count, instruction);
        pthread_mutex_unlock(&state->pipeline->IF.stage_mutex);
        unlock_process_manager(cpu->process_manager);
        pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
        return;
    }

    show_pipeline_fetch(instruction);
    state->pipeline->IF.instruction = instruction;
    pthread_mutex_unlock(&state->pipeline->IF.stage_mutex);

    // 2. Instruction Decode Stage
    pthread_mutex_lock(&state->pipeline->ID.stage_mutex);
    type_of_instruction instr_type = decode_instruction(instruction);
    state->pipeline->ID.type = instr_type;
    show_pipeline_decode(get_instruction_name(instr_type));
    pthread_mutex_unlock(&state->pipeline->ID.stage_mutex);

    // 3. Execute Stage
    pthread_mutex_lock(&state->pipeline->EX.stage_mutex);
    instruction_processor instr_processor = {0};
    char* program = NULL;
    execute_instruction(cpu, memory_ram, instruction, instr_type, core_id, &instr_processor, program);
    show_pipeline_execute(get_instruction_name(instr_type));
    pthread_mutex_unlock(&state->pipeline->EX.stage_mutex);

    // 4. Memory Access Stage
    pthread_mutex_lock(&state->pipeline->MEM.stage_mutex);
    handle_memory_stage(instr_type);
    pthread_mutex_unlock(&state->pipeline->MEM.stage_mutex);

    // 5. Write Back Stage
    pthread_mutex_lock(&state->pipeline->WB.stage_mutex);
    handle_writeback_stage(instr_type);
    pthread_mutex_unlock(&state->pipeline->WB.stage_mutex);

    show_pipeline_end();

    // Atualização de estado e estatísticas
    update_process_state(state, current_process, current_core, cpu, core_id, cycle_count);

    // Cleanup
    if (instruction) {
        free(instruction);
    }
    
    unlock_process_manager(cpu->process_manager);
    pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
}

// Funções auxiliares do pipeline
type_of_instruction decode_instruction(const char* instruction) {
    if (!instruction) return INVALID;
    
    if (strncmp(instruction, "LOAD", 4) == 0) return LOAD;
    if (strncmp(instruction, "STORE", 5) == 0) return STORE;
    if (strncmp(instruction, "ADD", 3) == 0) return ADD;
    if (strncmp(instruction, "SUB", 3) == 0) return SUB;
    if (strncmp(instruction, "MUL", 3) == 0) return MUL;
    if (strncmp(instruction, "DIV", 3) == 0) return DIV;
    if (strncmp(instruction, "LOOP", 4) == 0) return LOOP;
    if (strncmp(instruction, "IF", 2) == 0) return IF;
    if (strncmp(instruction, "ELSE", 4) == 0) return ELSE;
    if (strncmp(instruction, "I_END", 5) == 0) return I_END;
    if (strncmp(instruction, "L_END", 5) == 0) return L_END;
    if (strncmp(instruction, "ELS_END", 7) == 0) return ELS_END;
    
    return INVALID;
}

const char* get_instruction_name(type_of_instruction type) {
    switch (type) {
        case LOAD: return "LOAD";
        case STORE: return "STORE";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case LOOP: return "LOOP";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case I_END: return "I_END";
        case L_END: return "L_END";
        case ELS_END: return "ELS_END";
        default: return "INVALID";
    }
}



void lock_pipeline_stage(pipeline_stage* stage) {
    if (stage) {
        pthread_mutex_lock(&stage->stage_mutex);
    }
}

void unlock_pipeline_stage(pipeline_stage* stage) {
    if (stage) {
        pthread_mutex_unlock(&stage->stage_mutex);
    }
}

void cleanup_pipeline(pipeline* p) {
    if (!p) return;
    
    // Destruir mutexes dos estágios
    pthread_mutex_destroy(&p->IF.stage_mutex);
    pthread_mutex_destroy(&p->ID.stage_mutex);
    pthread_mutex_destroy(&p->EX.stage_mutex);
    pthread_mutex_destroy(&p->MEM.stage_mutex);
    pthread_mutex_destroy(&p->WB.stage_mutex);
    
    // Destruir mutex global do pipeline
    pthread_mutex_destroy(&p->pipeline_mutex);
}