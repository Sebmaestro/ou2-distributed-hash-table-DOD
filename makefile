CC = gcc -std=gnu11

CFLAGS = -g -Wall

all: node

node: node.o
	$(CC) $(CFLAGS) -o node node.o

node.o: node.c node.h c_header/pdu.h
	$(CC) $(CFLAGS) -c node.c
