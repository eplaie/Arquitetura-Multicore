#include "reader.h"

char* read_program(const char *filename) {
    FILE *arq = fopen(filename, "r"); 
    if (arq == NULL) {
        printf("error opening file\n");
        exit(1);
    }

    char *program = NULL;  
    size_t length = 0;     
    int ch;

    while ((ch = fgetc(arq)) != EOF) {
        program = (char*) realloc(program, length + 2);  
        if (program == NULL) {
            printf("memory allocation failed\n");
            exit(1);
        }
        program[length] = ch;
        length++;
        program[length] = '\0';
    }

    fclose(arq);
    return program;  
}

char* get_line_of_program(char *program, int base_line) {
    if (!program) {
        printf("Error: Null program pointer\n");
        return NULL;
    }

    // Avança até a posição base
    program += base_line;

    // Encontra o início da próxima linha não vazia
    while (*program && isspace(*program)) {
        program++;
    }

    if (!*program) {
        return NULL;
    }

    // Encontra o fim da linha
    char* line_end = program;
    while (*line_end && *line_end != '\n') {
        line_end++;
    }

    // Calcula o tamanho da linha
    size_t line_length = line_end - program;

    // Aloca espaço para a linha + terminador nulo
    char* line = (char*)malloc(line_length + 1);
    if (!line) {
        printf("Error: Memory allocation failed\n");
        return NULL;
    }

    // Copia a linha
    strncpy(line, program, line_length);
    line[line_length] = '\0';

    return line;
}

unsigned short int count_lines(char *program) {
    unsigned short int count = 0;

    char* program_copy = strdup(program);

    char *line = strtok(program_copy, "\n"); 

    while (line != NULL) {
        count++; 
        line = strtok(NULL, "\n"); 
    }

    return count; 
}

unsigned short int count_tokens_in_line(char *line) {
    unsigned short int count = 0;

    char* line_copy = strdup(line);

    char *token = strtok(line_copy, " "); 

    while (token != NULL) {
        count++; 
        token = strtok(NULL, " "); 
    }

    free(line_copy);
    return count; 
}