#include "architecture.h"
#include "pcb.h"
#include <unistd.h>

#define DEFAULT_QUANTUM 4
#define BASE_ADDRESS_OFFSET 3
#define MAX_CYCLES 3
#define SLEEP_INTERVAL 10000  // Reduzido para 10ms

typedef struct {
    const char* program_files[3];
    int num_programs;
    int quantum;
    int print_interval;
} SystemConfig;

void load_test_program(ram* memory_ram, PCB* process, const char* filename, int base_address) {
    printf("\nAttempting to read file: %s\n", filename);
    
    char* program = read_program(filename);
    if (program == NULL) {
        printf("Error: Could not read program file '%s'\n", filename);
        return;
    }

    // Mantém os espaços, apenas remove linhas vazias extras
    char cleaned_program[1024] = {0};
    int clean_index = 0;
    int line_count = 0;

    char* line_start = program;
    char* current = program;

    while (*current) {
        if (*current == '\n') {
            // Se não é uma linha vazia, copia
            if (current > line_start) {
                int line_length = current - line_start;
                memcpy(cleaned_program + clean_index, line_start, line_length);
                clean_index += line_length;
                cleaned_program[clean_index++] = '\n';
                line_count++;
            }
            line_start = current + 1;
        }
        current++;
    }

    // Trata a última linha se existir
    if (current > line_start) {
        int line_length = current - line_start;
        memcpy(cleaned_program + clean_index, line_start, line_length);
        clean_index += line_length;
        cleaned_program[clean_index++] = '\n';
        line_count++;
    }

    printf("Program content (%d bytes, %d lines):\n%s", 
           clean_index, line_count, cleaned_program);
    
    // Copia para a memória
    for (unsigned short int i = 0; i < clean_index; i++) {
        memory_ram->vector[base_address + i] = cleaned_program[i];
    }

    process->base_address = base_address;
    process->memory_limit = clean_index;
    
    printf("Loaded program at base address %d with %d bytes\n", 
           base_address, clean_index);
           
    char* first_instruction = get_line_of_program(memory_ram->vector + base_address, 0);
    if (first_instruction) {
        printf("First instruction: '%s'\n", first_instruction);
        free(first_instruction);
    } else {
        printf("Error: Could not read first instruction\n");
    }
    
    free(program);
}

void print_system_state(architecture_state* state __attribute__((unused)), 
                       cpu* cpu, ram* memory_ram, int cycle_count) {
    printf("\n=== System State at Cycle %d ===\n", cycle_count);
    
    // Estado dos cores
    for (int i = 0; i < NUM_CORES; i++) {
        printf("Core %d: ", i);
        if (!cpu->core[i].is_available && cpu->core[i].current_process) {
            PCB* process = cpu->core[i].current_process;
            printf("Running Process %d:\n", process->pid);
            printf("  PC: %d\n", cpu->core[i].PC);
            printf("  Base Address: %d\n", process->base_address);
            printf("  Memory Limit: %d\n", process->memory_limit);
            printf("  Quantum Remaining: %d\n", cpu->core[i].quantum_remaining);
        } else {
            printf("Idle\n");
        }
    }
    
    printf("\nQueue Status:\n");
    printf("Ready Queue: %d processes\n", cpu->process_manager->ready_count);
    for (int i = 0; i < cpu->process_manager->ready_count; i++) {
        PCB* process = cpu->process_manager->ready_queue[i];
        printf("  Process %d (Base: %d, Limit: %d)\n", 
               process->pid, process->base_address, process->memory_limit);
    }
    
    printf("Blocked Queue: %d processes\n", cpu->process_manager->blocked_count);
    
    printf("\nMemory State:\n");
    print_ram(memory_ram);
    printf("\n");
}

void initialize_system(SystemConfig* config) {
    config->program_files[0] = "dataset/program.txt";
    config->program_files[1] = "dataset/program2.txt";
    config->program_files[2] = "dataset/program3.txt";
    config->num_programs = 3;
    config->quantum = DEFAULT_QUANTUM;
    config->print_interval = 10;
}

