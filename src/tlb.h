#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tamanho da TLB
#define TLB_SIZE 16

// Estrutura para entrada da TLB
typedef struct {
    int pid;              // ID original do processo
    char* binary_pid;     // ID em formato binário
    int valid;            // Flag de entrada válida
} TLBEntry;


void insert_pid_in_tlb(int pid);
char* lookup_pid_in_tlb(int pid);
void init_process_tlb();
void free_process_tlb();
void print_tlb_contents();


#ifndef TLB_H
#define TLB_H

#endif //TLB_H
