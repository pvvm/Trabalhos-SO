/**********************************************************
 ** Título: Trabalho Prático 2
 ** Disciplina: Sistemas Operacionais UnB 2020-1
 ** Finalidade: Exercitar a sincronização two-way com
 **     chamadas Unix
 ** Responsáveis:
 **     - Pedro Vitor Valença Mizuno 17/0043665
 **     - Rodrigo Ferreira Guimarães 14/0170740
 ** Sistema operacional: Ubuntu 20.04.1 LTS
 ** Compilador: GCC 9.3.0
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>


/**
 *  Erros tratáveis:
 *      - Clonagem de processo
 *      - Aquisição de semáforo
 *      - Controle do semáforo
 *      - Bloqueio de semáforo
 *      - Desbloqueio de semáforo
 *      - Aquisição de memória compartilhada
 *      - Ligação à memória compartilhada
 *      - Defazer ligação à memória compartilhada
 *      - Controle da memória compartilhada
 */
enum erros {
    ERRO_FORK = 2,
    ERRO_SEM_GET,
    ERRO_SEM_CTL,
    ERRO_SEM_P,
    ERRO_SEM_V,
    ERRO_SHM_GET,
    ERRO_SHM_ATC,
    ERRO_SHM_DTC,
    ERRO_SHM_CTL
};
/**  Erro padrão das chamadas de sistema */
#define ERRO_SISTEMA -1

/** Flags de acesso, baseado Unix: rwx(usuário) rwx(grupo) rwx(todos) **/
#define FLG_USER_LER 0x100
#define FLG_USER_ECV 0x080
#define FLG_USER_CMPLT (FLG_USER_LER | FLG_USER_ECV)
#define FLG_GROUP_LER 0x020
#define FLG_GROUP_ECV 0x010
#define FLG_GROUP_CMPLT (FLG_GROUP_LER | FLG_GROUP_ECV)
#define FLG_USERS_LER 0x004
#define FLG_USERS_ECV 0x002
#define FLG_USERS_CMPLT (FLG_USERS_LER | FLG_USERS_ECV)

/**  Chave padrão para os mecanismos IPC */
#define CHAVE_IPC 0x24807916
/**  Contador de iterações */
#define CNT_LMT 20

/** Idenficadores dos Semáforos */
enum semaforos {
    SEM_ESCRITA,
    SEM_LEITURA,
};

/** Identificação de operação bem sucessida */
#define SUCESSO 0
/* Processo idenficado como sendo o filho */
#define E_PROCESSO_FILHO 0


/** Gerência de processos */
long clonar_processo ();
void processo_filho (int id_shm, int id_sem);

/** Gerência de semáforo */
int sem_iniciar (key_t chave, int cnt, int flag_acesso);
int sem_p (int id, int num);
int sem_v (int id, int num);
void sem_liberar (int id);

/** Gerência de memória compartilhada */
int shm_iniciar (key_t chave, size_t tam, int flag_acesso);
void* shm_link (int id, int flag_acesso);
int shm_unlink (const void *endereco);
void shm_liberar (int id);


int main (){
    int id_sem = sem_iniciar(
        CHAVE_IPC,
        2,
        IPC_CREAT | FLG_USER_CMPLT
    );
    int id_shm = shm_iniciar(
        CHAVE_IPC,
        sizeof(int),
        IPC_CREAT | FLG_USER_CMPLT
    );

    if (clonar_processo() == E_PROCESSO_FILHO){
        processo_filho(id_shm, id_sem);

        exit(SUCESSO);
    }

    int retorno_filho;
    int *p_shm = (int *) shm_link(id_shm, FLG_USER_LER);

    sem_p(id_sem, SEM_ESCRITA);

    for (int cnt = 0; cnt < CNT_LMT; cnt++){
        sem_p(id_sem, SEM_ESCRITA);

        printf("processo %d leu %d\n\n", getpid(), *p_shm);

        sem_v(id_sem, SEM_LEITURA);
    }

    shm_unlink(p_shm);

    wait(&retorno_filho);

    shm_liberar(id_shm);
    sem_liberar(id_sem);

    return SUCESSO;
}

/** Gerência de processos **/
/**
 *  Clonagem a partir do processo atual, com tratamento de eventual
 *  problema
 *  @return ID do processo filho criado ou E_PROCESSO_FILHO para o processo
 *  recém criado, ou saída do programa com código ERRO_FORK
 */
long clonar_processo (){
    int pid = fork();

    if (pid == ERRO_SISTEMA){
        perror("Erro ao clonar o processo");
        exit(ERRO_FORK);
    }

    return pid;
}
/**
 *  Execução do processo filho, tendo memória compartilhada e semáforo
 *  @param id_shm Identificador da memória compartilhada
 *  @param id_sem Idenficador do semáforo
 */
