#ifndef __GXCRYPTO_VIRDEV_H__
#define __GXCRYPTO_VIRDEV_H__

#include "gxse_core.h"
#include "gxse_hwobj_tfm.h"

typedef struct {
	int32_t (*get_alg)(GxTfmAlg alg);
	int32_t (*get_opt)(GxTfmOpt opt);
	int32_t (*get_key)(GxTfmKeyBuf *param);
	int32_t (*get_src)(GxTfmSrcBuf *param);
	int32_t (*get_dst)(GxTfmDstBuf *param);
} GxCryptoResOps;

typedef struct {
	int32_t (*check_param)       (void *vreg, void *priv, GxTfmCrypto *param, uint32_t cmd);
	int32_t (*capability)        (void *vreg, void *priv, GxTfmCap *param);
	int32_t (*get_blocklen)      (void *vreg, void *priv);
	int32_t (*get_align_param)   (void *vreg, void *priv, uint32_t *buf, uint32_t *size, uint32_t *align, uint32_t *flag);

	int32_t (*set_endian)        (void *vreg, void *priv, uint32_t flag);
	int32_t (*set_work_mode)     (void *vreg, void *priv, uint32_t crypt_mode, uint32_t work_mode, uint32_t ts_mode);
	int32_t (*set_alg)           (void *vreg, void *priv, GxTfmAlg alg);
	int32_t (*set_key)           (void *vreg, void *priv, GxTfmKeyBuf key_sel, GxTfmKeyBuf oddkey_sel,
			uint8_t *key, uint32_t key_len);
	int32_t (*set_pvr_key)       (void *vreg, void *priv, GxTfmPVRKey *key);
	int32_t (*clr_key)           (void *vreg, void *priv);
	int32_t (*enable_key)        (void *vreg, void *priv);
	int32_t (*disable_key)       (void *vreg, void *priv);
	int32_t (*set_opt)           (void *vreg, void *priv, GxTfmOpt opt_sel, uint8_t *key, uint32_t key_len);
	int32_t (*set_residue_mode)  (void *vreg, void *priv, uint32_t mode);
	int32_t (*set_shortmsg_mode) (void *vreg, void *priv, uint32_t mode);
	int32_t (*set_switch_clr)    (void *vreg, void *priv, uint32_t flag);
	int32_t (*set_input_buf)     (void *vreg, void *priv, GxTfmSrcBuf in_sel, uint8_t *in, uint32_t in_len);
	int32_t (*set_output_buf)    (void *vreg, void *priv, GxTfmDstBuf out_sel, uint8_t *out, uint32_t out_len);
	int32_t (*fifo_put)          (void *vreg, void *priv, uint8_t *buf, uint32_t len, uint32_t first_block);
	int32_t (*fifo_get)          (void *vreg, void *priv, uint8_t *buf, uint32_t len);
	int32_t (*set_ready)         (void *vreg, void *priv);
	int32_t (*wait_finish)       (void *vreg, void *priv, uint32_t is_query);
	int32_t (*clr_config)        (void *vreg, void *priv);
	int32_t (*set_isr_en)        (void *vreg, void *priv);
	int32_t (*clr_isr_en)        (void *vreg, void *priv);
} GxCryptoOps;

extern GxSeModuleOps crypto_dev_ops;

int32_t gx_tfm_object_crypto_fifo_encrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param);
int32_t gx_tfm_object_crypto_fifo_decrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param);
int32_t gx_tfm_object_crypto_dma_encrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param);
int32_t gx_tfm_object_crypto_dma_decrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param);
int32_t gx_tfm_object_crypto_get_align_buf(GxSeModuleHwObj *obj, uint32_t *buf, uint32_t *size);
int32_t gx_tfm_object_crypto_clr_key(GxSeModuleHwObj *obj);
int32_t gx_tfm_object_crypto_enable_key(GxSeModuleHwObj *obj);
int32_t gx_tfm_object_crypto_disable_key(GxSeModuleHwObj *obj);
int32_t gx_tfm_object_crypto_set_pvr_key(GxSeModuleHwObj *obj, GxTfmPVRKey *param);
int32_t gx_tfm_object_crypto_capability(GxSeModuleHwObj *obj, GxTfmCap *param);

#endif
