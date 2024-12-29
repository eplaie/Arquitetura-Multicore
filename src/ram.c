#include "ram.h"

void init_ram(ram* memory_ram) {
    if (!memory_ram) {
        printf("Error: NULL pointer in init_ram\n");
        exit(1);
    }

    // Aloca memória para o vetor
    memory_ram->vector = (char*)calloc(NUM_MEMORY, sizeof(char));
    if (memory_ram->vector == NULL) {
        printf("Failed to allocate RAM memory\n");
        exit(1);
    }

    // Inicializa a memória com zeros
    memset(memory_ram->vector, 0, NUM_MEMORY);
    printf("RAM initialized and cleared (%d bytes)\n", NUM_MEMORY);
}

void print_ram(ram* memory_ram) {
    int last_non_empty = 0;
    
    // Encontra a última posição com conteúdo
    for (unsigned short int i = 0; i < NUM_MEMORY; i++) {
        if (memory_ram->vector[i] != '\0' && memory_ram->vector[i] != '_') {
            last_non_empty = i;
        }
    }
    
    // Imprime até um pouco depois do último conteúdo
    for (unsigned short int i = 0; i <= last_non_empty + 1; i++) {
        if(memory_ram->vector[i] == '\0') {
            printf("_");
        } else {
            printf("%c", memory_ram->vector[i]);
        }
    }
    printf("\n");
}

void print_ram_segment(ram* memory_ram, int start, int length) {
    printf("RAM segment from %d to %d:\n", start, start + length - 1);
    for (int i = start; i < start + length && i < NUM_MEMORY; i++) {
        if (memory_ram->vector[i] == '\0') {
            printf("_");
        } else {
            printf("%c", memory_ram->vector[i]);
        }
    }
    printf("\n");
}