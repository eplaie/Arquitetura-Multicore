#include "pipeline.h"
#include "os_display.h"
#include "instruction_utils.h"

void init_pipeline(pipeline* p) {
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
    printf("[Core %d] Processo %d finalizado\n", core_id, process->pid);

    pthread_mutex_lock(&state->global_mutex);
    state->completed_processes++;
    pthread_mutex_unlock(&state->global_mutex);

    show_process_state(process->pid, "RUNNING", "FINISHED");
    process->state = FINISHED;
    process->was_completed = true;
    process->cycles_executed = cycle_count;

    lock_process_manager(cpu->process_manager);
    release_core(cpu, core_id);
    unlock_process_manager(cpu->process_manager);

    if (instruction) {
        free(instruction);
    }

    // Verifica se todos os processos foram concluídos
    if (state->completed_processes >= total_processes) {
        pthread_mutex_lock(&state->global_mutex);
        state->program_running = false;
        pthread_mutex_unlock(&state->global_mutex);
    }
}

void execute_pipeline_cycle(architecture_state* state, cpu* cpu,
                         ram* memory_ram, int core_id, int cycle_count) {
   if (!state || !cpu || !cpu->memory_ram || !cpu->memory_ram->vector || 
       !cpu->memory_ram->initialized) {
       printf("\n[Core %d] Erro: CPU ou RAM inválida", core_id);
       return;
   }

   ram* active_ram = cpu->memory_ram; // Usar RAM da CPU

//    printf("\n[Debug] Pipeline - Core %d:", core_id);
//    printf("\n - RAM: %p", (void*)active_ram);
//    printf("\n - Vector: %p", (void*)active_ram->vector);

   pthread_mutex_lock(&active_ram->mutex);
   pthread_mutex_lock(&state->pipeline->pipeline_mutex);
   lock_process_manager(cpu->process_manager);

   core* current_core = &cpu->core[core_id];
   PCB* current_process = current_core->current_process;

   if (!current_process) {
       unlock_process_manager(cpu->process_manager);
       pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
       pthread_mutex_unlock(&active_ram->mutex);
       return;
   }

   show_pipeline_start(cycle_count, core_id, current_process->pid);
   
//    printf("\n[Debug] Pipeline - Core %d, Processo %d:", core_id, current_process->pid);
   printf("\n - Base address: %d", current_process->base_address);
   printf("\n - PC: %d", current_process->PC);
   printf("\n - Quantum remaining: %d", current_core->quantum_remaining);

   if (current_process->PC >= current_process->memory_limit) {
       printf("[Core %d] Processo %d: limite de memória atingido\n", core_id, current_process->pid);
       handle_process_completion(state, cpu, current_process, core_id, cycle_count, NULL);
       unlock_process_manager(cpu->process_manager);
       pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
       pthread_mutex_unlock(&active_ram->mutex);
       return;
   }

   // 1. Instruction Fetch
   pthread_mutex_lock(&state->pipeline->IF.stage_mutex);
   char* instruction = NULL;

   if (!active_ram->vector) {
       printf("[Core %d] Erro: RAM inválida\n", core_id);
       pthread_mutex_unlock(&state->pipeline->IF.stage_mutex);
       unlock_process_manager(cpu->process_manager);
       pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
       pthread_mutex_unlock(&active_ram->mutex);
       return;
   }

   printf("[Core %d] Buscando instrução: base=%d, PC=%d\n", 
          core_id, current_process->base_address, current_process->PC);

   instruction = get_line_of_program(
       active_ram->vector + current_process->base_address,
       current_process->PC
   );

if (!instruction || strlen(instruction) == 0 || current_process->PC >= current_process->memory_limit) {
    printf("\n[Core %d] Processo %d finalizado\n", core_id, current_process->pid);
    
    pthread_mutex_lock(&state->global_mutex);
    current_process->state = FINISHED;
    current_process->was_completed = true;
    state->completed_processes++;
    
    // Atualizar métricas finais do processo
    current_process->completion_time = cycle_count;
    current_process->turnaround_time = cycle_count - current_process->start_time;
    pthread_mutex_unlock(&state->global_mutex);
    
    show_process_state(current_process->pid, "RUNNING", "FINISHED");
        
    // Liberar o core de forma segura
    current_core->current_process = NULL;
    current_core->is_available = true;
    current_core->quantum_remaining = 0;
    memset(current_core->registers, 0, NUM_REGISTERS * sizeof(unsigned short int));
    
    // Liberar recursos na ordem correta
    if (instruction) free(instruction);
    
    pthread_mutex_unlock(&state->pipeline->IF.stage_mutex);
    unlock_process_manager(cpu->process_manager);
    pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
    pthread_mutex_unlock(&active_ram->mutex);
    
    return;
}

    show_pipeline_fetch(instruction);
    state->pipeline->IF.instruction = instruction;
    pthread_mutex_unlock(&state->pipeline->IF.stage_mutex);

    // 2. Instruction Decode
    pthread_mutex_lock(&state->pipeline->ID.stage_mutex);
    type_of_instruction instr_type = decode_instruction(instruction);
    state->pipeline->ID.type = instr_type;
    show_pipeline_decode(get_instruction_name(instr_type));
    pthread_mutex_unlock(&state->pipeline->ID.stage_mutex);

    // 3. Execute
    pthread_mutex_lock(&state->pipeline->EX.stage_mutex);
    instruction_processor instr_processor = {0};
    instr_processor.instruction = instruction;
    instr_processor.type = instr_type;
    instr_processor.num_instruction = current_process->PC;

    execute_instruction(cpu, memory_ram, instruction, instr_type, core_id, &instr_processor,
                       memory_ram->vector + current_process->base_address);
    show_pipeline_execute(get_instruction_name(instr_type));
    pthread_mutex_unlock(&state->pipeline->EX.stage_mutex);

    // 4. Memory Access
    pthread_mutex_lock(&state->pipeline->MEM.stage_mutex);
    handle_memory_stage(instr_type);
    pthread_mutex_unlock(&state->pipeline->MEM.stage_mutex);

    // 5. Write Back
    pthread_mutex_lock(&state->pipeline->WB.stage_mutex);
    handle_writeback_stage(instr_type);
    pthread_mutex_unlock(&state->pipeline->WB.stage_mutex);

    show_pipeline_end();

    // Atualização de estado
    pthread_mutex_lock(&state->global_mutex);
    current_process->PC++;
    current_core->PC = current_process->PC;
    current_process->total_instructions++;
    state->total_instructions++;
    pthread_mutex_unlock(&state->global_mutex);

    current_core->quantum_remaining--;
    current_process->cycles_executed = cycle_count;

    if (current_core->quantum_remaining <= 0) {
    printf("\n[Debug] Quantum expirado - Core %d", core_id);
    printf("\n - Processo: %d", current_process->pid);
    printf("\n - PC atual: %d", current_process->PC);
    printf("\n - Instruções executadas: %d", current_process->total_instructions);

    // Atualiza estado
    current_process->state = READY;
    show_process_state(current_process->pid, "RUNNING", "READY");

        pthread_mutex_lock(&state->global_mutex);
        state->context_switches++;
        pthread_mutex_unlock(&state->global_mutex);

        // Usa a função da política para tratar quantum expirado
        cpu->process_manager->policy->on_quantum_expired(cpu->process_manager, current_process);

        // printf("\n[Debug] Antes de release_core:");
       printf("\n - Ready count: %d", cpu->process_manager->ready_count);
       printf("\n - Core %d quantum: %d", core_id, current_core->quantum_remaining);

        release_core(cpu, core_id);

    //    printf("\n[Debug] Após release_core:");
    //    printf("\n - Ready count: %d", cpu->process_manager->ready_count);
    //    printf("\n - Core disponível: %d", current_core->is_available);

    }

    if (instruction) {
        free(instruction);
    }

    unlock_process_manager(cpu->process_manager);
    pthread_mutex_unlock(&state->pipeline->pipeline_mutex);
    pthread_mutex_unlock(&memory_ram->mutex);
}

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