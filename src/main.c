#include "architecture.h"
#include "os_display.h"
#include "pcb.h"
#include <unistd.h>
#include <stdbool.h>

// #define DEFAULT_QUANTUM 4
// #define BASE_ADDRESS_OFFSET 0
// #define MAX_CYCLES 13
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
    int running = 0, ready = 0, blocked = 0;

    // Contagem de processos em cada estado
    ready = cpu->process_manager->ready_count;
    blocked = cpu->process_manager->blocked_count;

    for (int i = 0; i < NUM_CORES; i++) {
        if (!cpu->core[i].is_available && cpu->core[i].current_process != NULL) {
            if (cpu->core[i].current_process->state != FINISHED) {
                running++;
                all_finished = false;
            }
        }
    }

    // Mostra estado atual do sistema
    show_scheduler_state(ready, blocked);

    if (ready > 0 || blocked > 0) {
        all_finished = false;
    }

    return all_finished;
}

int load_test_program(ram* memory_ram, PCB* process, const char* filename) {
    if (!memory_ram || !memory_ram->vector) {
        show_memory_operation(-1, "RAM not initialized", 0);
        return -1;
    }

    show_memory_operation(-1, "Loading Program", 0);

    char* program = read_program(filename);
    if (!program) {
        show_memory_operation(-1, "Program read failed", 0);
        return -1;
    }

    // Encontra espaço livre
    int free_space = 0;
    while (free_space < NUM_MEMORY && memory_ram->vector[free_space] != '\0') {
        free_space++;
    }

    if (free_space >= NUM_MEMORY) {
        show_memory_operation(-1, "No free memory space", NUM_MEMORY);
        free(program);
        return -1;
    }

    size_t len = strlen(program);
    if (free_space + len >= NUM_MEMORY) {
        show_memory_operation(-1, "Insufficient memory space", len);
        free(program);
        return -1;
    }

    // Copia programa para memória
    memcpy(memory_ram->vector + free_space, program, len);
    memory_ram->vector[free_space + len] = '\0';

    // Conta instruções
    int num_instructions = 0;
    for (size_t i = 0; i < len; i++) {
        if (program[i] == '\n') num_instructions++;
    }
    if (program[len-1] != '\n') num_instructions++;

    if (num_programs >= 10) {
        show_memory_operation(-1, "Maximum programs reached", 10);
        free(program);
        return -1;
    }

    // Configura programa e PCB
    programs[num_programs].start = memory_ram->vector + free_space;
    programs[num_programs].length = len;
    programs[num_programs].num_lines = num_instructions;

    if (process) {
        process->base_address = free_space;
        process->PC = 0;
        process->memory_limit = num_instructions;
        show_process_state(process->pid, "NEW", "INITIALIZED");
    }

    show_memory_operation(process ? process->pid : -1, "Program Loaded", free_space);
    num_programs++;
    free(program);
    return free_space;
}

void print_system_state(architecture_state* state __attribute__((unused)),
                       cpu* cpu,
                       ram* memory_ram __attribute__((unused)),
                       int cycle_count) {
    show_cycle_start(cycle_count);

    // Estado dos cores
    int core_pids[NUM_CORES];
    for (int i = 0; i < NUM_CORES; i++) {
        if (!cpu->core[i].is_available && cpu->core[i].current_process) {
            PCB* process = cpu->core[i].current_process;
            core_pids[i] = process->pid;
        } else {
            core_pids[i] = -1;
        }
    }
    show_cores_state(NUM_CORES, core_pids);

    // Estado do escalonador
    show_scheduler_state(cpu->process_manager->ready_count,
                        cpu->process_manager->blocked_count);

    // Estado do sistema como um todo
    int total = 0;
    int completed = 0;
    int instructions = 0;

    for (int i = 0; i < total_processes; i++) {
        if (all_processes[i]) {
            total++;
            if (all_processes[i]->state == FINISHED) {
                completed++;
            }
            instructions += all_processes[i]->total_instructions;
        }
    }

    show_system_summary(cycle_count, total, completed, instructions);
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
    show_memory_operation(-1, "Initializing Program Loading", 0);

    for (int i = 0; i < config->num_programs; i++) {
        PCB* process = create_process(cpu->process_manager);
        if (process != NULL) {
            int base_addr = load_test_program(memory_ram, process, config->program_files[i]);
            if (base_addr >= 0) {
                process->base_address = base_addr;
                process->state = READY;
                cpu->process_manager->ready_queue[cpu->process_manager->ready_count++] = process;
                show_process_state(process->pid, "INITIALIZED", "READY");
            }
        }
    }

    show_scheduler_state(cpu->process_manager->ready_count,
                        cpu->process_manager->blocked_count);
}

