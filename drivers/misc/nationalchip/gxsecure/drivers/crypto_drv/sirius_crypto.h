#ifndef __SIRIUS_CRYPTO_H__
#define __SIRIUS_CRYPTO_H__

#include "gxcrypto_virdev.h"
#include "sirius_crypto_reg.h"
#include "scpu_crypto.h"

#define SIRIUS_CRYPTO_BLOCKLEN (128)
#define SIRIUS_SKLM_BLOCKLEN   (16)

#define SIRIUS_CRYPTO_ALG_AES  (0)
#define SIRIUS_CRYPTO_ALG_SM4  (1)

#define SIRIUS_CRYPTO_OPT_ECB  (0)
#define SIRIUS_CRYPTO_OPT_CBC  (1)

#ifdef CPU_ACPU
#define SIRIUS_CRYPTO_DST_REG0 (0)
#define SIRIUS_CRYPTO_DST_REG1 (1)
#define SIRIUS_CRYPTO_DST_REG2 (2)
#define SIRIUS_CRYPTO_DST_DO   (3)

#define SIRIUS_CRYPTO_KEY_BCMK (0)
#define SIRIUS_CRYPTO_KEY_BCK  (1)
#define SIRIUS_CRYPTO_KEY_IMK  (2)
#define SIRIUS_CRYPTO_KEY_IK   (3)
#define SIRIUS_CRYPTO_KEY_REG0 (4)
#define SIRIUS_CRYPTO_KEY_REG1 (5)
#define SIRIUS_CRYPTO_KEY_REG2 (6)
#define SIRIUS_CRYPTO_KEY_CCCK (7)
#define SIRIUS_CRYPTO_KEY_OTP  (8)
#define SIRIUS_CRYPTO_KEY_SOFT (9)

#else

#define SIRIUS_CRYPTO_KEY_CSGK2 (0)
#define SIRIUS_CRYPTO_KEY_SIK   (1)
#define SIRIUS_CRYPTO_KEY_OTP   (2)
#define SIRIUS_CRYPTO_KEY_SOFT  (3)

#define SIRIUS_SKLM_ALG_AES   (0)
#define SIRIUS_SKLM_ALG_SM4   (1)
#define SIRIUS_SKLM_ALG_TDES  (2)

#define SIRIUS_SKLM_SRC_SOFT  (0)
#define SIRIUS_SKLM_SRC_REG0  (1)
#define SIRIUS_SKLM_SRC_REG1  (2)
#define SIRIUS_SKLM_SRC_REG2  (3)
#define SIRIUS_SKLM_SRC_REG3  (4)
#define SIRIUS_SKLM_SRC_REG4  (5)
#define SIRIUS_SKLM_SRC_REG5  (6)
#define SIRIUS_SKLM_SRC_REG6  (7)
#define SIRIUS_SKLM_SRC_REG7  (8)

#define SIRIUS_SKLM_DST_SOFT  (0)
#define SIRIUS_SKLM_DST_REG0  (1)
#define SIRIUS_SKLM_DST_REG1  (2)
#define SIRIUS_SKLM_DST_REG2  (3)
#define SIRIUS_SKLM_DST_REG3  (4)
#define SIRIUS_SKLM_DST_REG4  (5)
#define SIRIUS_SKLM_DST_REG5  (6)
#define SIRIUS_SKLM_DST_REG6  (7)
#define SIRIUS_SKLM_DST_REG7  (8)
#define SIRIUS_SKLM_DST_TS    (9)
#define SIRIUS_SKLM_DST_GP    (10)
#define SIRIUS_SKLM_DST_M2M   (11)

#define SIRIUS_SKLM_KEY_DSK1  (0)
#define SIRIUS_SKLM_KEY_DSK2  (1)
#define SIRIUS_SKLM_KEY_RK1   (2)
#define SIRIUS_SKLM_KEY_RK2   (3)
#define SIRIUS_SKLM_KEY_KDS   (4)
#define SIRIUS_SKLM_KEY_SMK   (5)
#define SIRIUS_SKLM_KEY_REG0  (6)
#define SIRIUS_SKLM_KEY_REG1  (7)
#define SIRIUS_SKLM_KEY_REG2  (8)
#define SIRIUS_SKLM_KEY_REG3  (9)
#define SIRIUS_SKLM_KEY_REG4  (10)
#define SIRIUS_SKLM_KEY_REG5  (11)
#define SIRIUS_SKLM_KEY_REG6  (12)
#define SIRIUS_SKLM_KEY_REG7  (13)
#define SIRIUS_SKLM_KEY_SOFT  (14)

