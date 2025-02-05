#include "pcb.h"
#include "cpu.h"
#include "os_display.h"
#include "cache.h"

extern CacheEntry cache[CACHE_SIZE];

PCB* all_processes[MAX_PROCESSES];
int total_processes = 0;

PCB* create_pcb(void) {
    if (total_processes >= MAX_PROCESSES) {
        printf("[Sistema] Erro: Limite de processos atingido (%d)\n", MAX_PROCESSES);
        return NULL;
    }

    PCB* pcb = malloc(sizeof(PCB));
    if (!pcb) {
        printf("[Sistema] Erro: Falha na alocação de processo\n");
        return NULL;
    }

    pcb->pid = total_processes;
    pcb->state = NEW;
    pcb->core_id = -1;
    pcb->PC = 0;
    pcb->cycles_executed = 0;
    pcb->quantum_remaining = DEFAULT_QUANTUM;
    pcb->base_address = 0;
    pcb->memory_limit = NUM_MEMORY;
    pcb->was_completed = false;
    pcb->start_time = 0;  
    pcb->lottery_selections = 0;
    pcb->waiting_time = 0;
    pcb->response_time = -1; // -1 indica que ainda não começou execução
    pcb->turnaround_time = 0;
    pcb->last_scheduled = 0;

    pcb->registers = calloc(NUM_REGISTERS, sizeof(unsigned short int));
    if (!pcb->registers) {
        printf("[Sistema] Erro: Falha na alocação dos registradores\n");
        free(pcb);
        return NULL;
    }

    pcb->using_io = false;
    pcb->io_block_cycles = 0;
    pcb->waiting_resource = false;
    pcb->resource_name = NULL;

    pcb->total_instructions = 0;
    pcb->waiting_time = 0;
    pcb->turnaround_time = 0;
    pcb->completion_time = 0;

    all_processes[total_processes++] = pcb;
    printf("[Sistema] Processo %d criado\n", pcb->pid);
    show_process_state(pcb->pid, "CREATED", "NEW");

    return pcb;
}

void save_context(PCB* pcb, core* current_core) {
    if (!pcb || !current_core) return;

    // Salva PC e outros estados
    pcb->PC = current_core->PC;
    memcpy(pcb->registers, current_core->registers, NUM_REGISTERS * sizeof(unsigned short int));
    pcb->quantum_remaining = current_core->quantum_remaining;
    
   // printf("\n[Context] Salvando contexto do processo %d", pcb->pid);
   // printf("\n - PC: %d", pcb->PC);
   // printf("\n - Quantum: %d", pcb->quantum_remaining);
}

void restore_context(PCB* pcb, core* current_core) {
    if (!pcb || !current_core) return;

    current_core->PC = pcb->PC;
    memcpy(current_core->registers, pcb->registers, NUM_REGISTERS * sizeof(unsigned short int));
    current_core->quantum_remaining = pcb->quantum_remaining;
    
    // printf("\n[Context] Restaurando contexto do processo %d", pcb->pid);
    // printf("\n - PC: %d", current_core->PC);
    //printf("\n - Quantum: %d", current_core->quantum_remaining);
}

void free_pcb(PCB* pcb) {
    if (!pcb) return;

    if (pcb->registers) {
        free(pcb->registers);
    }
    if (pcb->resource_name) {
        free(pcb->resource_name);
    }
    free(pcb);
}

const char* state_to_string(process_state state) {
    switch(state) {
        case NEW: return "NEW";
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case BLOCKED: return "BLOCKED";
        case FINISHED: return "FINISHED";
        default: return "UNKNOWN";
    }
}

