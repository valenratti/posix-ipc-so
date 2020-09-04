CC=gcc
CC_args=-Wall -g
CC_args_IPC=-lrt -pthread

all: master slave

master: master.c
	$(CC) master.c -o master $(CC_args)

slave: slave.c
	$(CC) slave.c -o slave $(CC_args)

clean:
	rm -f master slave

.PHONY: all clean