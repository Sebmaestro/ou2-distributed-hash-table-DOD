#include "hash.h"

void evp(char* msg, unsigned char* digest, uint32_t len) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_create();
    EVP_MD_CTX_init(ctx);
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(ctx, msg, len);
    EVP_DigestFinal_ex(ctx, digest, NULL);
    EVP_MD_CTX_free(ctx);
}

hash_t sha1_digest_to_uint8(char* digest) {
    uint16_t result = 0;
    int i = 20;
    while(i--) {
        result = (result + (uint8_t)*digest++ ) % 256;
    }
    return (hash_t) result;
}

hash_t hash_ssn(char* ssn) {
    char digest[20];
    evp(ssn, (unsigned char*)digest, 12);
    return sha1_digest_to_uint8(digest);
}
