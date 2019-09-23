#include "node.h"
#include "c_header/pdu.h"

#define MAXCLIENTS 6

struct socketData{
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
  struct socketData trackerSock = createSocket(0, SOCK_DGRAM);
  struct socketData agentSock = createSocket(0, SOCK_DGRAM);

  uint8_t *nodeIp;
  retrieveNodeIp(trackerPort, address, trackerSock, &nodeIp);
  printf("Ip: %s\n", nodeIp);
  //free(ip);

  //struct pollfd fds[MAXCLIENTS];

  //listen(trackerSock, 10);

  // while (1 < 2) {f
  //
  // }



  shutdown(successorSock.socketFd, SHUT_RDWR);
  shutdown(predecessorSock.socketFd, SHUT_RDWR);
  shutdown(newNodeSock.socketFd, SHUT_RDWR);
  shutdown(trackerSock.socketFd, SHUT_RDWR);
  shutdown(agentSock.socketFd, SHUT_RDWR);

  return 0;
}

/**
 * Retrieves ip for node with STUN request and stores it in the ip-pointer.
 */
void retrieveNodeIp(int trackerPort, char *address, struct socketData sd,
                                                            uint8_t **ip) {
  struct STUN_LOOKUP_PDU slp;
  slp.type = STUN_LOOKUP;
  slp.PAD = 0;
  slp.port = htons(sd.port);

  struct sockaddr_in sockAddress;


  sockAddress.sin_family = AF_INET;
  sockAddress.sin_port = htons(trackerPort);
  sockAddress.sin_addr.s_addr = inet_addr(address);

  sendto(sd.socketFd, (uint8_t*)&slp, sizeof(slp), 0,
        (struct sockaddr*)&sockAddress, sizeof(sockAddress));

  uint8_t *buffer = calloc(256, sizeof(uint8_t));
  socklen_t len = sizeof(sd.address);

  struct STUN_RESPONSE_PDU srp;


  if (recvfrom(sd.socketFd, buffer, 256, MSG_WAITALL,
              (struct sockaddr*)&sd.address, &len) == -1) {

    perror("recvfrom");
  } else {
    if (buffer[0] == STUN_RESPONSE) {
      memcpy(&srp, buffer, sizeof(struct STUN_RESPONSE_PDU));
      *ip = srp.address;
    }
  }
}

void handleArguments(int argc, char **argv, int *trackerPort, char **address) {
	if (argc != 3) {
    fprintf(stderr, "Wrong amount of arguments - ");
		fprintf(stderr, "Usage: ./node <tracker address> <tracker port>\n");
		exit(1);
	}

	*address = argv[1];

	*trackerPort = strtol(argv[2], NULL, 10);
  if (*trackerPort == 0) {

		fprintf(stderr, "Failed to convert port string to int\n");
		exit(1);
	}

	printf("port = %d\n", *trackerPort);
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
