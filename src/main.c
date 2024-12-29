#include "architecture.h"
#include "pcb.h"
#include <unistd.h>
#include <stdbool.h>

#define DEFAULT_QUANTUM 4
#define BASE_ADDRESS_OFFSET 0
#define MAX_CYCLES 10
#define SLEEP_INTERVAL 10000  // Reduzido para 10ms


typedef struct {
    const char* program_files[3];
    int num_programs;
    int quantum;
    int print_interval;
} SystemConfig;

// Protótipos das funções
void print_system_state(architecture_state* state, cpu* cpu, ram* memory_ram, int cycle_count);
void load_all_programs(cpu* cpu, ram* memory_ram, SystemConfig* config);
bool all_processes_finished(cpu* cpu);

bool all_processes_finished(cpu* cpu) {
    bool all_finished = true;
    
    // Verifica se há processos na fila de prontos
    if (cpu->process_manager->ready_count > 0) {
        all_finished = false;
    }
    
    // Verifica se há processos na fila de bloqueados
    if (cpu->process_manager->blocked_count > 0) {
        all_finished = false;
    }
    
    // Verifica se há processos executando nos cores
    for (int i = 0; i < NUM_CORES; i++) {
        if (!cpu->core[i].is_available && cpu->core[i].current_process != NULL) {
            if (cpu->core[i].current_process->state != FINISHED) {
                all_finished = false;
            }
        }
    }
    
    return all_finished;
}

int load_test_program(ram* memory_ram, PCB* process, const char* filename) {
    if (!memory_ram || !memory_ram->vector) {
        printf("Error: Invalid RAM pointer or RAM not initialized\n");
        return -1;
    }

    printf("\nAttempting to read file: %s\n", filename);
    
    char* program = read_program(filename);
    if (!program) {
        printf("Error: Could not read program file '%s'\n", filename);
        return -1;
    }

    printf("Program to load:\n%s", program);
    
    // Encontra próximo espaço livre
    int free_space = 0;
    while (free_space < NUM_MEMORY && memory_ram->vector[free_space] != '\0') {
        free_space++;
    }
    
    if (free_space >= NUM_MEMORY) {
        printf("Error: No free space in RAM\n");
        free(program);
        return -1;
    }
    
    // Copia programa
    size_t len = strlen(program);
    if (free_space + len >= NUM_MEMORY) {
        printf("Error: Not enough space in RAM for program\n");
        free(program);
        return -1;
    }
    
    memcpy(memory_ram->vector + free_space, program, len);
    memory_ram->vector[free_space + len] = '\0';
    
    // Conta instruções
    int num_instructions = 0;
    for (size_t i = 0; i < len; i++) {
        if (program[i] == '\n') num_instructions++;
    }
    if (program[len-1] != '\n') num_instructions++;
    
    // Verifica se programs[] tem espaço
    if (num_programs >= 10) {  // assumindo que o tamanho máximo é 10
        printf("Error: Maximum number of programs reached\n");
        free(program);
        return -1;
    }
    
    // Armazena informações do programa com verificações
    programs[num_programs].start = memory_ram->vector + free_space;
    programs[num_programs].length = len;
    programs[num_programs].num_lines = num_instructions;
    
    // Configura PCB
    if (process) {
        process->base_address = free_space;
        process->PC = 0;
        process->memory_limit = num_instructions;
    }
    
    printf("Program loaded at position %d with %d instructions\n", 
           free_space, num_instructions);
    
    char* first_instr = get_line_of_program(memory_ram->vector + free_space, 0);
    if (first_instr) {
        printf("First instruction at base %d: '%s'\n", free_space, first_instr);
        free(first_instr);
    }

    num_programs++;
    free(program);
    return free_space;
}


void print_system_state(architecture_state* state __attribute__((unused)), 
                       cpu* cpu, ram* memory_ram, int cycle_count) {
    printf("\n=== System State at Cycle %d ===\n", cycle_count);
    
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
    printf("\nLoading programs to ready queue...\n");
    
    for (int i = 0; i < config->num_programs; i++) {
        PCB* process = create_process(cpu->process_manager);
        if (process != NULL) {
            int base_addr = load_test_program(memory_ram, process, config->program_files[i]);
            if (base_addr >= 0) {
                process->base_address = base_addr;
                // Garantir que o processo está na fila de prontos
                process->state = READY;
                cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = process;
                printf("Process %d added to ready queue\n", process->pid);
            }
        }
    }
    
    printf("Total processes in ready queue: %d\n", cpu->process_manager->ready_count);
}

int main(void) {
    SystemConfig config;
    initialize_system(&config);

    // Alocação de memória
    cpu* cpu = (struct cpu*)malloc(sizeof(struct cpu));
    ram* memory_ram = (ram*)malloc(sizeof(ram));
    disc* memory_disc = (disc*)malloc(sizeof(disc));
    struct peripherals* p = (struct peripherals*)malloc(sizeof(struct peripherals));
    architecture_state* arch_state = (architecture_state*)malloc(sizeof(architecture_state));

    if (!cpu || !memory_ram || !memory_disc || !p || !arch_state) {
        printf("Error: Memory allocation failed\n");
        if (cpu) free(cpu);
        if (memory_ram) free(memory_ram);
        if (memory_disc) free(memory_disc);
        if (p) free(p);
        if (arch_state) free(arch_state);
        return 1;
    }

    // Inicializa com zeros
    memset(cpu, 0, sizeof(struct cpu));
    memset(memory_ram, 0, sizeof(ram));
    memset(memory_disc, 0, sizeof(disc));
    memset(p, 0, sizeof(struct peripherals));
    memset(arch_state, 0, sizeof(architecture_state));

    printf("\n=== System Initialization ===\n");
    
    init_architecture(cpu, memory_ram, memory_disc, p, arch_state);

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
        check_blocked_processes(cpu);

        // Escalona processos primeiro
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

        // Depois executa o pipeline em cada core
        for (int core_id = 0; core_id < NUM_CORES; core_id++) {
            if (!cpu->core[core_id].is_available && cpu->core[core_id].current_process != NULL) {
                PCB* current_process = cpu->core[core_id].current_process;
                printf("Core %d executing process %d:\n", core_id, current_process->pid);
                printf("  PC: %d\n", current_process->PC);
                printf("  Base Address: %d\n", current_process->base_address);
                printf("  Memory Limit: %d\n", current_process->memory_limit);
                printf("  Quantum Remaining: %d\n", cpu->core[core_id].quantum_remaining);

                execute_pipeline_cycle(arch_state, cpu, memory_ram, core_id, cycle_count);
            } else {
                printf("Core %d: Idle\n", core_id);
            }
        }

        arch_state->program_running = !all_processes_finished(cpu);
        if (!arch_state->program_running) {
            printf("\nNo more processes to execute (Ready: %d, Blocked: %d)\n", 
                   cpu->process_manager->ready_count,
                   cpu->process_manager->blocked_count);
        }
        
        sleep(10);
    }

    // Imprime sumário após o loop
    print_execution_summary(arch_state, cpu, memory_ram, cycle_count);

    // Libera memória
    if (cpu) free(cpu);
    if (memory_ram) free(memory_ram);
    if (memory_disc) free(memory_disc);
    if (p) free(p);
    if (arch_state) free(arch_state);

    return 0;
}