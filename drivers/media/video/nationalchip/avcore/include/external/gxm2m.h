#ifndef __GX_M2M_H__
#define __GX_M2M_H__

typedef enum {
	TFM_MOD_KLM_SCPU_STATIC,
	TFM_MOD_KLM_SCPU_DYNAMIC,
	TFM_MOD_KLM_SCPU_NC_DYNAMIC,
	TFM_MOD_KLM_IRDETO,
	TFM_MOD_KLM_GENERAL,
	TFM_MOD_KLM_MTC,

	TFM_MOD_CRYPTO,
	TFM_MOD_M2M,
	TFM_MOD_MISC,
	TFM_MOD_MAX,
} GxTfmModule;

typedef enum {
	/* KLM-related key */
	TFM_KEY_CWUK,       /* klm generate CW/GPCW */
	TFM_KEY_PVRK,       /* klm generate PVRK */
	TFM_KEY_TAUK,       /* klm update TA param */

	/* CRYPTO-related key */
	TFM_KEY_BCK,        /* acpu crypto decrypt loader */
	TFM_KEY_IK,         /* acpu crypto decrypt app */
	TFM_KEY_CCCK,       /* get chip config value */

	/* M2M-related key */
	TFM_KEY_RK,
	TFM_KEY_SSUK,
	TFM_KEY_MULTI,
	TFM_KEY_MEMORY,
	TFM_KEY_NDS,
	TFM_KEY_CDCAS,

	/* generic key from otp */
	TFM_KEY_OTP_SOFT,
	TFM_KEY_OTP_FIRM,
	TFM_KEY_OTP_FLASH,

	/* generic key from acpu/scpu */
	TFM_KEY_ACPU_SOFT,
	TFM_KEY_SCPU_SOFT,
	TFM_KEY_SCPU_KLM,
	TFM_KEY_SCPU_MISC,
	TFM_KEY_REG,

	/* scpu crypto/klm related rootkey */
	TFM_KEY_CSGK2,
	TFM_KEY_SIK,
	TFM_KEY_ROOTKEY,
	TFM_KEY_SMK,
	TFM_KEY_TK,
} GxTfmKey;

typedef enum {
	TFM_ALG_AES128,
	TFM_ALG_AES192,
	TFM_ALG_AES256,
	TFM_ALG_DES,
	TFM_ALG_TDES,
	TFM_ALG_T3DES,
	TFM_ALG_TAES,
	TFM_ALG_SM4,
	TFM_ALG_SM2,
	TFM_ALG_SM3,
	TFM_ALG_SHA1,
	TFM_ALG_SHA256,
	TFM_ALG_SM2_ZA,
	TFM_ALG_SM3_HMAC,
} GxTfmAlg;

typedef enum {
	TFM_SRC_MEM,
	TFM_SRC_REG,
} GxTfmSrc;

typedef enum {
	TFM_DST_TS,
	TFM_DST_GP,
	TFM_DST_M2M,
	TFM_DST_MEM,
	TFM_DST_REG,
} GxTfmDst;

typedef enum {
	TFM_OPT_ECB,
	TFM_OPT_CBC,
	TFM_OPT_CFB,
	TFM_OPT_OFB,
	TFM_OPT_CTR,
} GxTfmOpt;

typedef struct {
	unsigned char *buf;
	unsigned int   length;
} GxTfmBuf;

typedef struct {
	GxTfmSrc       id;
	unsigned char  sub;
} GxTfmSrcBuf;

typedef struct {
	GxTfmDst       id;
	unsigned char  sub;
} GxTfmDstBuf;

typedef struct {
	GxTfmKey       id;
	unsigned char  sub;
} GxTfmKeyBuf;

typedef enum {
	TFM_FLAG_BIG_ENDIAN           = (1 << 0),

	TFM_FLAG_KLM_CW_XOR           = (1 << 1),
	TFM_FLAG_KLM_CW_HALF          = (1 << 2),
	TFM_FLAG_KLM_CW_HIGH_8BYTE    = (1 << 3),
	TFM_FLAG_KLM_INPUT_HALF       = (1 << 4),

	TFM_FLAG_CRYPT_DES_IV_SHIFT   = (1 << 5),
	TFM_FLAG_CRYPT_EVEN_KEY_VALID = (1 << 6),
	TFM_FLAG_CRYPT_ODD_KEY_VALID  = (1 << 7),
	TFM_FLAG_CRYPT_TS_PACKET_MODE = (1 << 8),
	TFM_FLAG_CRYPT_SWITCH_CLR     = (1 << 9),
	TFM_FLAG_CRYPT_SHORT_MSG_IV1  = (1 << 10),
	TFM_FLAG_CRYPT_SHORT_MSG_IV2  = (1 << 11),
	TFM_FLAG_CRYPT_RESIDUAL_CTS   = (1 << 12),
	TFM_FLAG_CRYPT_RESIDUAL_RBT   = (1 << 13),
	TFM_FLAG_CRYPT_SET_AUDIO_PVRK = (1 << 14),
	TFM_FLAG_CRYPT_PVRK_FROM_ACPU = (1 << 15),
	TFM_FLAG_CRYPT_HW_PVR_MODE    = (1 << 16),

	TFM_FLAG_HASH_DMA_EN          = (1 << 20),
	TFM_FLAG_PARAM_CONFIG         = (1 << 21),
} GxTfmFlag;

typedef struct {
	GxTfmModule    module;
	unsigned int   module_sub;
	GxTfmAlg       alg;
	GxTfmOpt       opt;
	GxTfmSrcBuf    src;
	GxTfmDstBuf    dst;
	GxTfmKeyBuf    even_key;
	GxTfmKeyBuf    odd_key;
	GxTfmBuf       input;
	GxTfmBuf       output;
	unsigned int   input_paddr;
	unsigned int   output_paddr;
	unsigned int   key_switch_gate;
	unsigned char  soft_key[32];
	unsigned int   soft_key_len;
	unsigned char  iv[16];
	unsigned int   iv_id;
	GxTfmBuf       sm2_pub_key;

	int            ret;
	unsigned int   flags;
} GxTfmCrypto;

int gx_tfm_encrypt(GxTfmCrypto *param);
int gx_tfm_decrypt(GxTfmCrypto *param);

#endif
