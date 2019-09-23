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


void handleArguments(int argc, char **argv, int *trackerPort, char **address);
struct socketData createSocket(int socketPort, int type);
void retrieveNodeIp(int trackerPort, char *address, struct socketData sd, uint8_t **ip);
void receiveStun();
