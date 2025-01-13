#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "libs.h"
#include "reader.h"
#include "common_types.h"  // Inclui type_of_instruction daqui

// Funções de verificação de instruções
type_of_instruction verify_instruction(char *line, unsigned short int line_number);

// Funções de verificação de formato
bool check_load_format(char *line);
bool check_store_format(char *line);
bool check_add_format(char *line);
bool check_sub_format(char *line);
bool check_mul_format(char *line);
bool check_div_format(char *line);
bool check_if_format(char *line);
bool check_else_format(char *line);
bool check_loop_format(char *line);
bool check_loop_end_format(char *line);
bool check_if_end_format(char *line);
bool check_else_end_format(char *line);

#endif