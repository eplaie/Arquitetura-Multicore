#include "cpu.h"
#include "pipeline.h"
#include <unistd.h>
#include "os_display.h"

// Função auxiliar para identificar o core atual
core* get_current_core(cpu* cpu) {
    if (!cpu) return NULL;
    
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

// core* get_specific_core(cpu* cpu, int target_core_id) {
//     if (!cpu || target_core_id < 0 || target_core_id >= NUM_CORES) {
//         return NULL;
//     }
//     return &cpu->core[target_core_id];
// }

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

// void init_cpu_with_process_manager(cpu* cpu, int quantum_size) {
//     init_cpu(cpu);
//     cpu->process_manager = init_process_manager(quantum_size);
// }

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

// void control_unit(cpu* cpu, instruction_pipe* p) {
//     core* current_core = get_current_core(cpu);
//     if (current_core == NULL) {
//         printf("Error: No active core found for control unit\n");
//         return;
//     }
//
//     printf("Debug - Executing instruction: '%s' (Type: %d)\n", p->instruction, p->type);
//
//     if (p->type == LOAD) {
//         load(cpu, p->instruction);
//         printf("Debug - LOAD executed\n");
//     } else if (p->type == STORE) {
//         store(cpu, p->mem_ram, p->instruction);
//         printf("Debug - STORE executed\n");
//     }
//
//     // Verifica se o quantum expirou antes de executar a instrução
//     if (check_quantum_expired(cpu, current_core - cpu->core)) {
//         handle_preemption(cpu, current_core - cpu->core);
//         return;
//     }
//
//     p->result = 0;
//     lock_core(current_core);
//
//     if (p->type == ADD) {
//         p->result = add(cpu, p->instruction);
//         p->num_instruction++;
//     } else if (p->type == SUB) {
//         p->result = sub(cpu, p->instruction);
//         p->num_instruction++;
//     } else if (p->type == MUL) {
//         p->result = mul(cpu, p->instruction);
//         p->num_instruction++;
//     } else if (p->type == DIV) {
//         p->result = div_c(cpu, p->instruction);
//         p->num_instruction++;
//     } else if (p->type == LOOP) {
//         loop(cpu, p);
//         p->num_instruction++;
//     } else if (p->type == L_END) {
//         loop_end(cpu, p);
//     } else if(p->type == IF) {
//         if_i(cpu, p);
//         p->num_instruction++;
//     } else if(p->type == I_END) {
//         if_end(p);
//         p->num_instruction++;
//     } else if(p->type == ELSE) {
//         else_i(cpu, p);
//         p->num_instruction++;
//     } else if(p->type == ELS_END) {
//         else_end(p);
//         p->num_instruction++;
//     } else {
//         p->num_instruction++;
//     }
//
//     unlock_core(current_core);
// }

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
   if (!cpu || !memory_ram || !instruction) {
       return;
   }

   // Identifica o core que está executando
    int executing_core_id = -1;
    for (int i = 0; i < NUM_CORES; i++) {
        lock_core(&cpu->core[i]);
        PCB* process = cpu->core[i].current_process;
        if (!cpu->core[i].is_available && 
            process && 
            process->state == RUNNING &&
            process->core_id == i) {  // Verifica se o core_id corresponde
            executing_core_id = i;
            unlock_core(&cpu->core[i]);
            break;
        }
        unlock_core(&cpu->core[i]);
    }

   if (executing_core_id == -1) {
       printf("Debug: Store operation cancelled - no valid executing core found\n");
       return;
   }

   core* current_core = &cpu->core[executing_core_id];
   lock_core(current_core);
   lock_scheduler(cpu);
   
   PCB* current_process = current_core->current_process;
   if (!current_process || current_process->state != RUNNING) {
       unlock_core(current_core);
       unlock_scheduler(cpu);
       return;
   }

   if (current_core->quantum_remaining <= 0) {
       printf("Debug: Store operation cancelled - quantum expired\n");
       unlock_core(current_core);
       unlock_scheduler(cpu);
       return;
   }

   printf("\nDebug: Store operation for Process %d on Core %d (PC: %d, Base: %d, Limit: %d)\n", 
          current_process->pid, executing_core_id, current_process->PC,
          current_process->base_address, current_process->memory_limit);

   char* instruction_copy = strdup(instruction);
   if (!instruction_copy) {
       unlock_core(current_core);
       unlock_scheduler(cpu);
       return;
   }

   char *register_name, *address_str;
   strtok(instruction_copy, " ");
   register_name = strtok(NULL, " ");
   address_str = strtok(NULL, " ");

   if (!register_name || !address_str || address_str[0] != 'A') {
       printf("Error: Invalid STORE instruction format\n");
       free(instruction_copy);
       unlock_core(current_core);
       unlock_scheduler(cpu);
       return;
   }

   unsigned short int reg_index = get_register_index(register_name);
   if (reg_index >= NUM_REGISTERS) {
       printf("Error: Invalid register index\n");
       free(instruction_copy);
       unlock_core(current_core);
       unlock_scheduler(cpu);
       return;
   }

      unsigned short int register_value = current_core->registers[reg_index];
   unsigned short int address = atoi(address_str + 1);

if (address == 250) {
    printf("\nDebug: I/O operation requested by Process %d\n", current_process->pid);
    
    // Reseta quantum antes de qualquer outra operação
    current_core->quantum_remaining = cpu->process_manager->quantum_size;
    
    bool io_in_use = false;
    PCB* owner_process = NULL;

    for (int i = 0; i < total_processes; i++) {
        if (all_processes[i] && all_processes[i]->using_io && 
            all_processes[i]->pid != current_process->pid) {
            io_in_use = true;
            owner_process = all_processes[i];
            break;
        }
    }

    if (io_in_use) {
        printf("\nProcess %d blocked: I/O in use by Process %d\n", 
               current_process->pid, owner_process->pid);
        
        current_process->state = BLOCKED;
        current_process->io_block_cycles = 2;
        current_process->using_io = false;
        
        cpu->process_manager->blocked_queue[cpu->process_manager->blocked_count++] = current_process;
        
        unlock_scheduler(cpu);
        unlock_core(current_core);
        free(instruction_copy);
        
        release_core(cpu, executing_core_id);
        return;
    }

    // Se chegou aqui, pode usar o I/O
    current_process->using_io = true;
    printf("\nProcess %d acquired I/O resource\n", current_process->pid);
    
    // Simula escrita no I/O
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%d", register_value);
    memcpy(&memory_ram->vector[address], buffer, strlen(buffer) + 1);
    printf("STORE: Process %d wrote value %d to I/O\n", 
           current_process->pid, register_value);

    // Bloqueia processo
    current_process->state = BLOCKED;
    current_process->io_block_cycles = 2;
    
    cpu->process_manager->blocked_queue[cpu->process_manager->blocked_count++] = current_process;
    
    printf("Process %d will be blocked for 2 cycles\n", current_process->pid);
    
    unlock_scheduler(cpu);
    unlock_core(current_core);
    free(instruction_copy);
    
    release_core(cpu, executing_core_id);
    return;
}
   
   // Se não é I/O, verifica limites de memória
   if (address < current_process->base_address || 
       address >= (current_process->base_address + current_process->memory_limit)) {
       printf("Warning: Memory access violation for process %d - address %d outside bounds [%d, %d]\n",
              current_process->pid, address, 
              current_process->base_address,
              current_process->base_address + current_process->memory_limit);

       // Recupera usando endereço base
       address = current_process->base_address;
       printf("Recovery: Redirecting write to safe address %d\n", address);
   }

   // Realiza a escrita na memória
   printf("STORE: Process %d wrote value %d to address A%d\n", 
          current_process->pid, register_value, address);

   char buffer[20];
   snprintf(buffer, sizeof(buffer), "%d", register_value);
   memcpy(&memory_ram->vector[address], buffer, strlen(buffer) + 1);

   free(instruction_copy);
   unlock_core(current_core);
   unlock_scheduler(cpu);
}


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
        result = ula(current_core->registers[get_register_index(register_name1)], 
                    value, ADD);
    } else {
        register_name2 = token;
        result = ula(current_core->registers[get_register_index(register_name1)],
                    current_core->registers[get_register_index(register_name2)],
                    ADD);
    }

    // Atualiza o registrador com o resultado
    current_core->registers[get_register_index(register_name1)] = result;

    free(instruction_copy);
    unlock_core(current_core);
    return result;
}

