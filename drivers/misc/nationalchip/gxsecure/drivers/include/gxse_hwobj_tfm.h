#ifndef __GXSE_HWOBJ_TFM_H__
#define __GXSE_HWOBJ_TFM_H__

#include "gxtfm.h"
#include "gxtfm_uapi.h"

#define TFM_FLAG_GET(flag, mask) (flag & mask ? 1 : 0)
#define TFM_FLAG_CRYPT_ENCRYPT   (1<<31)
#define TFM_FLAG_IS_SRC          (1<<31)
#define TFM_FLAG_KLM_CW_READY    (1<<30)
#define TFM_FLAG_ISR_QUERY_UNTIL (1<<25)

#define TFM_MAX_CIPHER_UNIT      (0x2800)
#define TFM_MAX_DGST_UNIT        (0x400)
#define TFM_FORCE_ALIGN          (1)
#define TFM_UNFORCE_ALIGN        (2)

#define TFM_ISR_INTERRUPT        (0)
#define TFM_ISR_QUERY_TIMEOUT    (1)
#define TFM_ISR_QUERY_UNTIL      (2)

#define TFM_MOD_MISC_DGST        (0x90)
#define TFM_MOD_MISC_AKCIPHER    (0x91)

#define TFM_MASK_DONE         (0x1<<0)

#define tfm_akcipher tfm_u.akcipher_ops
#define tfm_cipher   tfm_u.cipher_ops
#define tfm_klm      tfm_u.klm_ops
#define tfm_dgst     tfm_u.dgst_ops

#define TFM_CRYPT_KMALLOC GXSE_IOR('t', 14, unsigned int)

/*
 * The parameters passed to scpu by mailbox must not contain pointers.
 */
typedef struct {
	GxTfmModule    module;
	GxTfmAlg       alg;
	GxTfmSrcBuf    src;
	GxTfmDstBuf    dst;
	GxTfmKeyBuf    key;
	unsigned int   input[4];
	unsigned int   output[4];
	unsigned int   reserve[4];
	unsigned int   nonce[4];
	unsigned int   stage;

	int            ret;
	unsigned int   flags;
} GxTfmScpuKlm;

#ifdef CPU_ACPU
	typedef GxTfmKlm     GxTfmKeyLadder;
	#define TFM_GET_KLM_BUF(_buf)    ((_buf).buf)
	#define TFM_GET_KLM_BUFLEN(_buf) ((_buf).length)
#else
	typedef GxTfmScpuKlm GxTfmKeyLadder;
	#define TFM_GET_KLM_BUF(_buf)    ((void *)(_buf))
	#define TFM_GET_KLM_BUFLEN(_buf) (sizeof((_buf)))
	#ifdef TFM_KLM_SELECT_ROOTKEY
		#undef TFM_KLM_SELECT_ROOTKEY
	    #define TFM_KLM_SELECT_ROOTKEY   GXSE_IOW ('t', 0, GxTfmScpuKlm)
	#endif
	#ifdef TFM_KLM_SET_KN
		#undef TFM_KLM_SET_KN
		#define TFM_KLM_SET_KN           GXSE_IOW ('t', 1, GxTfmScpuKlm)
	#endif
	#ifdef TFM_KLM_SET_CW
		#undef TFM_KLM_SET_CW
		#define TFM_KLM_SET_CW           GXSE_IOW ('t', 2, GxTfmScpuKlm)
	#endif
	#ifdef TFM_KLM_UPDATE_TD_PARAM
		#undef TFM_KLM_UPDATE_TD_PARAM
		#define TFM_KLM_UPDATE_TD_PARAM  GXSE_IOW ('t', 3, GxTfmScpuKlm)
	#endif
	#ifdef TFM_KLM_GET_RESP
		#undef TFM_KLM_GET_RESP
		#define TFM_KLM_GET_RESP         GXSE_IOWR('t', 4, GxTfmScpuKlm)
	#endif
#endif

typedef struct {
	int32_t (*capability)      (GxSeModuleHwObj *obj, GxTfmCap *param);
	int32_t (*select_rootkey)  (GxSeModuleHwObj *obj, GxTfmKeyLadder *param);
	int32_t (*set_kn)          (GxSeModuleHwObj *obj, GxTfmKeyLadder *param, uint32_t pos);
	int32_t (*set_cw)          (GxSeModuleHwObj *obj, GxTfmKeyLadder *param);
	int32_t (*update_TD_param) (GxSeModuleHwObj *obj, GxTfmKeyLadder *param);
	int32_t (*get_resp)        (GxSeModuleHwObj *obj, GxTfmKeyLadder *param);
} GxTfmKLMOps;

