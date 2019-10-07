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

struct socketData createSocket(int socketPort, int type);

struct sockaddr_in getSocketAddress(int trackerPort, char *address);