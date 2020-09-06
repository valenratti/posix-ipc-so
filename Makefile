CC=gcc
CC_args=-Wall -g
CC_args_IPC=-lrt -pthread

all: master slave vista

master: master.c
	$(CC) master.c -o master $(CC_args) $(CC_args_IPC)

slave: slave.c
	$(CC) slave.c -o slave $(CC_args)

vista: vista.c
	$(CC) vista.c -o vista $(CC_args) $(CC_args_IPC)
clean:
	rm -f master slave

.PHONY: all clean