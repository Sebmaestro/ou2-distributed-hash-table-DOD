#include "node.h"
#include "c_header/pdu.h"


struct socketData{
  int socketFd;
  int port;
};


int main(int argc, char **argv) {

  int port;
  char *address;
  handleArguments(argc, argv, &port, &address);


  struct socketData successorSock = createSocket(0, SOCK_STREAM);
  struct socketData predecessorSock = createSocket(0, SOCK_STREAM);
  struct socketData newNodeSock = createSocket(0, SOCK_STREAM);
  struct socketData trackerSock = createSocket(0, SOCK_DGRAM);
  struct socketData agentSock = createSocket(0, SOCK_DGRAM);

  shutdown(successorSock, SHUT_RDWR);
  shutdown(predecessorSock, SHUT_RDWR);
  shutdown(newNodeSock, SHUT_RDWR);
  shutdown(trackerSock, SHUT_RDWR);
  shutdown(agentSock, SHUT_RDWR);


  return 0;
}

void handleArguments(int argc, char **argv, int *port, char **address) {
	if (argc != 3) {
		fprintf(stderr, "No more than two arguments\n");
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



struct socketData createSocket(int port, int type) {

    struct socketData socketData;
	  struct sockaddr_in inAddr;
    socketData.socketFd = socket(AF_INET, type, 0);

    if(socketData.socketFd < 0) {
        perror("Failed to create incoming socket");
        exit(-4);
    } else {
        inAddr.sin_family = AF_INET;
        inAddr.sin_port = htons(port);
        inAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if(bind(socketData.socketFd, (struct sockaddr*)&inAddr, sizeof(inAddr))) {
        perror("Failed to bind socket");
        exit(-5);
    }

    /* Lös hur du får ut porten som socketen lyssnar på */
    //socketData.port = GETSOCKOPT

    return socketData;
}
