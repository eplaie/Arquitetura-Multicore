#include "architecture.h"
#include "pcb.h"
#include <unistd.h>

#define DEFAULT_QUANTUM 4
#define BASE_ADDRESS_OFFSET 1000

typedef struct {
    const char* program_files[3];
    int num_programs;
    int quantum;
    int print_interval;
} SystemConfig;

void load_test_program(ram* memory_ram, PCB* process, const char* filename, int base_address) {
    char* program = read_program(filename);
    if (program == NULL) {
        printf("Error: Could not read program file '%s'\n", filename);
        return;
    }

    unsigned short int num_characters = strlen(program);
    for (unsigned short int i = 0; i < num_characters; i++) {
        memory_ram->vector[base_address + i] = program[i];
    }

    process->base_address = base_address;
    process->memory_limit = num_characters;
    
    printf("Loaded program '%s' at base address %d\n", filename, base_address);
    free(program);
}

void print_system_state(architecture_state* state __attribute__((unused)), 
                       cpu* cpu, ram* memory_ram, int cycle_count) {
    printf("\n=== System State at Cycle %d ===\n", cycle_count);
    
    // Estado dos cores
    for (int i = 0; i < NUM_CORES; i++) {
        printf("Core %d: ", i);
        if (!cpu->core[i].is_available && cpu->core[i].current_process) {
            printf("Running Process %d (PC: %d, Quantum: %d)\n",
                cpu->core[i].current_process->pid,
                cpu->core[i].PC,
                cpu->core[i].quantum_remaining);
        } else {
            printf("Idle\n");
        }
    }
    
    printf("\nQueue Status:\n");
    printf("Ready Queue: %d processes\n", cpu->process_manager->ready_count);
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

    printf("Initializing system...\n");
    init_architecture(cpu, memory_ram, memory_disc, peripherals, arch_state);
    
    printf("\nLoading programs...\n");
    load_all_programs(cpu, memory_ram, &config);

    printf("\nInitializing pipeline...\n");
    init_pipeline_multicore(arch_state, cpu, memory_ram);

    printf("\nStarting execution...\n");
    int cycle_count = 0;
    arch_state->program_running = true;

    while (arch_state->program_running) {
        cycle_count++;

        for (int core_id = 0; core_id < NUM_CORES; core_id++) {
            if (!cpu->core[core_id].is_available && cpu->core[core_id].current_process) {
                execute_pipeline_cycle(arch_state, cpu, memory_ram, core_id);
            }
        }

        if (cpu->process_manager->ready_count > 0) {
            for (int core_id = 0; core_id < NUM_CORES; core_id++) {
                if (cpu->core[core_id].is_available) {
                    schedule_next_process(cpu, core_id);
                }
            }
        }

        if (cycle_count % config.print_interval == 0) {
            print_system_state(arch_state, cpu, memory_ram, cycle_count);
        }

        arch_state->program_running = check_program_running(cpu);
        usleep(100000);
    }

    printf("\nExecution finished after %d cycles\n", cycle_count);
    free_architecture(cpu, memory_ram, memory_disc, peripherals, arch_state);

    return 0;
}