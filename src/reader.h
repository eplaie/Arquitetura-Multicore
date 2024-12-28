#ifndef READER_H
#define READER_H

#include "libs.h"

#define PROGRAM_SEPARATOR "\0\0\0"  // Três bytes nulos para separar programas
#define MAX_PROGRAM_SIZE 1024


char* read_program(const char* filename);  
char* get_line_of_program(char *program, int line_number);
unsigned short int count_lines(char *program);
unsigned short int count_tokens_in_line(char *line);

#endif