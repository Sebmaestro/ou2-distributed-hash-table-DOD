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
#include <math.h>
#include "sockethandler.h"
#include "c_header/pdu.h"
#include "hashtable/hash.h"
#include "hashtable/hash_table.h"



struct node {
  int hashMin;
  int hashMax;
  char *ip;
  struct node *successor;
  struct node *predecessor;
  struct hash_table *hashTable;
  int port;
};

void handleArguments(int argc, char **argv, int *port, char **address);
char *retrieveNodeIp(struct socketData trackerSock, struct sockaddr_in trackerAddress);
void sendPDU(int socket, struct sockaddr_in address, void *pduSend, int size);
uint8_t *receivePDU(int socket);
struct NET_GET_NODE_RESPONSE_PDU sendNetGetNode(struct socketData trackerSock, struct sockaddr_in trackerAddress);
void sendNetAlive(int trackerSocket, struct socketData agentSock, struct sockaddr_in trackerAddress);
void sendNetJoin(struct NET_GET_NODE_RESPONSE_PDU ngnrp, char *ip,
                 struct socketData agentSock);
void handleNetJoin(struct NET_JOIN_PDU njp, struct node *node, int socket);
void getHashRanges(struct node *node, uint8_t *minS, uint8_t *maxS);
struct node* createNode(char *ipAddress, int port);
void sendInsert(char *buffer, int socket);
