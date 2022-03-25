#ifndef __DES_API__
#define __DES_API__

#ifdef  __cplusplus
extern "C" {
#endif

#define DES_LONG unsigned long
#define des_cblock DES_cblock
#define des_key_schedule DES_key_schedule
#define des_ede3_cbc_encrypt(i,o,l,k1,k2,k3,iv,e)\
		DES_ede3_cbc_encrypt((i),(o),(l),&(k1),&(k2),&(k3),(iv),(e))

typedef unsigned char DES_cblock[8];
typedef /* const */ unsigned char const_DES_cblock[8];

typedef struct DES_ks {
	union {
		DES_cblock cblock;
		DES_LONG deslong[2];
	} ks[16];
} DES_key_schedule;

// enc 
# define DES_ENCRYPT     1
# define DES_DECRYPT     0

int DES_set_key(const_DES_cblock *key, DES_key_schedule *schedule);
// Des
void DES_cbc_encrypt(const unsigned char *input, unsigned char *output,
                     long length, DES_key_schedule *schedule,
                     DES_cblock *ivec, int enc);
void DES_cfb_encrypt(const unsigned char *in, unsigned char *out, int numbits,
                     long length, DES_key_schedule *schedule,
                     DES_cblock *ivec, int enc);
void DES_ecb_encrypt(const_DES_cblock *input, DES_cblock *output,
                     DES_key_schedule *ks, int enc);

// 3Des 
void DES_ecb3_encrypt(const_DES_cblock *input, DES_cblock *output,
                      DES_key_schedule *ks1, DES_key_schedule *ks2,
                      DES_key_schedule *ks3, int enc);

void DES_ede3_cbc_encrypt(const unsigned char *input, unsigned char *output,
                          long length,
                          DES_key_schedule *ks1, DES_key_schedule *ks2,
                          DES_key_schedule *ks3, DES_cblock *ivec, int enc);
void DES_ede3_cbcm_encrypt(const unsigned char *in, unsigned char *out,
                           long length,
                           DES_key_schedule *ks1, DES_key_schedule *ks2,
                           DES_key_schedule *ks3,
                           DES_cblock *ivec1, DES_cblock *ivec2, int enc);
void DES_ede3_cfb_encrypt(const unsigned char *in, unsigned char *out,
                          int numbits, long length, DES_key_schedule *ks1,
                          DES_key_schedule *ks2, DES_key_schedule *ks3,
                          DES_cblock *ivec, int enc);

#ifdef  __cplusplus
}
#endif

#endif
