#include "reader.h"

char* read_program(const char* filename) {
    if (!filename) {
        printf("[Sistema] Erro: Nome de arquivo inválido\n");
        return NULL;
    }

    FILE* arq = fopen(filename, "r");
    if (arq == NULL) {
        printf("[Sistema] Erro ao abrir arquivo: %s\n", filename);
        return NULL;
    }

    // Determina o tamanho do arquivo
    fseek(arq, 0, SEEK_END);
    long fsize = ftell(arq);
    fseek(arq, 0, SEEK_SET);

    if (fsize <= 0) {
        printf("[Sistema] Erro: Arquivo vazio ou inválido\n");
        fclose(arq);
        return NULL;
    }

    // Aloca memória para o conteúdo
    char* program = (char*)malloc(fsize + 1);
    if (program == NULL) {
        printf("[Sistema] Erro: Falha na alocação de memória\n");
        fclose(arq);
        return NULL;
    }

    // Lê o arquivo
    size_t read_size = fread(program, 1, fsize, arq);
    if (read_size != (size_t)fsize) {
        printf("[Sistema] Erro: Falha na leitura do arquivo\n");
        free(program);
        fclose(arq);
        return NULL;
    }

    program[fsize] = '\0';
    fclose(arq);
    return program;
}

char* get_line_of_program(char* program_start, unsigned short int line_number) {
    if (!program_start) {
        printf("[Programa] Erro: Ponteiro inválido\n");
        return NULL;
    }

    // printf("[Debug] Buscando linha %hu\n", line_number);

    // Encontra a linha desejada
    char* current = program_start;
    unsigned short int current_line = 0;

    // Avança até a linha desejada
    while (current_line < line_number) {
        while (*current && *current != '\n') {
            current++;
        }
        if (*current == '\n') {
            current++;
            current_line++;
        } else {
            printf("[Programa] Fim do programa na linha %hu\n", current_line);
            return NULL;
        }
    }

    //printf("[Position] Posição atual: %.20s...\n", current);

    // Encontra fim da linha atual
    char* line_end = strchr(current, '\n');
    if (!line_end) {
        // printf("[Debug] Não encontrou \\n, usando strlen\n");
        line_end = current + strlen(current);
    }

    // Calcula tamanho e aloca
    size_t len = line_end - current;
    char* result = malloc(len + 1);
    if (!result) {
        printf("[Programa] Erro: Falha na alocação de memória\n");
        return NULL;
    }

    // Copia a linha
    strncpy(result, current, len);
    result[len] = '\0';

    // Remove espaços finais
    while (len > 0 && isspace(result[len - 1])) {
        result[--len] = '\0';
    }

    // printf("[Debug] Linha encontrada: '%s'\n", result);
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

/* Função comentada 
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