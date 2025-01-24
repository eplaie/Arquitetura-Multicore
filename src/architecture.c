#include "architecture.h"
#include "os_display.h"
#include "cpu.h"
#include "ram.h"
#include "disc.h"
#include "peripherals.h"
#include "pcb.h"
#include "pipeline.h"
#include "cache.h"
extern CacheEntry cache[];

void init_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc, 
                     peripherals* peripherals, architecture_state* state) {
   (void)memory_disc;    
   (void)peripherals;     
   printf("\n[Init] Iniciando arquitetura");
   printf("\n[Init] Verificação inicial:");
   printf("\n - CPU: %p", (void*)cpu);
   printf("\n - RAM: %p", (void*)memory_ram);
   printf("\n - RAM vector: %p", (void*)(memory_ram ? memory_ram->vector : NULL));

   if (!cpu || !memory_ram || !memory_ram->vector) {
       printf("\n[Init] ERRO: Ponteiro NULL detectado");
       printf("\n - CPU: %p", (void*)cpu);
       printf("\n - RAM: %p", (void*)memory_ram);
       printf("\n - RAM vector: %p", (void*)(memory_ram ? memory_ram->vector : NULL));
       exit(1);
   }

   init_cpu(cpu, memory_ram);

   printf("\n[Init] Verificação após init_cpu:");
   printf("\n - RAM: %p", (void*)memory_ram);
   printf("\n - RAM vector: %p", (void*)memory_ram->vector);
   printf("\n - CPU memory_ram: %p", (void*)cpu->memory_ram);
   printf("\n - CPU memory_ram vector: %p", (void*)(cpu->memory_ram ? cpu->memory_ram->vector : NULL));

   printf("\n[Init] Criando threads dos cores");
   for (int i = 0; i < NUM_CORES; i++) {
       core_thread_args* args = malloc(sizeof(core_thread_args));
       if (!args) {
           printf("\n[Init] ERRO: Falha na alocação dos argumentos da thread %d", i);
           exit(1);
       }

       args->cpu = cpu;
       args->memory_ram = memory_ram;  
       args->core_id = i;
       args->state = state;
       cpu->core[i].arch_state = state;

       printf("\n[Debug] Argumentos da thread %d:", i);
       printf("\n - RAM: %p", (void*)args->memory_ram);
       printf("\n - RAM vector: %p", (void*)args->memory_ram->vector);
       
       if (pthread_create(&cpu->core[i].thread, NULL, core_execution_thread, args) != 0) {
           printf("\n[Init] ERRO: Falha na criação da thread do core %d", i);
           exit(1);
       }

       printf("\n[Init] Thread do core %d criada com sucesso", i);
   }

   state->pipeline = malloc(sizeof(pipeline));
   if (!state->pipeline) {
       printf("\n[Init] ERRO: Falha na alocação do pipeline");
       exit(1);
   }
   init_pipeline(state->pipeline);

   // Inicializa métricas e estado global 
   state->program_running = true;
   state->cycle_count = 0;
   state->total_instructions = 0;
   state->completed_processes = 0;
   state->blocked_processes = 0;
   state->context_switches = 0;
   state->avg_turnaround = 0;

   pthread_mutex_init(&state->global_mutex, NULL);

   printf("\n[Init] Arquitetura inicializada com sucesso");
   printf("\n - Cores ativos: %d", NUM_CORES);
   printf("\n - Quantum: %d", DEFAULT_QUANTUM);
}

void update_system_metrics(architecture_state* state) {
    pthread_mutex_lock(&state->global_mutex);
    
    float total_turnaround = 0;
    int completed = 0;
    
    for (int i = 0; i < total_processes; i++) {
        if (all_processes[i] && all_processes[i]->was_completed) {
            total_turnaround += all_processes[i]->cycles_executed;
            completed++;
        }
    }
    
    if (completed > 0) {
        state->avg_turnaround = total_turnaround / completed;
    }
    
    pthread_mutex_unlock(&state->global_mutex);
}

void check_system_state(architecture_state* state, cpu* cpu) {
    pthread_mutex_lock(&state->global_mutex);
    
    int running = 0, ready = 0, blocked = 0;
    
    ready = cpu->process_manager->ready_count;
    blocked = cpu->process_manager->blocked_count;
    
    for (int i = 0; i < NUM_CORES; i++) {
        if (!cpu->core[i].is_available && 
            cpu->core[i].current_process && 
            cpu->core[i].current_process->state == RUNNING) {
            running++;
        }
    }
    
    state->blocked_processes = blocked;
    state->program_running = (running + ready + blocked) > 0;
    
    pthread_mutex_unlock(&state->global_mutex);
    show_scheduler_state(ready, blocked);
}

