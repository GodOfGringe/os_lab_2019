CC = gcc
CFLAGS = -Wall -Wextra

psum: parallel_sum.o sum.o ../../../lab3/src/utils.o
	$(CC) parallel_sum.o sum.o ../../../lab3/src/utils.o -o psum -pthread

parallel_sum.o: parallel_sum.c sum.h
	$(CC) $(CFLAGS) -c parallel_sum.c

sum.o: sum.c sum.h
	$(CC) $(CFLAGS) -c sum.c

../../../lab3/src/utils.o: ../../../lab3/src/utils.c ../../../lab3/src/utils.h
	$(CC) $(CFLAGS) -c ../../../lab3/src/utils.c -o ../../../lab3/src/utils.o

clean:
	rm -f psum *.o ../../../lab3/src/utils.o