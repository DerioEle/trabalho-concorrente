#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include "ipc.h"

#define NUM_COZINHEIROS 3
#define MAX_BUFFER 10

// Buffer circular de pedidos
Pedido fila[MAX_BUFFER];
int in = 0;
int out = 0;
int count = 0;

// Sincronização por mutex + cond
pthread_mutex_t mutex_fila;
pthread_cond_t cond_fila;

// Semáforos POSIX (Etapa 2)
sem_t sem_itens;            // quantos pedidos existem
sem_t sem_espacos;          // quantos espaços sobrando

// Painel em memória compartilhada (Etapa 3)
PainelStatus *painel;
pthread_mutex_t mutex_painel; // proteção do painel

/* --------------------------------------------------------------------------
   Funções para manipular painel
----------------------------------------------------------------------------*/
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

/* --------------------------------------------------------------------------
   Produtor — servidor insere pedidos
----------------------------------------------------------------------------*/
void colocar_na_fila(Pedido p) {

    // Espera ter espaço (semáforo)
    sem_wait(&sem_espacos);

    pthread_mutex_lock(&mutex_fila);

    fila[in] = p;
    in = (in + 1) % MAX_BUFFER;
    count++;

    // Acorda consumidores
    pthread_cond_signal(&cond_fila);

    pthread_mutex_unlock(&mutex_fila);

    // Atualiza painel: aumentou total de pedidos
    painel_incrementa_total();

    // Indica que há mais um item disponível
    sem_post(&sem_itens);
}

/* --------------------------------------------------------------------------
   Consumidor — threads cozinheiras retiram pedidos
----------------------------------------------------------------------------*/
Pedido retirar_da_fila() {

    // Espera até existir item para retirar (semáforo)
    sem_wait(&sem_itens);

    pthread_mutex_lock(&mutex_fila);

    Pedido p = fila[out];
    out = (out + 1) % MAX_BUFFER;
    count--;

    // Acorda produtores (caso antes estivesse cheio)
    pthread_cond_signal(&cond_fila);

    pthread_mutex_unlock(&mutex_fila);

    // Indica que há mais um espaço disponível
    sem_post(&sem_espacos);

    return p;
}

/* --------------------------------------------------------------------------
   Thread cozinheira
----------------------------------------------------------------------------*/
void* cozinheiro_thread(void* arg) {
    int id = (long)arg;

    while (1) {
        Pedido p = retirar_da_fila();

        // Atualiza painel: começou preparo
        painel_comeca_preparo();

        printf("[COZINHEIRO %d] Preparando: %s (%d s)\n",
               id, p.descricao, p.tempo_preparo);

        sleep(p.tempo_preparo);

        printf("[COZINHEIRO %d] Finalizou: %s\n",
               id, p.descricao);

        // Atualiza painel: finalizou pedido
        painel_finaliza_pedido();
    }

    return NULL;
}

/* --------------------------------------------------------------------------
   Função principal do servidor
----------------------------------------------------------------------------*/
int main() {
    printf("[SERVIDOR] Inicializando servidor...\n");

    // Inicializa fila de mensagens POSIX
    mqd_t mq = init_message_queue();
    (void)mq;

    // Inicializa mutex e variável de condição
    pthread_mutex_init(&mutex_fila, NULL);
    pthread_cond_init(&cond_fila, NULL);

    // Inicializa semáforos (Etapa 2)
    sem_init(&sem_itens, 0, 0);               // nenhum item no início
    sem_init(&sem_espacos, 0, MAX_BUFFER);    // buffer completamente vazio

    // Inicializa mutex do painel
    pthread_mutex_init(&mutex_painel, NULL);

    // Inicializa memória compartilhada para PainelStatus (Etapa 3)
    int shm_fd = init_shared_memory(sizeof(PainelStatus));
    painel = (PainelStatus*) map_shared_memory(shm_fd, sizeof(PainelStatus));

    // Zera painel
    painel->total_pedidos = 0;
    painel->em_preparo = 0;
    painel->finalizados = 0;

    // Cria threads cozinheiras
    pthread_t threads[NUM_COZINHEIROS];
    for (int i = 0; i < NUM_COZINHEIROS; i++) {
        pthread_create(&threads[i], NULL, cozinheiro_thread, (void*)(long)i);
    }

    // Loop principal de recebimento de pedidos
    while (1) {
        Pedido p;
        receive_pedido(&p);

        printf("[SERVIDOR] Recebido do cliente %d: %s\n",
               p.id_cliente, p.descricao);

        colocar_na_fila(p);

        // (Opcional) debug rápido do painel
        pthread_mutex_lock(&mutex_painel);
        printf("[PAINEL] Total: %d | Em preparo: %d | Finalizados: %d\n",
               painel->total_pedidos,
               painel->em_preparo,
               painel->finalizados);
        pthread_mutex_unlock(&mutex_painel);
    }

    return 0;
}
