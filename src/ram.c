#include "ram.h"

void init_ram(ram* memory_ram) {
    memory_ram->vector = malloc(NUM_MEMORY * sizeof(char));

    if (memory_ram->vector == NULL) {
            printf("memory allocation failed in ram\n");
            exit(1);
        }
    
    for (unsigned short int i = 0; i < NUM_MEMORY; i++) {
        memory_ram->vector[i] = '\0'; 
    }
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