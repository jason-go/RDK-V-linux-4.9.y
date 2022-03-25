/* crypto/sha/sha256.c */
/* ====================================================================
 * Copyright (c) 2004 The OpenSSL Project.  All rights reserved
 * according to the OpenSSL license [found in ../../LICENSE].
 * ====================================================================
 */

#include "kernelcalls.h"
# include "gx_soft_sha.h"

#define HASH_MAKE_STRING(c,s)   do {    \
        unsigned long ll;               \
        ll=(c)->h0; HOST_l2c(ll,(s));   \
        ll=(c)->h1; HOST_l2c(ll,(s));   \
        ll=(c)->h2; HOST_l2c(ll,(s));   \
        ll=(c)->h3; HOST_l2c(ll,(s));   \
        ll=(c)->h4; HOST_l2c(ll,(s));   \
        } while (0)

#define K_00_19 0x5a827999UL
#define K_20_39 0x6ed9eba1UL
#define K_40_59 0x8f1bbcdcUL
#define K_60_79 0xca62c1d6UL
/*
 * As pointed out by Wei Dai <weidai@eskimo.com>, F() below can be simplified
 * to the code in F_00_19.  Wei attributes these optimisations to Peter
 * Gutmann's SHS code, and he attributes it to Rich Schroeppel. #define
 * F(x,y,z) (((x) & (y)) | ((~(x)) & (z))) I've just become aware of another
 * tweak to be made, again from Wei Dai, in F_40_59, (x&a)|(y&a) -> (x|y)&a
 */
#define F_00_19(b,c,d)  ((((c) ^ (d)) & (b)) ^ (d))
#define F_20_39(b,c,d)  ((b) ^ (c) ^ (d))
#define F_40_59(b,c,d)  (((b) & (c)) | (((b)|(c)) & (d)))
#define F_60_79(b,c,d)  F_20_39(b,c,d)

#  define Xupdate(a,ix,ia,ib,ic,id)     ( (a)=(ia^ib^ic^id),    \
                                          ix=(a)=ROTATE((a),1)  \
                                        )
# define BODY_00_15(xi)           do {   \
        T=E+K_00_19+F_00_19(B,C,D);     \
        E=D, D=C, C=ROTATE(B,30), B=A;  \
        A=ROTATE(A,5)+T+xi;         } while(0)

# define BODY_16_19(xa,xb,xc,xd)  do {   \
        Xupdate(T,xa,xa,xb,xc,xd);      \
        T+=E+K_00_19+F_00_19(B,C,D);    \
        E=D, D=C, C=ROTATE(B,30), B=A;  \
        A=ROTATE(A,5)+T;            } while(0)

# define BODY_20_39(xa,xb,xc,xd)  do {   \
        Xupdate(T,xa,xa,xb,xc,xd);      \
        T+=E+K_20_39+F_20_39(B,C,D);    \
        E=D, D=C, C=ROTATE(B,30), B=A;  \
        A=ROTATE(A,5)+T;            } while(0)

# define BODY_40_59(xa,xb,xc,xd)  do {   \
        Xupdate(T,xa,xa,xb,xc,xd);      \
        T+=E+K_40_59+F_40_59(B,C,D);    \
        E=D, D=C, C=ROTATE(B,30), B=A;  \
        A=ROTATE(A,5)+T;            } while(0)

# define BODY_60_79(xa,xb,xc,xd)  do {   \
        Xupdate(T,xa,xa,xb,xc,xd);      \
        T=E+K_60_79+F_60_79(B,C,D);     \
        E=D, D=C, C=ROTATE(B,30), B=A;  \
        A=ROTATE(A,5)+T+xa;         } while(0)

