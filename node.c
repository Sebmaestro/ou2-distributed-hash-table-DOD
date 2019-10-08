#include "node.h"


#define MAXCLIENTS 6
#define BUFFERSIZE 256


//https://git.cs.umu.se/courses/5dv197ht19/tree/master/assignment2
int main(int argc, char **argv) {

  int trackerPort;
  char *address;
  handleArguments(argc, argv, &trackerPort, &address);


  struct socketData successorSock = createSocket(0, SOCK_STREAM);
  printf("\nsuccessorSock: %d port: %d\n", successorSock.socketFd, successorSock.port);
  struct socketData predecessorSock = createSocket(0, SOCK_STREAM);
  printf("predecessorSock: %d port: %d\n", predecessorSock.socketFd, predecessorSock.port);
  struct socketData newNodeSock = createSocket(0, SOCK_STREAM);
  printf("newNodeSock: %d port: %d\n", newNodeSock.socketFd, newNodeSock.port);
  struct socketData trackerSock = createSocket(0, SOCK_DGRAM);
  printf("trackerSock: %d port: %d\n", trackerSock.socketFd, trackerSock.port);
  struct socketData agentSock = createSocket(newNodeSock.port, SOCK_DGRAM);
  printf("agentSock: %d port: %d\n", agentSock.socketFd, agentSock.port);


  /* This is the address we need to speak with tracker heheh :⁾ */
  struct sockaddr_in trackerAddress = getSocketAddress(trackerPort, address);
 

  /* This is the node ip we get when asking the tracker for it :^) */
  uint8_t* nodeIp = retrieveNodeIp(trackerSock, trackerAddress);
  int publicPort = newNodeSock.port;
  
  
  
  struct node *node = createNode(nodeIp, publicPort);
  printf("Node IP: %s\n", node->ip);


  struct NET_GET_NODE_RESPONSE_PDU ngnrp;
  ngnrp = getNodePDU(trackerSock, trackerAddress);

  // struct hash_table* hashTable;
  listen(newNodeSock.socketFd, 5);
  /* Empty response, I.E no node in network */
  if (ntohs(ngnrp.port == 0) && ngnrp.address[0] == 0) {
    printf("No other nodes in the network, initializing hashrange.\n");
    node->hashMin = 0;
    node->hashMax = 255;
  } else { /* Non empty response */
    /* Net join */
    printf("\nNon-empty response from NET_GET_NODE_RESPONSE\n");
	printf("There are other nodes in the network.\n");
    joinNetwork(ngnrp, predecessorSock, node->ip, agentSock);
    struct sockaddr_in predAddr;
    socklen_t len = sizeof(predAddr);

    int predecessor = accept(newNodeSock.socketFd, (struct sockaddr*)&predAddr, &len);

    printf("Accepted predecessor on socket: %d\n", predecessor);
    predecessorSock.socketFd = predecessor;
     //table_shrink(hashTable, )

  }

  struct pollfd pollFds[MAXCLIENTS];
  pollFds[0].fd = STDIN_FILENO;
  pollFds[0].events = POLLIN;
  // pollFds[1].fd = successorSock.socketFd;
  // pollFds[1].events = POLLIN;
  // pollFds[2].fd = predecessorSock.socketFd;
  // pollFds[2].events = POLLIN;
  // pollFds[3].fd = newNodeSock.socketFd;
  // pollFds[3].events = POLLIN;
  // pollFds[4].fd = trackerSock.socketFd;
  // pollFds[4].events = POLLIN;
  // pollFds[5].fd = agentSock.socketFd;
  // pollFds[5].events = POLLIN;
  pollFds[1].fd = agentSock.socketFd;
  pollFds[1].events = POLLIN;
  pollFds[2].fd = newNodeSock.socketFd;
  pollFds[2].events = POLLIN;

  listen(newNodeSock.socketFd, 5);


  int currentClients = 3;
  bool loop = true;
  while (loop) {
    /* Ping tracker */
    sendNetAlive(trackerSock.socketFd, agentSock, trackerAddress);

    int ret = poll(pollFds, currentClients, 7000);
    if(ret <= 0) {
      fprintf(stderr, "Pinging tracker.\n");
    }
    for (size_t i = 0; i < currentClients; i++){
      if (pollFds[i].revents & POLLIN && i == 0) {
        /*Reading from stdin*/
        char *buffer = calloc(BUFFERSIZE, sizeof(char));
				int readValue = read(pollFds[i].fd, buffer, BUFFERSIZE-1);
        if(strcmp("quit\n", buffer) == 0 || readValue <= 0) {
          printf("Exiting loop\n");
          free(buffer);
          loop = false;
          break;
        }
        free(buffer);
      } else if(pollFds[i].revents & POLLIN) {
        uint8_t *buffer = calloc(256, sizeof(uint8_t));
				int readValue = read(pollFds[i].fd, buffer, BUFFERSIZE-1);
        printf("Read %d bytes\n", readValue);
        switch(buffer[0]){
          case NET_JOIN:
            printf("Handling net join!\n");
            struct NET_JOIN_PDU njp;
            memcpy(&njp, buffer, sizeof(struct NET_JOIN_PDU));
            handleNetJoin(njp, node, successorSock.socketFd);
            break;
        }
        free(buffer);
      }
    }
  }




  shutdown(successorSock.socketFd, SHUT_RDWR);
  shutdown(predecessorSock.socketFd, SHUT_RDWR);
  shutdown(newNodeSock.socketFd, SHUT_RDWR);
  shutdown(trackerSock.socketFd, SHUT_RDWR);
  shutdown(agentSock.socketFd, SHUT_RDWR);

  return 0;
}

