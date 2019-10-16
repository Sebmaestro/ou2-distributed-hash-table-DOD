#include "pduhelper.h"

/*
 * function: getPDUSize
 *
 * description: returns size of different pdu's
 *
 * input: pdu type
 *
 * output: size of pdu
 *
 */
uint8_t getPDUSize(uint8_t type){
  switch (type) {
    case NET_GET_NODE_RESPONSE:
      return sizeof(struct NET_GET_NODE_RESPONSE_PDU);
    case NET_ALIVE:
      return sizeof(struct NET_ALIVE_PDU);
    case NET_CLOSE_CONNECTION:
      return sizeof(struct NET_CLOSE_CONNECTION_PDU);
    case NET_GET_NODE:
      return sizeof(struct NET_GET_NODE_PDU);
    case NET_JOIN:
      return sizeof(struct NET_JOIN_PDU);
    case NET_JOIN_RESPONSE:
      return sizeof(struct NET_JOIN_RESPONSE_PDU);
    case NET_NEW_RANGE:
      return sizeof(struct NET_NEW_RANGE_PDU);
    case NET_LEAVING:
      return sizeof(struct NET_LEAVING_PDU);
    case VAL_INSERT:
      return sizeof(struct VAL_INSERT_PDU);
    case VAL_REMOVE:
      return sizeof(struct VAL_REMOVE_PDU);
    case VAL_LOOKUP:
      return sizeof(struct VAL_LOOKUP_PDU);

  }
  return -1;
}


/*
 * function: copyValueToPDU
 *
 * description: allocates memory to a pdu pointer and copy stuff to it
 *
 * input: pdu component, src string, lenght
 *
 */
 void copyValueToPDU(uint8_t **dest, char *toCopy, int len) {
   *dest = malloc(len * sizeof(char));
   strcpy((char*)*dest, toCopy);
 }


 /* Function: extractPDU
  * Description: Formats a VAL_INSERT_PDU from a buffer.
  * Input: the buffer, size of buffer
  * Output: Formatted VAL_INSERT_PDU
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

   return vip;
 }
