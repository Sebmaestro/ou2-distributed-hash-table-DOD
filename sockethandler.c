#include "sockethandler.h"

/* Function: receivePDU
 * Description: Receives a message from a UDP-socket and stores it
 *              in a buffer.
 * Input: UDP-socket
 * Output: buffer containing message.
 */
uint8_t *receivePDU(int socket) {
  uint8_t *buffer = calloc(256, sizeof(uint8_t));
  struct sockaddr_in sender;
  socklen_t len = sizeof(sender);



  if (recvfrom(socket, buffer, 256, MSG_WAITALL,
               (struct sockaddr*)&sender, &len) == -1) {
    perror("recvfrom");
  } else {
    printf("Received message from %s\n", inet_ntoa(sender.sin_addr));
    return buffer;
  }
  return NULL;
}

/* Function: sendPDU
 * Description: sends a PDU through a UDP-socket to the given address.
 * Input: UDP-socket, destination-address, PDU, size of PDU.
 */
void sendPDU(int socket, struct sockaddr_in address, void *pduSend, int size) {
  int sent = sendto(socket, (uint8_t*)pduSend, size, 0,
         (struct sockaddr*)&address, sizeof(address));
  if(sent < 0) {
    perror("sendto");
  }
}

/* Function: createSocket
 * Description: creates a socket with the given type and port.
 * Input: port, UDP or TCP socket-type.
 * Output: socketData struct
 */
struct socketData createSocket(int socketPort, int type) {

  struct socketData socketData;
  struct sockaddr_in inAddr;
  socketData.socketFd = socket(AF_INET, type, 0);

  int enable = 1;
  setsockopt(socketData.socketFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  setsockopt(socketData.socketFd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int));

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

  return socketData;
}

/* Function: getSocketAddress
 * Description: forms a sockaddr_in struct from a portnumber and
 *              an address in string-form.
 * Input: port, ip-address
 * Output: sockaddr_in struct
 */
struct sockaddr_in getSocketAddress(int port, char *address) {

  struct sockaddr_in sockAddress;

  sockAddress.sin_family = AF_INET;
  sockAddress.sin_port = htons(port);
  sockAddress.sin_addr.s_addr = inet_addr(address);

  return sockAddress;
}

/* Function: connectToSocket
 * Description: connects a socket to the given ip-address and port.
 * Input: port, ip-address, socket to connect
 * Output: successful or failed connection.
 */
int connectToSocket(int port, char *address, int socket) {
  struct sockaddr_in sockAdr = getSocketAddress(ntohs(port), address);
  printf("Trying to connect to %s on port: %d\n", inet_ntoa(sockAdr.sin_addr), htons(sockAdr.sin_port));
  return connect(socket, (struct sockaddr*)&sockAdr, sizeof(sockAdr));
}

/* Function: readTCPMessage
 * Description: reads a message of expected size received on a TCP-socket.
 * Input: TCP-socket, size of expected message, type of message
 * Output: Buffer containing message. 
 */
uint8_t *readTCPMessage(int socket, uint8_t expectedSize, uint8_t type) {
  /* The type is already read */
  uint8_t nameLen = 0;
  uint8_t emailLen = 0;
  int readBytes = 1;
  uint8_t *buf = calloc(256, sizeof(uint8_t));
  buf[0] = type;
  if(type == VAL_INSERT){
    /* Because VAL_INSERT contains pointers we remove 16 bytes */
    expectedSize = expectedSize - 16;
  }
  while (readBytes != expectedSize) {


    int r = read(socket, buf+readBytes, expectedSize-readBytes);
    if(r != -1){
      readBytes = readBytes + r;
      /* Increase expectedSize if there's an name to read. */
      if(type == VAL_INSERT && readBytes > 14 && nameLen == 0){
        nameLen = buf[14];
        expectedSize = expectedSize + nameLen;
      }
      /* Increase expectedSize if there's an email to read */
      if(type == VAL_INSERT && readBytes > 14+2+nameLen  && emailLen == 0){
        emailLen = buf[14+2+nameLen];
        expectedSize = expectedSize + emailLen;
      }
    } else {
      perror("read");
      return NULL;
    }
  }
  return buf;
}