/**
 *
 *
 *
 *
 */
void handleNetJoin(struct NET_JOIN_PDU njp, struct node *node, int socket) {
  printf("JOIN PDU \n");
  printf("type %d\n", njp.type);
  printf("src_address %s\n", njp.src_address);
  printf("src_port %d\n", ntohs(njp.src_port));
  printf("max_span %d\n", njp.max_span);
  printf("%s %s\n", "max_address", njp.max_address);
  printf("%s %d\n", "max_port", njp.max_port);



  struct NET_JOIN_RESPONSE_PDU njrp;
  if(node->successor == NULL) {
    njrp.type = NET_JOIN_RESPONSE;
    strcpy(njrp.next_address, (char*)node->ip);
    njrp.PAD = 0;
    njrp.next_port = node->port;
    getHashRanges(node, &njrp.range_start, &njrp.range_end);

    struct sockaddr_in sockAdr = getSocketAddress(ntohs(njp.src_port), njp.src_address);
    printf("Trying to connect to %s on port: %d\n", inet_ntoa(sockAdr.sin_addr), htons(sockAdr.sin_port));
    int con = connect(socket, (struct sockaddr*)&sockAdr, sizeof(sockAdr));
    printf("con = %d\n", con);
    perror("Connect");

    //No other nodes in the network, send response now.

    // TODOOOOOOOOOOOOOOOOOOOOOOOOOOOO SEND PDU TILL NYA NODEN
  }



}

/**
 *
 *
 *
 *
 */
void getHashRanges(struct node *node, uint8_t *minS, uint8_t *maxS) {

  float minP = 0;
  float maxP = 0;

  minP = (float)node->hashMin;
  *maxS = (uint8_t)node->hashMax;
  maxP = floor((maxP - minP) /2 ) + minP;
  *minS = (uint8_t)maxP + 1;

  node->hashMax = (int)maxP;
  node->hashMin = (int)minP;

  //TODO: SENT NET JOIN RESPONSE WITH minS AND maxS.
}

/**
 *
 *
 *
 *
 */
void joinNetwork(struct NET_GET_NODE_RESPONSE_PDU ngnrp, struct socketData predSock,
                                  uint8_t *ip, struct socketData agentSock) {



  printf("Sending NET_JOIN to: %s, on port: %d\n", ngnrp.address, ntohs(ngnrp.port));
  struct NET_JOIN_PDU njp;
  njp.type = NET_JOIN;
  //memcpy(njp.src_address, ip, sizeof(njp.src_address));
  strcpy(njp.src_address, (char*)ip);
  njp.PAD = 0;
  njp.src_port = htons(agentSock.port);
  njp.PAD2 = 0;
  njp.max_span = 0;
  njp.max_address[0] = '\0';
  njp.max_port = htons(0);

  printf("ip: %s\n", njp.src_address);


  struct sockaddr_in nodeAddress = getSocketAddress(ntohs(ngnrp.port), ngnrp.address);

  sendPDU(agentSock.socketFd, nodeAddress, &njp, sizeof(struct NET_JOIN_PDU));


}

