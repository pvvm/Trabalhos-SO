 /**********************************************************
 ** Título: Trabalho Prático 3
 ** Disciplina: Sistemas Operacionais UnB 2020-1
 ** Finalidade: Exercitar temporizadores com chamadas Unix
 ** Responsáveis:
 **     - Pedro Vitor Valença Mizuno 17/0043665
 **     - Rodrigo Ferreira Guimarães 14/0170740
 **	    - Alison de Miranda Péres    13/0039870
 ** Sistema operacional: Ubuntu 20.04.1 LTS
 ** Compilador: GCC 9.3.0
 *********************************************************/

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

enum erros {
    ERRO_FORK = 2,
    ERRO_MSGGET,
    ERRO_MSGSND,
    ERRO_MSGRCV,
    ERRO_MSGCTL,
    ERRO_FILHO
};

/** Identificação de operação bem sucedida */
#define SUCESSO 0

/* Função de tratamento */
void trataAlarme(int sinal) {

} 

int main() {
    int pid, idfila, retorno_filho;
    sigset_t original, mascara;
    struct mensagem {
        long pid;
        int id;
    };
    struct mensagem mensagem_env, mensagem_rec;
    /* Função de criação da fila*/
    if((idfila = msgget(0x1223, IPC_CREAT|0x1ff)) < 0) {
        printf("Erro na criacao da fila\n");
        exit(ERRO_MSGGET);
    }
    /* Gera uma semente de numéros aleatórios com base no horário*/
    srand(time(NULL));
    /* Clona o processo gerando um processo filho */
    if((pid = fork()) == -1) {
        printf("Erro no fork\n");
        exit(ERRO_FORK);
    } else if (pid == 0) {
        int tempo;
        mensagem_env.pid = getpid();
        for(int i = 1; i <= 10; i++) {
            mensagem_env.id = i;
            tempo= ((rand() % 4) + 1);
            /* Faz o processo dormir por um tempo aleatório entre 1 e 4 segundos */
             sleep(tempo);
            printf("mensagem pid=%ld num=%d enviada\n", mensagem_env.pid, mensagem_env.id);
            /* Envia mensagem por meio da fila */ 
            if(msgsnd(idfila, &mensagem_env, sizeof(mensagem_env), 0) == -1) {
                printf("Erro no envio de mensagem\n");
                exit(ERRO_MSGSND);
            }
        }
        exit(SUCESSO);
    }
    signal(SIGALRM, trataAlarme);
    sigdelset(&mascara, SIGALRM);
    sigprocmask(SIG_BLOCK, &mascara, &original);
    for(int i = 1; i <= 10; i++) {
        /* Seta um alarme por 2 segundos */
        alarm(2);
        while(1) {
            //Aguarda o recebimento de mensagem pela fila
            if(msgrcv(idfila, &mensagem_rec, sizeof(mensagem_rec), 0, 0) == -1) {
                /* Caso o alarme dispare, isto é, passe dois segundos, é enviado um sinal de interrupção */
                if(errno == EINTR) {
                    printf("Ocorreu timeout - nao recebi mensagem em 2 segundos\n");
                } else { 
                    printf("Erro no recebimento de mensagem %d\n", errno == EINTR);
                    exit(ERRO_MSGRCV);
                }
            } else
                break;
            
        }
        printf("recebi mensagem pid=%ld num=%d\n", mensagem_rec.pid, mensagem_rec.id);    
    }
    sigprocmask(SIG_SETMASK, &original, NULL);
    /* Remove a fila */
    if(msgctl(idfila, IPC_RMID, NULL) == -1) {
        printf("Erro na remocao da fila de mensagem\n");
        exit(ERRO_MSGCTL);
    }

    wait(&retorno_filho);

    if(retorno_filho != SUCESSO) {
        printf("Processo filho retornou um exit de erro\n");
        exit(ERRO_FILHO);
    }

    return 0;
}