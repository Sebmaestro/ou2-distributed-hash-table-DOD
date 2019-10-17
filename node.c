/*
 * P2P implementation as a network of nodes communicating with each other over
 * TCP/UDP sockets
 *
 * Authors: Sebastian Arledal - Pontus Fält / c17sal - dv17pft
 * Date 16-10-19
 */


#include "node.h"


#define MAXCLIENTS 6
#define BUFFERSIZE 256

/*
 * No main no gain
 *
 */
int main(int argc, char **argv) {
  int trackerPort;
  char *address;
  handleArguments(argc, argv, &trackerPort, &address);


  struct socketData successorSock = createSocket(0, SOCK_STREAM);
  printf("\nsuccessorSock: %d port: %d\n", successorSock.socketFd, successorSock.port);
  struct socketData predecessorSock;
  struct socketData newNodeSock = createSocket(0, SOCK_STREAM);
  printf("newNodeSock: %d port: %d\n", newNodeSock.socketFd, newNodeSock.port);
  struct socketData trackerSock = createSocket(0, SOCK_DGRAM);
  printf("trackerSock: %d port: %d\n", trackerSock.socketFd, trackerSock.port);
  struct socketData agentSock = createSocket(newNodeSock.port, SOCK_DGRAM);
  printf("agentSock: %d port: %d\n", agentSock.socketFd, agentSock.port);


  /* This is the address we need to speak with tracker heheh :⁾ */
  struct sockaddr_in trackerAddress = getSocketAddress(trackerPort, address);


  /* This is the node ip we get when asking the tracker for it :^) */
  char* nodeIp = retrieveNodeIp(trackerSock, trackerAddress);
  int publicPort = newNodeSock.port;



  struct node *node = createNode(nodeIp, publicPort);
  free(nodeIp);
  printf("Node IP: %s\n", node->ip);


  struct NET_GET_NODE_RESPONSE_PDU ngnrp;
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

    /* New node accepting predecessor (Old node already in network )*/
    int predecessor = accept(newNodeSock.socketFd, (struct sockaddr*)&predAddr, &len);

    printf("Accepted predecessor on socket: %d\n", predecessor);
    predecessorSock.socketFd = predecessor;
    pollFds[currentConnections].fd = predecessorSock.socketFd;
    pollFds[currentConnections].events = POLLIN;
    currentConnections++;

    uint8_t *buf = calloc(16, sizeof(uint8_t));
    if(read(predecessorSock.socketFd, buf, 1) < 0){
      perror("read");
    }
    uint8_t type = buf[0];
    uint8_t *buffer = readTCPMessage(predecessorSock.socketFd, getPDUSize(type), type);
    struct NET_JOIN_RESPONSE_PDU njrp;
    memcpy(&njrp, buffer, sizeof(struct NET_JOIN_RESPONSE_PDU));

    node->hashMax = njrp.range_end;
    node->hashMin = njrp.range_start;
    connectToSocket(njrp.next_port, njrp.next_address, successorSock.socketFd);
    node->successor = createNode(njrp.next_address, ntohs(njrp.next_port));
    pollFds[currentConnections].fd = successorSock.socketFd;
    pollFds[currentConnections].events = POLLIN;
    currentConnections++;
    free(buf);
    free(buffer);
  }


  listen(newNodeSock.socketFd, 5);


  bool loop = true;
  while (loop) {

    /* Ping tracker */
    printf("\nHash-min: %d Hash-max: %d. publicPort: %d currentConnections: %d\n", node->hashMin, node->hashMax, node->port, currentConnections);
    if(node->successor){
      printf("My successor's ip is: %s\n", node->successor->ip);
    }

    sendNetAlive(trackerSock.socketFd, agentSock, trackerAddress);

    int ret = poll(pollFds, currentConnections, 7000);
    if(ret <= 0) {
      //fprintf(stderr, "\nSending NET_ALIVE to tracker.\n");
    }
    for (size_t i = 0; i < currentConnections; i++) {

      /* Reading from stdin */
      if (pollFds[i].revents & POLLIN && i == 0) {
        char *buffer = calloc(BUFFERSIZE, sizeof(char));
				int readBytes = read(pollFds[i].fd, buffer, BUFFERSIZE-1);
        if(strcmp("quit\n", buffer) == 0 || readBytes <= 0) {
          /* Node is leaving the network. */

          if(node->successor != NULL){
            printf("LEAVING NETWORK!!!\n");
            leaveNetwork(predecessorSock.socketFd, node, successorSock.socketFd);
          }


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
        uint8_t *buf = calloc(16, sizeof(uint8_t));
				int readBytes = 1;
        uint8_t *buffer;

        if(i == 1){
          /* UDP */
          buffer = receivePDU(pollFds[i].fd);
        } else {
          /* TCP */
          readBytes = read(pollFds[i].fd, buf, 1);
          // printf("Read %d bytes from %ld sock %d\n", readBytes, i, pollFds[i].fd);
          uint8_t type = buf[0];
          printf("Type %d\n", type);
          if(type == 0){
            continue;
          }
          buffer = readTCPMessage(pollFds[i].fd, getPDUSize(type), type);
        }




        /*Connection dropped, close socket.*/
        if(readBytes <= 0) {
          //Socket closed unexpectedly, shutdown.
          loop = false;
          break;
        }

        bool breakLoop = false;
        
        switch(buffer[0]) {
          case NET_JOIN: {
            bool firstConnection = false;
            if(node->successor == NULL){
              firstConnection = true;
            }
            struct NET_JOIN_PDU njp;
            memcpy(&njp, buffer, sizeof(struct NET_JOIN_PDU));
            int sock = handleNetJoin(njp, node, successorSock.socketFd);
            if (sock != -1) {
              successorSock.socketFd = sock;
              pollFds[currentConnections].fd = sock;
              pollFds[currentConnections].events = POLLIN;
              if(firstConnection){
                currentConnections++;
                breakLoop = true;
              }
            }

            break;
          }
          /* Expected disconnection, close the socket. */
          case NET_CLOSE_CONNECTION: {
            printf("Closing connection to predeccessor %d\n", pollFds[i].fd);
            close(pollFds[i].fd);
            pollFds[i] = pollFds[currentConnections -1];
            currentConnections--;
            i--;
            break;
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
            struct NET_NEW_RANGE_PDU nnrp;
            memcpy(&nnrp, buffer, sizeof(nnrp));
            node->hashMax = nnrp.new_range_end;
            break;
          }
          case NET_LEAVING: {
            printf("NET leaving, read from socket: %d\n", pollFds[i].fd);
            struct NET_LEAVING_PDU nlp;
            memcpy(&nlp, buffer, sizeof(nlp));

            close(pollFds[i].fd);

            successorSock = createSocket(0, SOCK_STREAM);


            table_free(node->successor->hashTable);
            free(node->successor->ip);
            free(node->successor);

            if (strcmp(nlp.next_address, node->ip) == 0 && ntohs(nlp.next_port) == node->port) {
              printf("Alone in network, should not connect to myself\n");
              pollFds[i] = pollFds[currentConnections -1];
              currentConnections--;
              i--;
              node->successor = NULL;
              break;
            }
            printf("Trying to connect to a new successor on %d\n", successorSock.socketFd);
            int con = connectToSocket(nlp.next_port, nlp.next_address, successorSock.socketFd);
            if(con < 0){
              /* Could not connect to a new successor */
              perror("connect");
            }

            node->successor = createNode(nlp.next_address, ntohs(nlp.next_port));
            pollFds[i].fd = successorSock.socketFd;
            pollFds[i].events = POLLIN;
            break;
          }
        }
        free(buf);
        free(buffer);
        if(breakLoop){
          break;
        }
      }
    }
  }
  shutdown(successorSock.socketFd, SHUT_RDWR);
  shutdown(predecessorSock.socketFd, SHUT_RDWR);
  shutdown(newNodeSock.socketFd, SHUT_RDWR);
  shutdown(trackerSock.socketFd, SHUT_RDWR);
  shutdown(agentSock.socketFd, SHUT_RDWR);

  close(successorSock.socketFd);
  close(predecessorSock.socketFd);
  close(newNodeSock.socketFd);
  close(trackerSock.socketFd);
  close(agentSock.socketFd);

  table_free(node->hashTable);
  if(node->successor != NULL){
    table_free(node->successor->hashTable);
    free(node->successor->ip);
    free(node->successor);
  }

  free(node->ip);
  free(node);
  return 0;
}

