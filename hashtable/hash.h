#include <string.h>
#include <openssl/evp.h>
typedef uint8_t hash_t;

/**
 * Computs a hash_t long hash of the given
 * 12-byte SSN.
 * @param ssn The ssn to hash
 * @returns The hash_t representation of the SSN
 */
hash_t hash_ssn(char* ssn);
