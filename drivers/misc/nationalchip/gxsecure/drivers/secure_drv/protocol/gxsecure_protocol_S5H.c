#include "gxsecure_protocol.h"

static int32_t gx_secure_S5H_user_tx(GxSeModuleHwObj *obj, GxSecureUserData *param)
{
	int32_t err = 0, ret = 0;
	GxSecureUserKey key = {0};
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);

	key.id = param->id & 0xff;
	key.check_val = param->check_val;
	memcpy(key.value, param->buf, 16);

	if (ops->trx) {
		if ((ret = ops->trx(obj->reg, obj->priv, MB_OOB_USER_TX,
						(void *)&key, sizeof(GxSecureUserKey), (void *)&err, 4)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send firmware user key error.\n");
			return ret;
		}
	}
	return err;
}

static int32_t gx_secure_S5H_user_rx(GxSeModuleHwObj *obj, GxSecureUserData *param)
{
	int32_t ret = 0;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->trx) {
		if ((ret = ops->trx(obj->reg, obj->priv, MB_OOB_USER_RX, NULL, 0, param->buf, param->size)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "S5H rx error.\n");
			return ret;
		}
		param->userdata_len = ret;
	}

	return GXSE_SUCCESS;
}

GxSecureProtocol secure_protocol_S5H = {
	.tx = gx_secure_S5H_user_tx,
	.rx = gx_secure_S5H_user_rx,
};