typedef struct {
	int32_t (*capability)      (GxSeModuleHwObj *obj, GxTfmCap *param);
	int32_t (*encrypt)         (GxSeModuleHwObj *obj, GxTfmCrypto *param);
	int32_t (*decrypt)         (GxSeModuleHwObj *obj, GxTfmCrypto *param);
	int32_t (*set_pvr_key)     (GxSeModuleHwObj *obj, GxTfmPVRKey *param);
	int32_t (*clr_key)         (GxSeModuleHwObj *obj);
	int32_t (*enable_key)      (GxSeModuleHwObj *obj);
	int32_t (*disable_key)     (GxSeModuleHwObj *obj);
	int32_t (*get_align_buf)   (GxSeModuleHwObj *obj, uint32_t *buf, uint32_t *size);
} GxTfmCipherOps;

typedef struct {
	int32_t (*capability)      (GxSeModuleHwObj *obj, GxTfmCap *param);
	int32_t (*dgst)            (GxSeModuleHwObj *obj, GxTfmDgst *param);
	int32_t (*dgst_hmac)       (GxSeModuleHwObj *obj, GxTfmDgst *param);
} GxTfmDgstOps;

typedef struct {
	int32_t (*capability)      (GxSeModuleHwObj *obj, GxTfmCap *param);
	int32_t (*encrypt)         (GxSeModuleHwObj *obj, GxTfmCrypto *param);
	int32_t (*decrypt)         (GxSeModuleHwObj *obj, GxTfmCrypto *param);
	int32_t (*cal_za)          (GxSeModuleHwObj *obj, GxTfmDgst *param);
	int32_t (*verify)          (GxSeModuleHwObj *obj, GxTfmVerify *param);
} GxTfmAKCipherOps;

typedef struct {
	void *devops;
	void *hwops;

	union {
		GxTfmKLMOps      klm_ops;
		GxTfmDgstOps     dgst_ops;
		GxTfmCipherOps   cipher_ops;
		GxTfmAKCipherOps akcipher_ops;
	} tfm_u;
} GxSeModuleHwObjTfmOps;

int32_t gx_tfm_capability(GxTfmCap *param);
int32_t gx_tfm_align_buf(GxTfmBuf *param, GxTfmBuf *align_buf, uint32_t *paddr, uint32_t align, uint32_t flags);
int32_t gx_tfm_restore_buf(GxTfmBuf *param, GxTfmBuf *old_buf, uint32_t flags);
int32_t gx_tfm_set_pvr_key(GxTfmPVRKey *param);
int32_t gx_tfm_enable_key(GxTfmModule module, uint32_t sub);
int32_t gx_tfm_disable_key(GxTfmModule module, uint32_t sub);
int32_t gx_tfm_clear_key(GxTfmModule module, uint32_t sub);

int gx_tfm_klm_select_rootkey ( GxTfmKeyLadder *param );
int gx_tfm_klm_set_kn         ( GxTfmKeyLadder *param );
int gx_tfm_klm_set_cw         ( GxTfmKeyLadder *param );
int gx_tfm_klm_update_TD_param( GxTfmKeyLadder *param );
int gx_tfm_klm_get_resp       ( GxTfmKeyLadder *param );

int gx_sm2_verify (
	unsigned char *pub_key, unsigned int pub_key_len,
	unsigned char *hash, unsigned int hash_len,
	unsigned char *sig, unsigned int sig_len);

int gx_sm2_cal_za (
	unsigned char *pub_key, unsigned int pub_key_len,
	unsigned char *ida, unsigned int ida_len,
	unsigned char *za, int flags);

int gx_sm2_encrypt (
	unsigned char *pub_key, unsigned int pub_key_len,
	unsigned char *inputData, unsigned int inputData_size,
	unsigned char *outputData);

int gx_sm2_decrypt (
	unsigned char *inputData, unsigned int inputData_size,
	unsigned char *outputData);

int gx_ecdsa_verify (
	int curve_id,
	unsigned char *pub_key, unsigned int pub_key_len,
	unsigned char *hash, unsigned int hash_len,
	unsigned char *sig, unsigned int sig_len);

int gx_sm3_perform (
	unsigned char* dataIn, unsigned int dataInLen,
	unsigned char* result, int flags);

int gx_sm3_perform_hmac (
	unsigned char* dataIn, unsigned int dataInLen,
	unsigned char* key, unsigned int keyLen,
	unsigned char* result, int flags);

int gx_sha256_perform_hmac (
	unsigned char* dataIn, unsigned int dataInLen,
	unsigned char* key, unsigned int keyLen,
	unsigned char* result, int flags);

int gx_sha256_perform (
	unsigned char *dataIn, unsigned int   dataInLen,
	unsigned char *result, int flags);

int gx_sha1_perform (
	unsigned char *dataIn, unsigned int   dataInLen,
	unsigned char *result, int flags);

#ifdef TEE_OS
TEE_Result crypto_dev_probe(void);
TEE_Result firewall_dev_probe(void);
#endif

#endif
