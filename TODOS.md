
## Prompt: DONE
- Exibir "fsh>" quando estiver esperando por comandos. 
## Funcionalidades
### Execução de Comandos:
- Foreground: O primeiro comando da linha é executado em primeiro plano. 
- Background: Comandos subsequentes são executados em segundo plano.
- Processos Secundários: Para cada comando em background, criar um processo secundário também em background que executa o mesmo comando.
### Tratamento de Sinais:
- __(DONE)__ SIGINT (Ctrl-C) : 
    Se a shell tem descendentes vivos (não contando processos secundários), deve perguntar se o usuário deseja finalizar a shell.
    Todos os sinais devem ser bloqueados durante o tratamento de SIGINT.
    Se não há descendentes vivos, a shell pode ser encerrada imediatamente.
- __(DONE)__ SIGTSTP (Ctrl-Z):
    Suspender todos os descendentes da shell, incluindo processos de foreground e background.
    Não suspender a shell em si.
### Operações Internas da Shell:
- __(DONE Mas ainda falta tratar o caso de erro que a roberta fala)__ waitall : Liberar todos os processos filhos no estado “Zombie” antes de exibir o prompt novamente.
- __(DIE)__ die: Terminar a shell e garantir que todos os descendentes vivos (não os processos secundários) também sejam terminados.
## Linguagem de Comandos
### Comandos e Argumentos:
- Comandos Externos: Nome do arquivo executável seguido de até dois argumentos.
Operações Internas: waitall e die.
Separador de Comandos: O símbolo # é usado para separar comandos na mesma linha.
- Linha de Comando: Pode conter até 5 comandos separados por #.
## Formato dos comandos:
- comando1 # comando2 # comando3
## Tratamento de Processos
- Processos Foreground: Executar em primeiro plano.
- Processos Background: Criar e executar em segundo plano.
- Processos Secundários: Criar um para cada processo em background, também em segundo plano.
