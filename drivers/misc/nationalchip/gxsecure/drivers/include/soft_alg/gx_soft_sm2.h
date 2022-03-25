#ifndef __SOFT_SM2_H__
#define __SOFT_SM2_H__

#ifndef DEFINED_U8
#define DEFINED_U8
typedef unsigned char  U8;
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
typedef unsigned short U16;
#endif

#ifndef DEFINED_U32
#define DEFINED_U32
typedef unsigned int U32;
#endif

#ifndef DEFINED_U64
#define DEFINED_U64
typedef unsigned long long U64;
#endif

/* Common signed types */
#ifndef DEFINED_S8
#define DEFINED_S8
typedef signed char  S8;
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
typedef signed short S16;
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
typedef signed int   S32;
#endif

#ifndef DEFINED_S64
#define DEFINED_S64
typedef signed long S64;
#endif

#define SM2_LEN 8 //Length, word unit, 8 means 256 bits
#define SMAX (SM2_LEN*2 + 2)

void xor(unsigned char *dA, unsigned char *dB, unsigned int dlen, unsigned char *dataout);
void gx_soft_sm2_signature(U32 p[SMAX], U32 a[SMAX], U32 b[SMAX], U32 n[SMAX], U32 xG[SMAX], U32 yG[SMAX], U32 k[SMAX], U32 d[SMAX], U32 e[SMAX], U32 *r, U32 *s);
int  gx_soft_sm2_verify(U32 p[SMAX], U32 a[SMAX], U32 b[SMAX], U32 n[SMAX], U32 xG[SMAX], U32 yG[SMAX], U32 xP[SMAX], U32 yP[SMAX], U32 e[SMAX], U32 r[SMAX], U32 s[SMAX]);
void gx_soft_sm2_encrypt(U32 p[SMAX], U32 a[SMAX], U32 b[SMAX], U32 k[SMAX], U32 xG[SMAX], U32 yG[SMAX], U32 xP[SMAX], U32 yP[SMAX], unsigned char *m, unsigned int mlen, unsigned char *cipher_text);
int  gx_soft_sm2_decrypt(U32 p[SMAX], U32 a[SMAX], U32 b[SMAX], U32 d[SMAX],unsigned int mlen,  unsigned char *cipher_text, unsigned char *plain_text);

#endif
