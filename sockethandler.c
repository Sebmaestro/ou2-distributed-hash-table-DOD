#include "sockethandler.h"

/**
 *
 *
 *
 *
 */
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