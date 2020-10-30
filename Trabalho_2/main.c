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
/**  Erro padrão do SO */
#define ERRO_SISTEMA -1

/** Flags de acesso, baseado Unix: rwx(usuário) rwx(grupo) rwx(todos) **/
#define IPC_FLG_USER_LER 0x100
#define IPC_FLG_USER_ECV 0x080
#define IPC_FLG_USER_CPT (IPC_FLG_USER_LER | IPC_FLG_USER_ECV)
#define IPC_FLG_GROUP_LER 0x020
#define IPC_FLG_GROUP_ECV 0x010
#define IPC_FLG_GROUP_CPT (IPC_FLG_GROUP_LER | IPC_FLG_GROUP_ECV)
#define IPC_FLG_USERS_LER 0x004
#define IPC_FLG_USERS_ECV 0x002
#define IPC_FLG_USERS_CPT (IPC_FLG_USERS_LER | IPC_FLG_USERS_ECV)

/**  Chave padrão para os mecanismos IPC */
#define CHAVE_IPC 0x24807916
/**  Contador de iterações */
#define CNT_INT 20

/** Idenficadores dos Semáforos */
enum semaforos {
    SEM_ESCREVEU,
    SEM_LEU,
};

/** Identificação de operação bem sucessida */
#define SUCESSO 0
/* Processo idenficado como sendo o filho */
#define E_PROCESSO_FILHO 0


/** Gerência de processos */
long clonar_processo ();
void processo_filho (int id_shm, int id_sem);

/** Gerência de semáforo */
int sem_init (key_t key, int cnt, int flg);
int sem_p (int id, int num);
int sem_v (int id, int num);
void sem_free (int id);

/** Gerência de memória compartilhada */
int shm_init (key_t key, size_t tam, int flg);
void* shm_link (int id, int flg);
int shm_unlink (const void *endereço);
void shm_free (int id);


int main (){
    int id_sem = sem_init(
        CHAVE_IPC,
        2,
        IPC_CREAT | IPC_FLG_USER_CPT
    );
    int id_shm = shm_init(
        CHAVE_IPC,
        sizeof(int),
        IPC_CREAT | IPC_FLG_USER_CPT
    );
    long pid = clonar_processo();

    if (pid == E_PROCESSO_FILHO){
        processo_filho(id_shm, id_sem);

        exit(SUCESSO);
    }

    int retorno_filho;
    int *p_shm = (int *) shm_init(id_shm, IPC_FLG_USER_LER);

    sem_p(id_sem, SEM_ESCREVEU);

    for (int cnt = 0; cnt < CNT_INT; cnt++){
        sem_p(id_sem, SEM_ESCREVEU);

        printf("processo %d leu %d\n", getpid(), *p_shm);

        sem_v(id_sem, SEM_LEU);
    }

    shm_unlink(p_shm);

    wait(&retorno_filho);

    shm_free(id_shm);
    sem_free(id_sem);

    return SUCESSO;
}

/** Gerência de processos **/
/**
 *  Clonagem a partir do processo atual, com tratamento de eventual
 *  problema
 *  @return ID do processo filho criado ou E_PROCESSO_FILHO para o processo
 *  recém criado
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
    int *p_shm = (int *) shm_link(id_shm, IPC_FLG_USER_ECV);

    for (int cnt = 1; cnt <= CNT_INT; cnt++){
        sem_p(id_sem, SEM_LEU);

        *p_shm = cnt;
        printf("processo %d escreveu %d\n", getpid(), *p_shm);

        sem_v(id_sem, SEM_ESCREVEU);
    }

    shm_unlink(p_shm);
}

/** Gerência de semáforo **/
/**
 *  Aquisição de semáforos
 *  @param key Chave de identificação do semáforo
 *  @param cnt Quantidade de semáforos a serem criados
 *  @param flg Flags da aquisição
 *  @return Idenficador do conjunto de semáforo adquiridos, ou
 *  sai do programa com código ERRO_SEM_GET
 */
int sem_init (key_t key, int cnt, int flg){
    int id = semget(key, cnt, semflg);

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
 *  @return SUCESSO, ou sai do programa com código ERRO_SEM_P
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
 *  @return SUCESSO, ou sai do programa com código ERRO_SEM_V
 */
int sem_v (int id_sem, int semnum){
    struct sembuf operacao;

    operacao.sem_num = semnum;
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
void sem_free (int id){
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
 *  @param flg Flags da aquisição
 *  @return Idenficador da memória compartilhada adquirida, ou
 *  sai do programa com código ERRO_SHM_GET
 */
int shm_init (key_t key, size_t tam, int flg){
    int id = shmget(key, tam, shmflg);

    if (id == ERRO_SISTEMA){
        perror("Erro na aquisição da memória compartilhada");
        exit(ERRO_SHM_GET);
    }

    return id;
}
/**
 *  Ligação à memória compartilhada
 *  @param id Idenficador da memória compartilhada
 *  @param flg Flags da ligação
 *  @return Ponteiro para a memória vinculada, ou
 *  saída do programa com código ERRO_SHM_ATC
 */
void* shm_link (int id, int flg){
    void *endereco = shmat(id, (void *) 0, flg);

    if (endereco == (void *) ERRO_SISTEMA){
        perror("Erro na ligação com a memória compartilhada");
        exit(ERRO_SHM_ATC);
    }

    return endereco;
}
/**
 *  Desfazer ligação à memória compartilhada
 *  @param endereço Ponteiro para a memória vinculada
 *  @return Retorno da chamada de sistema, ou
 *  sai do programa com código ERRO_SHM_DTC
 */
int shm_unlink (const void *endereço){
    int retorno = shmdt(endereço);

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
void shm_free (int id){
    if (shmctl(id, IPC_RMID, NULL) == ERRO_SISTEMA){
        perror("Erro ao liberar a memória compartilhada");
        exit(ERRO_SHM_CTL);
    }
}
