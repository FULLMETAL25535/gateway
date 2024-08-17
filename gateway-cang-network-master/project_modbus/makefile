CC = gcc
CFLAGS = -Wall -lm -lpthread -lmodbus
DEPS = mb.h cJSON.h list.h shmem.h msg_queue_peer.h
OBJ = main.o mb.o cJSON.o shmem.o msg_queue_peer.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

mb_program: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o mb_program
