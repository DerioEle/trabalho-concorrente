CC=gcc
CFLAGS=-Wall -pthread -lrt

SRC=src
INC=include

all: servidor cliente

servidor: $(SRC)/servidor.c $(SRC)/ipc.c
	$(CC) $(CFLAGS) -I$(INC) -o servidor $(SRC)/servidor.c $(SRC)/ipc.c

cliente: $(SRC)/cliente.c $(SRC)/ipc.c
	$(CC) $(CFLAGS) -I$(INC) -o cliente $(SRC)/cliente.c $(SRC)/ipc.c

clean:
	rm -f servidor cliente
