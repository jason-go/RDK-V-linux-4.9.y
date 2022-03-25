#ifndef __GXSECURE_VIRDEV_H__
#define __GXSECURE_VIRDEV_H__

#include "gxse_core.h"
#include "gxse_hwobj_secure.h"
#include "gxse_hwobj_tfm.h"

typedef struct {
	int32_t (*check_param)       (void *vreg, void *priv, void *param, uint32_t cmd);
	int32_t (*get_status)        (void *vreg, void *priv);
	int32_t (*get_blocklen)      (void *vreg, void *priv);
	int32_t (*get_siglen)        (void *vreg, void *priv);

	int32_t (*firmware_is_alive) (void *vreg, void *priv);
	int32_t (*tx)                (void *vreg, void *priv, uint32_t oob, uint8_t *buf, uint32_t size);
	int32_t (*rx)                (void *vreg, void *priv, uint32_t *oob, uint8_t *buf, uint32_t size);
	int32_t (*trx)               (void *vreg, void *priv, uint32_t oob,
									uint8_t *in, uint32_t insize, uint8_t *out,uint32_t outsize);
	int32_t (*set_BIV)           (void *vreg, void *priv, uint8_t *buf, uint32_t size);
	int32_t (*get_BIV)           (void *vreg, void *priv, uint8_t *buf, uint32_t size);
	int32_t (*set_isr_en)        (void *vreg, void *priv);
	int32_t (*clr_isr_en)        (void *vreg, void *priv);
} GxSecureOps;

int32_t gx_secure_protocol_read(GxSeModuleHwObj *obj, GxSecureUserData *param, uint32_t protocol);
int32_t gx_secure_protocol_write(GxSeModuleHwObj *obj, GxSecureUserData *param, uint32_t protocol);

extern GxSeModuleOps secure_dev_ops;

#ifdef CPU_ACPU
int32_t gx_secure_module_read(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t len);
int32_t gx_secure_module_write(GxSeModuleHwObj *obj, const uint8_t *buf, uint32_t len);
int32_t gx_secure_object_load_firmware(GxSeModuleHwObj *obj, GxSecureLoader *param);
int32_t gx_secure_object_get_status(GxSeModuleHwObj *obj, uint32_t *status);
int32_t gx_secure_object_set_BIV(GxSeModuleHwObj *obj, GxSecureImageVersion *param);
int32_t gx_secure_object_get_BIV(GxSeModuleHwObj *obj, GxSecureImageVersion *param);
int32_t gx_secure_object_send_rootkey(GxSeModuleHwObj *obj, GxTfmKlm *param);
int32_t gx_secure_object_send_kn(GxSeModuleHwObj *obj, GxTfmKlm *param);
int32_t gx_secure_object_send_cw(GxSeModuleHwObj *obj, GxTfmKlm *param);
int32_t gx_secure_object_send_irdeto_cmd(GxSeModuleHwObj *obj, GxTfmKlm *param);
int32_t gx_secure_object_get_resp(GxSeModuleHwObj *obj, GxTfmKlm *param);
int32_t gx_secure_object_send_user_key(GxSeModuleHwObj *obj, GxSecureUserKey *param);
#else
int32_t gx_secure_object_direct_tx(GxSeModuleHwObj *obj, GxSecurePacket *param);
int32_t gx_secure_object_direct_rx(GxSeModuleHwObj *obj, GxSecurePacket *param);
#endif

#endif
