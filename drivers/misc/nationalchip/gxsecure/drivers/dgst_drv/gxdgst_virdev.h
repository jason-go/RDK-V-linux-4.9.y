#ifndef __GXDGST_VIRDEV_H__
#define __GXDGST_VIRDEV_H__

#include "gxse_core.h"
#include "gxse_hwobj_tfm.h"

typedef struct {
	int32_t (*check_param)       (void *vreg, void *priv, GxTfmDgst *param);
	int32_t (*capability)        (void *vreg, void *priv, GxTfmCap *param);
	int32_t (*get_blocklen)      (void *vreg, void *priv);
	int32_t (*get_align_param)   (void *vreg, void *priv, uint32_t *buf, uint32_t *size, uint32_t *align, uint32_t *flag);

	int32_t (*set_endian)        (void *vreg, void *priv, uint32_t flag);
	int32_t (*set_work_mode)     (void *vreg, void *priv, uint32_t dma_mode);
	int32_t (*set_alg)           (void *vreg, void *priv, GxTfmAlg alg);

	int32_t (*set_dma_buf)       (void *vreg, void *priv, uint8_t *in, uint32_t in_len);
	int32_t (*fifo_put)          (void *vreg, void *priv, uint8_t *buf, uint32_t len);
	int32_t (*get_result)        (void *vreg, void *priv, uint8_t *buf, uint32_t len);
	int32_t (*set_ready)         (void *vreg, void *priv, uint32_t final_val);
	int32_t (*wait_finish)       (void *vreg, void *priv, uint32_t is_query);
	int32_t (*clr_config)        (void *vreg, void *priv);
	int32_t (*set_isr_en)        (void *vreg, void *priv);
	int32_t (*clr_isr_en)        (void *vreg, void *priv);
} GxDgstOps;

extern GxSeModuleOps dgst_dev_ops;

int32_t gx_tfm_object_dgst(GxSeModuleHwObj *obj, GxTfmDgst *param);
int32_t gx_tfm_object_dgst_hmac(GxSeModuleHwObj *obj, GxTfmDgst *param);
int32_t gx_tfm_object_dgst_capability(GxSeModuleHwObj *obj, GxTfmCap *param);

#endif
