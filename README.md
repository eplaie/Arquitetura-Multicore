<h1 align="center">

</h1>
<div style="display: inline-block;">
<img align="center" height="20px" width="90px" src="https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white"/> 
<img align="center" height="20px" width="90px" src="https://img.shields.io/badge/Made%20for-VSCode-1f425f.svg"/> 
<img align="center" height="20px" width="90px" src="https://img.shields.io/badge/Contributions-welcome-brightgreen.svg?style=flat"/>
</div>
<br>
<p style="font-size:120%;" align="center">
    <a href="#Introdução">Introdução</a> -
    <a href="#estrutura do projeto">Estrutura do Projeto</a> -
    <a href="#arquivos">Arquivos</a> -
    <a href="#Detalhemento do Código">Detalhemento do Código</a> -
    <a href="Testes e Análises dos Resultados">Testes e Análises dos Resultados</a> -
    <a href="#Conclusão">Conclusão</a> -
    <a href="#Compilação e Execução">Compilação e Execução</a> -
    <a href="#Contatos">Contatos</a>
</p>

 <div align="justify">


## Estrutura do Projeto

  De uma forma compacta e organizada, os arquivos e diretórios estão dispostos da seguinte forma:

  ```.
  |
  ├── dataset
  |   |   ├── program.txt
  ├── src
  │   │   ├── architecture.c
  │   │   ├── architecture.h
  │   │   ├── cache.c
  │   │   ├── cache.h
  │   │   ├── cpu.c
  │   │   ├── cpu.h
  │   │   ├── disc.c
  │   │   ├── dic.h
  │   │   ├── interpreter.c
  │   │   ├── interpreter.h
  │   │   ├── libs.h
  │   │   ├── main.c
  │   │   ├── peripherals.c
  │   │   ├── peripherals.h
  │   │   ├── ram.c
  │   │   ├── ram.h
  │   │   ├── reader.c
  │   │   ├── reader.h
  │   │   └── uthash.h
  ```

  ### Arquivos
  
  <div align="justify">
  
  Para a solução proposta os seguintes diretórios/funções foram utilizados: 
  
  - `dataset/program.txt` Arquivo em que se encontra as instruções de _entrada_; em txt.
  - `src/architecture.c`  Arquivo que inicializa componentes como CPU, RAM, disco e periféricos; carrega um programa na RAM, verificando se o tamanho é compatível com a capacidade da memória. Em seguida, verifica as instruções na RAM e realiza um pipeline de execução, onde cada instrução é buscada, decodificada, executada e, em seguida, os resultados são escritos de volta. 
  - `src/architecture.h`Funções principais para inicializar componentes (CPU, RAM, disco e periféricos)
  - `src/cache.c` Operações de gerenciamento de cache usando uma tabela hash. Ele permite adicionar `(add_cache)` e buscar `(search_cache)` entradas de cache com base no endereço, removê-las (remove_cache), exibir todo o conteúdo do cache `(print_cache)` e esvaziar a cache completamente `(empty_cache)`.
  - `src/cache.h` Estrutura básica para o gerenciamento de cache,  biblioteca `uthash` para criação e manipulação de uma tabela hash.
  - `src/cpu.c`  Implementa operações fundamentais para um emulador de CPU, incluindo a inicialização da CPU e de seus núcleos, execução de operações aritméticas por meio de uma unidade lógica aritmética (ULA), e manipulação de instruções para carregar e armazenar dados em registradores e memória RAM.
  - `src/cpu.h` Define a estrutura da CPU e de seus núcleos.
  - `src/disc.c` Inicialização de um "disco de memória", todos os valores são inicializados com zero, o que prepara o "disco" para ser utilizado em operações de leitura e escrita.
  - `src/disc.h` Define uma estrutura e uma função para gerenciar uma simulação de disco de memória.
  - `src/interpreter.c`  Uma implementação de um interpretador simples para um conjunto de instruções em uma "simulação" de um processador.
  - `src/interpreter.h`  Fornece as definições e declarações necessárias para validar e interpretar as instruções.
  - `src/libs.h` Arquivo de inclusão das bibliotecas utilizadas nos arquivos do sistema.
  - `src/main.c` Função principal que inicializa a arquitetura do sistema, incluindo a CPU, RAM, disco e periféricos. Ele carrega um programa em RAM a partir de um arquivo de entrada (dataset/program.txt), verifica as instruções carregadas, e inicia o pipeline de execução do programa. Após a execução, a função exibe o conteúdo da RAM e libera a memória alocada.
  - `src/peripherals.c` Inicializa os periféricos do sistema, configurando valores iniciais, como o valor de entrada, que é definido como zero.
  - `src/peripherals.h`Define a estrutura básica dos periféricos do sistema, incluindo o valor de entrada.
  - `src/pipeline.c` Implementa as etapas do pipeline de execução de instruções, incluindo busca (instruction_fetch), decodificação (instruction_decode), execução (execute), acesso à memória (memory_access) e escrita dos resultados (write_back). Cada etapa interage com a CPU e a RAM para processar instruções, gerenciar operações de carga e armazenamento, e atualizar registradores com os resultados das operações aritméticas.
  - `src/pipeline.h`  Declara as funções principais para o pipeline de execução de instruções, incluindo busca, decodificação, execução, acesso à memória e escrita de resultados.
  - `src/ram.c` Gerencia a inicialização e exibição do conteúdo da RAM. A função init_ram aloca memória para a RAM e inicializa cada posição com um caractere nulo ('\0'). A função print_ram exibe o conteúdo atual da RAM.
  - `src/ram.h` Define a estrutura e funções para o gerenciamento da memória RAM, incluindo a alocação e exibição de conteúdo. Especifica um tamanho máximo para a RAM (NUM_MEMORY) e declara as funções init_ram e print_ram.
  - `src/reader.c` Implementa funções para ler e manipular um programa a partir de um arquivo.
  - `src/reader.h`  Declara funções para a leitura e manipulação de um programa, incluindo a leitura do conteúdo de um arquivo (read_program), a extração de uma linha específica (get_line_of_program), a contagem do número total de linhas (count_lines), e a contagem do número de tokens em uma linha (count_tokens_in_line).
  - `src/uthash.h`  Implementação de tabelas hash em C.

