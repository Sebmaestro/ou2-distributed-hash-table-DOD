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
  struct socketData predecessorSock/*= createSocket(0, SOCK_STREAM)*/;
  // printf("predecessorSock: %d port: %d\n", predecessorSock.socketFd, predecessorSock.port);
  struct socketData newNodeSock = createSocket(0, SOCK_STREAM);
  printf("newNodeSock: %d port: %d\n", newNodeSock.socketFd, newNodeSock.port);
  struct socketData trackerSock = createSocket(0, SOCK_DGRAM);
  printf("trackerSock: %d port: %d\n", trackerSock.socketFd, trackerSock.port);
  struct socketData agentSock = createSocket(newNodeSock.port, SOCK_DGRAM);
  printf("agentSock: %d port: %d\n", agentSock.socketFd, agentSock.port);


  /* This is the address we need to speak with tracker heheh :⁾ */
  struct sockaddr_in trackerAddress = getSocketAddress(trackerPort, address);


  /* This is the node ip we get when asking the tracker for it :^) */
  //printf("\nSending STUN_LOOKUP to tracker.");
  char* nodeIp = retrieveNodeIp(trackerSock, trackerAddress);
  int publicPort = newNodeSock.port;



  struct node *node = createNode(nodeIp, publicPort);
  printf("Node IP: %s\n", node->ip);


  struct NET_GET_NODE_RESPONSE_PDU ngnrp;
  //printf("\nSending NET_GET_NODE to tracker.");
  ngnrp = sendNetGetNode(trackerSock, trackerAddress);

  int currentConnections = 3;
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
  /* Empty response, I.E no node in network */
  if (ntohs(ngnrp.port == 0) && ngnrp.address[0] == 0) {
    printf("No other nodes in the network, initializing hashrange.\n");
    node->hashMin = 0;
    node->hashMax = 255;
  } else { /* Non empty response */
    /* Net join */
    printf("\nNon-empty response from NET_GET_NODE_RESPONSE\n");
	  printf("There are other nodes in the network.\n");
    sendNetJoin(ngnrp, node->ip, agentSock);
    struct sockaddr_in predAddr;
    socklen_t len = sizeof(predAddr);

    int predecessor = accept(newNodeSock.socketFd, (struct sockaddr*)&predAddr, &len);

    printf("Accepted predecessor on socket: %d\n", predecessor);
    predecessorSock.socketFd = predecessor;
    uint8_t *buffer = calloc(256, sizeof(uint8_t));
    if(recv(predecessorSock.socketFd, buffer, BUFFERSIZE-1, 0) == -1) {
      perror("recv");
    }
    struct NET_JOIN_RESPONSE_PDU njrp;
    memcpy(&njrp, buffer, sizeof(struct NET_JOIN_RESPONSE_PDU));
    printf("Received NET_JOIN_RESPONSE:\n");
    printf("next_address: %s\n", njrp.next_address);
    printf("next_port: %d\n", ntohs(njrp.next_port));
    printf("%s: %d\n", "range_start", njrp.range_start);
    printf("%s: %d\n", "range_end", njrp.range_end);

    node->hashMax = njrp.range_end;
    node->hashMin = njrp.range_start;
    connectToSocket(njrp.next_port, njrp.next_address, successorSock.socketFd);
    node->successor = createNode(njrp.next_address, ntohs(njrp.next_port));
    pollFds[currentConnections].fd = predecessorSock.socketFd;
    pollFds[currentConnections].events = POLLIN;
    currentConnections++;
  }


  listen(newNodeSock.socketFd, 5);


  bool loop = true;
  while (loop) {
    /* Ping tracker */
    // printf("\nMy hash-ranges: min: %d max: %d.\n", node->hashMin, node->hashMax);
    sendNetAlive(trackerSock.socketFd, agentSock, trackerAddress);

    // printf("god aft1\n");
    int ret = poll(pollFds, currentConnections, 7000);
    // printf("god aft2\n");
    if(ret <= 0) {
      //fprintf(stderr, "\nSending NET_ALIVE to tracker.\n");
    }
    for (size_t i = 0; i < currentConnections; i++) {

      /* Reading from stdin */
      if (pollFds[i].revents & POLLIN && i == 0) {
        char *buffer = calloc(BUFFERSIZE, sizeof(char));
				int readValue = read(pollFds[i].fd, buffer, BUFFERSIZE-1);
        if(strcmp("quit\n", buffer) == 0 || readValue <= 0) {
          printf("Exiting loop\n");
          free(buffer);
          loop = false;
          break;
        } else if (strncmp("insert\n", buffer, 6) == 0) {

          sendInsert(buffer, successorSock.socketFd);



        }
        free(buffer);

      } else if(pollFds[i].revents & POLLIN && i == 2) {
        printf("dumbas\n");
        struct sockaddr_in predAddr;
        socklen_t len = sizeof(predAddr);
        int predecessor = accept(pollFds[i].fd, (struct sockaddr*)&predAddr, &len);

        printf("\x1B[36mAccepted predecessor on socket: %d\n", predecessor);
        printf("\x1B[0mAccepting on index: %d\n", currentConnections);
        predecessorSock.socketFd = predecessor;
        pollFds[currentConnections].fd = predecessorSock.socketFd;
        pollFds[currentConnections].events = POLLIN;
        currentConnections++;
        // printf("dumvas2\n");
        break;


        /* Reading from pollin */
      } else if(pollFds[i].revents & POLLIN) {
        uint8_t *buffer = calloc(256, sizeof(uint8_t));
				int readValue = read(pollFds[i].fd, buffer, BUFFERSIZE-1);
        printf("Read %d bytes from %ld sock %d\n", readValue, i, pollFds[i].fd);

        /*Connection dropped, close socket.*/
        if(readValue <= 0){
          printf("skratt\n");
          loop = false;
          break;
        }
        switch(buffer[0]) {
          case NET_JOIN: {
            // printf("galle\n");
            struct NET_JOIN_PDU njp;
            memcpy(&njp, buffer, sizeof(struct NET_JOIN_PDU));
            handleNetJoin(njp, node, successorSock.socketFd);
            break;
          }
          case NET_CLOSE_CONNECTION: {
            close(pollFds[i].fd);
            currentConnections--;
            i--;
            continue;
          }
          case VAL_INSERT: {
            struct VAL_INSERT_PDU vip;

            /*Size to copy*/
            uint8_t size = sizeof(vip.type) + sizeof(vip.ssn) +
                           sizeof(vip.name_length) + sizeof(vip.PAD);
            /*Number of bytes read from buffer*/
            uint8_t readBytes = 0;

            /*Read type, ssn, name_length, PAD*/
            memcpy(&vip, buffer, size);

            /*Update readBytes and next size to read*/
            readBytes = size;
            size = vip.name_length;

            /*Read name*/
            memcpy(vip.name, buffer+readBytes, size);

            readBytes = readBytes+vip.name_length;
            size = sizeof(vip.email_length);

            /*Read email_length*/
            memcpy(&vip.email_length, buffer+readBytes, 1);

            readBytes = readBytes + sizeof(vip.email_length) + sizeof(vip.PAD2);
            size = vip.email_length;

            /*Read email*/
            memcpy(vip.email, buffer+readBytes, vip.email_length);


            printf("SSN: %s\n", vip.ssn);
            printf("Name: %s\n", (char*)vip.name);
            printf("Email: %s\n", (char*)vip.email);
          }
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
 void sendInsert(char *buffer, int socket) {
   // char *newStr = buffer+7;
   // printf("%s\n", newStr);
   //
   //
   // char *ssn;
   // printf("%s\n", ssn = strsep(&newStr, ","));
   //
   // char *name;
   // name = strsep(&newStr, ",");
   // name++;
   // printf("%s\n", name);
   //
   // char *email;
   // email = strsep(&newStr, ",");
   // email++;
   // printf("%s\n", email);
   //
   //
   // struct VAL_INSERT_PDU vip;
   // printf("Sizeof %d\n", sizeof(vip));
   // vip.type = VAL_INSERT;
   // memset(&vip.PAD2, 0, sizeof(vip.PAD2));
   //
   // strcpy((char*)vip.ssn, ssn);
   //
   // vip.PAD = 0;
   //
   // vip.name_length = strlen(name);
   // vip.name = malloc(vip.name_length+1*sizeof(char));
   // strcpy((char*)vip.name, name);
   // printf("%s\n", vip.name);
   //
   //
   // vip.email_length = strlen(email);
   // vip.email = malloc(vip.email_length+1*sizeof(char));
   // strcpy((char*)vip.email, email);
   // printf("%s\n", vip.email);
   //
   // printf("Sizeof %d\n", sizeof(vip));
   // send(socket, &vip, sizeof(vip)+sizeof(vip.name)+sizeof(vip.email), 0);
 }





/**
 *
 *
 *
 *
 */
void handleNetJoin(struct NET_JOIN_PDU njp, struct node *node, int socket) {
  printf("JOIN PDU \n");
  // printf("type %d\n", njp.type);
  // printf("src_address %s\n", njp.src_address);
  // printf("src_port %d\n", ntohs(njp.src_port));
  // printf("max_span %d\n", njp.max_span);
  // printf("%s %s\n", "max_address", njp.max_address);
  // printf("%s %d\n", "max_port", njp.max_port);



  struct NET_JOIN_RESPONSE_PDU njrp;
  /* One node in network */
  if(node->successor == NULL) {
    njrp.type = NET_JOIN_RESPONSE;
    strcpy(njrp.next_address, node->ip);
    njrp.PAD = 0;
    njrp.next_port = htons(node->port);
    getHashRanges(node, &njrp.range_start, &njrp.range_end);

    /* More than one */
  } else {
    /* Check if max span is lower than what we currently have */
    if (njp.max_span < (node->hashMax - node->hashMin)) {
      njp.max_span = (node->hashMax - node->hashMin);
      strcpy(njp.max_address, node->ip);
      njp.max_port = htons(node->port);

      printf("Updating max-span to: %d! Trying to send to socket %d\n", njp.max_span, socket);
      send(socket, &njp, sizeof(struct NET_JOIN_PDU), 0);

      return;
    }

    /* Q13 */
    if (strcmp(njp.max_address, node->ip) == 0 && ntohs(njp.max_port) == node->port) {
      getHashRanges(node, &njrp.range_start, &njrp.range_end);

      njrp.type = NET_JOIN_RESPONSE;
      strcpy(njrp.next_address, node->successor->ip);
      njrp.PAD = 0;
      njrp.next_port = htons(node->successor->port);

      struct NET_CLOSE_CONNECTION_PDU nccp;
      nccp.type = NET_CLOSE_CONNECTION;
      send(socket, &nccp, sizeof(struct NET_CLOSE_CONNECTION_PDU), 0);
      close(socket);

      //TODO!!! IS THIS OK?!?!?! DO WE NEED TO UPDATE successorSock?!?!?
      struct socketData suc = createSocket(0, SOCK_STREAM);
      socket = suc.socketFd;
    } else {
      /* Q14 */
      send(socket, &njp, sizeof(struct NET_JOIN_PDU), 0);
      printf("Did not update max-span. Trying to send to socket %d\n", socket);
      return;
    }
  }
  int connection = connectToSocket(njp.src_port, njp.src_address, socket);
  node->successor = createNode(njp.src_address, ntohs(njp.src_port));
  if(connection == 0) {
    printf("Connected successfully.\n");
    printf("Sending response to socket %d\n", socket);
    send(socket, &njrp, sizeof(struct NET_JOIN_RESPONSE_PDU), 0);
  } else {
    perror("connect");
  }




}

/**
 *
 *
 *
 *48585
 */
void getHashRanges(struct node *node, uint8_t *minS, uint8_t *maxS) {

  float minP = (float)node->hashMin;
  float maxP = (float)node->hashMax;

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
void sendNetJoin(struct NET_GET_NODE_RESPONSE_PDU ngnrp, char *ip,
                 struct socketData agentSock) {
  printf("Sending NET_JOIN to: %s, on port: %d\n", ngnrp.address, ntohs(ngnrp.port));
  struct NET_JOIN_PDU njp;
  njp.type = NET_JOIN;
  strcpy(njp.src_address, (char*)ip);
  njp.PAD = 0;
  njp.src_port = htons(agentSock.port);
  njp.PAD2 = 0;
  njp.max_span = 0;
  njp.max_address[0] = '\0';
  njp.max_port = htons(0);

  printf("ip: %s\n", njp.src_address);


  struct sockaddr_in nodeAddress = getSocketAddress(ntohs(ngnrp.port), ngnrp.address);

  //Send Join-request over UDP.
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
struct NET_GET_NODE_RESPONSE_PDU sendNetGetNode(struct socketData trackerSock, struct sockaddr_in trackerAddress) {

  struct NET_GET_NODE_PDU ngnp;
  ngnp.type = NET_GET_NODE;
  ngnp.pad = 0;
  ngnp.port = htons(trackerSock.port);

  sendPDU(trackerSock.socketFd, trackerAddress, &ngnp, sizeof(struct NET_GET_NODE_PDU));

  uint8_t *buff = receivePDU(trackerSock.socketFd);
  struct NET_GET_NODE_RESPONSE_PDU ngnrp;


  if (buff[0] == NET_GET_NODE_RESPONSE) {
    memcpy(&ngnrp, buff, sizeof(struct NET_GET_NODE_RESPONSE_PDU));
	  //printf("\nReceived a NET_GET_NODE_RESPONSE\n");
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
char *retrieveNodeIp(struct socketData trackerSock,
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
      return (char*)addr;

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


struct node* createNode(char *ipAddress, int port) {
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
