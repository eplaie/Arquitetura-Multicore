#ifndef READER_H
#define READER_H

#include "libs.h"
#include "common_types.h"

#define PROGRAM_SEPARATOR "\0\0\0"  // Três bytes nulos para separar programas
#define RAM_SIZE 1024

// Funções de leitura e análise de programa
char* read_program(const char* filename);  
char* get_line_of_program(char* program_start, unsigned short int line_number);  // Mudado para unsigned short int
unsigned short int count_tokens_in_line(char* line);

#endif