#include "gx_soft_sm3.h"
#include "kernelcalls.h"

void gx_soft_sm3_init(sm3_ctx_t *ctx)
{
	ctx->digest[0] = 0x7380166F;
	ctx->digest[1] = 0x4914B2B9;
	ctx->digest[2] = 0x172442D7;
	ctx->digest[3] = 0xDA8A0600;
	ctx->digest[4] = 0xA96F30BC;
	ctx->digest[5] = 0x163138AA;
	ctx->digest[6] = 0xE38DEE4D;
	ctx->digest[7] = 0xB0FB0E4E;

	ctx->nblocks = 0;
	ctx->num = 0;
}

void gx_soft_sm3_update(sm3_ctx_t *ctx, const unsigned char* data, unsigned int data_len)
{
	if (ctx->num) {
		unsigned int left = SM3_BLOCK_SIZE - ctx->num;
		if (data_len < left) {
			memcpy(ctx->block + ctx->num, data, data_len);
			ctx->num += data_len;
			return;
		} else {
			memcpy(ctx->block + ctx->num, data, left);
			gx_soft_sm3_compress(ctx->digest, ctx->block);
			ctx->nblocks++;
			data += left;
			data_len -= left;
		}
	}
	while (data_len >= SM3_BLOCK_SIZE) {
		gx_soft_sm3_compress(ctx->digest, data);
		ctx->nblocks++;
		data += SM3_BLOCK_SIZE;
		data_len -= SM3_BLOCK_SIZE;
	}
	ctx->num = data_len;
	if (data_len) {
		memcpy(ctx->block, data, data_len);
	}
}

void gx_soft_sm3_final(sm3_ctx_t *ctx, unsigned char *digest)
{
	int i;
	unsigned int pdigest;
	unsigned int count;

	ctx->block[ctx->num] = 0x80;

	if (ctx->num + 9 <= SM3_BLOCK_SIZE) {
		memset(ctx->block + ctx->num + 1, 0, SM3_BLOCK_SIZE - ctx->num - 9);
	} else {
		memset(ctx->block + ctx->num + 1, 0, SM3_BLOCK_SIZE - ctx->num - 1);
		gx_soft_sm3_compress(ctx->digest, ctx->block);
		memset(ctx->block, 0, SM3_BLOCK_SIZE - 8);
	}

	count = cpu_to_be32((ctx->nblocks) >> 23);
	memcpy(ctx->block + SM3_BLOCK_SIZE - 8, &count, 4);
	count = cpu_to_be32((ctx->nblocks << 9) + (ctx->num << 3));
	memcpy(ctx->block + SM3_BLOCK_SIZE - 4, &count, 4);

	gx_soft_sm3_compress(ctx->digest, ctx->block);
	for (i = 0; i < sizeof(ctx->digest)/sizeof(ctx->digest[0]); i++) {
		pdigest = cpu_to_be32(ctx->digest[i]);
		memcpy(digest+4*i, &pdigest, 4);
	}
}

#define ROTATELEFT(X,n)  (((X)<<(n)) | ((X)>>(32-(n))))

#define P0(x) ((x) ^  ROTATELEFT((x),9)  ^ ROTATELEFT((x),17))
#define P1(x) ((x) ^  ROTATELEFT((x),15) ^ ROTATELEFT((x),23))

#define FF0(x,y,z) ( (x) ^ (y) ^ (z))
#define FF1(x,y,z) (((x) & (y)) | ( (x) & (z)) | ( (y) & (z)))

#define GG0(x,y,z) ( (x) ^ (y) ^ (z))
#define GG1(x,y,z) (((x) & (y)) | ( (~(x)) & (z)) )

static unsigned int _rotateleft(unsigned int val, unsigned int bits)
{
	unsigned int real_bits = bits%32;
	if (real_bits == 0)
		return val;

	return ROTATELEFT(val, real_bits);
}

void gx_soft_sm3_compress(unsigned int digest[8], const unsigned char block[64])
{
	int j;
	unsigned int W[68], W1[64];
	unsigned int pblock;

	unsigned int A = digest[0];
	unsigned int B = digest[1];
	unsigned int C = digest[2];
	unsigned int D = digest[3];
	unsigned int E = digest[4];
	unsigned int F = digest[5];
	unsigned int G = digest[6];
	unsigned int H = digest[7];
	unsigned int SS1,SS2,TT1,TT2,T[64];

	for (j = 0; j < 16; j++) {
		memcpy(&pblock, block+4*j, 4);
		W[j] = cpu_to_be32(pblock);
	}
	for (j = 16; j < 68; j++) {
		W[j] = P1( W[j-16] ^ W[j-9] ^ _rotateleft(W[j-3],15)) ^ _rotateleft(W[j - 13],7 ) ^ W[j-6];;
	}
	for( j = 0; j < 64; j++) {
		W1[j] = W[j] ^ W[j+4];
	}

	for(j =0; j < 16; j++) {

		T[j] = 0x79CC4519;
		SS1 = _rotateleft((_rotateleft(A,12) + E + _rotateleft(T[j],j)), 7);
		SS2 = SS1 ^ _rotateleft(A,12);
		TT1 = FF0(A,B,C) + D + SS2 + W1[j];
		TT2 = GG0(E,F,G) + H + SS1 + W[j];
		D = C;
		C = _rotateleft(B,9);
		B = A;
		A = TT1;
		H = G;
		G = _rotateleft(F,19);
		F = E;
		E = P0(TT2);
	}

	for(j =16; j < 64; j++) {

		T[j] = 0x7A879D8A;
		SS1 = _rotateleft((_rotateleft(A,12) + E + _rotateleft(T[j],j)), 7);
		SS2 = SS1 ^ _rotateleft(A,12);
		TT1 = FF1(A,B,C) + D + SS2 + W1[j];
		TT2 = GG1(E,F,G) + H + SS1 + W[j];
		D = C;
		C = _rotateleft(B,9);
		B = A;
		A = TT1;
		H = G;
		G = _rotateleft(F,19);
		F = E;
		E = P0(TT2);
	}

	digest[0] ^= A;
	digest[1] ^= B;
	digest[2] ^= C;
	digest[3] ^= D;
	digest[4] ^= E;
	digest[5] ^= F;
	digest[6] ^= G;
	digest[7] ^= H;
}

void gx_soft_sm3(const unsigned char *msg, unsigned int msglen, unsigned char dgst[SM3_DIGEST_LENGTH])
{
	sm3_ctx_t ctx;

	gx_soft_sm3_init(&ctx);
	gx_soft_sm3_update(&ctx, msg, msglen);
	gx_soft_sm3_final(&ctx, dgst);

	memset(&ctx, 0, sizeof(sm3_ctx_t));
}