int main(void) {
    // Inicialização do sistema
    show_os_banner();
    printf("Inicializando sistema...\n");

    // Configuração inicial
    SystemConfig config;
    initialize_system(&config);

    // Alocação e inicialização dos componentes
    cpu* cpu = (struct cpu*)malloc(sizeof(struct cpu));
    ram* memory_ram = (ram*)malloc(sizeof(ram));
    disc* memory_disc = (disc*)malloc(sizeof(disc));
    peripherals* p = (struct peripherals*)malloc(sizeof(struct peripherals));
    architecture_state* arch_state = (architecture_state*)malloc(sizeof(architecture_state));

    // Verifica alocações
    if (!cpu || !memory_ram || !memory_disc || !p || !arch_state) {
        printf("Erro: Falha na alocação de memória\n");
        // Cleanup...
        return 1;
    }

    // Inicialização da arquitetura
    init_architecture(cpu, memory_ram, memory_disc, p, arch_state);

    printf("\nCarregando programas...\n");
    load_all_programs(cpu, memory_ram, &config);

    printf("\nIniciando simulação do sistema operacional...\n");
    int cycle_count = 0;
    arch_state->program_running = true;

    // Loop principal de simulação
    while (arch_state->program_running && cycle_count < MAX_CYCLES) {
        cycle_count++;
        show_cycle_start(cycle_count);

        // Verifica processos bloqueados
        check_blocked_processes(cpu);

        // Escalonamento
        if (cpu->process_manager->ready_count > 0) {
            printf("\nEscalonando processos da fila de prontos (%d processos)\n",
                  cpu->process_manager->ready_count);

            for (int core_id = 0; core_id < NUM_CORES; core_id++) {
                if (cpu->core[core_id].is_available) {
                    schedule_next_process(cpu, core_id);
                }
            }
        }

        // Execução nos cores
        int core_pids[NUM_CORES] = {-1};  // -1 indica core ocioso
        for (int core_id = 0; core_id < NUM_CORES; core_id++) {
            if (!cpu->core[core_id].is_available && cpu->core[core_id].current_process != NULL) {
                PCB* current_process = cpu->core[core_id].current_process;
                core_pids[core_id] = current_process->pid;
                execute_pipeline_cycle(arch_state, cpu, memory_ram, core_id, cycle_count);
            }
        }

        // Mostra estado do sistema
        show_cores_state(NUM_CORES, core_pids);
        show_scheduler_state(cpu->process_manager->ready_count,
                           cpu->process_manager->blocked_count);

        // Verifica término
        arch_state->program_running = !all_processes_finished(cpu);
        if (!arch_state->program_running) {
            printf("\nTodos os processos foram concluídos\n");
        }

        usleep(SLEEP_INTERVAL);  // Atraso para visualização
    }

    // Resumo final
    int completed = 0;
    int total_instructions = 0;
    for (int i = 0; i < total_processes; i++) {
        if (all_processes[i] && all_processes[i]->was_completed) {
            completed++;
            total_instructions += all_processes[i]->total_instructions;
        }
    }

    show_system_summary(cycle_count, total_processes, completed, total_instructions);

    // Cleanup
    free_architecture(cpu, memory_ram, memory_disc, p, arch_state);
    return 0;
}