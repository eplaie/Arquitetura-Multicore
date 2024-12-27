#include "cpu.h"
#include "pipeline.h"
#include <unistd.h>

// Função auxiliar para identificar o core atual
static core* get_current_core(cpu* cpu) {
    for (int i = 0; i < NUM_CORES; i++) {
        lock_core(&cpu->core[i]);
        if (!cpu->core[i].is_available && cpu->core[i].current_process != NULL) {
            unlock_core(&cpu->core[i]);
            return &cpu->core[i];
        }
        unlock_core(&cpu->core[i]);
    }
    return NULL;
}

void init_cpu(cpu* cpu) {
    cpu->core = malloc(NUM_CORES * sizeof(core));
    if (cpu->core == NULL) {
        printf("memory allocation failed in cpu\n");
        exit(1);
    }

    // Inicializa mutex do escalonador
    pthread_mutex_init(&cpu->scheduler_mutex, NULL);

    for (unsigned short int i = 0; i < NUM_CORES; i++) {
        cpu->core[i].registers = malloc(NUM_REGISTERS * sizeof(unsigned short int));
        if (cpu->core[i].registers == NULL) {
            printf("memory allocation failed in cpu->core[%d].registers\n", i);
            exit(1);
        }
        
        // Inicializa dados básicos
        cpu->core[i].PC = 0;
        cpu->core[i].current_process = NULL;
        cpu->core[i].is_available = true;
        cpu->core[i].quantum_remaining = 0;
        cpu->core[i].running = false;
        
        // Inicializa mutex do core
        pthread_mutex_init(&cpu->core[i].mutex, NULL);
        
        for (unsigned short int j = 0; j < NUM_REGISTERS; j++) {
            cpu->core[i].registers[j] = 0;
        }
    }
}

void init_cpu_with_process_manager(cpu* cpu, int quantum_size) {
    init_cpu(cpu);
    cpu->process_manager = init_process_manager(quantum_size);
}

// Funções de sincronização
void lock_core(core* c) {
    pthread_mutex_lock(&c->mutex);
}

void unlock_core(core* c) {
    pthread_mutex_unlock(&c->mutex);
}

void lock_scheduler(cpu* cpu) {
    pthread_mutex_lock(&cpu->scheduler_mutex);
}

void unlock_scheduler(cpu* cpu) {
    pthread_mutex_unlock(&cpu->scheduler_mutex);
}

void control_unit(cpu* cpu, instruction_pipe* p) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        printf("Error: No active core found for control unit\n");
        return;
    }

    // Verifica se o quantum expirou antes de executar a instrução
    if (check_quantum_expired(cpu, current_core - cpu->core)) {
        handle_preemption(cpu, current_core - cpu->core);
        return;
    }

    p->result = 0;
    lock_core(current_core);

    if (p->type == ADD) {
        p->result = add(cpu, p->instruction);
        p->num_instruction++;
    } else if (p->type == SUB) {
        p->result = sub(cpu, p->instruction);
        p->num_instruction++;
    } else if (p->type == MUL) {
        p->result = mul(cpu, p->instruction);
        p->num_instruction++;
    } else if (p->type == DIV) {
        p->result = div_c(cpu, p->instruction);
        p->num_instruction++;
    } else if (p->type == LOOP) {
        loop(cpu, p);
        p->num_instruction++;
    } else if (p->type == L_END) {
        loop_end(cpu, p);
    } else if(p->type == IF) {
        if_i(cpu, p);
        p->num_instruction++;
    } else if(p->type == I_END) {
        if_end(p);
        p->num_instruction++;
    } else if(p->type == ELSE) {
        else_i(cpu, p);
        p->num_instruction++;
    } else if(p->type == ELS_END) {
        else_end(p);
        p->num_instruction++;
    } else {
        p->num_instruction++;
    }

    unlock_core(current_core);
}

