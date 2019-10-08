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

struct socketData {
  int socketFd;
  int port;
};

uint8_t *receivePDU(int socket);

void sendPDU(int socket, struct sockaddr_in address, void *pduSend, int size);

struct socketData createSocket(int socketPort, int type);

struct sockaddr_in getSocketAddress(int trackerPort, char *address);