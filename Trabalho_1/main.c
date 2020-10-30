/**********************************************************
 ** Título: Trabalho Prático 1
 ** Disciplina: Sistemas Operacionais UnB 2020-1
 ** Finalidade: Aprendizagem do fork para N processos
 ** Responsáveis:
 **     - Pedro Vitor Valença Mizuno 17/0043665
 **     - Rodrigo Ferreira Guimarães 14/0170740
 ** Sistema operacional: Ubuntu 20.04.1 LTS
 ** Compilador: GCC 9.3.0
 *********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/*  Erro no fork */
#define ERRO_FORK 2

/* Argumentos inválido */
#define ARG_INVALIDO -1
/* Processo idenficado como sendo o filho */
#define E_PROCESSO_FILHO 0

/* Mínimo de processos que podem ser criados */
#define MIN_NUM_PROCESSOS 0
/* Máximo de processos que podem ser criados */
#define MAX_NUM_PROCESSOS 10
/* Tempo a ser esperado pelo processos */
#define TEMPO_ESPERA 30


int get_num_processos_de_arg(int argc, char* argv[]);
int get_num_processos(int argc, char* argv[]);
long clonar_processo();


int main (int argc, char* argv[]){
    int num_processos = get_num_processos(argc, argv), lixo;
    long processo_pai_id = getpid();

    for (int cnt = 0; cnt < num_processos; cnt++)
        if (clonar_processo() == E_PROCESSO_FILHO) break;

    sleep(TEMPO_ESPERA);

    printf("sou o processo %s com pid = %d\n", processo_pai_id == getpid() ? "pai" : "filho", getpid());

    for (int cnt = 0; cnt < num_processos; cnt++)
        wait(&lixo);

    return 0;
}


/**
 *  Aquisição do número de processos pela interface ou pela entrada padrão,
 *  esperando por valor válido
 *  @param argc Contador de argumentos da interface, padrão
 *  @param argv Argumentos da interface, padrão
 *  @return Número de processos
 */
int get_num_processos(int argc, char* argv[]){
    int num_processos = get_num_processos_de_arg(argc, argv);

    while (1){
        if (num_processos >= MIN_NUM_PROCESSOS && num_processos <= MAX_NUM_PROCESSOS)
            return num_processos;

        printf("Entrada inválida, tente novamente!\n");

        printf("Quantos processos deseja criar? (%d - %d)\n", MIN_NUM_PROCESSOS, MAX_NUM_PROCESSOS);
        scanf("%d", &num_processos);
        getchar();
    }
}

/**
 *  Aquisição do número de processos pela interface do sistema
 *  @param argc Contador de argumentos da interface, padrão
 *  @param argv Argumentos da interface, padrão
 *  @return Número passado pela interface ou ARG_INVALIDO
 *  em caso de falha
 */
int get_num_processos_de_arg(int argc, char* argv[]){
    if (argc < 2){
        printf("A quantidade de processos poderia ter sido informada de antemão\n");
        return ARG_INVALIDO;
    }

    return atoi(argv[1]);
}

/**
 *  Clonagem a partir do processo atual, com tratamento de eventual
 *  problema
 *  @return ID do processo filho criado ou E_PROCESSO_FILHO para o processo
 *  recém criado
 */
long clonar_processo(){
    int pid = E_PROCESSO_FILHO;

    if ((pid = fork()) == -1){
        printf("Aconteceu algum problema ao clonar o processo");
        exit(ERRO_FORK);
    }

    return pid;
}
