#include "node.h"
#include "c_header/pdu.h"
#include "hashtable/hash.h"
#include "hashtable/hash_table.h"

#define MAXCLIENTS 6
#define BUFFERSIZE 256

struct socketData {
  int socketFd;
  int port;
  struct sockaddr_in address;
};


int main(int argc, char **argv) {

  int trackerPort;
  char *address;
  handleArguments(argc, argv, &trackerPort, &address);


  struct socketData successorSock = createSocket(0, SOCK_STREAM);
  struct socketData predecessorSock = createSocket(0, SOCK_STREAM);
  struct socketData newNodeSock = createSocket(0, SOCK_STREAM);
  struct socketData UDPSocket = createSocket(0, SOCK_DGRAM);
  struct socketData agentSock = createSocket(0, SOCK_DGRAM);

  struct socketData trackerSend;
  trackerSend.socketFd = UDPSocket.socketFd;
  trackerSend.port = trackerPort;
  trackerSend.address = getSocketAddress(trackerSend.port, address);


  /* This is the node ip we get when asking the tracker for it :^) */
  uint8_t *nodeIp;
  retrieveNodeIp(trackerSend, UDPSocket, &nodeIp);
  printf("Node IP: %s\n", nodeIp);

  struct NET_GET_NODE_RESPONSE_PDU ngnrp;

  ngnrp = getNodePDU(trackerSend, UDPSocket);

  struct hash_table* hashTable;
  //joinNetwork(ngnrp);
  /* Empty response, I.E no node in network */
  if (ntohs(ngnrp.port == 0) && ngnrp.address[0] == 0) {
    printf("No other nodes in the network, initializing hashtable.\n");
    hashTable = table_create(hash_ssn, 255);
  } else { /* Non empty response */
    printf("Net Join inc\n");
    /* Net join */
  }

  struct pollfd pollFds[MAXCLIENTS];
  pollFds[0].fd = STDIN_FILENO;
  pollFds[0].events = POLLIN;
  int currentClients = 1;
  bool loop = true;
  while (loop) {
    /*Ping tracker*/
    sendNetAlive(trackerSend, UDPSocket.port);

    int ret = poll(pollFds, currentClients, 7000);
    if(ret <= 0){
      fprintf(stderr, "Waited 7 seconds. No data received from a client.");
			fprintf(stderr, " Retrying...\n");
    }
    for (size_t i = 0; i < currentClients; i++){
      if (pollFds[i].revents & POLLIN && i == 0) {
        /*Reading from stdin*/
        char *buffer = calloc(BUFFERSIZE, sizeof(char));
				int readValue = read(pollFds[i].fd, buffer, BUFFERSIZE-1);
        if(strcmp("quit\n", buffer) == 0 || readValue <= 0){
          printf("Exiting loop\n");
          free(buffer);
          loop = false;
          break;
        }
        free(buffer);
      }
    }
  }




  //listen(UDPSocket, 10);

  //TODO: HASHTABLE PÅ NOD - NETJOIN sen


  shutdown(successorSock.socketFd, SHUT_RDWR);
  shutdown(predecessorSock.socketFd, SHUT_RDWR);
  shutdown(newNodeSock.socketFd, SHUT_RDWR);
  shutdown(UDPSocket.socketFd, SHUT_RDWR);
  shutdown(agentSock.socketFd, SHUT_RDWR);

  return 0;
}

void joinNetwork(struct NET_GET_NODE_RESPONSE_PDU ngnrp, struct socketData predSock, uint8_t *ip) {
  /*
  printf("Sending NET_JOIN to: %s, on port: %d\n", ngnrp.address, ntohs(ngnrp.port));
  struct NET_JOIN_PDU njp;
  njp.TYPE = NET_JOIN;
  njp.SRC_ADDRESS = ip;
  njp.PAD = 0;
  njp.SRC_PORT = htons(predSock.port);
  njp.MAX_SPAN = 0;
  njp.MAX_ADDRESS = 0;
  njp.MAX_PORT = htons(0);
*/



}

