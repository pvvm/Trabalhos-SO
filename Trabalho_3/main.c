#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/msg.h>

void trataAlarme(int sinal) {

} 

int main() {
    int pid, idfila, estado;
    sigset_t original, mascara;
    struct mensagem {
        long pid;
        int id;
    };
    struct mensagem mensagem_env, mensagem_rec;
    if((idfila = msgget(0x1223, IPC_CREAT|0x1ff)) < 0) {
        printf("Erro na criacao da fila\n");
        exit(1);
    }
    srand(time(NULL));
    if((pid = fork()) == -1) {
        printf("Erro no fork\n");
        exit(1);
    } else if (pid == 0) {
        int tempo;
        mensagem_env.pid = getpid();
        for(int i = 1; i <= 10; i++) {
            mensagem_env.id = i;
            tempo= ((rand() % 4) + 1);
            printf("%d\n", tempo);
            sleep(tempo);
            printf("mensagem pid=%ld num=%d enviada\n", mensagem_env.pid, mensagem_env.id);
            msgsnd(idfila, &mensagem_env, sizeof(mensagem_env), 0);
        }
        exit(0);
    }
    signal(SIGALRM, trataAlarme);
    sigdelset(&mascara, SIGALRM);
    sigprocmask(SIG_BLOCK, &mascara, &original);
    for(int i = 1; i <= 10; i++) {
        alarm(2);
        while(1) {
            if(msgrcv(idfila, &mensagem_rec, sizeof(mensagem_rec), 0, 0) == -1) {
                if(errno == EINTR) {
                    printf("Ocorreu timeout - nao recebi mensagem em 2 segundos\n");
                } else { 
                    printf("erro no rcv %d\n", errno == EINTR);
                    exit(1);
                }
            } else
                break;
            
        }
        printf("recebi mensagem pid=%ld num=%d\n", mensagem_rec.pid, mensagem_rec.id);    
    }
    sigprocmask(SIG_SETMASK, &original, NULL);

    msgctl(idfila, IPC_RMID, NULL);

    wait(&estado);
    exit(0);
}