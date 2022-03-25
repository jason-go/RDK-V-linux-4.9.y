#include "gxsecure_protocol.h"

static int32_t gx_secure_generic_user_tx(GxSeModuleHwObj *obj, GxSecureUserData *param)
{
	int32_t ret = 0;
	GxSecureUserMsgHead head = {0};
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);
	uint32_t pos = 0, size = 0, totallen = param->size, blocklen = 128;

	if (ops->get_blocklen) {
		if ((ret = ops->get_blocklen(obj->reg, obj->priv)) > 0)
			blocklen = ret;
	}

	/* Send packet desc */
	head.sync_byte = SECURE_SYNC_BYTE;
	head.id        = param->id & 0xff;
	head.size      = (param->size + 3) / 4 * 4;
	if (ops->trx) {
		if ((ret = ops->trx(obj->reg, obj->priv, MB_OOB_USER_TX, (void *)&head, sizeof(GxSecureUserMsgHead), NULL, 0)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "generic TX header error.\n");
			return ret;
		}
	}

	/* Send packet data */
	while (pos < totallen) {
		size = totallen - pos;
		size = (size > blocklen) ? blocklen : size;
		size = (size + 3) / 4 * 4;

		if (ops->trx) {
			if ((ret = ops->trx(obj->reg, obj->priv, MB_OOB_USER_TX,
							param->buf+pos, size, (void *)&param->userdata_len, 4)) < 0) {
				gxlog_e(GXSE_LOG_MOD_SECURE, "generic TX final data error.\n");
				return ret;
			}
		}

		pos += size;
	}

	return GXSE_SUCCESS;
}

static int32_t gx_secure_generic_user_rx(GxSeModuleHwObj *obj, GxSecureUserData *param)
{
#define MAX_RX_SIZE (96)

	int ret = 0;
	GxSecureUserMsgHead head = {0};
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);
	uint32_t pos = 0, size = 0, totallen = param->size, blocklen = 128;
	uint8_t msgbuf[128] = {0};

	if (ops->get_blocklen) {
		if ((ret = ops->get_blocklen(obj->reg, obj->priv)) > 0)
			blocklen = ret;
	}

	blocklen = (blocklen > MAX_RX_SIZE) ? blocklen : MAX_RX_SIZE;
	if (param->userdata_len > blocklen - sizeof(GxSecureUserMsgHead)) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "RX userdata is too large.\n");
		return -2;
	}

	/* Send packet desc */
	head.sync_byte = SECURE_SYNC_BYTE;
	head.id        = param->id & 0xff;
	head.size      = (param->size + 3) / 4 * 4;
	memcpy(msgbuf, &head, sizeof(GxSecureUserMsgHead));
	memcpy(msgbuf+sizeof(GxSecureUserMsgHead), param->buf, param->userdata_len);
	if (ops->trx) {
		if ((ret = ops->trx(obj->reg, obj->priv, MB_OOB_USER_RX,
						 msgbuf, sizeof(GxSecureUserMsgHead)+param->userdata_len, NULL, 0)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "generic RX header error.\n");
			return ret;
		}
	}

	/* Get packet data */
	while (pos < totallen) {
		size = totallen - pos;
		size = (totallen > blocklen) ? blocklen : size;
		size = (size + 3) / 4 * 4;
		head.sync_byte = 0;
		head.id   = param->id & 0xff;
		head.size = size;

		if (ops->trx) {
			if ((ret = ops->trx(obj->reg, obj->priv, MB_OOB_USER_RX,
							(void *)&head, sizeof(head), param->buf+pos, size)) < 0) {
				gxlog_e(GXSE_LOG_MOD_SECURE, "generic send final data error.\n");
				return ret;
			}
		}
		pos += size;
	}

	return GXSE_SUCCESS;
}

GxSecureProtocol secure_protocol_generic = {
	.tx = gx_secure_generic_user_tx,
	.rx = gx_secure_generic_user_rx,
};
