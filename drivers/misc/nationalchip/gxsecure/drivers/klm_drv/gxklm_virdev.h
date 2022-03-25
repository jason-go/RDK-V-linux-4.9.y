#ifndef __GXKLM_VIRDEV_H__
#define __GXKLM_VIRDEV_H__

#include "gxse_core.h"
#include "gxse_hwobj_tfm.h"

typedef struct {
	int32_t (*get_alg)(GxTfmAlg alg);
	int32_t (*get_key)(GxTfmKeyBuf *param);
	int32_t (*get_dst)(GxTfmDstBuf *param);
} GxKLMResOps;

typedef struct {
	int32_t (*check_param)       (void *vreg, void *priv, GxTfmKeyLadder *param, uint32_t cmd);
	int32_t (*capability)        (void *vreg, void *priv, GxTfmCap *param);

	int32_t (*set_rootkey)       (void *vreg, void *priv, GxTfmKeyBuf key);
	int32_t (*set_stage)         (void *vreg, void *priv, uint32_t stage);
	int32_t (*set_alg)           (void *vreg, void *priv, GxTfmAlg alg);
	int32_t (*set_dst)           (void *vreg, void *priv, GxTfmDstBuf out_sel);
	int32_t (*set_ekn)           (void *vreg, void *priv, uint8_t *in, uint32_t in_len, uint32_t pos);
	int32_t (*set_ecw)           (void *vreg, void *priv, uint8_t *in, uint32_t in_len);
	int32_t (*set_ready)         (void *vreg, void *priv);
	int32_t (*wait_finish)       (void *vreg, void *priv, uint32_t is_query);
	int32_t (*set_tdc_data)      (void *vreg, void *priv, uint8_t *in, uint32_t in_len);
	int32_t (*set_tdc_ready)     (void *vreg, void *priv, uint32_t tdc_size);
	int32_t (*wait_tdc_idle)     (void *vreg, void *priv, uint32_t is_query);
	int32_t (*set_resp_rootkey)  (void *vreg, void *priv, GxTfmKeyBuf key);
	int32_t (*set_resp_kn)       (void *vreg, void *priv, uint8_t *in, uint32_t in_len, uint32_t pos);
	int32_t (*get_resp)          (void *vreg, void *priv, uint8_t *nonce, uint32_t nonce_len, uint8_t *out, uint32_t out_len);
	int32_t (*clr_config)        (void *vreg, void *priv);
	int32_t (*set_isr_en)        (void *vreg, void *priv);
	int32_t (*clr_isr_en)        (void *vreg, void *priv);
} GxKLMOps;

extern GxSeModuleOps klm_dev_ops;

int32_t gx_tfm_object_klm_select_rootkey(GxSeModuleHwObj *obj, GxTfmKeyLadder *param);
int32_t gx_tfm_object_klm_set_kn(GxSeModuleHwObj *obj, GxTfmKeyLadder *param, uint32_t pos);
int32_t gx_tfm_object_klm_set_cw(GxSeModuleHwObj *obj, GxTfmKeyLadder *param);
int32_t gx_tfm_object_klm_get_resp(GxSeModuleHwObj *obj, GxTfmKeyLadder *param);
int32_t gx_tfm_object_klm_update_TD_param(GxSeModuleHwObj *obj, GxTfmKeyLadder *param);
int32_t gx_tfm_object_klm_capability(GxSeModuleHwObj *obj, GxTfmCap *param);

#endif
