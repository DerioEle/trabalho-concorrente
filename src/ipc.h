#ifndef IPC_H
#define IPC_H

#include <mqueue.h>
#include <semaphore.h>

#define MQ_NAME  "/fila_pedidos"
#define SHM_NAME "/painel_status"
#define MAX_DESC 128

typedef struct {
    int id_cliente;
    char descricao[MAX_DESC];
    int tempo_preparo;
} Pedido;

typedef struct {
    int total_pedidos;
    int em_preparo;
    int finalizados;
} PainelStatus;

mqd_t init_message_queue();
void send_pedido(const Pedido *p);
void receive_pedido(Pedido *p);

int init_shared_memory(int size);
void* map_shared_memory(int shm_fd, int size);

#endif
