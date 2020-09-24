/**
 * Título: Trabalho Prático 1
 * Disciplina: Sistemas Operacionais UnB 2020-1
 * Responsáveis:
 *     - Pedro Vitor Valença Mizuno 17/0043665
 *     - Rodrigo Ferreira Guimarães 14/0170740
 *     -
 *     -
 **/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main (int argc, char* argv[]){
    if (argc < 2){
        printf("Deveria informar, pelo menos, a quantidade de processos filhos como parâmetro\n");
        exit(1);
    }

    int num_processos = (int) strtol(argv[1], NULL, 10),
        ppid = getpid();

    for (int cnt = 0; cnt < num_processos; cnt++)
        if (!fork()) break;

    if (num_processos) sleep(10);
    
    printf("sou o processo %s com pid = %d\n", ppid == getpid() ? "pai" : "filho", getpid());

    return 0;
}
