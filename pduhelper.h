#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include "c_header/pdu.h"

#ifndef _PDUHELPER_
#define _PDUHELPER_

uint8_t getPDUSize(uint8_t type);

void copyValueToPDU(uint8_t **dest, char *toCopy, int len);

struct VAL_INSERT_PDU extractPDU(uint8_t *buffer, int *bufferSize);

#endif
