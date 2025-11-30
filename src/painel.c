#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ipc.h"

int main() {
    printf("[PAINEL] Iniciando monitor...\n");

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open (painel)");
        exit(1);
    }

    PainelStatus *painel = mmap(
        NULL,
        sizeof(PainelStatus),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );

    if (painel == MAP_FAILED) {
        perror("mmap (painel)");
        exit(1);
    }

    while (1) {
        printf("\033[2J\033[H"); // clear
        printf("====== PAINEL DE STATUS ======\n");
        printf("Total de pedidos recebidos : %d\n", painel->total_pedidos);
        printf("Pedidos em preparo          : %d\n", painel->em_preparo);
        printf("Pedidos finalizados         : %d\n", painel->finalizados);
        printf("==============================\n");
        fflush(stdout);
        sleep(1);
    }

    return 0;
}
