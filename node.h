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
void retrieveNodeIp(struct socketData trackerSend, struct socketData trackerReceive, uint8_t **ip);
void sendPDU(struct socketData trackerSend, void *pduSend);
uint8_t *receivePDU(struct socketData trackerReceive);
struct sockaddr_in getSocketAddress(int trackerPort, char *address);
struct NET_GET_NODE_RESPONSE_PDU getNodePDU(struct socketData trackerSend, struct socketData trackerReceive);
void sendNetAlive(struct socketData trackerSend, int port);
void joinNetwork(struct NET_GET_NODE_RESPONSE_PDU ngnrp, struct socketData predSock, uint8_t *ip);
