#ifndef __SOFT_SM3_H__
#define __SOFT_SM3_H__

#define SM3_DIGEST_LENGTH	32
#define SM3_BLOCK_SIZE		64
#define SM3_CBLOCK		(SM3_BLOCK_SIZE)

//#include <string.h>

typedef struct {
	unsigned int digest[8];
	int nblocks;
	unsigned char block[64];
	int num;
} sm3_ctx_t;

void gx_soft_sm3_init(sm3_ctx_t *ctx);
void gx_soft_sm3_update(sm3_ctx_t *ctx, const unsigned char* data, unsigned int data_len);
void gx_soft_sm3_final(sm3_ctx_t *ctx, unsigned char digest[SM3_DIGEST_LENGTH]);
void gx_soft_sm3_compress(unsigned int digest[8], const unsigned char block[SM3_BLOCK_SIZE]);
void gx_soft_sm3(const unsigned char *data, unsigned int datalen, unsigned char digest[SM3_DIGEST_LENGTH]);

#endif
