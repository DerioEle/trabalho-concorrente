#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Pedidos automáticos para demonstrar concorrência
const char* pedidos_exemplo[] = {
    "Hamburguer",
    "Pizza",
    "Batata Frita",
    "Refrigerante",
    "Cachorro Quente",
    "Milkshake",
    "Coxinha",
    "X-Salada",
    "Açaí",
    "Macarronada"
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <numero_de_clientes>\n", argv[0]);
        exit(1);
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        printf("Número inválido de clientes.\n");
        exit(1);
    }

    printf("[LAUNCHER] Criando %d processos cliente...\n", n);

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            // Processo filho: gera pedido aleatório
            srand(getpid());
            const char *pedido = pedidos_exemplo[rand() % (sizeof(pedidos_exemplo)/sizeof(char*))];

            printf("[LAUNCHER] Filho %d enviando pedido: %s\n", getpid(), pedido);

            execlp("./cliente", "cliente", pedido, NULL);

            // Se execlp falhar:
            perror("execlp");
            exit(1);
        }
        else if (pid < 0) {
            perror("fork");
            exit(1);
        }
    }

    // Processo pai espera todos os filhos terminarem
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    printf("[LAUNCHER] Todos os clientes terminaram.\n");

    return 0;
}