void free_architecture(cpu* cpu, ram* memory_ram, disc* memory_disc,
                     peripherals* peripherals, architecture_state* state,
                     int cycle_count) {
   printf("\n═══════════ Métricas Finais ═══════════");

    //    printf("\nDebug - Process Manager: %p", state->process_manager);
    if (state->process_manager) {
        // printf("\nDebug - Policy: %p", state->process_manager->policy);
        if (state->process_manager->policy) {
            // printf("\nDebug - Policy Type: %d", state->process_manager->policy->type);
        }
    }
   
   if (state && state->process_manager && state->process_manager->policy) {
       switch(state->process_manager->policy->type) {
            case POLICY_CACHE_AWARE:
            printf("\nDesempenho de Cache:");
            {
                long total_hits = 0, total_misses = 0;
                float avg_hit_ratio = 0.0;
                
                for(int i = 0; i < total_processes; i++) {
                    if (all_processes[i]) {
                        int idx = all_processes[i]->base_address % CACHE_SIZE;
                        total_hits += cache[idx].hits;
                        total_misses += cache[idx].misses;
                        float hit_ratio = (cache[idx].hits + cache[idx].misses) > 0 ? 
                            (float)cache[idx].hits / (cache[idx].hits + cache[idx].misses) * 100 : 0.0;
                            
                        printf("\n - P%d: %d hits, %d misses (%.1f%% hit ratio)", 
                            i, cache[idx].hits, cache[idx].misses, hit_ratio);
                        printf("\n   * Acessos totais: %d", cache[idx].hits + cache[idx].misses);
                        printf("\n   * Eficiência: %.1f%%", (hit_ratio * cache[idx].hits * 100) / 
                            (hit_ratio * cache[idx].hits + MISS_PENALTY * cache[idx].misses));
                    }
                }

                avg_hit_ratio = (total_hits + total_misses) > 0 ? 
                    (float)total_hits / (total_hits + total_misses) * 100 : 0.0;

                printf("\n\nEstatísticas Globais da Cache:");
                printf("\n - Total hits: %ld", total_hits);
                printf("\n - Total misses: %ld", total_misses);
                printf("\n - Hit ratio médio: %.1f%%", avg_hit_ratio);
                // printf("\n - Ciclos perdidos em misses: %ld", total_misses * MISS_PENALTY);
                printf("\n - Penalidade média por processo: %.1f ciclos", 
                    (float)(total_misses * MISS_PENALTY) / total_processes);

                    float cache_throughput = (total_hits + total_misses) / (float)cycle_count;
                    printf("\n - Cache throughput: %.2f acessos/ciclo", cache_throughput);

                float total_efficiency = 0;
                        for(int i = 0; i < total_processes; i++) {
                            if (all_processes[i]) {
                                int idx = all_processes[i]->base_address % CACHE_SIZE;
                                float process_ratio = (float)cache[idx].hits / (cache[idx].hits + cache[idx].misses);
                                total_efficiency += process_ratio;
                            }
                        }
                        printf("\n - Eficiência média: %.1f%%", (total_efficiency / total_processes) * 100);
                                    }

                                    printf("\n - Regiões mais acessadas:");
                    for(int i = 0; i < CACHE_SIZE; i++) {
                        if(cache[i].hits + cache[i].misses > 0) {
                            printf("\n   * Endereço %d: %d acessos (H:%d/M:%d) - %.1f%% eficiência", 
                                i, 
                                cache[i].hits + cache[i].misses,
                                cache[i].hits,
                                cache[i].misses,
                                (float)cache[i].hits/(cache[i].hits + cache[i].misses) * 100);
                        }
                    }
            break;
               
           case POLICY_SJF:
               printf("\nMétricas SJF:");
               for(int i = 0; i < total_processes; i++) {
                   if (all_processes[i]) {
                       printf("\n - P%d: Tamanho %zu bytes, Tempo execução %d ciclos", 
                           i, all_processes[i]->program_size,
                           all_processes[i]->cycles_executed);
                   }
               }
               break;
               
           case POLICY_LOTTERY:
    printf("\nMétricas Lottery:");
    
    float avg_waiting = 0, avg_response = 0, avg_turnaround = 0;
    int completed = 0;
    
    for(int i = 0; i < total_processes; i++) {
        if (all_processes[i]) {
            avg_waiting += all_processes[i]->waiting_time;
            avg_response += all_processes[i]->response_time;
            avg_turnaround += all_processes[i]->turnaround_time;
            completed++;
        }
    }
    
    printf("\nTempos Médios:");
    printf("\n - Espera: %.2f ciclos", avg_waiting/completed);
    printf("\n - Resposta: %.2f ciclos", avg_response/completed); 
    printf("\n - Turnaround: %.2f ciclos", avg_turnaround/completed);
    break;
               
           case POLICY_RR:
               printf("\nMétricas Round Robin:");
               for(int i = 0; i < total_processes; i++) {
                   if (all_processes[i]) {
                       printf("\n - P%d: Preempções: %d", 
                           i, all_processes[i]->cycles_executed / DEFAULT_QUANTUM);
                   }
               }
               break;
       }
   }

   printf("\n\nExecução:");
   printf("\n - Ciclos totais: %d", cycle_count);
   printf("\n - Instruções totais: %d", state->total_instructions);
   printf("\n - IPC médio: %.2f", (float)state->total_instructions / cycle_count);
   printf("\n - Trocas de contexto: %d", state->context_switches);

   printf("\n\nUtilização:");
   printf("\n - Ocupação dos cores: %.1f%%", 
          (float)(state->total_instructions * 100) / (cycle_count * NUM_CORES));
   printf("\n═════════════════════════════════════\n");

   cleanup_cpu_threads(cpu);

   if (memory_ram) {
       free(memory_ram->vector);
       free(memory_ram);
   }

   if (state->process_manager) {
       free(state->process_manager->ready_queue);
       free(state->process_manager->blocked_queue);
       free(state->process_manager);
   }

   if (state->pipeline) {
       free(state->pipeline);
   }

   if (memory_disc) {
       free(memory_disc);
   }
   if (peripherals) {
       free(peripherals);
   }

   pthread_mutex_destroy(&state->global_mutex);

   free(state);

   for (int i = 0; i < total_processes; i++) {
       if (all_processes[i]) {
           free_pcb(all_processes[i]);
       }
   }
}