void sendNetAlive(struct socketData trackerSend, int port) {

  struct NET_ALIVE_PDU nap;
  nap.type = NET_ALIVE;
  nap.pad = 0;
  nap.port = htons(port);

  sendPDU(trackerSend, &nap);
}

struct NET_GET_NODE_RESPONSE_PDU getNodePDU(struct socketData trackerSend,
                                            struct socketData UDPSocket) {

  struct NET_GET_NODE_PDU ngnp;
  ngnp.type = NET_GET_NODE;
  ngnp.pad = 0;
  ngnp.port = htons(UDPSocket.port);

  sendPDU(trackerSend, &ngnp);

  uint8_t *buff = receivePDU(UDPSocket);
  struct NET_GET_NODE_RESPONSE_PDU ngnrp;


  if (buff[0] == NET_GET_NODE_RESPONSE) {
    memcpy(&ngnrp, buff, sizeof(struct NET_GET_NODE_RESPONSE_PDU));
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
void retrieveNodeIp(struct socketData trackerSend, struct socketData UDPSocket,
                                                            uint8_t **ip) {
  struct STUN_LOOKUP_PDU slp;
  slp.type = STUN_LOOKUP;
  slp.PAD = 0;
  slp.port = htons(UDPSocket.port);

  sendPDU(trackerSend, &slp);

  uint8_t *buff = receivePDU(UDPSocket);
  struct STUN_RESPONSE_PDU srp;
  if (buff[0] == STUN_RESPONSE) {
      memcpy(&srp, buff, sizeof(struct STUN_RESPONSE_PDU));
      *ip = srp.address;

  }
  free(buff);
}

uint8_t *receivePDU(struct socketData UDPSocket) {
  uint8_t *buffer = calloc(256, sizeof(uint8_t));
  socklen_t len = sizeof(UDPSocket.address);

  if (recvfrom(UDPSocket.socketFd, buffer, 256, MSG_WAITALL,
               (struct sockaddr*)&UDPSocket.address, &len) == -1) {
    perror("recvfrom");
  } else {
    return buffer;
  }
  return NULL;
}

void sendPDU(struct socketData trackerSend, void *pduSend) {

  sendto(trackerSend.socketFd, (uint8_t*)pduSend, sizeof(pduSend), 0,
         (struct sockaddr*)&trackerSend.address, sizeof(trackerSend.address));
}

/**
 * Creates a sockaddress to be used when talking with the (tracker). Struct
 * contains port and address to tracker cause obvious reasons
 *
 */
struct sockaddr_in getSocketAddress(int port, char *address) {

  struct sockaddr_in sockAddress;

  sockAddress.sin_family = AF_INET;
  sockAddress.sin_port = htons(port);
  sockAddress.sin_addr.s_addr = inet_addr(address);

  return sockAddress;
}



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

	printf("port = %d\n", *port);
	printf("address = %s\n", *address);
	printf("number of args = %d\n", argc);
}



struct socketData createSocket(int socketPort, int type) {

  struct socketData socketData;
  struct sockaddr_in inAddr;
  socketData.socketFd = socket(AF_INET, type, 0);

  if(socketData.socketFd < 0) {
      perror("Failed to create incoming socket");
      exit(-4);
  } else {
      inAddr.sin_family = AF_INET;
      inAddr.sin_port = htons(socketPort);
      inAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }

  if(bind(socketData.socketFd, (struct sockaddr*)&inAddr, sizeof(inAddr))) {
      perror("Failed to bind socket");
      exit(-5);
  }

  socketData.address = inAddr;

  /* Get socket port */
  socklen_t len = sizeof(inAddr);
  if (getsockname(socketData.socketFd, (struct sockaddr *)&inAddr, &len) == -1) {
    perror("getsockname");
  }
  else {
    socketData.port = ntohs(inAddr.sin_port);
  }

  printf("porten = %d\n", socketData.port);

  return socketData;
}
