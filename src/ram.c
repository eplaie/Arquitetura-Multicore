#include "ram.h"
#include "cpu.h" 

ram* allocate_ram(size_t memory_size) {
    ram* memory_ram = malloc(sizeof(ram));
    if (!memory_ram) {
        printf("ERRO: Falha ao alocar estrutura RAM\n");
        return NULL;
    }

    // Alocar e verificar o vetor
    memory_ram->vector = calloc(memory_size, sizeof(char));
    if (!memory_ram->vector) {
        printf("ERRO: Falha ao alocar memória para o vetor RAM\n");
        free(memory_ram);
        return NULL;
    }

    //printf("RAM alocada com sucesso (%zu bytes)\n", memory_size);
    //printf("RAM vector: %p\n", (void*)memory_ram->vector);

    memory_ram->size = memory_size;
    memory_ram->initialized = true;
    pthread_mutex_init(&memory_ram->mutex, NULL);
    
    return memory_ram;
}

bool verify_ram(ram* memory_ram, const char* context) {
    if (!memory_ram) {
        printf("\n[Verify RAM] RAM nula em %s", context);
        return false;
    }
    if (!memory_ram->vector) {
      //  printf("\n[Verify RAM] Vector nulo em %s", context);
        return false;
    }
    //printf("\n[Verify RAM] RAM ok em %s:", context);
    //printf("\n - RAM: %p", (void*)memory_ram);
    //printf("\n - Vector: %p", (void*)memory_ram->vector);
    return true;
}

void write_ram(ram* memory_ram, unsigned short int address, const char* data) {
    if (!memory_ram || !memory_ram->vector) {
       // printf("\n[RAM] Erro: RAM inválida ou não inicializada");
        return;
    }

    if (address >= NUM_MEMORY) {
        //printf("\n[RAM] Erro: Endereço fora dos limites");
        return;
    }

    // Copiar a string para o endereço especificado na RAM
    size_t data_length = strlen(data);

    // Verificar se há espaço suficiente
    if (address + data_length > NUM_MEMORY) {
        printf("\n[RAM] Erro: Espaço insuficiente na memória");
        return;
    }

    //printf("\n[RAM] Escrevendo '%s' no endereço %d", data, address);

    // Copiar os dados para a RAM
    strncpy(memory_ram->vector + address, data, data_length);

    // Garantir terminador nulo se houver espaço
    if (address + data_length < NUM_MEMORY) {
        memory_ram->vector[address + data_length] = '\0';
    }

    //printf("\n[RAM] Escrita concluída com sucesso");
}

void load_program_on_ram(struct cpu* cpu, char* program_content, unsigned int base_address, PCB* pcb) {
    if (!cpu || !cpu->memory_ram || !cpu->memory_ram->vector || 
        !cpu->memory_ram->initialized || !pcb) {  // Adicionada verificação do pcb
        printf("\nERRO: CPU, RAM ou PCB inválido\n");
        return;
    }

     pthread_mutex_lock(&cpu->memory_ram->mutex);

    if (!program_content) {
        printf("\nERRO: Programa inválido\n");
        return;
    }

    size_t program_length = strlen(program_content);

    pcb->program_size = program_length;

    // Verifica se há espaço suficiente na memória
    if (base_address + program_length >= NUM_MEMORY) {
        printf("\nERRO: Programa excede limite da memória\n");
        return;
    }

    // Limpa a área de memória antes de carregar
    char* dest_addr = cpu->memory_ram->vector + base_address;
    memset(dest_addr, 0, program_length + 1);

    // Copia o programa
    strncpy(dest_addr, program_content, program_length);
    dest_addr[program_length] = '\0';

    pthread_mutex_unlock(&cpu->memory_ram->mutex);

}