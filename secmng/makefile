
WORKDIR=.
VPATH = ./src

CC=gcc
CFLGS= -Wall -g -I$(WORKDIR)/inc/
LIBFLAG = -L$(HOME)/lib

BIN = keymngclient keymngserver

all:$(BIN)

#myipc_shm.o keymng_shmop.o
keymngclient:keymngclient.o keymnglog.o keymngclientop.o keymng_shmop.o myipc_shm.o
	$(CC) $(LIBFLAG) -lpthread -litcastsocket -lmessagereal $^ -o $@ 

keymngserver:keymngserver.o keymngserverop.o keymnglog.o myipc_shm.o keymng_shmop.o keymng_dbop.o
	$(CC) $(LIBFLAG) $^ -o $@ -lpthread -litcastsocket -lmessagereal  -lclntsh  -licdbapi
 
%.o:%.c
	$(CC) $(CFLGS) -c $< -o $@	
clean:
	rm -f *.o $(BIN)

.PHONY:clean all
	
	




