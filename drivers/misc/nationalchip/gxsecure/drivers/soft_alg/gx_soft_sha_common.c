#include "kernelcalls.h"
#include "gx_soft_sha.h"
#include "gx_soft_sm2.h"

unsigned char cleanse_ctr = 0;
void OPENSSL_cleanse(void *ptr, size_t len)
{
    unsigned char *p = ptr;
    size_t loop = len, ctr = cleanse_ctr;
    while (loop--) {
        *(p++) = (unsigned char)ctr;
        ctr += (17 + ((size_t)p & 0xF));
    }
    p = memchr(ptr, (unsigned char)ctr, len);
    if (p)
        ctr += (63 + (size_t)p);
    cleanse_ctr = (unsigned char)ctr;
}

void xor(unsigned char *dA, unsigned char *dB, unsigned int dlen, unsigned char *dataout)
{
	unsigned int i;

	for(i=0; i<dlen; i++)
	{
		dataout[i] = dA[i] ^ dB[i];
	}
}

