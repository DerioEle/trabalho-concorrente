CC=gcc
CFLAGS=-Wall -pthread -lrt
SRC=src
INC=include

all: servidor cliente painel launcher

servidor: $(SRC)/servidor.c $(SRC)/ipc.c
	$(CC) $(CFLAGS) -I$(INC) -o servidor $(SRC)/servidor.c $(SRC)/ipc.c

cliente: $(SRC)/cliente.c $(SRC)/ipc.c
	$(CC) $(CFLAGS) -I$(INC) -o cliente $(SRC)/cliente.c $(SRC)/ipc.c

painel: $(SRC)/painel.c $(SRC)/ipc.c
	$(CC) $(CFLAGS) -I$(INC) -o painel $(SRC)/painel.c $(SRC)/ipc.c

launcher: $(SRC)/launcher.c
	$(CC) $(CFLAGS) -I$(INC) -o launcher $(SRC)/launcher.c

clean:
	rm -f servidor cliente painel launcher
	rm -f $(SRC)/*.o
