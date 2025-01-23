#ifndef CACHE_H
#define CACHE_H

#include "pcb.h"
#include <stdbool.h>


void init_cache();
bool check_cache(unsigned int address);
void update_cache(unsigned int address, char* data);
float calculate_similarity(PCB* p1, PCB* p2, ram* memory_ram);
float get_cache_benefit(PCB* process);
char* get_program_content(PCB* pcb, ram* memory_ram);

#endif