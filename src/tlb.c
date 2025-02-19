#include "tlb.h"

TLBEntry tlb[TLB_SIZE];

// Converte PID para binário
char* convert_pid_to_binary(int pid) {
    char* binary = (char*)malloc(33); // 32 bits + terminador nulo
    binary[32] = '\0';

    // Inicializa string com zeros
    for(int i = 0; i < 32; i++) {
        binary[i] = '0';
    }

    // Converte para binário
    int i = 31;
    while(pid > 0 && i >= 0) {
        binary[i] = (pid % 2) + '0';
        pid = pid / 2;
        i--;
    }

    return binary;
}

// Função de hash
int hash_pid(int pid) {
    return pid % TLB_SIZE;
}

// Inicializa a TLB
void init_process_tlb() {
    for(int i = 0; i < TLB_SIZE; i++) {
        tlb[i].pid = -1;
        tlb[i].binary_pid = NULL;
        tlb[i].valid = 0;
    }
}

// Insere PID na TLB
void insert_pid_in_tlb(int pid) {
    int index = hash_pid(pid);

    // Libera entrada anterior se existir
    if(tlb[index].binary_pid != NULL) {
        free(tlb[index].binary_pid);
    }

    // Insere nova entrada
    tlb[index].pid = pid;
    tlb[index].binary_pid = convert_pid_to_binary(pid);
    tlb[index].valid = 1;
}

// Busca PID na TLB
char* lookup_pid_in_tlb(int pid) {
    int index = hash_pid(pid);

    if(tlb[index].valid && tlb[index].pid == pid) {
        return tlb[index].binary_pid;
    }
    return NULL;
}

// Libera a TLB
void free_process_tlb() {
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb[i].binary_pid != NULL) {
            free(tlb[i].binary_pid);
            tlb[i].binary_pid = NULL;
        }
    }
}

// Imprime conteúdo da TLB
void print_tlb_contents() {
    printf("\n=== TLB Contents ===\n");
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb[i].valid) {
            printf("Index %d: PID %d -> Binary: %s\n",
                   i, tlb[i].pid, tlb[i].binary_pid);
        }
    }
    printf("==================\n");
}