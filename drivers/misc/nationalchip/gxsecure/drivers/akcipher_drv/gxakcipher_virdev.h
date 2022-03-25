#ifndef __GXAKCIPHER_VIRDEV_H__
#define __GXAKCIPHER_VIRDEV_H__

#include "gxse_core.h"
#include "gxse_hwobj_tfm.h"

typedef struct {
	int32_t (*check_param)       (void *vreg, void *priv, void *param, uint32_t cmd);
	int32_t (*capability)        (void *vreg, void *priv, GxTfmCap *param);
	int32_t (*get_curve_param)   (void *vreg, void *priv, uint8_t **n, uint8_t **p,
								  uint8_t **a, uint8_t **b, uint8_t **xG, uint8_t **yG);

	int32_t (*do_verify)         (void *vreg, void *priv, uint8_t *pub_key, uint32_t pub_key_len,
			uint8_t *hash, uint32_t hash_len, uint8_t *sig, uint32_t sig_len);
} GxAkcipherOps;

extern GxSeModuleOps akcipher_dev_ops;

int32_t gx_tfm_object_akcipher_encrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param);
int32_t gx_tfm_object_akcipher_decrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param);
int32_t gx_tfm_object_akcipher_verify(GxSeModuleHwObj *obj, GxTfmVerify *param);
int32_t gx_tfm_object_akcipher_cal_za(GxSeModuleHwObj *obj, GxTfmDgst *param);
int32_t gx_tfm_object_akcipher_capability(GxSeModuleHwObj *obj, GxTfmCap *param);

#endif
