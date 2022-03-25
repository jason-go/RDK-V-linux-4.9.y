#include "gxsecure_protocol.h"

static int32_t gx_secure_IFCP_user_tx(GxSeModuleHwObj *obj, GxSecureUserData *param)
{
	int32_t ret = 0;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->tx) {
		if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_IFCP, param->buf, param->size)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "IFCP tx error.\n");
			return ret;
		}
	}

	return GXSE_SUCCESS;
}

static int32_t gx_secure_IFCP_user_rx(GxSeModuleHwObj *obj, GxSecureUserData *param)
{
	int32_t ret = 0;
	uint32_t oob = 0;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->rx) {
		if ((ret = ops->rx(obj->reg, obj->priv, &oob, param->buf, param->size)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "IFCP rx error.\n");
			return ret;
		}
		if (oob != MB_OOB_IFCP) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "IFCP rx oob error.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	return GXSE_SUCCESS;
}

GxSecureProtocol secure_protocol_IFCP = {
	.tx = gx_secure_IFCP_user_tx,
	.rx = gx_secure_IFCP_user_rx,
};
