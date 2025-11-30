#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ipc.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: ./cliente \"descricao do pedido\"\n");
        return 1;
    }

    Pedido p;
    p.id_cliente = getpid();
    strncpy(p.descricao, argv[1], MAX_DESC);
    p.tempo_preparo = 2;

    send_pedido(&p);
    printf("[CLIENTE %d] Pedido enviado!\n", p.id_cliente);

    return 0;
}
