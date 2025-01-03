#include "reader.h"
#include "os_display.h"

char* read_program(const char *filename) {
    if (!filename) {
        show_system_summary(0, 0, 0, 0);  // Mostra estado do sistema com erro
        return NULL;
    }

    FILE *arq = fopen(filename, "r");
    if (arq == NULL) {
        show_memory_operation(-1, "Failed to open program file", 0);
        return NULL;
    }

    // Determina o tamanho do arquivo
    fseek(arq, 0, SEEK_END);
    long fsize = ftell(arq);
    fseek(arq, 0, SEEK_SET);

    if (fsize <= 0) {
        show_memory_operation(-1, "Invalid program file size", 0);
        fclose(arq);
        return NULL;
    }

    // Aloca memória para o conteúdo + caractere nulo
    char *program = (char*)malloc(fsize + 1);
    if (program == NULL) {
        show_memory_operation(-1, "Memory allocation failed", fsize + 1);
        fclose(arq);
        return NULL;
    }

    // Lê o arquivo
    size_t read_size = fread(program, 1, fsize, arq);
    if (read_size != (size_t)fsize) {
        show_memory_operation(-1, "Failed to read program", fsize);
        free(program);
        fclose(arq);
        return NULL;
    }

    program[fsize] = '\0';
    fclose(arq);

    // Programa lido com sucesso
    show_memory_operation(-1, "Program loaded successfully", fsize);
    return program;
}

char* get_line_of_program(char* program_start, int line_number) {
    if (!program_start) return NULL;

    // Encontra a linha desejada
    char* current = program_start;
    int current_line = 0;

    while (current_line < line_number) {
        if (*current == '\0') return NULL;
        if (*current == '\n') current_line++;
        current++;
    }

    // Encontra fim da linha atual
    char* line_end = strchr(current, '\n');
    if (!line_end) line_end = current + strlen(current);

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

    // Se a linha for uma instrução válida, mostra na visualização
    if (len > 0) {
        // show_cpu_execution(-1, -1, result, line_number);  // -1 indica que ainda não está associado a um core/processo
    }

    return result;
}

unsigned short int count_lines(char *program) {
    if (!program) return 0;

    unsigned short int count = 0;
    char* program_copy = strdup(program);

    char *line = strtok(program_copy, "\n");
    while (line != NULL) {
        count++;
        line = strtok(NULL, "\n");
    }

    free(program_copy);
    return count;
}

unsigned short int count_tokens_in_line(char *line) {
    if (!line) return 0;

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