// unsigned short int sub(cpu* cpu, char* instruction) {
//     core* current_core = get_current_core(cpu);
//     if (current_core == NULL) {
//         printf("Error: No active core found for sub operation\n");
//         return 0;
//     }
//
//     lock_core(current_core);
//     char *instruction_copy = strdup(instruction);
//     char *token, *register_name1, *register_name2;
//     unsigned short int value, result;
//
//     token = strtok(instruction_copy, " ");
//     if (strcmp(token, "SUB") != 0) {
//         printf("Error: Invalid instruction\n");
//         free(instruction_copy);
//         unlock_core(current_core);
//         exit(1);
//     }
//
//     token = strtok(NULL, " ");
//     register_name1 = token;
//
//     token = strtok(NULL, " ");
//     if (isdigit(token[0])) {
//         value = atoi(token);
//         result = ula(current_core->registers[get_register_index(register_name1)], value, SUB);
//     } else {
//         register_name2 = token;
//         result = ula(current_core->registers[get_register_index(register_name1)],
//                     current_core->registers[get_register_index(register_name2)],
//                     SUB);
//     }
//
//     free(instruction_copy);
//     unlock_core(current_core);
//     return result;
// }

// unsigned short int mul(cpu* cpu, char* instruction) {
//     core* current_core = get_current_core(cpu);
//     if (current_core == NULL) {
//         printf("Error: No active core found for mul operation\n");
//         return 0;
//     }
//
//     lock_core(current_core);
//     char *instruction_copy = strdup(instruction);
//     char *token, *register_name1, *register_name2;
//     unsigned short int value, result;
//
//     token = strtok(instruction_copy, " ");
//     if (strcmp(token, "MUL") != 0) {
//         printf("Error: Invalid instruction\n");
//         free(instruction_copy);
//         unlock_core(current_core);
//         exit(1);
//     }
//
//     token = strtok(NULL, " ");
//     register_name1 = token;
//
//     token = strtok(NULL, " ");
//     if (isdigit(token[0])) {
//         value = atoi(token);
//         result = ula(current_core->registers[get_register_index(register_name1)], value, MUL);
//     } else {
//         register_name2 = token;
//         result = ula(current_core->registers[get_register_index(register_name1)],
//                     current_core->registers[get_register_index(register_name2)],
//                     MUL);
//     }
//
//     free(instruction_copy);
//     unlock_core(current_core);
//     return result;
// }

