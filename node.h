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


void handleArguments(int argc, char **argv, int *port, char **address);
struct socketData createSocket(int socketPort, int type);
void retrieveNodeIp(struct socketData trackerSock, struct sockaddr_in trackerAddress, uint8_t **ip);
void sendPDU(int socket, struct sockaddr_in address, void *pduSend);
uint8_t *receivePDU(int socket);
struct sockaddr_in getSocketAddress(int trackerPort, char *address);
struct NET_GET_NODE_RESPONSE_PDU getNodePDU(struct socketData trackerSock, struct sockaddr_in trackerAddress);
void sendNetAlive(struct socketData trackerSock, struct sockaddr_in trackerAddress);
void joinNetwork(struct NET_GET_NODE_RESPONSE_PDU ngnrp, struct socketData predSock, uint8_t *ip);
