#include "node.h"
#include "c_header/pdu.h"

#define MAXCLIENTS 6

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
  struct socketData trackerReceive = createSocket(0, SOCK_DGRAM);
  struct socketData agentSock = createSocket(0, SOCK_DGRAM);

  struct socketData trackerSend;
  trackerSend.socketFd = trackerReceive.socketFd;
  trackerSend.port = trackerPort;
  trackerSend.address = getSocketAddress(trackerSend.port, address);





  /* This is the node ip we get when asking the tracker for it :^) */
  uint8_t *nodeIp;
  retrieveNodeIp(trackerSend, trackerReceive, &nodeIp);
  printf("Node IP: %s\n", nodeIp);
  getNodePDU(trackerSend, trackerReceive);

  //struct pollfd fds[MAXCLIENTS];

  //listen(trackerReceive, 10);

  //TODO: NETALIVE OCH INITIERA HASHTABLE, SEDAN IMPLEMENTERA NET JOIN


  shutdown(successorSock.socketFd, SHUT_RDWR);
  shutdown(predecessorSock.socketFd, SHUT_RDWR);
  shutdown(newNodeSock.socketFd, SHUT_RDWR);
  shutdown(trackerReceive.socketFd, SHUT_RDWR);
  shutdown(agentSock.socketFd, SHUT_RDWR);

  return 0;
}

void sendNetAlive(int trackerPort, char *address) {

  printf("system.exit(-5000)\ndö");
}

uint8_t *receivePDU(struct socketData trackerReceive) {
  uint8_t *buffer = calloc(256, sizeof(uint8_t));
  socklen_t len = sizeof(trackerReceive.address);

  if (recvfrom(trackerReceive.socketFd, buffer, 256, MSG_WAITALL,
               (struct sockaddr*)&trackerReceive.address, &len) == -1) {
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

struct NET_GET_NODE_RESPONSE_PDU getNodePDU(struct socketData trackerSend,
                                            struct socketData trackerReceive) {

  struct NET_GET_NODE_PDU ngnp;
  ngnp.type = NET_GET_NODE;
  ngnp.pad = 0;
  ngnp.port = htons(trackerReceive.port);

  sendPDU(trackerSend, &ngnp);

  uint8_t *buff = receivePDU(trackerReceive);
  struct NET_GET_NODE_RESPONSE_PDU ngnrp;

  if (buff[0] == NET_GET_NODE_RESPONSE) {
      memcpy(&ngnrp, buff, sizeof(struct NET_GET_NODE_RESPONSE_PDU));
      printf("pdu :%d\n", ngnrp.type);
      printf("adde = %s\n", ngnrp.address);
      printf("porra = %d\n", ntohs(ngnrp.port));
  }
  free(buff);
  return ngnrp;
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

/**
 * Retrieves ip for node with STUN request and stores it in the ip-pointer.
 */
void retrieveNodeIp(struct socketData trackerSend, struct socketData trackerReceive,
                                                            uint8_t **ip) {
  struct STUN_LOOKUP_PDU slp;
  slp.type = STUN_LOOKUP;
  slp.PAD = 0;
  slp.port = htons(trackerReceive.port);

  sendPDU(trackerSend, &slp);

  uint8_t *buff = receivePDU(trackerReceive);
  struct STUN_RESPONSE_PDU srp;
  if (buff[0] == STUN_RESPONSE) {
      memcpy(&srp, buff, sizeof(struct STUN_RESPONSE_PDU));
      *ip = srp.address;

  }
  free(buff);
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