// unsigned short int div_c(cpu* cpu, char* instruction) {
//     core* current_core = get_current_core(cpu);
//     if (current_core == NULL) {
//         printf("Error: No active core found for div operation\n");
//         return 0;
//     }
//
//     lock_core(current_core);
//     char *instruction_copy = strdup(instruction);
//     char *token, *register_name1, *register_name2;
//     unsigned short int value, result;
//
//     token = strtok(instruction_copy, " ");
//     if (strcmp(token, "DIV") != 0) {
//         printf("Error: Invalid instruction\n");
//         free(instruction_copy);
//         unlock_core(current_core);
//         exit(1);
//     }
//
//     token = strtok(NULL, " ");
//     register_name1 = token;
//
//     token = strtok(NULL, " ");
//     if (isdigit(token[0])) {
//         value = atoi(token);
//         result = ula(current_core->registers[get_register_index(register_name1)], value, DIV);
//     } else {
//         register_name2 = token;
//         result = ula(current_core->registers[get_register_index(register_name1)],
//                     current_core->registers[get_register_index(register_name2)],
//                     DIV);
//     }
//
//     free(instruction_copy);
//     unlock_core(current_core);
//     return result;
// }
// void if_i(cpu* cpu, instruction_pipe* p) {
//     core* current_core = get_current_core(cpu);
//     if (current_core == NULL) {
//         printf("Error: No active core found for if operation\n");
//         return;
//     }
//
//     lock_core(current_core);
//     char *instruction_copy = strdup(p->instruction);
//     char *token = strtok(instruction_copy, " ");
//     p->has_if = true;
//
//     if (strcmp(token, "IF") != 0) {
//         printf("Error: Invalid instruction\n");
//         free(instruction_copy);
//         unlock_core(current_core);
//         exit(1);
//     }
//
//     token = strtok(NULL, " ");
//     unsigned short int register1_value = get_register_index(token);
//     register1_value = current_core->registers[register1_value];
//
//     token = strtok(NULL, " ");
//     const char *operator = token;
//
//     token = strtok(NULL, " ");
//     unsigned short int operand_value;
//     if (isdigit(token[0])) {
//         operand_value = atoi(token);
//     } else {
//         operand_value = current_core->registers[get_register_index(token)];
//     }
//
//     int result = 0;
//     if (strcmp(operator, "==") == 0) {
//         result = register1_value == operand_value;
//     } else if (strcmp(operator, "!=") == 0) {
//         result = register1_value != operand_value;
//     } else if (strcmp(operator, "<=") == 0) {
//         result = register1_value <= operand_value;
//     } else if (strcmp(operator, ">=") == 0) {
//         result = register1_value >= operand_value;
//     } else if (strcmp(operator, ">") == 0) {
//         result = register1_value > operand_value;
//     } else if (strcmp(operator, "<") == 0) {
//         result = register1_value < operand_value;
//     } else {
//         printf("Error: Invalid operator. Line %hd.\n", p->num_instruction + 1);
//     }
//
//     if (result == 0) {
//         p->valid_if = false;
//         while (1) {
//             p->num_instruction++;
//             p->instruction = cpu_fetch(cpu, p->mem_ram);
//             p->type = cpu_decode(cpu, p->instruction, p->num_instruction);
//
//             instruction_copy = strdup(p->instruction);
//             token = strtok(instruction_copy, " ");
//
//             if (strcmp(token, "I_END") == 0)
//                 break;
//
//             free(instruction_copy);
//         }
//     } else {
//         p->valid_if = true;
//         p->running_if = true;
//     }
//
//     free(instruction_copy);
//     unlock_core(current_core);
// }

