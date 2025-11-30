#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include "ipc.h"
#include <sys/mman.h>

#define NUM_COZINHEIROS 3
#define MAX_BUFFER 10

// Buffer circular
Pedido fila[MAX_BUFFER];
int in = 0;
int out = 0;
int count = 0;

// Sincronização
pthread_mutex_t mutex_fila;
pthread_cond_t cond_fila;

// Semáforos POSIX
sem_t sem_itens;
sem_t sem_espacos;

// Painel em memória compartilhada
PainelStatus *painel;
pthread_mutex_t mutex_painel;

mqd_t mq_descriptor;
int shm_descriptor;

/* --------------------------------------------------------------------------
    FUNÇÃO DE LIMPEZA (cleanup)
----------------------------------------------------------------------------*/
void cleanup(int signum) {
    printf("\n[SERVIDOR] Encerrando... (signal %d)\n", signum);

    // Fecha fila de mensagens
    mq_close(mq_descriptor);
    mq_unlink(MQ_NAME);

    // Fecha memória compartilhada
    munmap(painel, sizeof(PainelStatus));
    shm_unlink(SHM_NAME);

    // Destrói mutexes, condvars e semáforos
    pthread_mutex_destroy(&mutex_fila);
    pthread_mutex_destroy(&mutex_painel);
    pthread_cond_destroy(&cond_fila);
    sem_destroy(&sem_itens);
    sem_destroy(&sem_espacos);

    printf("[SERVIDOR] Recursos liberados com sucesso.\n");
    exit(0);
}

/* -------------------------------------------------------------------------- */
void painel_incrementa_total() {
    pthread_mutex_lock(&mutex_painel);
    painel->total_pedidos++;
    pthread_mutex_unlock(&mutex_painel);
}

void painel_comeca_preparo() {
    pthread_mutex_lock(&mutex_painel);
    painel->em_preparo++;
    pthread_mutex_unlock(&mutex_painel);
}

void painel_finaliza_pedido() {
    pthread_mutex_lock(&mutex_painel);
    painel->em_preparo--;
    painel->finalizados++;
    pthread_mutex_unlock(&mutex_painel);
}

/* -------------------------------------------------------------------------- */
void colocar_na_fila(Pedido p) {
    sem_wait(&sem_espacos);

    pthread_mutex_lock(&mutex_fila);

    fila[in] = p;
    in = (in + 1) % MAX_BUFFER;
    count++;

    pthread_cond_signal(&cond_fila);

    pthread_mutex_unlock(&mutex_fila);

    painel_incrementa_total();
    sem_post(&sem_itens);
}

/* -------------------------------------------------------------------------- */
Pedido retirar_da_fila() {
    sem_wait(&sem_itens);

    pthread_mutex_lock(&mutex_fila);

    Pedido p = fila[out];
    out = (out + 1) % MAX_BUFFER;
    count--;

    pthread_cond_signal(&cond_fila);

    pthread_mutex_unlock(&mutex_fila);

    sem_post(&sem_espacos);
    return p;
}

/* -------------------------------------------------------------------------- */
void* cozinheiro_thread(void* arg) {
    int id = (long)arg;

    while (1) {
        Pedido p = retirar_da_fila();

        painel_comeca_preparo();
        printf("[COZINHEIRO %d] Preparando: %s (%d s)\n",
               id, p.descricao, p.tempo_preparo);

        sleep(p.tempo_preparo);

        printf("[COZINHEIRO %d] Finalizou: %s\n", id, p.descricao);
        painel_finaliza_pedido();
    }
    return NULL;
}

/* -------------------------------------------------------------------------- */
int main() {
    printf("[SERVIDOR] Iniciando...\n");

    signal(SIGINT, cleanup);

    pthread_mutex_init(&mutex_fila, NULL);
    pthread_mutex_init(&mutex_painel, NULL);
    pthread_cond_init(&cond_fila, NULL);

    sem_init(&sem_itens, 0, 0);
    sem_init(&sem_espacos, 0, MAX_BUFFER);

    mq_descriptor = init_message_queue();
    shm_descriptor = init_shared_memory(sizeof(PainelStatus));
    painel = (PainelStatus*) map_shared_memory(shm_descriptor, sizeof(PainelStatus));

    painel->total_pedidos = 0;
    painel->em_preparo = 0;
    painel->finalizados = 0;

    pthread_t threads[NUM_COZINHEIROS];
    for (int i = 0; i < NUM_COZINHEIROS; i++) {
        pthread_create(&threads[i], NULL, cozinheiro_thread, (void*)(long)i);
    }

    while (1) {
        Pedido p;
        receive_pedido(&p);

        printf("[SERVIDOR] Pedido recebido do cliente %d: %s\n",
               p.id_cliente, p.descricao);

        colocar_na_fila(p);
    }

    return 0;
}
