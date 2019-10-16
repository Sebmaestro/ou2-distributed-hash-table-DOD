#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include "c_header/pdu.h"

#ifndef _SOCKETHANDLER_
#define _SOCKETHANDLER_

struct socketData {
  int socketFd;
  int port;
};

uint8_t *receivePDU(int socket);

void sendPDU(int socket, struct sockaddr_in address, void *pduSend, int size);

struct socketData createSocket(int socketPort, int type);

struct sockaddr_in getSocketAddress(int trackerPort, char *address);

int connectToSocket(int port, char *address, int socket);

uint8_t *readTCPMessage(int socket, uint8_t expectedSize, uint8_t type);

#endif