static void sha1_block_data_order(SHA_CTX *c, const void *p,
                                    size_t num)
{
	const unsigned char *data = p;
	register unsigned int A, B, C, D, E, T, l;
	int i;
	SHA_LONG X[16];

	A = c->h0;
	B = c->h1;
	C = c->h2;
	D = c->h3;
	E = c->h4;

	for (;;) {
		for (i = 0; i < 16; i++) {
			HOST_c2l(data, l);
			X[i] = l;
			BODY_00_15(X[i]);
		}
		for (i = 0; i < 4; i++) {
			BODY_16_19(X[i], X[i + 2], X[i + 8], X[(i + 13) & 15]);
		}
		for (; i < 24; i++) {
			BODY_20_39(X[i & 15], X[(i + 2) & 15], X[(i + 8) & 15],
					X[(i + 13) & 15]);
		}
		for (i = 0; i < 20; i++) {
			BODY_40_59(X[(i + 8) & 15], X[(i + 10) & 15], X[i & 15],
					X[(i + 5) & 15]);
		}
		for (i = 4; i < 24; i++) {
			BODY_60_79(X[(i + 8) & 15], X[(i + 10) & 15], X[i & 15],
					X[(i + 5) & 15]);
		}

		c->h0 = (c->h0 + A) & 0xffffffffL;
		c->h1 = (c->h1 + B) & 0xffffffffL;
		c->h2 = (c->h2 + C) & 0xffffffffL;
		c->h3 = (c->h3 + D) & 0xffffffffL;
		c->h4 = (c->h4 + E) & 0xffffffffL;

		if (--num == 0)
			break;

		A = c->h0;
		B = c->h1;
		C = c->h2;
		D = c->h3;
		E = c->h4;
	}
}

#define INIT_DATA_h0 0x67452301UL
#define INIT_DATA_h1 0xefcdab89UL
#define INIT_DATA_h2 0x98badcfeUL
#define INIT_DATA_h3 0x10325476UL
#define INIT_DATA_h4 0xc3d2e1f0UL
static int SHA1_Init(SHA_CTX *c)
{
	memset(c, 0, sizeof(*c));
	c->h0 = INIT_DATA_h0;
	c->h1 = INIT_DATA_h1;
    c->h2 = INIT_DATA_h2;
    c->h3 = INIT_DATA_h3;
    c->h4 = INIT_DATA_h4;
    return 1;
}
static int SHA1_Update(SHA_CTX *c, const void *data_, size_t len)
{
    const unsigned char *data = data_;
    unsigned char *p;
    SHA_LONG l;
    size_t n;

    if (len == 0)
        return 1;

    l = (c->Nl + (((SHA_LONG) len) << 3)) & 0xffffffffUL;
    /*
     * 95-05-24 eay Fixed a bug with the overflow handling, thanks to Wei Dai
     * <weidai@eskimo.com> for pointing it out.
     */
    if (l < c->Nl)              /* overflow */
        c->Nh++;
    c->Nh += (SHA_LONG) (len >> 29); /* might cause compiler warning on
                                       * 16-bit */
    c->Nl = l;

    n = c->num;
    if (n != 0) {
        p = (unsigned char *)c->data;

        if (len >= SHA_CBLOCK || len + n >= SHA_CBLOCK) {
            memcpy(p + n, data, SHA_CBLOCK - n);
            sha1_block_data_order(c, p, 1);
            n = SHA_CBLOCK - n;
            data += n;
            len -= n;
            c->num = 0;
            memset(p, 0, SHA_CBLOCK); /* keep it zeroed */
        } else {
            memcpy(p + n, data, len);
            c->num += (unsigned int)len;
            return 1;
        }
    }

    n = len / SHA_CBLOCK;
    if (n > 0) {
        sha1_block_data_order(c, data, n);
        n *= SHA_CBLOCK;
        data += n;
        len -= n;
    }

    if (len != 0) {
        p = (unsigned char *)c->data;
        c->num = (unsigned int)len;
        memcpy(p, data, len);
    }
    return 1;
}

static int SHA1_Final(unsigned char *md, SHA_CTX *c)
{
    unsigned char *p = (unsigned char *)c->data;
    size_t n = c->num;

    p[n] = 0x80;                /* there is always room for one */
    n++;

    if (n > (SHA_CBLOCK - 8)) {
        memset(p + n, 0, SHA_CBLOCK - n);
        n = 0;
        sha1_block_data_order(c, p, 1);
    }
    memset(p + n, 0, SHA_CBLOCK - 8 - n);

    p += SHA_CBLOCK - 8;
    (void)HOST_l2c(c->Nh, p);
    (void)HOST_l2c(c->Nl, p);
    p -= SHA_CBLOCK;
    sha1_block_data_order(c, p, 1);
    c->num = 0;
    memset(p, 0, SHA_CBLOCK);

    HASH_MAKE_STRING(c, md);

    return 1;
}

int gx_soft_sha1(const unsigned char *d, size_t n, unsigned char *md)
{
    SHA_CTX c;
    static unsigned char m[SHA_DIGEST_LENGTH];

    if (md == NULL)
        md = m;
    SHA1_Init(&c);
    SHA1_Update(&c, d, n);
    SHA1_Final(md, &c);
    OPENSSL_cleanse(&c, sizeof(c));
    return 0;
}