// Operações básicas adaptadas para multicore
void load(cpu* cpu, char* instruction) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        printf("Error: No active core found for load operation\n");
        return;
    }

    lock_core(current_core);
    char *instruction_copy = strdup(instruction);
    char *token, *register_name;
    unsigned short int value;

    token = strtok(instruction_copy, " ");
    if (strcmp(token, "LOAD") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        unlock_core(current_core);
        exit(1);
    }

    token = strtok(NULL, " ");
    register_name = token;

    token = strtok(NULL, " ");
    value = atoi(token);

    current_core->registers[get_register_index(register_name)] = value;
    
    free(instruction_copy);
    unlock_core(current_core);
}

void store(cpu* cpu, ram* memory_ram, char* instruction) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        printf("Error: No active core found for store operation\n");
        return;
    }

    lock_core(current_core);
    char *instruction_copy = strdup(instruction);
    char *token, *register_name1, *memory_address;
    char buffer[10];
    unsigned short int address, num_positions;

    token = strtok(instruction_copy, " ");
    if (strcmp(token, "STORE") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        unlock_core(current_core);
        exit(1);
    }

    token = strtok(NULL, " ");
    register_name1 = token;

    token = strtok(NULL, " ");
    memory_address = token;

    unsigned short int register_value = current_core->registers[get_register_index(register_name1)];
    sprintf(buffer, "%d", register_value);
    num_positions = strlen(buffer);
    address = verify_address(memory_ram, memory_address, num_positions);

    strcpy(&memory_ram->vector[address], buffer);
    
    free(instruction_copy);
    unlock_core(current_core);
}
// Operações aritméticas adaptadas para multicore
unsigned short int add(cpu* cpu, char* instruction) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        printf("Error: No active core found for add operation\n");
        return 0;
    }

    lock_core(current_core);
    char *instruction_copy = strdup(instruction);
    char *token, *register_name1, *register_name2;
    unsigned short int value, result;

    token = strtok(instruction_copy, " ");
    if (strcmp(token, "ADD") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        unlock_core(current_core);
        exit(1);
    }

    token = strtok(NULL, " ");
    register_name1 = token;

    token = strtok(NULL, " ");
    if (isdigit(token[0])) {
        value = atoi(token);
        result = ula(current_core->registers[get_register_index(register_name1)], value, ADD);
    } else {
        register_name2 = token;
        result = ula(current_core->registers[get_register_index(register_name1)],
                    current_core->registers[get_register_index(register_name2)],
                    ADD);
    }

    free(instruction_copy);
    unlock_core(current_core);
    return result;
}

unsigned short int sub(cpu* cpu, char* instruction) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        printf("Error: No active core found for sub operation\n");
        return 0;
    }

    lock_core(current_core);
    char *instruction_copy = strdup(instruction);
    char *token, *register_name1, *register_name2;
    unsigned short int value, result;

    token = strtok(instruction_copy, " ");
    if (strcmp(token, "SUB") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        unlock_core(current_core);
        exit(1);
    }

    token = strtok(NULL, " ");
    register_name1 = token;

    token = strtok(NULL, " ");
    if (isdigit(token[0])) {
        value = atoi(token);
        result = ula(current_core->registers[get_register_index(register_name1)], value, SUB);
    } else {
        register_name2 = token;
        result = ula(current_core->registers[get_register_index(register_name1)],
                    current_core->registers[get_register_index(register_name2)],
                    SUB);
    }

    free(instruction_copy);
    unlock_core(current_core);
    return result;
}

