#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>

#ifndef OPENSSL_NO_SHA1
int calc_sha1(const unsigned char *d, size_t n, unsigned char *md)
{
    SHA_CTX c;

    if (md == NULL)
		return -1;
    if (!SHA1_Init(&c))
        return -2;
    SHA1_Update(&c, d, n);
    SHA1_Final(md, &c);
    OPENSSL_cleanse(&c, sizeof(c));
    return 0;
}
#endif
