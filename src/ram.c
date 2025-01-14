#include "ram.h"


ram* allocate_ram(size_t memory_size) {
    // Alocar a estrutura ram
    ram* memory_ram = malloc(sizeof(ram));
    if (!memory_ram) {
        printf("ERRO: Falha ao alocar estrutura RAM\n");
        return NULL;
    }

    // Alocar memória para o vetor interno
    memory_ram->vector = malloc(memory_size);
    if (!memory_ram->vector) {
        printf("ERRO: Falha ao alocar memória para o vetor RAM\n");
        free(memory_ram);  // Liberar a estrutura caso o vetor falhe
        return NULL;
    }

    printf("RAM alocada com sucesso (%zu bytes)\n", memory_size);
    return memory_ram;
}



void write_ram(ram* memory_ram, unsigned short int address, const char* data) {
    if (!memory_ram || !memory_ram->vector) {
        printf("Error: Invalid RAM or uninitialized memory\n");
        return;
    }

    if (address >= NUM_MEMORY) {
        printf("Error: Memory address out of bounds\n");
        return;
    }

    // Copiar a string para o endereço especificado na RAM
    size_t data_length = strlen(data);
    
    // Verificar se há espaço suficiente
    if (address + data_length > NUM_MEMORY) {
        printf("Error: Not enough memory space to write data\n");
        return;
    }

    // Copiar os dados para a RAM
    strncpy(memory_ram->vector + address, data, data_length);
    
    // Garantir terminador nulo se houver espaço
    if (address + data_length < NUM_MEMORY) {
        memory_ram->vector[address + data_length] = '\0';
    }
}

void load_program_on_ram(ram* memory_ram, char* program_content) {
    if (!memory_ram || !program_content) {
        printf("ERRO: RAM ou programa inválido\n");
        return;
    }
    // Verificar tamanho do programa
    size_t program_length = strlen(program_content);
    printf("\n[RAM] Carregando programa:");
    printf("\n  - Tamanho: %zu bytes", program_length);
    printf("\n  - Endereço RAM: %p", (void*)memory_ram->vector);
    printf("\n  - Conteúdo: %s\n", program_content);
    if (program_length >= NUM_MEMORY) {
        printf("ERRO: Programa muito grande para a memória RAM\n");
        return;
    }
    // Limpar área da RAM antes de copiar
    size_t clear_size = program_length + 1;
    if (clear_size > NUM_MEMORY) clear_size = NUM_MEMORY;
    if (!memory_ram->vector) {
        printf("ERRO: Memória RAM não alocada\n");
        return;
    }
    memset(memory_ram->vector, 0, clear_size);
    // Copiar programa para RAM
    strncpy(memory_ram->vector, program_content, program_length);
    memory_ram->vector[program_length] = '\0';

    printf("[RAM] Programa carregado com sucesso: %zu bytes\n", program_length);
}