// void loop(cpu* cpu, instruction_pipe* p) {
//     core* current_core = get_current_core(cpu);
//     if (current_core == NULL) {
//         printf("Error: No active core found for loop operation\n");
//         return;
//     }
//
//     lock_core(current_core);
//     char *instruction_copy = strdup(p->instruction);
//     char *token, *register_name;
//     unsigned short int value;
//
//     token = strtok(instruction_copy, " ");
//     if (strcmp(token, "LOOP") != 0) {
//         printf("Error: Invalid instruction\n");
//         free(instruction_copy);
//         unlock_core(current_core);
//         exit(1);
//     }
//
//     if (!p->loop) {
//         token = strtok(NULL, " ");
//         if (isdigit(token[0])) {
//             value = atoi(token);
//             if (value == 0) {
//                 printf("Error: Loop value can't be 0. Line %hd.\n", p->num_instruction + 1);
//                 free(instruction_copy);
//                 unlock_core(current_core);
//                 exit(1);
//             }
//         } else {
//             register_name = token;
//             value = current_core->registers[get_register_index(register_name)];
//             if (value == 0) {
//                 printf("Error: Loop value can't be 0. Line %hd.\n", p->num_instruction + 1);
//                 free(instruction_copy);
//                 unlock_core(current_core);
//                 exit(1);
//             }
//         }
//         p->loop_value = value;
//         p->loop_start = p->num_instruction;
//         p->loop = true;
//     }
//
//     free(instruction_copy);
//     unlock_core(current_core);
// }

// void loop_end(cpu* cpu, instruction_pipe* p) {
//     core* current_core = get_current_core(cpu);
//     if (current_core == NULL) {
//         printf("Error: No active core found for loop end operation\n");
//         return;
//     }
//
//     lock_core(current_core);
//     char *instruction_copy = strdup(p->instruction);
//     char *token;
//
//     token = strtok(instruction_copy, " ");
//     if (strcmp(token, "L_END") != 0) {
//         printf("Error: Invalid instruction\n");
//         free(instruction_copy);
//         unlock_core(current_core);
//         exit(1);
//     }
//
//     int decrease = p->num_instruction - p->loop_start + 1;
//     p->loop_value--;
//     if (p->loop_value == 0) {
//         p->loop = false;
//         p->loop_start = 0;
//         p->num_instruction++;
//     } else {
//         for (int i = 0; i < decrease; i++) {
//             current_core->PC--;
//         }
//         p->num_instruction = p->loop_start;
//     }
//
//     free(instruction_copy);
//     unlock_core(current_core);
// }
// void decrease_pc(cpu* cpu) {
//     core* current_core = get_current_core(cpu);
//     if (current_core != NULL) {
//         lock_core(current_core);
//         current_core->PC--;
//         unlock_core(current_core);
//     }
// }

// char* cpu_fetch(cpu* cpu, ram* memory) {
//     core* current_core = get_current_core(cpu);
//     if (current_core == NULL) {
//         return NULL;
//     }
//
//     lock_core(current_core);
//     char* instruction = get_line_of_program(memory->vector, current_core->PC);
//     current_core->PC++;
//     unlock_core(current_core);
//
//     return instruction;
// }