unsigned short int mul(cpu* cpu, char* instruction) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        printf("Error: No active core found for mul operation\n");
        return 0;
    }

    lock_core(current_core);
    char *instruction_copy = strdup(instruction);
    char *token, *register_name1, *register_name2;
    unsigned short int value, result;

    token = strtok(instruction_copy, " ");
    if (strcmp(token, "MUL") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        unlock_core(current_core);
        exit(1);
    }

    token = strtok(NULL, " ");
    register_name1 = token;

    token = strtok(NULL, " ");
    if (isdigit(token[0])) {
        value = atoi(token);
        result = ula(current_core->registers[get_register_index(register_name1)], value, MUL);
    } else {
        register_name2 = token;
        result = ula(current_core->registers[get_register_index(register_name1)],
                    current_core->registers[get_register_index(register_name2)],
                    MUL);
    }

    free(instruction_copy);
    unlock_core(current_core);
    return result;
}

unsigned short int div_c(cpu* cpu, char* instruction) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        printf("Error: No active core found for div operation\n");
        return 0;
    }

    lock_core(current_core);
    char *instruction_copy = strdup(instruction);
    char *token, *register_name1, *register_name2;
    unsigned short int value, result;

    token = strtok(instruction_copy, " ");
    if (strcmp(token, "DIV") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        unlock_core(current_core);
        exit(1);
    }

    token = strtok(NULL, " ");
    register_name1 = token;

    token = strtok(NULL, " ");
    if (isdigit(token[0])) {
        value = atoi(token);
        result = ula(current_core->registers[get_register_index(register_name1)], value, DIV);
    } else {
        register_name2 = token;
        result = ula(current_core->registers[get_register_index(register_name1)],
                    current_core->registers[get_register_index(register_name2)],
                    DIV);
    }

    free(instruction_copy);
    unlock_core(current_core);
    return result;
}
void if_i(cpu* cpu, instruction_pipe* p) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        printf("Error: No active core found for if operation\n");
        return;
    }

    lock_core(current_core);
    char *instruction_copy = strdup(p->instruction);
    char *token = strtok(instruction_copy, " ");
    p->has_if = true;

    if (strcmp(token, "IF") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        unlock_core(current_core);
        exit(1);
    }

    token = strtok(NULL, " ");
    unsigned short int register1_value = get_register_index(token);
    register1_value = current_core->registers[register1_value];

    token = strtok(NULL, " ");
    const char *operator = token;

    token = strtok(NULL, " ");
    unsigned short int operand_value;
    if (isdigit(token[0])) {
        operand_value = atoi(token);
    } else {
        operand_value = current_core->registers[get_register_index(token)];
    }

    int result = 0;
    if (strcmp(operator, "==") == 0) {
        result = register1_value == operand_value;
    } else if (strcmp(operator, "!=") == 0) {
        result = register1_value != operand_value;
    } else if (strcmp(operator, "<=") == 0) {
        result = register1_value <= operand_value;
    } else if (strcmp(operator, ">=") == 0) {
        result = register1_value >= operand_value;
    } else if (strcmp(operator, ">") == 0) {
        result = register1_value > operand_value;
    } else if (strcmp(operator, "<") == 0) {
        result = register1_value < operand_value;
    } else {
        printf("Error: Invalid operator. Line %hd.\n", p->num_instruction + 1);
    }

    if (result == 0) {
        p->valid_if = false;
        while (1) {
            p->num_instruction++;
            p->instruction = cpu_fetch(cpu, p->mem_ram);
            p->type = cpu_decode(cpu, p->instruction, p->num_instruction);

            instruction_copy = strdup(p->instruction);
            token = strtok(instruction_copy, " "); 

            if (strcmp(token, "I_END") == 0)
                break;

            free(instruction_copy);
        }
    } else {
        p->valid_if = true;
        p->running_if = true;
    }

    free(instruction_copy);
    unlock_core(current_core);
}