void gx_bootcrypto_isr(int32_t flag);
int32_t sirius_dynamic_crypto_get_alg(GxTfmAlg alg);
int32_t sirius_dynamic_crypto_get_key(GxTfmKeyBuf *param);
int32_t sirius_dynamic_crypto_get_src(GxTfmSrcBuf *param);
int32_t sirius_dynamic_crypto_get_dst(GxTfmDstBuf *param);
int32_t sirius_dynamic_crypto_check_param(void *vreg, void *priv, GxTfmCrypto *param, uint32_t cmd);
int32_t sirius_dynamic_crypto_set_key(void *vreg, void *priv, GxTfmKeyBuf key_sel, GxTfmKeyBuf oddkey_sel,
		uint8_t *key, uint32_t key_len);
int32_t sirius_dynamic_crypto_set_alg(void *vreg, void *priv, GxTfmAlg alg);
int32_t sirius_dynamic_crypto_set_input_buf(void *vreg, void *priv, GxTfmSrcBuf in_sel, uint8_t *in, uint32_t in_len);
int32_t sirius_dynamic_crypto_set_output_buf(void *vreg, void *priv, GxTfmDstBuf out_sel, uint8_t *out, uint32_t out_len);
int32_t sirius_dynamic_crypto_get_blocklen(void *vreg, void *priv);
int32_t sirius_dynamic_crypto_fifo_put(void *vreg, void *priv, uint8_t *buf, uint32_t len, uint32_t first_block);
int32_t sirius_dynamic_crypto_fifo_get(void *vreg, void *priv, uint8_t *buf, uint32_t len);
int32_t sirius_dynamic_crypto_set_ready(void *vreg, void *priv);
int32_t sirius_dynamic_crypto_wait_finish(void *vreg, void *priv, uint32_t is_query);
int32_t sirius_dynamic_crypto_clr_config(void *vreg, void *priv);
int32_t sirius_dynamic_crypto_clr_key(void *vreg, void *priv);
int32_t sirius_dynamic_crypto_init(GxSeModuleHwObj *obj);
int32_t sirius_dynamic_crypto_deinit(GxSeModuleHwObj *obj);
int32_t sirius_dynamic_crypto_scpu_capability(void *vreg, void *priv, GxTfmCap *param);
#endif

struct crypto_priv {
#ifdef CPU_SCPU
	struct crypto_isr_info *isr_info;
#endif
	GxTfmCrypto param;

	GxCryptoResOps *resops;
	volatile SiriusCryptoReg *reg;
	volatile int crypto_done;

	gx_event_t queue;
	gx_spin_lock_t spin_lock;
	gx_mutex_t mutex;
};

int32_t sirius_crypto_get_alg(GxTfmAlg alg);
int32_t sirius_crypto_get_opt(GxTfmOpt opt);
int32_t sirius_crypto_get_key(GxTfmKeyBuf *param);
int32_t sirius_crypto_check_param(void *vreg, void *priv, GxTfmCrypto *param, uint32_t cmd);
int32_t sirius_crypto_set_alg(void *vreg, void *priv, GxTfmAlg alg);
int32_t sirius_crypto_set_key(void *vreg, void *priv, GxTfmKeyBuf key_sel, GxTfmKeyBuf oddkey_sel,
		uint8_t *key, uint32_t key_len);
int32_t sirius_crypto_set_opt(void *vreg, void *priv, GxTfmOpt opt_sel, uint8_t *key, uint32_t key_len);
int32_t sirius_crypto_get_blocklen(void *vreg, void *priv);
int32_t sirius_crypto_fifo_put(void *vreg, void *priv, uint8_t *buf, uint32_t len, uint32_t first_block);
int32_t sirius_crypto_fifo_get(void *vreg, void *priv, uint8_t *buf, uint32_t len);
int32_t sirius_crypto_set_ready(void *vreg, void *priv);
int32_t sirius_crypto_wait_finish(void *vreg, void *priv, uint32_t is_query);
int32_t sirius_crypto_init(GxSeModuleHwObj *obj);
int32_t sirius_crypto_deinit(GxSeModuleHwObj *obj);
int32_t sirius_crypto_clr_key(void *vreg, void *priv);
int32_t sirius_crypto_capability(void *vreg, void *priv, GxTfmCap *param);

#endif
