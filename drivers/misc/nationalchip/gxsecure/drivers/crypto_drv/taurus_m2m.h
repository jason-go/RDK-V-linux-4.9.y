#ifndef __TAURUS_M2M_H__
#define __TAURUS_M2M_H__

#include "gxcrypto_virdev.h"
#include "sirius_m2m.h"

struct mtc_priv {
	GxTfmCrypto         param;
	GxCryptoResOps     *resops;
	gx_mutex_t         *mutex;
	uint8_t            *align_buf;
	uint32_t            align_buf_len;
	int32_t             nds_wait_flag;
	volatile M2MStatus  status;
};

extern GxSeModuleDevOps taurus_m2m_devops;
#define TAURUS_M2M_MAX_MOD       (2)
#define TAURUS_M2M_BLOCKLEN      (64)

#define TAURUS_M2M_ALG_DES       (0)
#define TAURUS_M2M_ALG_TDES3K    (1)
#define TAURUS_M2M_ALG_AES128    (2)
#define TAURUS_M2M_ALG_AES192    (3)
#define TAURUS_M2M_ALG_AES256    (4)

#define TAURUS_M2M_OPT_ECB       (0)
#define TAURUS_M2M_OPT_CBC       (1)
#define TAURUS_M2M_OPT_CFB       (2)
#define TAURUS_M2M_OPT_OFB       (3)
#define TAURUS_M2M_OPT_CTR       (4)

#define TAURUS_M2M_KEY_ACPU_SOFT (0)
#define TAURUS_M2M_KEY_SCPU_KLM  (1)
#define TAURUS_M2M_KEY_MULTI     (5)
#define TAURUS_M2M_KEY_MEMORY    (6) //目前没有实际的应用
#define TAURUS_M2M_KEY_OTP_FLASH (7)
#define TAURUS_M2M_KEY_SCPU_PVR  (8)
#define TAURUS_M2M_KEY_SCPU_SOFT (9)
#define TAURUS_M2M_KEY_OTP_SOFT  (10)
#define TAURUS_M2M_KEY_OTP_FIRM  (11)

#define TAURUS_M2M_REG_LEN       (0x300)

int32_t taurus_m2m_get_alg(GxTfmAlg alg);
int32_t taurus_m2m_get_opt(GxTfmOpt opt);
int32_t taurus_m2m_get_key(GxTfmKeyBuf *param);
int32_t taurus_m2m_get_align_param(void *vreg, void *priv, uint32_t *buf, uint32_t *size, uint32_t *align, uint32_t *flag);
int32_t taurus_m2m_set_work_mode(void *vreg, void *priv, uint32_t crypt_mode, uint32_t work_mode, uint32_t ts_mode);
int32_t taurus_m2m_set_endian(void *vreg, void *priv, uint32_t flag);
int32_t taurus_m2m_set_alg(void *vreg, void *priv, GxTfmAlg alg);
int32_t taurus_m2m_set_opt(void *vreg, void *priv, GxTfmOpt opt_sel, uint8_t *iv, uint32_t key_len);
int32_t taurus_m2m_set_residue_mode(void *vreg, void *priv, uint32_t mode);
int32_t taurus_m2m_set_shortmsg_mode(void *vreg, void *priv, uint32_t mode);
int32_t taurus_m2m_set_input_buf(void *vreg, void *priv, GxTfmSrcBuf in_sel, uint8_t *in, uint32_t in_len);
int32_t taurus_m2m_set_output_buf(void *vreg, void *priv, GxTfmDstBuf out_sel, uint8_t *out, uint32_t out_len);
int32_t taurus_m2m_set_ready(void *vreg, void *priv);
int32_t taurus_m2m_clr_config(void *vreg, void *priv);
int32_t taurus_m2m_isr(GxSeModuleHwObj *obj);
int32_t taurus_m2m_wait_finish(void *vreg, void *priv, uint32_t is_query);
int32_t taurus_m2m_set_isr_en(void *vreg, void *priv);
void    taurus_set_soft_key(void *reg , uint32_t alg, uint8_t *key, uint32_t key_big_endian);
#endif
