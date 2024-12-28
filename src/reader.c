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

char* get_line_of_program(char* program_start, int line_number) {
    if (!program_start) {
        printf("Debug - Null program pointer\n");
        return NULL;
    }
    
    printf("\nDebug - Getting line %d from program at %p\n", line_number, (void*)program_start);
    printf("Debug - First few bytes: %.30s\n", program_start);
    
    // Encontra a linha desejada
    char* current = program_start;
    int current_line = 0;
    
    while (current_line < line_number) {
        if (*current == '\0') {
            printf("Debug - End of program at line %d\n", current_line);
            return NULL;
        }
        if (*current == '\n') {
            current_line++;
        }
        current++;
    }
    
    // Encontra fim da linha atual
    char* line_end = strchr(current, '\n');
    if (!line_end) {
        line_end = current + strlen(current);
    }
    
    // Aloca e copia a linha
    size_t len = line_end - current;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    strncpy(result, current, len);
    result[len] = '\0';
    
    // Remove espaços finais
    while (len > 0 && isspace(result[len - 1])) {
        result[--len] = '\0';
    }
    
    printf("Debug - Found line: '%s'\n", result);
    return result;
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