void loop(cpu* cpu, instruction_pipe* p) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        printf("Error: No active core found for loop operation\n");
        return;
    }

    lock_core(current_core);
    char *instruction_copy = strdup(p->instruction);
    char *token, *register_name;
    unsigned short int value;

    token = strtok(instruction_copy, " ");
    if (strcmp(token, "LOOP") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        unlock_core(current_core);
        exit(1);
    }

    if (!p->loop) {
        token = strtok(NULL, " ");
        if (isdigit(token[0])) {
            value = atoi(token);
            if (value == 0) {
                printf("Error: Loop value can't be 0. Line %hd.\n", p->num_instruction + 1);
                free(instruction_copy);
                unlock_core(current_core);
                exit(1);
            }
        } else {
            register_name = token;
            value = current_core->registers[get_register_index(register_name)];
            if (value == 0) {
                printf("Error: Loop value can't be 0. Line %hd.\n", p->num_instruction + 1);
                free(instruction_copy);
                unlock_core(current_core);
                exit(1);
            }
        }
        p->loop_value = value;
        p->loop_start = p->num_instruction;
        p->loop = true;
    }

    free(instruction_copy);
    unlock_core(current_core);
}

void loop_end(cpu* cpu, instruction_pipe* p) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        printf("Error: No active core found for loop end operation\n");
        return;
    }

    lock_core(current_core);
    char *instruction_copy = strdup(p->instruction);
    char *token;

    token = strtok(instruction_copy, " ");
    if (strcmp(token, "L_END") != 0) {
        printf("Error: Invalid instruction\n");
        free(instruction_copy);
        unlock_core(current_core);
        exit(1);
    }

    int decrease = p->num_instruction - p->loop_start + 1;
    p->loop_value--;
    if (p->loop_value == 0) {
        p->loop = false;
        p->loop_start = 0;
        p->num_instruction++;
    } else {
        for (int i = 0; i < decrease; i++) {
            current_core->PC--;
        }
        p->num_instruction = p->loop_start;
    }

    free(instruction_copy);
    unlock_core(current_core);
}
void decrease_pc(cpu* cpu) {
    core* current_core = get_current_core(cpu);
    if (current_core != NULL) {
        lock_core(current_core);
        current_core->PC--;
        unlock_core(current_core);
    }
}

char* cpu_fetch(cpu* cpu, ram* memory) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        return NULL;
    }

    lock_core(current_core);
    char* instruction = get_line_of_program(memory->vector, current_core->PC);
    current_core->PC++;
    unlock_core(current_core);

    return instruction;
}

type_of_instruction cpu_decode(cpu* cpu, char* instruction, unsigned short int num_instruction) {
    core* current_core = get_current_core(cpu);
    if (current_core == NULL) {
        return INVALID;
    }

    lock_core(current_core);
    type_of_instruction type = verify_instruction(instruction, num_instruction);
    unlock_core(current_core);

    if (type == INVALID) {
        exit(1);    
    }
    return type;
}

// Funções de gerenciamento de processos
void assign_process_to_core(cpu* cpu, PCB* process, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) {
        printf("Invalid core ID\n");
        return;
    }
    
    lock_core(&cpu->core[core_id]);
    if (!cpu->core[core_id].is_available) {
        printf("Core %d is not available\n", core_id);
        unlock_core(&cpu->core[core_id]);
        return;
    }
    
    cpu->core[core_id].current_process = process;
    cpu->core[core_id].is_available = false;
    cpu->core[core_id].quantum_remaining = cpu->process_manager->quantum_size;
    
    restore_context(process, &cpu->core[core_id]);
    
    process->state = RUNNING;
    process->core_id = core_id;
    unlock_core(&cpu->core[core_id]);
}

void release_core(cpu* cpu, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) {
        printf("Invalid core ID\n");
        return;
    }
    
    lock_core(&cpu->core[core_id]);
    core* current_core = &cpu->core[core_id];
    if (current_core->current_process != NULL) {
        save_context(current_core->current_process, current_core);
        current_core->current_process->state = READY;
        current_core->current_process->core_id = -1;
        current_core->current_process = NULL;
        current_core->is_available = true;
        current_core->quantum_remaining = 0;
    }
    unlock_core(current_core);
}

