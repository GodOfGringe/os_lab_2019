CC = gcc
AR = ar

CFLAGS = -Wall -Wextra -pthread

all: client server

multimodulo.o: multimodulo.c multimodulo.h
	$(CC) $(CFLAGS) -c multimodulo.c -o multimodulo.o

libmultimodulo.a: multimodulo.o
	$(AR) rcs libmultimodulo.a multimodulo.o

client: client.c multimodulo.h libmultimodulo.a
	$(CC) $(CFLAGS) -o client client.c -L. -lmultimodulo

server: server.c multimodulo.h libmultimodulo.a
	$(CC) $(CFLAGS) -o server server.c -L. -lmultimodulo

clean:
	rm -f client server multimodulo.o libmultimodulo.a