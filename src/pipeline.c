#include "pipeline.h"

void init_pipeline(pipeline* p) {
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
}

char* instruction_fetch(cpu* cpu, ram* memory, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) {
        return NULL;
    }

    core* current_core = &cpu->core[core_id];
    if (!current_core->is_available || current_core->current_process == NULL) {
        return NULL;
    }

    // Verifica quantum antes de buscar instrução
    if (check_quantum_expired(cpu, core_id)) {
        handle_preemption(cpu, core_id);
        return NULL;
    }

    char* instruction = get_line_of_program(memory->vector, current_core->PC);
    current_core->PC++;

    return instruction;
}

void execute(cpu* cpu, instruction_pipe* p, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) {
        return;
    }

    core* current_core = &cpu->core[core_id];
    if (!current_core->is_available || current_core->current_process == NULL) {
        return;
    }

    // Verifica quantum antes da execução
    if (check_quantum_expired(cpu, core_id)) {
        handle_preemption(cpu, core_id);
        return;
    }

    // Executa a instrução normalmente
    control_unit(cpu, p);

    // Atualiza estatísticas do processo
    if (current_core->current_process) {
        current_core->quantum_remaining--;
    }
}

void memory_access(cpu* cpu, ram* memory_ram, type_of_instruction type, char* instruction, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES || !instruction) {
        return;
    }

    core* current_core = &cpu->core[core_id];
    if (!current_core->is_available || current_core->current_process == NULL) {
        return;
    }

    // Implementa acesso à memória com verificação de permissões do processo
    PCB* current_process = current_core->current_process;
    
    // Verifica se o acesso está dentro dos limites de memória do processo
    if (type == STORE || type == LOAD) {
        // Extrair endereço da instrução
        char* token = strtok(strdup(instruction), " ");
        token = strtok(NULL, " "); // Skip instruction name
        token = strtok(NULL, " "); // Get address
        
        unsigned short int address = atoi(token + 1); // Remove 'A' prefix
        
        if (address < current_process->base_address || 
            address >= (current_process->base_address + current_process->memory_limit)) {
            printf("Memory access violation for process %d\n", current_process->pid);
            return;
        }
    }

    // Executa o acesso à memória
    if (type == STORE) {
        store(cpu, memory_ram, instruction);
    } else if (type == LOAD) {
        load(cpu, instruction);
    }
}

void write_back(cpu* cpu, type_of_instruction type, char* instruction, unsigned short int result, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) {
        return;
    }

    core* current_core = &cpu->core[core_id];
    if (!current_core->is_available || current_core->current_process == NULL) {
        return;
    }

    // Implementa write-back considerando o processo atual
    if (type == ADD || type == SUB || type == MUL || type == DIV) {
        // Extrai o registrador de destino da instrução
        char* token = strtok(strdup(instruction), " ");
        token = strtok(NULL, " "); // Get destination register
        
        unsigned short int reg_index = get_register_index(token);
        current_core->registers[reg_index] = result;
    }
}

type_of_instruction instruction_decode(cpu* cpu, char* instruction, unsigned short int num_instruction) {
    return cpu_decode(cpu, instruction, num_instruction);
}

void check_pipeline_preemption(pipeline* p, cpu* cpu) {
    int core_id = p->current_core;
    
    if (check_quantum_expired(cpu, core_id)) {
        // Limpa o pipeline
        reset_pipeline_stage(&p->IF);
        reset_pipeline_stage(&p->ID);
        reset_pipeline_stage(&p->EX);
        reset_pipeline_stage(&p->MEM);
        reset_pipeline_stage(&p->WB);
        
        // Realiza a preempção
        handle_preemption(cpu, core_id);
        
        // Seleciona próximo core/processo
        // Implementar lógica de escalonamento aqui
    }
}

void switch_pipeline_context(pipeline* p, cpu* cpu, int new_core_id) {
    if (new_core_id < 0 || new_core_id >= NUM_CORES) {
        return;
    }
    
    // Salva contexto do core atual se necessário
    if (p->current_core >= 0 && p->current_core < NUM_CORES) {
        core* current_core = &cpu->core[p->current_core];
        if (current_core->current_process) {
            save_context(current_core->current_process, current_core);
        }
    }
    
    // Muda para o novo core
    p->current_core = new_core_id;
    
    // Limpa pipeline para novo contexto
    reset_pipeline_stage(&p->IF);
    reset_pipeline_stage(&p->ID);
    reset_pipeline_stage(&p->EX);
    reset_pipeline_stage(&p->MEM);
    reset_pipeline_stage(&p->WB);
}