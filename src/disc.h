#ifndef DISC_H
#define DISC_H

#define NUM_MEMORY 1024

#include "libs.h"

typedef struct disc {
    unsigned short int **matriz;
} disc;

void init_disc(disc* memory_disc);

#endif