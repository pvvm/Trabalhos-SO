/*
Trabalho Pratíco 1 - Sistemas Operacionais
-- COLOQUEM SEUS NOMES AQUI --
Pedro Vitor Valença Mizuno - 17/0043665
Linux Ubuntu versao 18.04
GCC versao 8.4.0
*/

#include <stdio.h>
#include <unistd.h>

int main(){
    int pid, N;
    int ppid = getpid();

    printf("Quantos processos serao criados? ");
    do
        scanf("%d", &N);
    while(N > 10 || N < 0);

    if(N > 0)
        pid = fork();

    for(int i = 0; i < N-1; i++)
        if(pid == 0)
            pid = fork();
    
    sleep(1);

    if(ppid == getpid())
        printf("Sou o processo pai com pid=%d\n", getpid());
    else
        printf("Sou o processo filho com pid=%d\n", getpid());
}