/**********************************************************
 ** Título: Trabalho Prático 2
 ** Disciplina: Sistemas Operacionais UnB 2020-1
 ** Finalidade: Exercitar a sincronização two-way com chamadas Unix
 ** Responsáveis:
 **     - Pedro Vitor Valença Mizuno 17/0043665
 **     - Rodrigo Ferreira Guimarães 14/0170740
 ** Sistema operacional: Ubuntu 20.04.1 LTS
 ** Compilador: GCC 9.3.0
 *********************************************************/

#include <stdio.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#define ERRO_FORK 2

#define ERRO_SEMGET 3

#define ERRO_SHMGET 4

#define ERRO_ATTACH 5

#define ERRO_SHMCTL 6

#define ERRO_SEMCTL 7

struct sembuf operacao[2];

int p_sem(int idsem, int semnum) {
    operacao[0].sem_num = semnum;
    operacao[0].sem_op = 0;
    operacao[0].sem_flg = 0;
    operacao[1].sem_num = semnum;
    operacao[1].sem_op = 1;
    operacao[1].sem_flg = 0;
    if(semop(idsem, operacao, 2) < 0)
        printf("Erro no p=%d\n", errno);
}

int v_sem(int idsem, int semnum) {
    operacao[0].sem_num = semnum;
    operacao[0].sem_op = -1;
    operacao[0].sem_flg = 0;
    if(semop(idsem, operacao, 1) < 0)
        printf("Erro no p=%d\n", errno);
}

int main () {
    int idsem, idshm;
    int pid, estado;
    int *psem, *pshm;
    if((idsem = semget(0x1223, 2, IPC_CREAT|0x1ff)) < 0) {
        printf("Erro na criacao do semaforo\n");
        exit(ERRO_SEMGET);
    }
    if((idshm = shmget(0x1223, sizeof(int), IPC_CREAT|0x1ff)) < 0) {
        printf("Erro na criacao da fila\n");
        exit(ERRO_SHMGET);
    }
    if((pid = fork()) == -1) {
        printf("Erro no fork\n");
        exit(ERRO_FORK);
    } else if(pid == 0) {
        pshm = (int *) shmat (idshm, (char *)0, 0);
        if(pshm == (int *)-1) {
            printf("Erro no attach do filho\n");
            exit(ERRO_ATTACH);
        }
        for(int i = 1; i <= 20; i++) {
            p_sem(idsem, 1);
            *pshm = i;
            printf("processo %d escreveu %d\n", getpid(), *pshm);
            v_sem(idsem, 0);
        }
        exit(1);
    }
    pshm = (int *) shmat (idshm, (char *)0, 0);
    if(pshm == (int *)-1) {
        printf("Erro no attach do pai\n");
        exit(ERRO_ATTACH);
    }
    p_sem(idsem, 0);
    for(int i = 1; i <= 20; i++) {
        p_sem(idsem, 0);
        printf("processo %d leu %d\n", getpid(), *pshm);
        v_sem(idsem, 1);
    }
    wait(&estado);

    if(shmctl(idshm, IPC_RMID, NULL) == -1) {
        printf("Erro na remocao da memoria compartilhada\n");
        exit(ERRO_SHMCTL);
    }
    if(semctl(idsem, 0, IPC_RMID) == -1) {
        printf("Erro na remocao do semaforo\n");
        exit(ERRO_SEMCTL);
    }

    return 0;
}