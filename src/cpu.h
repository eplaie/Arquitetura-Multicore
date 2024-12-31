#ifndef CPU_H
#define CPU_H

#define NUM_CORES 4
#define NUM_REGISTERS 32
#define DEFAULT_QUANTUM 6

#include "libs.h"
#include "interpreter.h"
#include "common_types.h"
#include "ram.h"
#include "pcb.h"
#include "architecture_state.h"


// Inclua após todas as estruturas serem definidas
#include "instruction_utils.h"

// Estruturas existentes atualizadas com suporte a threads
typedef struct core {
    unsigned short int *registers;
    unsigned short int PC;
    PCB* current_process;      // Processo atual executando no core
    bool is_available;         // Indica se o core está disponível
    int quantum_remaining;     // Quantum restante do processo atual
    pthread_t thread;          // Thread do core
    pthread_mutex_t mutex;     // Mutex para proteção do core
    bool running;             // Flag para controle da thread
} core;

typedef struct cpu {
    core *core;
    ProcessManager* process_manager;
    pthread_mutex_t scheduler_mutex;  // Mutex para o escalonador
} cpu;


typedef struct instruction_pipe {
    char *instruction;
    unsigned short int num_instruction;
    type_of_instruction type;
    unsigned short int result;
    bool loop;
    unsigned short int loop_start;
    unsigned short int loop_value;
    bool has_if;
    bool valid_if;
    bool running_if;
    ram* mem_ram;
} instruction_pipe;

// Estrutura para argumentos da thread
typedef struct core_thread_args {
    cpu* cpu;
    ram* memory_ram;
    int core_id;
    architecture_state* state;
} core_thread_args;

// Funções originais do CPU
void init_cpu(cpu* cpu);
void control_unit(cpu* cpu, instruction_pipe* p);
unsigned short int ula(unsigned short int operating_a, unsigned short int operating_b, type_of_instruction operation);
unsigned short int verify_address(ram* memory_ram, char* address, unsigned short int num_positions);
void load(cpu* cpu, char* instruction);
void store(cpu* cpu, ram* memory_ram, char* instruction);
unsigned short int add(cpu* cpu, char* instruction);
unsigned short int sub(cpu* cpu, char* instruction);
unsigned short int mul(cpu* cpu, char* instruction);
unsigned short int div_c(cpu* cpu, char* instruction);
void if_i(cpu* cpu, instruction_pipe* pipe);
void if_end(instruction_pipe* pipe);
void else_i(cpu* cpu, instruction_pipe* pipe);
void else_end(instruction_pipe* pipe);
void loop(cpu* cpu, instruction_pipe* p);
void loop_end(cpu* cpu, instruction_pipe *p);
void decrease_pc(cpu* cpu);
char* instruc_fetch(cpu* cpu, ram* memory);
type_of_instruction instruc_decode(cpu* cpu, char* instruction, unsigned short int num_instruction);

// Funções de gerenciamento de processos
void init_cpu_with_process_manager(cpu* cpu, int quantum_size);
void assign_process_to_core(cpu* cpu, PCB* process, int core_id);
void release_core(cpu* cpu, int core_id);
bool check_quantum_expired(cpu* cpu, int core_id);
void handle_preemption(cpu* cpu, int core_id);

// Funções de gerenciamento de threads
void* core_execution_thread(void* arg);
void init_cpu_threads(cpu* cpu, ram* memory_ram, architecture_state* state);
void cleanup_cpu_threads(cpu* cpu);

// Novas funções para sincronização de threads
void lock_core(core* c);
void unlock_core(core* c);
void lock_scheduler(cpu* cpu);
void unlock_scheduler(cpu* cpu);

char* cpu_fetch(cpu* cpu, ram* memory);
type_of_instruction cpu_decode(cpu* cpu, char* instruction, unsigned short int num_instruction);

#include "instruction_utils.h"

#endif