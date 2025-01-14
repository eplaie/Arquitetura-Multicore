#include "ram.h"
#include "cpu.h" 

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

void load_program_on_ram(struct cpu* cpu, char* program_content, unsigned int base_address) {
    printf("\n[RAM] Verificação antes de carregar:");
    printf("\n - CPU: %p", (void*)cpu);
    printf("\n - CPU memory_ram: %p", cpu ? (void*)cpu->memory_ram : NULL);
    printf("\n - CPU memory_ram vector: %p", (cpu && cpu->memory_ram) ? (void*)cpu->memory_ram->vector : NULL);
    printf("\n - Program base address: %u", base_address);

    if (!cpu || !cpu->memory_ram || !cpu->memory_ram->vector) {
        printf("\nERRO: CPU ou RAM inválida\n");
        return;
    }

    if (!program_content) {
        printf("\nERRO: Programa inválido\n");
        return;
    }

    size_t program_length = strlen(program_content);
    printf("\n[RAM] Carregando programa:");
    printf("\n - Tamanho: %zu bytes", program_length);
    printf("\n - Endereço base: %u", base_address);
    printf("\n - Conteúdo: %s\n", program_content);
    
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

    printf("[RAM] Programa carregado com sucesso em 0x%p\n", (void*)dest_addr);
}