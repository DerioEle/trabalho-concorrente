#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

mqd_t init_message_queue() {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Pedido);
    attr.mq_curmsgs = 0;

    mqd_t queue = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (queue == -1) {
        perror("mq_open");
        exit(1);
    }

    return queue;
}

void send_pedido(const Pedido *p) {
    mqd_t queue = mq_open(MQ_NAME, O_WRONLY);
    if (queue == -1) {
        perror("mq_open");
        exit(1);
    }

    if (mq_send(queue, (const char*)p, sizeof(Pedido), 0) == -1) {
        perror("mq_send");
        exit(1);
    }

    mq_close(queue);
}

void receive_pedido(Pedido *p) {
    mqd_t queue = mq_open(MQ_NAME, O_RDONLY);
    if (queue == -1) {
        perror("mq_open");
        exit(1);
    }

    if (mq_receive(queue, (char*)p, sizeof(Pedido), NULL) == -1) {
        perror("mq_receive");
        exit(1);
    }

    mq_close(queue);
}

int init_shared_memory(int size) {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    if (ftruncate(shm_fd, size) == -1) {
        perror("ftruncate");
        exit(1);
    }

    return shm_fd;
}

void* map_shared_memory(int shm_fd, int size) {
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    return ptr;
}
