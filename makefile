CC = gcc -std=gnu11

CFLAGS = -g -Wall -lssl -lcrypto -L/usr/lib -lm

all: node

node: node.o sockethandler.o hash.o hash_table.o 
	$(CC) $(CFLAGS) -o node node.o sockethandler.o hash_table.o hash.o

node.o: node.c node.h c_header/pdu.h sockethandler.h hashtable/hash.h hashtable/hash_table.h
	$(CC) $(CFLAGS) -c node.c

hash.o: hashtable/hash.c hashtable/hash.h
	$(CC) $(CFLAGS) -c hashtable/hash.c

hash_table.o: hashtable/hash_table.c hashtable/hash_table.h
	$(CC) $(CFLAGS) -c hashtable/hash_table.c

sockethandler.o: sockethandler.c sockethandler.h
	$(CC) $(CFLAGS) -c sockethandler.c