/**
 * function: lookupValue
 *
 * Description: Looks in nodes hashtable for given ssn. If is in range and node
 *              has ssn in table send back to asker. If in range but not found,
 *              send empty response. Otherwise send request to next node.
 *
 * input: pdu, node, successorsocket, agentsocket
 *
 *
 */
void lookupValue(struct VAL_LOOKUP_PDU vlp, struct node *node, int socket, int agentSocket) {


  if (isInRange(node, vlp.ssn)) {
    /* Send val lookup response */

    struct VAL_LOOKUP_RESPONSE_PDU vlrp;
    struct table_entry *tableEntry;
    vlrp.type = VAL_LOOKUP_RESPONSE;
    tableEntry = table_lookup(node->hashTable, vlp.ssn);

    int size = 0;
    /* If entry does not exist */
    if (tableEntry == NULL) {
      printf("Searched value was within hashrange but does not exist.\n");
      vlrp.name_length = 0;
      vlrp.name = NULL;
      vlrp.email_length = 0;
      vlrp.email = NULL;
    } else { /* If entry do exist */
      printf("Found value!\n");
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
    printf("Entry was not within hashrange, sending to successor\n");

    send(socket, &vlp, sizeof(vlp), 0);
  }
}

/*
 * function: removeValue
 *
 * description: Removes value from node hashtable if it exists.
 *
 * input: pdu, node, socket
 *
 */
void removeValue(struct VAL_REMOVE_PDU vrp, struct node *node, int socket) {
  if (isInRange(node, (char*)vrp.ssn)) {
    table_remove(node->hashTable, (char*)vrp.ssn);
    printf("Removing value if exist!\n");
  } else {
    send(socket, &vrp, sizeof(vrp), 0);
  }
}

/*
 * function: isInRange
 *
 * description: Checks if asked ssn is in nodes hashrange
 *
 * input: node, ssn
 *
 * output: true or false
 *
 */
 bool isInRange(struct node *node, char *ssn) {
   hash_t hashIndex = hash_ssn(ssn);
   /* In index, node will receive value */
   if (node->hashMin <= hashIndex && hashIndex <= node->hashMax) {
     return true;
   }
   return false;
 }

/* Function: handleValInsert
 * Description: inserts a table_entry with data from the INSERT_PDU.
 *              If the entry is not within the node's hashrange the buffer
 *              containing the PDU is sent to the successor.
 * Input: INSERT_PDU, the node, successor-socket, size of buffer,
 *        buffer containing the INSERT_PDU.
 */
void handleValInsert(struct VAL_INSERT_PDU vip, struct node *node, int socket,
                    int size, uint8_t *buffer) {
  if (isInRange(node, (char*)vip.ssn)) {
    // printf("Inserting value!\n");
    // printf("SSN: %s\n", (char*)vip.ssn);
    // printf("Name: %s\n", (char*)vip.name);
    // printf("Email: %s\n", (char*)vip.email);
    table_insert(node->hashTable, (char*)vip.ssn, (char*)vip.name, (char*)vip.email);
  } else {
    printf("Sending values to next node\n");
    send(socket, buffer, size, 0);
  }
  free(vip.name);
  free(vip.email);
}


/* Function: handleNetJoin
 * Description: Checks if the node is to receive a new successor.
 *              If the node is the one with highest hashrange,
 *              the node connects to the new successor and sends
 *              a NET_JOIN_RESPONSE.
 *              If the node is not to connect, it sends the JOIN-request
*               to its current successor.
 * Input: The NET_JOIN request, the node, successor-socket.
 * Output: filedescriptor to the socket.
 */
int handleNetJoin(struct NET_JOIN_PDU njp, struct node *node, int socket) {
  printf("Handling NET JOIN \n");

  struct NET_JOIN_RESPONSE_PDU njrp;
  /* One node in network */
  if(node->successor == NULL) {
    njrp.type = NET_JOIN_RESPONSE;
    strcpy(njrp.next_address, node->ip);
    njrp.PAD = 0;
    njrp.next_port = htons(node->port);
    setHashRanges(node, &njrp.range_start, &njrp.range_end);

    /* More than one */
  } else {
    /* Check if max span is lower than what we currently have */
    if (njp.max_span < (node->hashMax - node->hashMin)) {
      njp.max_span = (node->hashMax - node->hashMin);
      strcpy(njp.max_address, node->ip);
      njp.max_port = htons(node->port);

      printf("Updating max-span to: %d! Trying to send to socket %d\n", njp.max_span, socket);
      send(socket, &njp, sizeof(struct NET_JOIN_PDU), 0);

      return -1;
    }

    /* Q13 */
    if (strcmp(njp.max_address, node->ip) == 0 && ntohs(njp.max_port) == node->port) {
      setHashRanges(node, &njrp.range_start, &njrp.range_end);

      njrp.type = NET_JOIN_RESPONSE;
      strcpy(njrp.next_address, node->successor->ip);
      njrp.PAD = 0;
      njrp.next_port = htons(node->successor->port);

      struct NET_CLOSE_CONNECTION_PDU nccp;
      nccp.type = NET_CLOSE_CONNECTION;
      send(socket, &nccp, sizeof(struct NET_CLOSE_CONNECTION_PDU), 0);
      close(socket);

      struct socketData suc = createSocket(0, SOCK_STREAM);
      socket = suc.socketFd;
      table_free(node->successor->hashTable);
      free(node->successor->ip);
      free(node->successor);
    } else {
      /* Q14 */
      send(socket, &njp, sizeof(struct NET_JOIN_PDU), 0);
      printf("Did not update max-span. Trying to send to socket %d\n", socket);
      return -1;
    }
  }
  int connection = connectToSocket(njp.src_port, njp.src_address, socket);
  node->successor = createNode(njp.src_address, ntohs(njp.src_port));
  if(connection == 0) {
    printf("Connected successfully to successor.\n");
    printf("Sending response on socket: %d\n", socket);
    send(socket, &njrp, sizeof(struct NET_JOIN_RESPONSE_PDU), 0);

    transferTableEntries(socket, node);
    return socket;
  } else {
    perror("connect");
  }
  return -1;
}

/* Function: leaveNetwork
 * Description: leaves the network by sending a NET_CLOSE_CONNECTION to
 *              the successor and a NET_LEAVING_PDU to the predecessor.
 *              The table-entries are transferred to the predecessor before
 *              sending NET_LEAVING_PDU.
 * Input: predecessor-socket, the node, successor-socket
 * Output: N/A
 */
void leaveNetwork(int predSocket, struct node* node, int succSock) {
  /* Node is leaving the network. */


   struct NET_CLOSE_CONNECTION_PDU nccp;
   nccp.type = NET_CLOSE_CONNECTION;
   send(succSock, &nccp, sizeof(nccp), 0);
   close(succSock);

   struct NET_NEW_RANGE_PDU nnrp;
   nnrp.type = NET_NEW_RANGE;
   nnrp.new_range_end = node->hashMax;

   printf("Sending NET_NEW_RANGE. Socket: %d size: %ld\n", predSocket, sizeof(nnrp));
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

   printf("Sending NET_LEAVING_PDU. Socket: %d size: %ld \n", predSocket,sizeof(nlp));
   int s = send(predSocket, &nlp, sizeof(nlp), 0);
   if (s == -1) {
     perror("send2");
   }

}

/**
 * function: transferTableEntries
 *
 * description: Transfers all table entries in node that no longer belongs here
 * to another node
 *
 * input: socket, node
 *
 */
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
        table_remove(node->hashTable, entry->ssn);
        free(vip.name);
        free(vip.email);
        free(buffer);
      }
  }
  printf("Transfered values to successor\n");

}