// type_of_instruction cpu_decode(cpu* cpu, char* instruction, unsigned short int num_instruction) {
//     core* current_core = get_current_core(cpu);
//     if (current_core == NULL) {
//         return INVALID;
//     }
//
//     lock_core(current_core);
//     type_of_instruction type = verify_instruction(instruction, num_instruction);
//     unlock_core(current_core);
//
//     if (type == INVALID) {
//         exit(1);
//     }
//     return type;
// }

// type_of_instruction instruc_decode(cpu* cpu, char* instruction, unsigned short int num_instruction) {
//     core* current_core = get_current_core(cpu);
//     if (current_core == NULL) {
//         return INVALID;
//     }
//
//     lock_core(current_core);
//     type_of_instruction type = verify_instruction(instruction, num_instruction);
//     unlock_core(current_core);
//
//     return type;
// }

// void assign_process_to_core(cpu* cpu, PCB* process, int core_id) {
//     if (!cpu || !process) {
//         printf("Error: Invalid CPU or process pointer\n");
//         return;
//     }
//
//     if (core_id < 0 || core_id >= NUM_CORES) {
//         printf("Error: Invalid core ID %d\n", core_id);
//         return;
//     }
//
//     lock_scheduler(cpu);
//     lock_core(&cpu->core[core_id]);
//
//     // Verifica se o core está realmente disponível
//     if (!cpu->core[core_id].is_available) {
//         printf("Warning: Core %d is not available\n", core_id);
//         unlock_core(&cpu->core[core_id]);
//         unlock_scheduler(cpu);
//         return;
//     }
//
//     // Libera o processo de outros cores se necessário
//     for (int i = 0; i < NUM_CORES; i++) {
//         if (i != core_id && cpu->core[i].current_process == process) {
//             release_core(cpu, i);
//         }
//     }
//
//     show_process_state(process->pid, "READY", "RUNNING");
//
//     // Configura o core
//     cpu->core[core_id].current_process = process;
//     cpu->core[core_id].quantum_remaining = cpu->process_manager->quantum_size;
//     cpu->core[core_id].PC = process->PC;
//     cpu->core[core_id].is_available = false;
//
//     // Configura o processo
//     process->state = RUNNING;
//     process->core_id = core_id;
//
//     // Restaura contexto ou inicializa registradores
//     if (process->cycles_executed > 0) {
//         memcpy(cpu->core[core_id].registers, process->registers,
//                NUM_REGISTERS * sizeof(unsigned short int));
//     } else {
//         memset(cpu->core[core_id].registers, 0,
//                NUM_REGISTERS * sizeof(unsigned short int));
//     }
//
//     show_cpu_execution(core_id, process->pid, "ASSIGNED", process->PC);
//
//     unlock_core(&cpu->core[core_id]);
//     unlock_scheduler(cpu);
// }

// Enhanced process release
void release_core(cpu* cpu, int core_id) {
    if (core_id < 0 || core_id >= NUM_CORES) {
        printf("Warning: Invalid core_id in release_core\n");
        return;
    }

    lock_core(&cpu->core[core_id]);

    core* current_core = &cpu->core[core_id];
    if (current_core->current_process) {
        PCB* process = current_core->current_process;
        process_state prev_state = process->state;

        // Não marca como FINISHED se estiver BLOCKED
        if (process->state != BLOCKED) {
            // Salva contexto se necessário
            if (prev_state != FINISHED) {
                save_context(process, current_core);
            }
        }

        // Limpa o core
        current_core->is_available = true;
        current_core->quantum_remaining = 0;
        current_core->current_process = NULL;
        memset(current_core->registers, 0, NUM_REGISTERS * sizeof(unsigned short int));

        printf("Core %d is now available\n", core_id);
    }

    unlock_core(current_core);
}

// Helper function to convert state to string (for logging)
// const char* state_to_string(process_state state) {
//     switch(state) {
//         case NEW: return "NEW";
//         case READY: return "READY";
//         case RUNNING: return "RUNNING";
//         case BLOCKED: return "BLOCKED";
//         case FINISHED: return "FINISHED";
//         default: return "UNKNOWN";
//     }
// }

