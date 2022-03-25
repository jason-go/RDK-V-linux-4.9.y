#ifndef __GX3211_KLM_H__
#define __GX3211_KLM_H__

#include "gxklm_virdev.h"
#include "gx3211_m2m.h"

#define GX3211_KLM_ALG_DES      (0)
#define GX3211_KLM_ALG_TDES     (1)
#define GX3211_KLM_ALG_AES128   (2)
#define GX3211_KLM_ALG_AES192   (3)
#define GX3211_KLM_ALG_AES256   (4)

#define GX3211_KLM_DST_TS    (0)
#define GX3211_KLM_DST_M2M  (1)

#define GX3211_KLM_KEY_CDCAS_AES  (3)
#define GX3211_KLM_KEY_CDCAS_TDES (4)

extern GxKLMResOps gx3211_klm_resops;
extern GxSeModuleDevOps gx3211_klm_devops;
int32_t gx3211_klm_get_alg(GxTfmAlg alg);
int32_t gx3211_klm_get_key(GxTfmKeyBuf *param);
int32_t gx3211_klm_get_dst(GxTfmDstBuf *param);
int32_t gx3211_klm_check_param(void *vreg, void *priv, GxTfmKlm *param, uint32_t cmd);
int32_t gx3211_klm_clr_config(void *vreg, void *priv);
int32_t gx3211_klm_init(GxSeModuleHwObj *obj);
int32_t gx3211_klm_deinit(GxSeModuleHwObj *obj);
int32_t gx3211_klm_wait_finish(void *vreg, void *priv, uint32_t is_query);
int32_t gx3211_klm_isr(GxSeModuleHwObj *obj);
int32_t gx3211_klm_set_ready(void *vreg, void *priv);
int32_t gx3211_klm_set_ekn(void *vreg, void *priv, uint8_t *in, uint32_t in_len, uint32_t pos);
int32_t gx3211_klm_set_ecw(void *vreg, void *priv, uint8_t *in, uint32_t in_len);
int32_t gx3211_klm_set_alg(void *vreg, void *priv, GxTfmAlg alg);
int32_t gx3211_klm_set_rootkey(void *vreg, void *priv, GxTfmKeyBuf key_sel);
int32_t gx3211_klm_set_stage(void *vreg, void *priv, uint32_t stage);

typedef enum {
	KLM_STATUS_IDLE,
	KLM_STATUS_RUNNING,
	KLM_STATUS_DONE,
} KLMStatus;

struct klm_priv {
	GxTfmKlm param;
	GxKLMResOps *resops;
	unsigned int stage;
	gx_mutex_t *mutex;

	volatile int run_flag;
	volatile KLMStatus status;
};

#endif