/**
 *
 *
 *
 *
 */
void sendNetAlive(int trackerSocket, struct socketData agentSock, struct sockaddr_in trackerAddress) {

  struct NET_ALIVE_PDU nap;
  nap.type = NET_ALIVE;
  nap.pad = 0;
  nap.port = htons(agentSock.port);

  sendPDU(trackerSocket, trackerAddress, &nap, sizeof(struct NET_ALIVE_PDU));
}

/**
 *
 *
 *
 *
 */
struct NET_GET_NODE_RESPONSE_PDU getNodePDU(struct socketData trackerSock, struct sockaddr_in trackerAddress) {

  struct NET_GET_NODE_PDU ngnp;
  ngnp.type = NET_GET_NODE;
  ngnp.pad = 0;
  ngnp.port = htons(trackerSock.port);

  sendPDU(trackerSock.socketFd, trackerAddress, &ngnp, sizeof(struct NET_GET_NODE_PDU));

  uint8_t *buff = receivePDU(trackerSock.socketFd);
  struct NET_GET_NODE_RESPONSE_PDU ngnrp;


  if (buff[0] == NET_GET_NODE_RESPONSE) {
    memcpy(&ngnrp, buff, sizeof(struct NET_GET_NODE_RESPONSE_PDU));
	printf("\nReceived a NET_GET_NODE_RESPONSE\n");
    printf("pdu-type: %d\n", ngnrp.type);
    printf("address: %s\n", ngnrp.address);
    printf("port: %d\n", ntohs(ngnrp.port));
  } else {
    ngnrp.type = 0;
    ngnrp.address[0] = '\0';
    ngnrp.port = 0;
  }
  free(buff);
  return ngnrp;
}

/**
 * Retrieves ip for node with STUN request and stores it in the ip-pointer.
 */
uint8_t *retrieveNodeIp(struct socketData trackerSock,
                        struct sockaddr_in trackerAddress) {
  struct STUN_LOOKUP_PDU slp;
  slp.type = STUN_LOOKUP;
  slp.PAD = 0;
  slp.port = htons(trackerSock.port);

  sendPDU(trackerSock.socketFd, trackerAddress, &slp, sizeof(struct STUN_LOOKUP_PDU));

  uint8_t *buff = receivePDU(trackerSock.socketFd);

  struct STUN_RESPONSE_PDU srp;
  if (buff[0] == STUN_RESPONSE) {
      memcpy(&srp, buff, sizeof(struct STUN_RESPONSE_PDU));
      uint8_t *addr = calloc(ADDRESS_LENGTH, sizeof(uint8_t));
      memcpy(addr, srp.address, sizeof(srp.address));
      return addr;

  }

  free(buff);
  return NULL;
}


/**
 *
 *
 *
 *
 */
void handleArguments(int argc, char **argv, int *port, char **address) {
	if (argc != 3) {
    fprintf(stderr, "Wrong amount of arguments - ");
		fprintf(stderr, "Usage: ./node <tracker address> <tracker port>\n");
		exit(1);
	}

	*address = argv[1];

	*port = strtol(argv[2], NULL, 10);
  if (*port == 0) {

		fprintf(stderr, "Failed to convert port string to int\n");
		exit(1);
	}

	printf("Trackerport = %d\n", *port);
	printf("Trackeraddress = %s\n", *address);
	printf("number of args = %d\n", argc);
}


struct node* createNode(uint8_t *ipAddress, int port){
	struct node *n = calloc(1, sizeof(struct node));
	n->ip = ipAddress;
	n->port = port;
	n->hashTable = table_create(hash_ssn, 256);
	return n;
}




// int recv_all(int sockfd, void *buf, size_t len, int flags)
// {
//     size_t toread = len;
//     char  *bufptr = (char*) buf;
//
//     while (toread > 0)
//     {
//         ssize_t rsz = recv(sockfd, bufptr, toread, flags);
//         if (rsz <= 0)
//             return rsz;  /* Error or other end closed cnnection */
//
//         toread -= rsz;  /* Read less next time */
//         bufptr += rsz;  /* Next buffer position to read into */
//     }
//
//     return len;
// }