void load_all_programs(cpu* cpu, ram* memory_ram, SystemConfig* config) {
    for (int i = 0; i < config->num_programs; i++) {
        PCB* process = create_process(cpu->process_manager);
        if (process != NULL) {
            int base_address = i * BASE_ADDRESS_OFFSET;
            load_test_program(memory_ram, process, config->program_files[i], base_address);
            printf("Created process %d with base address %d\n", process->pid, base_address);
        }
    }
}

int main() {
    SystemConfig config;
    initialize_system(&config);

    cpu* cpu = malloc(sizeof(cpu));
    ram* memory_ram = malloc(sizeof(ram));
    disc* memory_disc = malloc(sizeof(disc));
    peripherals* peripherals = malloc(sizeof(peripherals));
    architecture_state* arch_state = malloc(sizeof(architecture_state));

    if (!cpu || !memory_ram || !memory_disc || !peripherals || !arch_state) {
        printf("Error: Memory allocation failed\n");
        return 1;
    }

    printf("\n=== System Initialization ===\n");
    init_architecture(cpu, memory_ram, memory_disc, peripherals, arch_state);
    
    printf("\n=== Loading Programs ===\n");
    load_all_programs(cpu, memory_ram, &config);

    printf("\n=== Initializing Pipeline ===\n");
    init_pipeline_multicore(arch_state, cpu, memory_ram);

    printf("\n=== Starting Execution ===\n");
    int cycle_count = 0;
    arch_state->program_running = true;

    while (arch_state->program_running && cycle_count < MAX_CYCLES) {
        cycle_count++;
        printf("\n--- Cycle %d ---\n", cycle_count);

        // Executa pipeline em cada core
        for (int core_id = 0; core_id < NUM_CORES; core_id++) {
            if (!cpu->core[core_id].is_available && cpu->core[core_id].current_process != NULL) {
                PCB* current_process = cpu->core[core_id].current_process;
                printf("Core %d executing process %d:\n", core_id, current_process->pid);
                printf("  PC: %d\n", cpu->core[core_id].PC);
                printf("  Base Address: %d\n", current_process->base_address);
                printf("  Memory Limit: %d\n", current_process->memory_limit);
                printf("  Quantum Remaining: %d\n", cpu->core[core_id].quantum_remaining);
                
                // Tenta ler a próxima instrução
                char* next_instruction = get_line_of_program(memory_ram->vector, 
                                                          current_process->base_address + cpu->core[core_id].PC);
                if (next_instruction != NULL) {
                    printf("  Next Instruction: %s\n", next_instruction);
                    free(next_instruction);
                }
                
                execute_pipeline_cycle(arch_state, cpu, memory_ram, core_id);
            } else {
                printf("Core %d: Idle\n", core_id);
            }
        }

        // Escalona processos
        if (cpu->process_manager->ready_count > 0) {
            printf("\nScheduling processes from ready queue (size: %d)\n", 
                cpu->process_manager->ready_count);
            
            for (int core_id = 0; core_id < NUM_CORES; core_id++) {
                if (cpu->core[core_id].is_available) {
                    printf("Attempting to schedule process to Core %d\n", core_id);
                    schedule_next_process(cpu, core_id);
                }
            }
        }

        // Imprime estado completo a cada 50 ciclos
        if (cycle_count % 50 == 0) {
            print_system_state(arch_state, cpu, memory_ram, cycle_count);
        }

        // Verifica condição de término
        arch_state->program_running = check_program_running(cpu);
        if (!arch_state->program_running) {
            printf("\nNo more processes to execute\n");
        }
        
        usleep(SLEEP_INTERVAL);
    }

    printf("\n=== Execution Summary ===\n");
    if (cycle_count >= MAX_CYCLES) {
        printf("Execution stopped after reaching maximum cycle count (%d)\n", MAX_CYCLES);
    } else {
        printf("Execution completed after %d cycles\n", cycle_count);
    }

    free_architecture(cpu, memory_ram, memory_disc, peripherals, arch_state);
    return 0;
}