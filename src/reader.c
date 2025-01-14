#include "reader.h"

char* read_program(const char* filename) {
    if (!filename) {
        printf("Error: NULL filename\n");
        return NULL;
    }

    FILE* arq = fopen(filename, "r");
    if (arq == NULL) {
        printf("Error opening file: %s\n", filename);
        return NULL;
    }

    // Determina o tamanho do arquivo
    fseek(arq, 0, SEEK_END);
    long fsize = ftell(arq);
    fseek(arq, 0, SEEK_SET);

    if (fsize <= 0) {
        printf("Error: Empty or invalid file\n");
        fclose(arq);
        return NULL;
    }

    // Aloca memória para o conteúdo + caractere nulo
    char* program = (char*)malloc(fsize + 1);
    if (program == NULL) {
        printf("Error: Memory allocation failed for program\n");
        fclose(arq);
        return NULL;
    }

    // Lê o arquivo
    size_t read_size = fread(program, 1, fsize, arq);
    if (read_size != (size_t)fsize) {
        printf("Error: Failed to read entire file\n");
        free(program);
        fclose(arq);
        return NULL;
    }

    program[fsize] = '\0';
    fclose(arq);
    return program;
}

char* get_line_of_program(char* program_start, unsigned short int line_number) {
        printf("\n[Program] Lendo linha %hu", line_number);
    printf("\n[Program] Endereço: %p", (void*)program_start);
    if (!program_start) {
        printf("\n╔═══ Program Status ═══╗");
        printf("\n║ Error: Null Pointer  ║");
        printf("\n╚══════════════════════╝\n");
        return NULL;
    }

    printf("\n╔═══ Program Read Operation ═══╗");
    printf("\n├── Line: %hu", line_number);
    printf("\n├── Address: %p", (void*)program_start);
    printf("\n└── Content Preview: %.30s\n", program_start);

    // Encontra a linha desejada
    char* current = program_start;
    unsigned short int current_line = 0;

    while (current_line < line_number) {
        if (*current == '\0') {
            printf("\n[Program Status] End reached at line %hu\n", current_line);
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
    if (!result) {
        printf("\n[Memory Error] Failed to allocate line buffer\n");
        return NULL;
    }

    strncpy(result, current, len);
    result[len] = '\0';

    // Remove espaços finais
    while (len > 0 && isspace(result[len - 1])) {
        result[--len] = '\0';
    }

    printf("\n[Line Found] '%s'\n", result);
    return result;
}

unsigned short int count_tokens_in_line(char* line) {
    if (!line) return 0;
    
    unsigned short int count = 0;
    char* line_copy = strdup(line);
    char* token;
    char* saveptr;
    
    token = strtok_r(line_copy, " ", &saveptr);
    while (token != NULL) {
        count++;
        token = strtok_r(NULL, " ", &saveptr);
    }
    
    free(line_copy);
    return count;
}

/* Função comentada - pode ser reativada se necessário
unsigned short int count_lines(char* program) {
    if (!program) return 0;
    
    unsigned short int count = 0;
    char* program_copy = strdup(program);
    char* line = strtok(program_copy, "\n");
    
    while (line != NULL) {
        count++;
        line = strtok(NULL, "\n");
    }
    
    free(program_copy);
    return count;
}
*/