bool check_quantum_expired(cpu* cpu, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) {
        return false;
    }
    
    lock_core(&cpu->core[core_id]);
    core* current_core = &cpu->core[core_id];
    bool expired = false;
    
    if (current_core->current_process != NULL) {
        current_core->quantum_remaining--;
        expired = current_core->quantum_remaining <= 0;
    }
    
    unlock_core(current_core);
    return expired;
}

void handle_preemption(cpu* cpu, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) {
        return;
    }
    
    lock_scheduler(cpu);
    lock_core(&cpu->core[core_id]);
    
    core* current_core = &cpu->core[core_id];
    if (current_core->current_process != NULL) {
        PCB* current_process = current_core->current_process;
        release_core(cpu, core_id);
        current_process->state = READY;
        cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = current_process;
    }
    
    unlock_core(current_core);
    unlock_scheduler(cpu);
}

// Thread principal de execução do core
void* core_execution_thread(void* arg) {
    core_thread_args* args = (core_thread_args*)arg;
    cpu* cpu = args->cpu;
    ram* memory_ram = args->memory_ram;
    int core_id = args->core_id;
    core* current_core = &cpu->core[core_id];

    printf("Core %d thread started\n", core_id);

    while (current_core->running) {
        lock_core(current_core);

        if (!current_core->is_available && current_core->current_process != NULL) {
            if (check_quantum_expired(cpu, core_id)) {
                handle_preemption(cpu, core_id);
                unlock_core(current_core);
                continue;
            }

            instruction_pipe pipe_data = {0};
            char* instruction = cpu_fetch(cpu, memory_ram);
            
            if (instruction != NULL) {
                pipe_data.instruction = instruction;
                pipe_data.type = cpu_decode(cpu, instruction, current_core->PC);
                pipe_data.mem_ram = memory_ram;
                pipe_data.num_instruction = current_core->PC;

                control_unit(cpu, &pipe_data);

                printf("Core %d executed instruction at PC %d (Quantum: %d)\n",
                       core_id, current_core->PC - 1, current_core->quantum_remaining);
            } else {
                current_core->current_process->state = READY;
                current_core->is_available = true;
                current_core->current_process = NULL;
            }
        }

        unlock_core(current_core);
        usleep(1000);
    }

    printf("Core %d thread finished\n", core_id);
    free(arg);
    return NULL;
}
// Inicialização das threads dos cores
void init_cpu_threads(cpu* cpu, ram* memory_ram, architecture_state* state) {
    // Inicializa mutex do escalonador
    pthread_mutex_init(&cpu->scheduler_mutex, NULL);

    for (int i = 0; i < NUM_CORES; i++) {
        // Inicializa mutex do core
        pthread_mutex_init(&cpu->core[i].mutex, NULL);
        cpu->core[i].running = true;

        // Prepara argumentos para a thread
        core_thread_args* args = malloc(sizeof(core_thread_args));
        if (args == NULL) {
            printf("Failed to allocate thread arguments for core %d\n", i);
            exit(1);
        }

        args->cpu = cpu;
        args->memory_ram = memory_ram;
        args->core_id = i;
        args->state = state;

        // Cria a thread
        int result = pthread_create(&cpu->core[i].thread, 
                                  NULL, 
                                  core_execution_thread, 
                                  args);
        if (result != 0) {
            printf("Failed to create thread for core %d\n", i);
            exit(1);
        }

        printf("Created thread for core %d\n", i);
    }
}

// Limpeza das threads
void cleanup_cpu_threads(cpu* cpu) {
    for (int i = 0; i < NUM_CORES; i++) {
        // Sinaliza para a thread parar
        cpu->core[i].running = false;
        
        // Espera a thread terminar
        pthread_join(cpu->core[i].thread, NULL);
        
        // Destroi o mutex do core
        pthread_mutex_destroy(&cpu->core[i].mutex);
    }

    // Destroi o mutex do escalonador
    pthread_mutex_destroy(&cpu->scheduler_mutex);
}

