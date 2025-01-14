#include "pipeline.h"
#include "os_display.h"
#include "instruction_utils.h"

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
    pthread_mutex_init(&stage->stage_mutex, NULL);
}

static void handle_process_completion(architecture_state* state, cpu* cpu, 
                                   PCB* process, int core_id, int cycle_count,
                                   char* instruction) {
    printf("\n[Core %d] Processo %d completado", core_id, process->pid);
    
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
    // Verificações iniciais com logs detalhados
    printf("\n[Core %d] Iniciando ciclo de execução %d", core_id, cycle_count);
    printf("\n[Core %d] Verificando estado inicial:", core_id);
    printf("\n - state: %p", (void*)state);
    printf("\n - cpu: %p", (void*)cpu);
    printf("\n - memory_ram: %p", (void*)memory_ram);
    printf("\n - memory_ram->vector: %p", memory_ram ? (void*)memory_ram->vector : NULL);

    if (!state || !cpu || !memory_ram || !memory_ram->vector) {
        printf("\n[Core %d] ERRO: Parâmetros inválidos, abortando ciclo", core_id);
        return;
    }
    
    // Lock global do pipeline
    pthread_mutex_lock(&state->pipeline->pipeline_mutex);
    show_pipeline_start(cycle_count, core_id, -1);

    core* current_core = &cpu->core[core_id];
    if (!current_core) {
        printf("\n[Core %d] ERRO: Core inválido", core_id);
        show_core_state(core_id, -1, "ERRO: Core inválido");
        pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
        return;
    }

    // Preparação e sincronização inicial
    printf("\n[Core %d] Obtendo processo atual", core_id);
    lock_process_manager(cpu->process_manager);
    PCB* current_process = current_core->current_process;
    
    if (!current_process) {
        printf("\n[Core %d] Core ocioso - sem processo", core_id);
        show_core_state(core_id, -1, "Ocioso");
        unlock_process_manager(cpu->process_manager);
        pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
        return;
    }

    printf("\n[Core %d] Executando processo %d", core_id, current_process->pid);
    printf("\n - PC: %d", current_process->PC);
    printf("\n - Base: %d", current_process->base_address);
    printf("\n - Limite: %d", current_process->memory_limit);
    printf("\n - RAM atual: %p", (void*)memory_ram->vector);
    show_pipeline_start(cycle_count, core_id, current_process->pid);

    // Verifica se processo terminou
    if (current_process->PC >= current_process->memory_limit) {
        printf("\n[Core %d] Processo %d atingiu limite de memória", core_id, current_process->pid);
        printf("\n - PC atual: %d", current_process->PC);
        printf("\n - Limite: %d", current_process->memory_limit);
        handle_process_completion(state, cpu, current_process, core_id, cycle_count, NULL);
        unlock_process_manager(cpu->process_manager);
        pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
        return;
    }

    // 1. Instruction Fetch Stage
    printf("\n[Core %d] Iniciando Fetch", core_id);
    pthread_mutex_lock(&state->pipeline->IF.stage_mutex);
    char* instruction = NULL;
    
    // Verificação da RAM antes do fetch
    if (!memory_ram->vector) {
        printf("\n[Core %d] ERRO: RAM inválida durante fetch", core_id);
        pthread_mutex_unlock(&state->pipeline->IF.stage_mutex);
        unlock_process_manager(cpu->process_manager);
        pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
        return;
    }

    printf("\n[Core %d] Buscando instrução na RAM", core_id);
    printf("\n - Endereço base: %p", (void*)(memory_ram->vector + current_process->base_address));
    printf("\n - PC: %d", current_process->PC);
    
    instruction = get_line_of_program(
        memory_ram->vector + current_process->base_address,
        current_process->PC
    );

    if (!instruction || strlen(instruction) == 0) {
        printf("\n[Core %d] ERRO: Instrução inválida ou vazia", core_id);
        handle_process_completion(state, cpu, current_process, core_id, cycle_count, instruction);
        pthread_mutex_unlock(&state->pipeline->IF.stage_mutex);
        unlock_process_manager(cpu->process_manager);
        pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
        return;
    }

    printf("\n[Core %d] Instrução lida: '%s'", core_id, instruction);
    show_pipeline_fetch(instruction);
    state->pipeline->IF.instruction = instruction;
    pthread_mutex_unlock(&state->pipeline->IF.stage_mutex);

    // 2. Instruction Decode Stage
    printf("\n[Core %d] Iniciando Decode", core_id);
    pthread_mutex_lock(&state->pipeline->ID.stage_mutex);
    type_of_instruction instr_type = decode_instruction(instruction);
    printf("\n[Core %d] Instrução decodificada: %s", core_id, get_instruction_name(instr_type));
    state->pipeline->ID.type = instr_type;
    show_pipeline_decode(get_instruction_name(instr_type));
    pthread_mutex_unlock(&state->pipeline->ID.stage_mutex);

    // 3. Execute Stage
    printf("\n[Core %d] Iniciando Execute", core_id);
    pthread_mutex_lock(&state->pipeline->EX.stage_mutex);
    instruction_processor instr_processor = {0};
    instr_processor.instruction = instruction;
    instr_processor.type = instr_type;
    instr_processor.num_instruction = current_process->PC;

    printf("\n[Core %d] Executando instrução %s", core_id, instruction);
    execute_instruction(cpu, memory_ram, instruction, instr_type, core_id, &instr_processor, 
                       memory_ram->vector + current_process->base_address);
    printf("\n[Core %d] Instrução executada com sucesso", core_id);
    show_pipeline_execute(get_instruction_name(instr_type));
    pthread_mutex_unlock(&state->pipeline->EX.stage_mutex);

    // 4. Memory Access Stage
    printf("\n[Core %d] Iniciando Memory Access", core_id);
    pthread_mutex_lock(&state->pipeline->MEM.stage_mutex);
    handle_memory_stage(instr_type);
    pthread_mutex_unlock(&state->pipeline->MEM.stage_mutex);

    // 5. Write Back Stage
    printf("\n[Core %d] Iniciando Write Back", core_id);
    pthread_mutex_lock(&state->pipeline->WB.stage_mutex);
    handle_writeback_stage(instr_type);
    pthread_mutex_unlock(&state->pipeline->WB.stage_mutex);

    show_pipeline_end();

    // Atualização de estado e estatísticas
    printf("\n[Core %d] Atualizando estatísticas", core_id);
    pthread_mutex_lock(&state->global_mutex);
    current_process->PC++;
    current_process->total_instructions++;
    state->total_instructions++;
    printf("\n[Core %d] Novo PC: %d", core_id, current_process->PC);
    printf("\n[Core %d] Total de instruções: %d", core_id, current_process->total_instructions);
    pthread_mutex_unlock(&state->global_mutex);

    current_core->quantum_remaining--;
    current_process->cycles_executed = cycle_count;

    // Verifica quantum
    printf("\n[Core %d] Quantum restante: %d", core_id, current_core->quantum_remaining);
    if (current_core->quantum_remaining <= 0) {
        printf("\n[Core %d] Quantum expirado para processo %d", core_id, current_process->pid);
        show_process_state(current_process->pid, "RUNNING", "READY (Quantum expirado)");
        
        pthread_mutex_lock(&state->global_mutex);
        state->context_switches++;
        pthread_mutex_unlock(&state->global_mutex);
        
        current_process->state = READY;
        cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = current_process;
        release_core(cpu, core_id);
    }

    if (instruction) {
        free(instruction);
    }
    
    printf("\n[Core %d] Ciclo de execução completo", core_id);
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
    
    pthread_mutex_destroy(&p->IF.stage_mutex);
    pthread_mutex_destroy(&p->ID.stage_mutex);
    pthread_mutex_destroy(&p->EX.stage_mutex);
    pthread_mutex_destroy(&p->MEM.stage_mutex);
    pthread_mutex_destroy(&p->WB.stage_mutex);
    pthread_mutex_destroy(&p->pipeline_mutex);
}