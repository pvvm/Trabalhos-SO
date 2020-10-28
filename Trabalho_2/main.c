#include <stdio.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

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
        exit(1);
    }
    if((idshm = shmget(0x1223, sizeof(int), IPC_CREAT|0x1ff)) < 0) {
        printf("Erro na criacao da fila\n");
        exit(1);
    }
    if((pid = fork()) == -1) {
        printf("Erro no fork\n");
        exit(1);
    } else if(pid == 0) {
        pshm = (int *) shmat (idshm, (char *)0, 0);
        if(pshm == (int *)-1) {
            printf("Erro no attach\n");
            exit(1);
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
        printf("Erro no attach\n");
        exit(1);
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
        exit(1);
    }
    if(semctl(idsem, 0, IPC_RMID) == -1) {
        printf("Erro na remocao do semaforo\n");
    }
    exit(0);
}