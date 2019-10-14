/* TODOOOOOOOOOOOO HALLÅ KOLLA HÄR. SÄTT IN SUCCESSORSOCK FÖR VARJE NOD I STRUCEN PAJAS */

#include "node.h"


#define MAXCLIENTS 6
#define BUFFERSIZE 256


//https://git.cs.umu.se/courses/5dv197ht19/tree/master/assignment2
int main(int argc, char **argv) {
  printf("hash: %d\n", hash_ssn("199807184198"));
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
    free(buffer);
  }


  listen(newNodeSock.socketFd, 5);


  bool loop = true;
  while (loop) {
    /* Ping tracker */
    printf("\nMy hash-ranges: min: %d max: %d.\n", node->hashMin, node->hashMax);
    sendNetAlive(trackerSock.socketFd, agentSock, trackerAddress);

    int ret = poll(pollFds, currentConnections, 7000);
    if(ret <= 0) {
      //fprintf(stderr, "\nSending NET_ALIVE to tracker.\n");
    }
    for (size_t i = 0; i < currentConnections; i++) {

      /* Reading from stdin */
      if (pollFds[i].revents & POLLIN && i == 0) {
        char *buffer = calloc(BUFFERSIZE, sizeof(char));
				int readValue = read(pollFds[i].fd, buffer, BUFFERSIZE-1);
        if(strcmp("quit\n", buffer) == 0 || readValue <= 0) {


          /* Node is leaving the network. */

          /* 1. Send NET_NEW_RANGE
           * 2. Transfer all table_entries
           * 3. Send NET_LEAVING
           */


          leaveNetwork(predecessorSock.socketFd, node, successorSock.socketFd);

          free(buffer);
          loop = false;
          break;
        }

        free(buffer);

      } else if(pollFds[i].revents & POLLIN && i == 2) {
        struct sockaddr_in predAddr;
        socklen_t len = sizeof(predAddr);
        int predecessor = accept(pollFds[i].fd, (struct sockaddr*)&predAddr, &len);

        printf("\x1B[36mAccepted predecessor on socket: %d\n", predecessor);
        printf("\x1B[0mAccepting on index: %d\n", currentConnections);
        predecessorSock.socketFd = predecessor;
        pollFds[currentConnections].fd = predecessorSock.socketFd;
        pollFds[currentConnections].events = POLLIN;
        currentConnections++;
        break;


        /* Reading from pollin */
      } else if(pollFds[i].revents & POLLIN) {
        uint8_t *buffer = calloc(256, sizeof(uint8_t));
				int readValue = read(pollFds[i].fd, buffer, BUFFERSIZE-1);
        printf("Read %d bytes from %ld sock %d\n", readValue, i, pollFds[i].fd);

        /*Connection dropped, close socket.*/
        if(readValue <= 0) {
          //Socket closed unexpectedly, shutdown.
          loop = false;
          break;
        }
        switch(buffer[0]) {
          case NET_JOIN: {
            struct NET_JOIN_PDU njp;
            memcpy(&njp, buffer, sizeof(struct NET_JOIN_PDU));
            handleNetJoin(njp, node, successorSock.socketFd);
            break;
          }
          /* Expected disconnection, close the socket. */
          case NET_CLOSE_CONNECTION: {
            close(pollFds[i].fd);
            currentConnections--;
            i--;
            continue;
          }
          case VAL_INSERT: {
            int bufferSize = 0;
            struct VAL_INSERT_PDU vip = extractPDU(buffer, &bufferSize);
            handleValInsert(vip, node, successorSock.socketFd, bufferSize, buffer);
            break;
          }
          case VAL_REMOVE: {
            struct VAL_REMOVE_PDU vrp;
            memcpy(&vrp, buffer, sizeof(vrp));
            removeValue(vrp, node, successorSock.socketFd);
            break;
          }
          case VAL_LOOKUP: {
            struct VAL_LOOKUP_PDU vlp;
            memcpy(&vlp, buffer, sizeof(vlp));
            lookupValue(vlp, node, successorSock.socketFd, agentSock.socketFd);
            break;
          }
          case NET_NEW_RANGE: {
            printf("Joller\n");
            struct NET_NEW_RANGE_PDU nnrp;
            memcpy(&nnrp, buffer, sizeof(nnrp));
            node->hashMax = nnrp.new_range_end;
            break;
          }
          case NET_LEAVING: {
            struct NET_LEAVING_PDU nlp;
            memcpy(&nlp, buffer, sizeof(nlp));

            close(successorSock.socketFd);

            connectToSocket(nlp.next_port, nlp.next_address, successorSock.socketFd);
            break;
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
 */
void lookupValue(struct VAL_LOOKUP_PDU vlp, struct node *node, int socket, int agentSocket) {


  if (isInRange(node, vlp.ssn)) {
    printf("Här finns jag\n");
    /* Send val lookup response */

    struct VAL_LOOKUP_RESPONSE_PDU vlrp;
    struct table_entry *tableEntry;
    vlrp.type = VAL_LOOKUP_RESPONSE;
    tableEntry = table_lookup(node->hashTable, vlp.ssn);

    int size = 0;
    /* If entry does not exist */
    if (tableEntry == NULL) {
      printf("Finns ej lagrat\n");
      vlrp.name_length = 0;
      vlrp.name = NULL;
      vlrp.email_length = 0;
      vlrp.email = NULL;
    } else { /* If entry do exist */
      printf("Finns lagrat:\n");
      strcpy((char*)vlrp.ssn, tableEntry->ssn);
      vlrp.name_length = strlen(tableEntry->name) + 1;
      copyValueToPDU(&vlrp.name, tableEntry->name, vlrp.name_length);
      vlrp.email_length = strlen(tableEntry->email) + 1;
      copyValueToPDU(&vlrp.email, tableEntry->email, vlrp.email_length);

    }
    vlrp.PAD = 0;
    memset(&vlrp.PAD2, 0, sizeof(vlrp.PAD2));

    size = sizeof(vlrp) - 16 + vlrp.name_length + vlrp.email_length;

    uint8_t *buffer = calloc(size, sizeof(uint8_t));

    int copiedBytes = 0;

    memcpy(buffer, &vlrp, 16);
    copiedBytes = 16;

    memcpy(buffer + copiedBytes, vlrp.name, vlrp.name_length);
    copiedBytes = copiedBytes + vlrp.name_length;
    memcpy(buffer + copiedBytes, &vlrp.email_length, 1);
    copiedBytes++;
    memcpy(buffer + copiedBytes, vlrp.PAD2, sizeof(vlrp.PAD2));
    copiedBytes = copiedBytes + sizeof(vlrp.PAD2);
    memcpy(buffer + copiedBytes, vlrp.email, vlrp.email_length);

    struct sockaddr_in address = getSocketAddress(htons(vlp.sender_port), vlp.sender_address);
    sendPDU(agentSocket, address, buffer, size);
    free(vlrp.name);
    free(vlrp.email);

  } else {
    /* Entry was not within the node's index-range. Send to successor. */
    printf("Jag fanns inte här\n");

    send(socket, &vlp, sizeof(vlp), 0);
  }
}

/*
 *
 *
 *
 *
 */
void removeValue(struct VAL_REMOVE_PDU vrp, struct node *node, int socket) {
  if (isInRange(node, (char*)vrp.ssn)) {
    table_remove(node->hashTable, (char*)vrp.ssn);
    printf("Removed value!\n");
  } else {
    send(socket, &vrp, sizeof(vrp), 0);
  }
}

/*
 *
 *
 *
 */
 bool isInRange(struct node *node, char *ssn) {

   //IS THIS THE RIGHT WAY?!?!?! OR ONLY USE HASH_FUNC?!
   //hash_t hashIndex = node->hashTable->hash_func(ssn) % node->hashTable->max_entries;
   hash_t hashIndex = hash_ssn(ssn);
   //printf("Hash index is: %d. Search and destroy for node with same hash index\n", hashIndex);
   /* In index, node will receive value */
   if (node->hashMin <= hashIndex && hashIndex <= node->hashMax) {
     return true;
   }
   return false;
 }

/*
 *
 *
 *
 */
void handleValInsert(struct VAL_INSERT_PDU vip, struct node *node, int socket, int size, uint8_t *buffer) {



  if (isInRange(node, (char*)vip.ssn)) {
    table_insert(node->hashTable, (char*)vip.ssn, (char*)vip.name, (char*)vip.email);
    printf("Inserted value into table\n");
  } else {
    printf("Sending values to next node\n");
    send(socket, buffer, size, 0);
    free(vip.name);
    free(vip.email);
  }

}

/*
 *
 *
 *
 */
struct VAL_INSERT_PDU extractPDU(uint8_t *buffer, int *bufferSize) {
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
  vip.name = calloc(vip.name_length, sizeof(uint8_t));
  memcpy(vip.name, buffer+readBytes, size);
  readBytes = readBytes+vip.name_length;
  size = sizeof(vip.email_length);

  /*Read email_length*/
  memcpy(&vip.email_length, buffer+readBytes, size);

  readBytes = readBytes + sizeof(vip.email_length);
  size = sizeof(vip.PAD2);

  memcpy(&vip.PAD2, buffer+readBytes, size);

  readBytes = readBytes + sizeof(vip.PAD2);
  size = vip.email_length;
  vip.email = calloc(vip.email_length, sizeof(uint8_t));
  /*Read email*/
  memcpy(vip.email, buffer+readBytes, vip.email_length);

  readBytes = readBytes + vip.email_length;
  *bufferSize = readBytes;

  printf("SSN: %s\n", vip.ssn);
  printf("Name: %s\n", (char*)vip.name);
  printf("Email: %s\n", (char*)vip.email);

  printf("%d\n", *bufferSize);
  printf("%ld\n", sizeof(vip));

  return vip;
}




/**
 *
 *
 *
 *
 */
void handleNetJoin(struct NET_JOIN_PDU njp, struct node *node, int socket) {
  printf("JOIN PDU \n");

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
    printf("Connected successfully to successor.\n");
    printf("Sending response on socket: %d\n", socket);
    send(socket, &njrp, sizeof(struct NET_JOIN_RESPONSE_PDU), 0);

    transferTableEntries(socket, node);
  } else {
    perror("connect");
  }
}

/**
 *
 * socket - predecessor
 * node - swag
 */
void leaveNetwork(int predSocket, struct node* node, int succSock) {
  /* Node is leaving the network. */

  /* 1. Send NET_NEW_RANGE
   * 2. Transfer all table_entries
   * 3. Send NET_LEAVING
   */

   struct NET_CLOSE_CONNECTION_PDU nccp;
   nccp.type = NET_CLOSE_CONNECTION;
   send(succSock, &nccp, sizeof(nccp), 0);
   close(succSock);

   struct NET_NEW_RANGE_PDU nnrp;
   nnrp.type = NET_NEW_RANGE;
   nnrp.new_range_end = node->hashMax;

   printf("Sendar första. Socket: %d\n", predSocket);
   if(send(predSocket, &nnrp, sizeof(nnrp), 0) == -1) {
     perror("send1");
   }
   node->hashMin = -1;
   node->hashMax = -1;

   transferTableEntries(predSocket, node);

   struct NET_LEAVING_PDU nlp;
   nlp.type = NET_LEAVING;
   strcpy(nlp.next_address, node->successor->ip);
   nlp.pad = 0;
   nlp.next_port = htons(node->successor->port);

   printf("Sendar andra. Socket: %d\n", predSocket);
   if (send(predSocket, &nlp, sizeof(nlp), 0) == -1) {
     perror("send2");
   }

}

void transferTableEntries(int socket, struct node *node) {
  struct table_entry *entry;
  while((entry = get_entry_iterator(node->hashTable)) != NULL){
      if(!isInRange(node, entry->ssn)){
        /* Not within the node's range anymore, transfer to successor. */
        struct VAL_INSERT_PDU vip;
        vip.type = VAL_INSERT;
        strcpy((char*)vip.ssn, entry->ssn);
        vip.name_length = strlen(entry->name) + 1;
        vip.PAD = 0;
        copyValueToPDU(&vip.name, entry->name, vip.name_length);
        vip.email_length = strlen(entry->email) + 1;
        memset(&vip.PAD2, 0, sizeof(vip.PAD2));
        copyValueToPDU(&vip.email, entry->email, vip.email_length);

        int size = sizeof(vip) - 16 + vip.name_length + vip.email_length;
        uint8_t *buffer = calloc(size, sizeof(uint8_t));

        int copiedBytes = 0;

        memcpy(buffer, &vip, 16);
        copiedBytes = 16;

        memcpy(buffer + copiedBytes, vip.name, vip.name_length);
        copiedBytes = copiedBytes + vip.name_length;
        memcpy(buffer + copiedBytes, &vip.email_length, 1);
        copiedBytes++;
        memcpy(buffer + copiedBytes, vip.PAD2, sizeof(vip.PAD2));
        copiedBytes = copiedBytes + sizeof(vip.PAD2);
        memcpy(buffer + copiedBytes, vip.email, vip.email_length);

        send(socket, buffer, size, 0);

        printf("bordet tar bort\n");
        table_remove(node->hashTable, entry->ssn);
      }
  }

}

void copyValueToPDU(uint8_t **dest, char *toCopy, int len) {
  *dest = malloc(len * sizeof(char));
  strcpy((char*)*dest, toCopy);
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
