# fsh - First Shell

## Descrição do Projeto

`fsh` (First Shell) é um projeto de shell customizada desenvolvida em linguagem C como parte do primeiro trabalho de programação da disciplina de Sistemas Operacionais (Período 2024/1). O objetivo principal é aplicar e entender os princípios de manipulação de processos, incluindo execução de programas em foreground e background, tratamento de sinais e comandos internos.

## Motivação

Insatisfeitos com os shells existentes, decidimos criar a `fsh` para explorar e implementar funcionalidades essenciais de um interpretador de comandos, focando no controle de processos e na interação com o sistema operacional.

## Funcionalidades Principais

A `fsh` foi projetada para suportar as seguintes funcionalidades:

* **Execução de Comandos:**
    * Executa programas externos.
    * Permite a execução de múltiplos comandos em uma única linha, separados pelo caractere `#`.
        * Exemplo: `fsh> comando1 # comando2 # comando3`
    * O primeiro comando da linha é executado em **foreground**.
    * Os comandos subsequentes (até um máximo de 5 comandos por linha no total) são executados em **background**.
* **Processos Secundários:**
    * Para cada processo criado em background (ex: `comando2`, `comando3` no exemplo acima), um processo secundário filho é criado, também em background, executando o mesmo comando.
* **Comandos Internos:**
    * `waitall`: Libera todos os processos descendentes diretos (não os secundários) que estejam no estado "Zombie" antes de exibir um novo prompt.
    * `die`: Termina a operação da `fsh`. Antes de finalizar, garante que todos os seus descendentes vivos (incluindo os secundários) também sejam terminados.
* **Tratamento de Sinais:**
    * **SIGINT (Ctrl-C):**
        * Se a `fsh` tiver descendentes vivos (exceto os processos secundários), ela perguntará ao usuário se deseja finalizar. Se confirmado, a shell termina. Durante o tratamento do SIGINT, outros sinais são bloqueados.
        * Se a `fsh` não tiver descendentes vivos (exceto secundários), ela é finalizada diretamente com Ctrl-C.
        * Todos os descendentes da `fsh` (foreground ou background) devem ignorar o SIGINT.
    * **SIGTSTP (Ctrl-Z):**
        * A `fsh` em si não é suspensa.
        * Todos os seus descendentes (foreground, background e secundários) devem ser suspensos.
* **Comportamento em Caso de Finalização/Suspensão por Sinal:**
    * Se um processo principal (criado diretamente pela `fsh`, ex: P1, P2, P3) morrer ou for suspenso devido a um sinal, os demais processos criados na mesma linha de comando (e seus respectivos secundários) também devem morrer/suspender, recebendo o mesmo sinal.
    * Se um processo secundário (Px') morrer devido a um sinal, nada acontece com os demais processos do grupo.
    * Se os processos terminarem normalmente (via `return` ou `exit`), nada acontece com os demais processos.

## Linguagem da Shell

* **Termos:** Sequências de caracteres diferentes de espaço.
* **Tipos de Termos:**
    1.  **Operações Internas:** `waitall`, `die`. Devem ser digitadas sozinhas em uma linha.
    2.  **Operadores Especiais:** `#` (para separar múltiplos comandos).
    3.  **Nomes de Programas:** Arquivos executáveis.
    4.  **Argumentos:** Até dois argumentos podem ser passados para os programas.
* O prompt da shell é `fsh>`.
* Cada vez que um processo em foreground retorna, a `fsh` deve exibir imediatamente o prompt.

## Estrutura de Processos (Exemplo)

Para o comando: `fsh> comando1 # comando2 # comando3`
```
    fsh
     |
+----+----+
|    |    |
P1   P2   P3
(fg) (bg) (bg)
     P2'  P3'
     (bg) (bg)
```
* P1 executa `comando1` em foreground.
* P2 executa `comando2` em background, e P2' (filho de P2) também executa `comando2` em background.
* P3 executa `comando3` em background, e P3' (filho de P3) também executa `comando3` em background.

## Como Compilar e Executar

1.  **Pré-requisitos:**
    * Compilador C (GCC recomendado).
    * Make (o projeto deve incluir um `Makefile`).

2.  **Compilação:**
    ```bash
    make
    ```

3.  **Execução:**
    ```bash
    ./fsh
    ```
    Após a execução, o prompt `fsh>` será exibido, aguardando comandos.

## Status do Projeto

*(Esta seção deve ser atualizada pelo grupo para refletir o estado atual da implementação, mencionando quais funcionalidades do PDF foram completamente implementadas, quais estão parciais e quais não foram implementadas. É importante referenciar os "TODOs" e partes marcadas em amarelo no PDF.)*

Exemplo:
* [X] Execução de comando único em foreground.
* [X] Execução de múltiplos comandos (foreground + background).
* [X] Criação de processos secundários.
* [X] Comando interno `die` (parcialmente, sem considerar filhos secundários).
* [X] Comando interno `waitall`.
* [X] Tratamento de SIGINT para a shell.
* [X] Tratamento de SIGINT para descendentes.
* [X] Tratamento de SIGTSTP.
* [X] Propagação de sinais para finalização/suspensão de grupo de processos.

## Autores

* [Jullie de Castro Quadros](https://github.com/jcquadros)
* [Yuri Groener da Victoria](https://github.com/YuriVictoria)

---

**Disciplina:** Sistemas Operacionais  
**Trabalho:** 1º Trabalho de Programação  
**Período:** 2024/1  
**Professor:** Roberta Lima Gomes
**Instituição:** Universidade Federal do Espírito Santo
