CC=gcc

CFLAGS=-I../../lab3/src

all : parallel_min_max process_memory

parallel_min_max :
	$(CC) -o parallel_min_max ../../lab3/src/parallel_min_max.c ../../lab3/src/utils.c ../../lab3/src/find_min_max.c $(CFLAGS)

process_memory :
	$(CC) -o process_memory process_memory.c

clean :
	rm -f parallel_min_max process_memory