/**
 * function: setHashRanges
 *
 * description: sets min and max hash range for node and its successor
 *
 * input: node, min and max to be sent to successor
 *
 */
void setHashRanges(struct node *node, uint8_t *minS, uint8_t *maxS) {

  float minP = (float)node->hashMin;
  float maxP = (float)node->hashMax;

  *maxS = (uint8_t)node->hashMax;
  maxP = floor((maxP - minP) /2 ) + minP;
  *minS = (uint8_t)maxP + 1;

  node->hashMax = (int)maxP;
  node->hashMin = (int)minP;
}

/**
 * function: sendNetJoin
 *
 * description: Creates pdu and sends the request to a node in network
 *
 * input: pdu, ip, socket
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

  /* Send Join-request over UDP. */
  sendPDU(agentSock.socketFd, nodeAddress, &njp, sizeof(struct NET_JOIN_PDU));
}


/**
 * function: sendNetAlive
 *
 * description: Creates pdu and sends net alive to tracker
 *
 * input: trackersocket, agentsocket, trackeraddress
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
 * function: sendNetGetNode
 *
 * description: Sends getnode request no tracker to get address to a node in
 *              network
 *
 * input: trackersocket, trackerAddress
 *
 * output: response from tracker as pdu
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
  } else {
    ngnrp.type = 0;
    ngnrp.address[0] = '\0';
    ngnrp.port = 0;
  }
  free(buff);
  return ngnrp;
}

/**
 * Function: retrieveNodeIp
 *
 * description: Retrieves ip for node with STUN request and stores it in the ip-pointer.
 *
 * input: trackersocket, trackerAddress
 *
 * output: ip
 *
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
      free(buff);
      return (char*)addr;

  }

  free(buff);
  return NULL;
}


/**
 * function: handleArguments
 *
 * description: Parses arguments and checks that they are correct when starting
 *              program from commandline
 *
 * input: command line args
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
}

/**
 * function: createNode
 *
 * description: Creates a node
 *
 * input: ip, port
 *
 * output: the node
 *
 */
struct node* createNode(char *ipAddress, int port) {
	struct node *n = calloc(1, sizeof(struct node));
  n->ip = calloc(strlen(ipAddress) + 1, sizeof(char));
	strcpy(n->ip, ipAddress);
	n->port = port;
	n->hashTable = table_create(hash_ssn, 256);
	return n;
}