void processo_filho (int id_shm, int id_sem){
    int *p_shm = (int *) shm_link(id_shm, FLG_USER_ECV);

    for (int cnt = 1; cnt <= CNT_LMT; cnt++){
        sem_p(id_sem, SEM_LEITURA);

        *p_shm = cnt;
        printf("processo %d escreveu %d\n", getpid(), *p_shm);

        sem_v(id_sem, SEM_ESCRITA);
    }

    shm_unlink(p_shm);
}

/** Gerência de semáforo **/
/**
 *  Aquisição de semáforos
 *  @param key Chave de identificação do semáforo
 *  @param cnt Quantidade de semáforos a serem criados
 *  @param flag_acesso Flags da aquisição
 *  @return Idenficador do conjunto de semáforos adquiridos, ou
 *  saída do programa com código ERRO_SEM_GET
 */
int sem_iniciar (key_t chave, int cnt, int flag_acesso){
    int id = semget(chave, cnt, flag_acesso);

    if (id == ERRO_SISTEMA){
        perror("Erro na aquisição do semáforo");
        exit(ERRO_SEM_GET);
    }

    return id;
}
/**
 *  Bloqueio de semáforo
 *  @param id_sem Idenficador do conjunto de semáforo
 *  @param num Número do semáforo
 *  @return SUCESSO, ou saída do programa com código ERRO_SEM_P
 */
int sem_p (int id_sem, int num){
    struct sembuf operacao[2];

    // Check se está bloqueado
    operacao[0].sem_num = num;
    operacao[0].sem_op = 0;
    operacao[0].sem_flg = 0;

    // Se não, bloqueia
    operacao[1].sem_num = num;
    operacao[1].sem_op = 1;
    operacao[1].sem_flg = 0;

    if (semop(id_sem, operacao, 2) == ERRO_SISTEMA){
        perror("Erro ao bloquear semáforo");
        exit(ERRO_SEM_P);
    }

    return SUCESSO;
}
/**
 *  Desbloqueio do semáforo
 *  @param id_sem Idenficador do conjunto de semáforo
 *  @param num Número do semáforo
 *  @return SUCESSO, ou saída do programa com código ERRO_SEM_V
 */
int sem_v (int id_sem, int num){
    struct sembuf operacao;

    operacao.sem_num = num;
    operacao.sem_op = -1;
    operacao.sem_flg = 0;

    if(semop(id_sem, &operacao, 1) == ERRO_SISTEMA){
        perror("Erro ao liberar semáforo");
        exit(ERRO_SEM_V);
    }

    return SUCESSO;
}
/**
 *  Liberação do conjunto de semáforos
 *  @param id Idenficador do conjunto de semáforo
 *  @return Void ou saída do programa com código ERRO_SHM_CTL
 */
void sem_liberar (int id){
    if (semctl(id, 0, IPC_RMID) == ERRO_SISTEMA){
        perror("Erro na liberação do semáforo");
        exit(ERRO_SEM_CTL);
    }
}

/** Gerência de memória compartilhada **/
/**
 *  Aquisição de memória compartilhada
 *  @param key Chave de identificação do semáforo
 *  @param tam Tamanho em bytes da memória compartilhada
 *  @param flag_acesso Flags da aquisição
 *  @return Idenficador da memória compartilhada adquirida, ou
 *  saída do programa com código ERRO_SHM_GET
 */
int shm_iniciar (key_t chave, size_t tam, int flag_acesso){
    int id = shmget(chave, tam, flag_acesso);

    if (id == ERRO_SISTEMA){
        perror("Erro na aquisição da memória compartilhada");
        exit(ERRO_SHM_GET);
    }

    return id;
}
/**
 *  Ligação à memória compartilhada
 *  @param id Idenficador da memória compartilhada
 *  @param flag_acesso Flags da ligação
 *  @return Ponteiro para a memória vinculada, ou
 *  saída do programa com código ERRO_SHM_ATC
 */
void* shm_link (int id, int flag_acesso){
    void *endereco = shmat(id, (void *) 0, flag_acesso);

    if (endereco == (void *) ERRO_SISTEMA){
        perror("Erro na ligação com a memória compartilhada");
        exit(ERRO_SHM_ATC);
    }

    return endereco;
}
/**
 *  Desfazer ligação à memória compartilhada
 *  @param endereco Ponteiro para a memória vinculada
 *  @return Retorno da chamada de sistema, ou
 *  saída do programa com código ERRO_SHM_DTC
 */
int shm_unlink (const void *endereco){
    int retorno = shmdt(endereco);

    if (retorno == ERRO_SISTEMA){
        perror("Erro ao defazer link à memória compartilhada");
        exit(ERRO_SHM_DTC);
    }

    return retorno;
}
/**
 *  Liberação da memória compartilhada
 *  @param id Idenficador da memória compartilhada
 *  @return Void ou saída do programa com código ERRO_SHM_CTL
 */
void shm_liberar (int id){
    if (shmctl(id, IPC_RMID, NULL) == ERRO_SISTEMA){
        perror("Erro ao liberar a memória compartilhada");
        exit(ERRO_SHM_CTL);
    }
}