ProcessManager* init_process_manager(int quantum_size) {
    ProcessManager* pm = malloc(sizeof(ProcessManager));
    if (!pm) {
        printf("[Sistema] Erro: Falha na alocação do gerenciador\n");
        return NULL;
    }

    // printf("\n[Debug] Inicializando Process Manager");
    // printf("\n - Endereço: %p", (void*)pm);
    
    pm->ready_queue = malloc(sizeof(PCB*) * MAX_PROCESSES);
    pm->blocked_queue = malloc(sizeof(PCB*) * MAX_PROCESSES);

    if (!pm->ready_queue || !pm->blocked_queue) {
        printf("[Sistema] Erro: Falha na alocação das filas\n");
        free(pm);
        return NULL;
    }

    pm->ready_count = 0;
    pm->blocked_count = 0;
    pm->quantum_size = quantum_size;
    pm->current_time = 0;
    pm->policy = NULL;  

    pthread_mutex_init(&pm->queue_mutex, NULL);
    pthread_mutex_init(&pm->resource_mutex, NULL);
    pthread_cond_init(&pm->resource_condition, NULL);

    printf("[Sistema] Gerenciador de processos iniciado com sucesso\n");
    return pm;
}

void schedule_next_process(cpu* cpu, int core_id) {
  if (!cpu || !cpu->process_manager) return;

  ProcessManager* pm = cpu->process_manager;
  lock_process_manager(pm);

  if (pm->ready_count > 0) {
      // Métricas específicas para Cache-Aware
      if (pm->policy->type == POLICY_CACHE_AWARE) {
          printf("\n[Cache Analysis] Escalonando processos");
        //   printf("\n[Debug] Core %d scheduling (Ready: %d)", core_id, pm->ready_count);
          printf("\n - Core %d available: %d", core_id, cpu->core[core_id].is_available);
          
          for(int i = 0; i < pm->ready_count; i++) {
              PCB* process = pm->ready_queue[i];
              CacheEntry* entry = &cache[process->base_address % CACHE_SIZE];
              
              printf("\n\nProcesso P%d:", process->pid);
              printf("\n - Hits/Misses: %d/%d", entry->hits, entry->misses);
              printf("\n - Hit Ratio: %.2f%%", entry->hit_ratio * 100);
              printf("\n - Último acesso: %lds atrás", time(NULL) - entry->last_access);
          }
        //   printf("\n[Debug] About to call select_next");
      }

      PCB* next_process = pm->policy->select_next(pm);
      if (next_process) {
          if (next_process->start_time == 0) {
              next_process->start_time = pm->current_time;
          }

            if (next_process->response_time == -1) {
        next_process->response_time = pm->current_time;
    }
          
          cpu->core[core_id].quantum_remaining = pm->quantum_size;
          cpu->core[core_id].is_available = false;
          cpu->core[core_id].current_process = next_process;
          cpu->core[core_id].PC = next_process->PC;
          
          next_process->state = RUNNING;
          next_process->core_id = core_id;

          next_process->waiting_time += (pm->current_time - next_process->last_scheduled);
          next_process->last_scheduled = pm->current_time;
          
          restore_context(next_process, &cpu->core[core_id]);
          show_process_state(next_process->pid, "READY", "RUNNING");
          
          if (pm->policy->type == POLICY_CACHE_AWARE) {
            //   printf("\n[Cache Debug] Process %d scheduled on core %d", 
            //       next_process->pid, core_id);
              printf("\n - PC: %d", next_process->PC);
              printf("\n - Quantum: %d", cpu->core[core_id].quantum_remaining);
          }
      }
  }

  unlock_process_manager(pm);
}

void check_blocked_processes(cpu* cpu) {
    ProcessManager* pm = cpu->process_manager;
    
    lock_process_manager(pm);

    for (int i = 0; i < pm->blocked_count; i++) {
        PCB* process = pm->blocked_queue[i];
        if (!process) continue;

        if (process->io_block_cycles > 0) {
            process->io_block_cycles--;
            if (process->io_block_cycles == 0) {
                process->state = READY;
                pm->ready_queue[pm->ready_count++] = process;
                
                for (int j = i; j < pm->blocked_count - 1; j++) {
                    pm->blocked_queue[j] = pm->blocked_queue[j + 1];
                }
                pm->blocked_count--;
                i--;

                show_process_state(process->pid, "BLOCKED", "READY");
            }
        }
    }

    unlock_process_manager(pm);
}

void lock_process_manager(ProcessManager* pm) {
    pthread_mutex_lock(&pm->queue_mutex);
}

void unlock_process_manager(ProcessManager* pm) {
    pthread_mutex_unlock(&pm->queue_mutex);
}

int count_ready_processes(ProcessManager* pm) {
    if (!pm) return 0;
    return pm->ready_count;
}