// bool check_quantum_expired(cpu* cpu, int core_id) {
//     if (core_id < 0 || core_id >= NUM_CORES) {
//         return false;
//     }
//
//     core* current_core = &cpu->core[core_id];
//     return current_core->current_process != NULL &&
//            current_core->quantum_remaining <= 0;
// }

// void handle_preemption(cpu* cpu, int core_id) {
//     if (core_id < 0 || core_id >= NUM_CORES) return;
//
//     lock_scheduler(cpu);
//     lock_core(&cpu->core[core_id]);
//
//     core* current_core = &cpu->core[core_id];
//     if (current_core->current_process) {
//         PCB* current_process = current_core->current_process;
//
//         // Usa a política atual para tratar quantum expirado
//         cpu->process_manager->current_policy->on_quantum_expired(
//             cpu->process_manager,
//             current_process
//         );
//
//         cpu->process_manager->total_preemptions++;
//         release_core(cpu, core_id);
//     }
//
//     unlock_core(current_core);
//     unlock_scheduler(cpu);
// }

// Thread principal de execução do core
// void* core_execution_thread(void* arg) {
//     core_thread_args* args = (core_thread_args*)arg;
//     cpu* cpu = args->cpu;
//     ram* memory_ram = args->memory_ram;
//     int core_id = args->core_id;
//     core* current_core = &cpu->core[core_id];
//
//     printf("Core %d thread started\n", core_id);
//
//     while (current_core->running) {
//         lock_core(current_core);
//
//         if (!current_core->is_available && current_core->current_process != NULL) {
//             if (check_quantum_expired(cpu, core_id)) {
//                 handle_preemption(cpu, core_id);
//                 unlock_core(current_core);
//                 continue;
//             }
//
//             instruction_pipe pipe_data = {0};
//             char* instruction = cpu_fetch(cpu, memory_ram);
//
//             if (instruction != NULL) {
//                 pipe_data.instruction = instruction;
//                 pipe_data.type = cpu_decode(cpu, instruction, current_core->PC);
//                 pipe_data.mem_ram = memory_ram;
//                 pipe_data.num_instruction = current_core->PC;
//
//                 control_unit(cpu, &pipe_data);
//
//                 printf("Core %d executed instruction at PC %d (Quantum: %d)\n",
//                        core_id, current_core->PC - 1, current_core->quantum_remaining);
//             } else {
//                 current_core->current_process->state = READY;
//                 current_core->is_available = true;
//                 current_core->current_process = NULL;
//             }
//         }
//
//         unlock_core(current_core);
//         usleep(1000);
//     }
//
//     printf("Core %d thread finished\n", core_id);
//     free(arg);
//     return NULL;
// }
// // Inicialização das threads dos cores
// void init_cpu_threads(cpu* cpu, ram* memory_ram, architecture_state* state) {
//     // Inicializa mutex do escalonador
//     pthread_mutex_init(&cpu->scheduler_mutex, NULL);
//
//     for (int i = 0; i < NUM_CORES; i++) {
//         // Inicializa mutex do core
//         pthread_mutex_init(&cpu->core[i].mutex, NULL);
//         cpu->core[i].running = true;
//
//         // Prepara argumentos para a thread
//         core_thread_args* args = malloc(sizeof(core_thread_args));
//         if (args == NULL) {
//             printf("Failed to allocate thread arguments for core %d\n", i);
//             exit(1);
//         }
//
//         args->cpu = cpu;
//         args->memory_ram = memory_ram;
//         args->core_id = i;
//         args->state = state;
//
//         // Cria a thread
//         int result = pthread_create(&cpu->core[i].thread,
//                                   NULL,
//                                   core_execution_thread,
//                                   args);
//         if (result != 0) {
//             printf("Failed to create thread for core %d\n", i);
//             exit(1);
//         }
//
//         printf("Created thread for core %d\n", i);
//     }
// }

// Limpeza das threads
// void cleanup_cpu_threads(cpu* cpu) {
//     for (int i = 0; i < NUM_CORES; i++) {
//         // Sinaliza para a thread parar
//         cpu->core[i].running = false;
//
//         // Espera a thread terminar
//         pthread_join(cpu->core[i].thread, NULL);
//
//         // Destroi o mutex do core
//         pthread_mutex_destroy(&cpu->core[i].mutex);
//     }
//
//     // Destroi o mutex do escalonador
//     pthread_mutex_destroy(&cpu->scheduler_mutex);
// }
//