## Detalhemento do Código e Lógica Utilizada

#### Instruções
 A execução de instruções em um sistema baseado na arquitetura de Von Neumann segue um ciclo característico, conhecido como ciclo de instrução. Esse ciclo consiste em quatro fases:
 Busca (Fetch): A unidade de controle busca a próxima instrução na memória principal.

* Decodificação (Decode): A instrução é decodificada para que a unidade de controle compreenda qual operação deve ser realizada.

* Execução (Execute): A instrução é executada pela unidade lógica e aritmética (ULA) ou outra unidade especializada.

* Armazenamento (Store): O resultado da execução pode ser armazenado de volta na memória ou em outro local apropriado.

 | Instrução | Descrição                                                                                  | Observações                       |
|-----------|--------------------------------------------------------------------------------------------|-----------------------------------|
| `LOAD`    | Carrega um valor de uma posição de memória para um registrador.                            | ---                               |
| `STORE`   | Armazena o valor de um registrador em uma posição de memória.                              | ---                               |
| `ADD`     | Soma os valores de dois registradores e armazena o resultado em um registrador.            | ---                               |
| `SUB`     | Subtrai o valor de um registrador pelo valor de outro e armazena o resultado.              | ---                               |
| `MUL`     | Multiplica os valores de dois registradores e armazena o resultado.                        | ---                               |
| `DIV`     | Divide o valor de um registrador pelo valor de outro e armazena o resultado.               | ---                               |
| `IF`      | Inicia uma instrução condicional.                                                          | ---                               |
| `ELSE`    | Define a alternativa a ser executada caso a condição de `IF` seja falsa.                   | ---                               |
| `LOOP`    | Inicia um loop, repetindo instruções até que uma condição seja atendida.                   | ---                               |
| `INVALID` | Instrução não reconhecida ou inválida.                                                     | ---                               |





## Compilação e Execução

Um arquivo Makefile que realiza todo o procedimento de compilação e execução está disponível no código. Para utilizá-lo, siga as diretrizes de execução no terminal:

| Comando                |  Função                                                                                           |                     
| -----------------------| ------------------------------------------------------------------------------------------------- |
|  `make clean`          | Apaga a última compilação realizada contida na pasta build                                        |
|  `make`                | Executa a compilação do programa utilizando o gcc, e o executável vai para a pasta build          |
|  `make run`            | Executa o programa da pasta build após a realização da compilação                                 |

##  Referências

TANENBAUM, A. S.; AUSTIN, T. Structured Computer Organization. [S.l.], 2012. 800 p.


## Contatos

<div align="center">
   <i>JOAO MARCELO GONCALVES LISBOA Engenharia de Computação @ CEFET-MG</i>
<br><br>

[![Gmail][gmail-badge]][gmail-autor]
</div>





<div align="center">
   <i> YGOR SANTOS VIEIRA Engenharia de Computação @ CEFET-MG</i>
<br><br>

[![Gmail][gmail-badge]][gmail-autor4]
</div>

[gmail-badge]: https://img.shields.io/badge/-Gmail-D14836?style=for-the-badge&logo=Gmail&logoColor=white
[gmail-autor]: joaoeletricgl@outlook.com

[gmail-badge]: https://img.shields.io/badge/-Gmail-D14836?style=for-the-badge&logo=Gmail&logoColor=white
[gmail-autor4]:ygorvieira111@gmail.com

[^1]: TANENBAUM, A. S.; AUSTIN, T. **Structured Computer Organization**. [S.l.], 2012. 800 p.
