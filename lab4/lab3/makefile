CC=gcc
CFLAGS=-I. -w

all: parallel_min_max exec process_memory

parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

exec:
	$(CC) -o $@ exec.c $(CFLAGS)

process_memory:
	$(CC) -o process_memory.o -c process_memory.c $(CFLAGS)

clean :
	rm utils.o find_min_max.o parallel_min_max exec process_memory
