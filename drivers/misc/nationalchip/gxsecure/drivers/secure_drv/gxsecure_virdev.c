#include "gxsecure_virdev.h"
#include "crc32.h"

static int32_t gx_secure_ioctl(GxSeModule *module, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleHwObjSecureOps *secure_ops = NULL;

	if (gxse_module_ioctl_check(module, GXSE_HWOBJ_TYPE_FIRMWARE, cmd, size) < 0)
		return GXSE_ERR_GENERIC;

	obj = module->hwobj;
	secure_ops = (GxSeModuleHwObjSecureOps *)obj->ops;
	gx_mutex_lock(obj->mutex);
	switch (cmd) {
#ifdef CPU_ACPU
	case SECURE_SEND_LOADER:
		{
			GxSecureLoader *loader = (GxSecureLoader *)param;
			if (secure_ops->load_firmware)
				ret = secure_ops->load_firmware(obj, loader);
		}
		break;

	case SECURE_GET_STATUS:
		if (secure_ops->get_status)
			ret = secure_ops->get_status(obj, param);
		break;

	case SECURE_SET_BIV:
	case SECURE_GET_BIV:
		{
			GxSecureImageVersion *BIV = (GxSecureImageVersion *)param;

			if (cmd == SECURE_SET_BIV) {
				if (secure_ops->set_BIV)
					ret = secure_ops->set_BIV(obj, BIV);
			} else {
				if (secure_ops->get_BIV)
					ret = secure_ops->get_BIV(obj, BIV);
			}
		}
		break;

	case SECURE_SEND_ROOTKEY:
	case SECURE_SEND_KN:
	case SECURE_SEND_CW:
	case SECURE_GET_RESP:
	case SECURE_SEND_IRDETO_CMD:
		{
			GxTfmKlm *klm = (GxTfmKlm *)param;

			if (cmd == SECURE_SEND_ROOTKEY) {
				if (secure_ops->send_rootkey)
					ret = secure_ops->send_rootkey(obj, klm);

			} else if (cmd == SECURE_SEND_KN){
				if (secure_ops->send_kn)
					ret = secure_ops->send_kn(obj, klm);

			} else if (cmd == SECURE_SEND_CW){
				if (secure_ops->send_cw)
					ret = secure_ops->send_cw(obj, klm);

			} else if (cmd == SECURE_GET_RESP){
				if (secure_ops->get_resp)
					ret = secure_ops->get_resp(obj, klm);

			} else if (cmd == SECURE_SEND_IRDETO_CMD){
				if (secure_ops->send_irdeto_cmd)
					ret = secure_ops->send_irdeto_cmd(obj, klm);
			}
		}
		break;

	case SECURE_SEND_USER_KEY:
		{
			GxSecureUserKey *key = (GxSecureUserKey *)param;

			if (secure_ops->send_user_key)
				ret = secure_ops->send_user_key(obj, key);
		}
		break;

#else
	case SECURE_DIRECT_RX:
	case SECURE_DIRECT_TX:
		{
			GxSecurePacket *pkt = (GxSecurePacket *)param;

			if (cmd == SECURE_DIRECT_TX) {
				if (secure_ops->direct_tx)
					ret = secure_ops->direct_tx(obj, pkt);

			} else {
				if (secure_ops->direct_rx)
					ret = secure_ops->direct_rx(obj, pkt);
			}
		}
		break;
#endif

	default:
		break;
	}

	gx_mutex_unlock(obj->mutex);
	return ret;
}

#ifdef CPU_ACPU
int32_t gx_secure_module_read(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t len)
{
	uint32_t protocol = 0, oob = 0;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);
	GxSecureUserData *param = (void *)buf;

	if (len != sizeof(GxSecureUserData)) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Error buffer length.\n");
		return GXSE_ERR_GENERIC;
	}

	if (ops->firmware_is_alive) {
		if (ops->firmware_is_alive(obj->reg, obj->priv) == 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "firmware is dead.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	oob = (param->id >> 24) & 0xff;
	if (oob == MB_OOB_IFCP)
		protocol = MBOX_PROTOCOL_IFCP;
	else
		protocol = param->protocol;

	return gx_secure_protocol_read(obj, param, protocol);
}

int32_t gx_secure_module_write(GxSeModuleHwObj *obj, const uint8_t *buf, uint32_t len)
{
	uint32_t protocol = 0, oob = 0;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);
	GxSecureUserData *param = (void *)buf;

	if (len != sizeof(GxSecureUserData)) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Error buffer length.\n");
		return GXSE_ERR_GENERIC;
	}

	if (ops->firmware_is_alive) {
		if (ops->firmware_is_alive(obj->reg, obj->priv) == 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "firmware is dead.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	oob = (param->id >> 24) & 0xff;
	if (oob == MB_OOB_IFCP)
		protocol = MBOX_PROTOCOL_IFCP;
	else
		protocol = param->protocol;

	return gx_secure_protocol_write(obj, param, protocol);
}

static int32_t _parse_firmware_head(uint8_t *buf, uint32_t buflen)
{
	int32_t i = 0;
	uint8_t unit_len = 0, unit[FW_HEAD_TOTAL_LEN];
	const char *unit_name_array[] = {FW_HEAD_VERSION, FW_HEAD_CHIP, FW_HEAD_VENDOR};
	uint8_t unit_pos_array[] = {FW_HEAD_VERSION_POS, FW_HEAD_CHIP_POS, FW_HEAD_VENDOR_POS};
	uint8_t unit_len_array[] = {FW_HEAD_VERSION_LEN, FW_HEAD_CHIP_LEN, FW_HEAD_VENDOR_LEN};

	if ((buflen < FW_HEAD_MAGIC_LEN) ||
		memcmp(buf, FW_HEAD_MAGIC, FW_HEAD_MAGIC_LEN) != 0)
		return 0;

	for (i = 0; i < sizeof(unit_pos_array)/sizeof(uint8_t); i++) {
		unit_len = buf[unit_pos_array[i]];
		if (unit_len >= unit_len_array[i]-1) {
			gxlog_i(GXSE_LOG_MOD_SECURE, "section \"%s\" size error\n", unit_name_array[i]);
			return 0;
		}
		memset(unit, 0, FW_HEAD_TOTAL_LEN);
		memcpy(unit, buf+unit_pos_array[i]+1, unit_len);

		gxlog_i(GXSE_LOG_MOD_SECURE, "probe section \"%7s\" = %s\n", unit_name_array[i], unit);
	}

	return FW_HEAD_TOTAL_LEN;
}

int32_t gx_secure_object_load_firmware(GxSeModuleHwObj *obj, GxSecureLoader *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);
	uint32_t blocklen = 128, siglen = 0, pos = 0, size = 0, crc32 = 0, datalen = 0;
	uint32_t fw_len = 0, fw_pos = 0;
	uint8_t *fw = NULL;

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, SECURE_SEND_LOADER)) < 0)
			return ret;
	}

	fw_pos = _parse_firmware_head(param->loader, param->size);
	fw_len = param->size-fw_pos;
	fw = param->loader+fw_pos;

	if (ops->get_blocklen) {
		if ((ret = ops->get_blocklen(obj->reg, obj->priv)) > 0)
			blocklen = ret;
	}

	if (ops->get_siglen) {
		if ((ret = ops->get_siglen(obj->reg, obj->priv)) > 0)
			siglen = ret;
	}

	if (ops->tx) {
		if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_ACPU_START, NULL, 0)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send sync byte error.\n");
			return ret;
		}
	}

	datalen = fw_len - siglen;
	if (ops->tx) {
		if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_DATA_LEN, (void *)&datalen, 4)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send firmware length error.\n");
			return ret;
		}
	}

	if (ops->tx) {
		if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_APP_ADDR, (void *)&param->dst_addr, 4)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send firmware addr error.\n");
			return ret;
		}
	}
	gxlog_i(GXSE_LOG_MOD_SECURE, "send total_len = [%d], addr = [0x%x].\n", fw_len+4, param->dst_addr);

	while (pos < siglen) {
		size = siglen - pos;
		size = size > blocklen ? blocklen : size;
		if (ops->tx) {
			if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_SIG, fw+pos, size)) < 0) {
				gxlog_e(GXSE_LOG_MOD_SECURE, "send firmware signature error.\n");
				return ret;
			}
		}
		pos += size;
	}

	while (pos < fw_len) {
		size = fw_len - pos;
		size = size > blocklen ? blocklen : size;
		if (ops->tx) {
			if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_APP, fw+pos, size)) < 0) {
				gxlog_e(GXSE_LOG_MOD_SECURE, "send firmware data error.\n");
				return ret;
			}
		}
		crc32 = gxse_common_crc32(fw+pos, size);
		pos += size;
	}

	if (ops->tx) {
		if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_CRC, (void *)&crc32, 4)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send firmware crc error.\n");
			return ret;
		}
	}

	return GXSE_SUCCESS;
}

int32_t gx_secure_object_get_status(GxSeModuleHwObj *obj, uint32_t *status)
{
	int32_t ret = 0;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->get_status) {
		if ((ret = ops->get_status(obj->reg, obj->priv)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "get status failed.\n");
			return ret;
		}
	}

	*status = ret;

	return GXSE_SUCCESS;
}

int32_t gx_secure_object_set_BIV(GxSeModuleHwObj *obj, GxSecureImageVersion *param)
{
	int32_t ret = 0;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->set_BIV) {
		if ((ret = ops->set_BIV(obj->reg, obj->priv, (void *)param->version, sizeof(param->version))) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "set BIV failed.\n");
			return ret;
		}
	}

	return GXSE_SUCCESS;
}

int32_t gx_secure_object_get_BIV(GxSeModuleHwObj *obj, GxSecureImageVersion *param)
{
	int32_t ret = 0;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->get_BIV) {
		if ((ret = ops->get_BIV(obj->reg, obj->priv, (void *)param->version, sizeof(param->version))) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "get BIV failed.\n");
			return ret;
		}
	}

	return GXSE_SUCCESS;
}

int32_t gx_secure_object_send_rootkey(GxSeModuleHwObj *obj, GxTfmKlm *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);
	GxTfmScpuKlm klm = {0};

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, SECURE_SEND_ROOTKEY)) < 0)
			return ret;
	}

	if (ops->firmware_is_alive) {
		if (ops->firmware_is_alive(obj->reg, obj->priv) == 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "firmware is dead.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	klm.module = param->module;
	klm.stage  = param->stage;
	klm.flags  = param->flags;
	memcpy(&klm.key, &param->key, sizeof(GxTfmKeyBuf));
	if (ops->tx) {
		if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_ACPU_SELECT_ROOT_KEY,
						(void *)&klm, sizeof(GxTfmScpuKlm))) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send Scpu-KLM rootkey error.\n");
			return ret;
		}
	}

	return GXSE_SUCCESS;
}

int32_t gx_secure_object_send_kn(GxSeModuleHwObj *obj, GxTfmKlm *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);
	GxTfmScpuKlm klm = {0};

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, SECURE_SEND_KN)) < 0)
			return ret;
	}

	if (ops->firmware_is_alive) {
		if (ops->firmware_is_alive(obj->reg, obj->priv) == 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "firmware is dead.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	klm.module = param->module;
	klm.alg    = param->alg;
	klm.flags  = param->flags;
	memcpy(klm.input, param->input.buf, param->input.length);

	if (ops->tx) {
		if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_ACPU_SET_KN, (void *)&klm, sizeof(GxTfmScpuKlm))) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send Scpu-KLM kn error.\n");
			return ret;
		}
	}

	return GXSE_SUCCESS;
}

int32_t gx_secure_object_send_cw(GxSeModuleHwObj *obj, GxTfmKlm *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);
	GxTfmScpuKlm klm = {0};

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, SECURE_SEND_CW)) < 0)
			return ret;
	}

	if (ops->firmware_is_alive) {
		if (ops->firmware_is_alive(obj->reg, obj->priv) == 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "firmware is dead.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	klm.module = param->module;
	klm.alg    = param->alg;
	klm.flags  = param->flags;
	memcpy(&klm.src, &param->src, sizeof(GxTfmSrcBuf));
	memcpy(&klm.dst, &param->dst, sizeof(GxTfmDstBuf));
	memcpy(klm.input, param->input.buf, param->input.length);

	if (ops->trx) {
		if ((ret = ops->trx(obj->reg, obj->priv, MB_OOB_ACPU_SET_CW,
						(void *)&klm, sizeof(GxTfmScpuKlm), NULL, 0)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send Scpu-KLM CW error.\n");
			return ret;
		}
	}

	return GXSE_SUCCESS;
}

int32_t gx_secure_object_send_irdeto_cmd(GxSeModuleHwObj *obj, GxTfmKlm *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);
	uint32_t blocklen = 128, pos = 0, size = 0;

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, SECURE_SEND_IRDETO_CMD)) < 0)
			return ret;
	}

	if (ops->firmware_is_alive) {
		if (ops->firmware_is_alive(obj->reg, obj->priv) == 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "firmware is dead.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	if (ops->get_blocklen) {
		if ((ret = ops->get_blocklen(obj->reg, obj->priv)) > 0)
			blocklen = ret;
	}

	if (ops->tx) {
		if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_ACPU_SET_IRDETO_CMD_INFO,
						(void *)&param->input.length, 4)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send Scpu-Irdeto-KLM length error.\n");
			return ret;
		}
	}

	while (pos < param->input.length) {
		size = param->input.length - pos;
		size = size > blocklen ? blocklen : size;
		if (ops->tx) {
			if ((ret = ops->tx(obj->reg, obj->priv, MB_OOB_ACPU_SET_IRDETO_CMD, param->input.buf+pos, size)) < 0) {
				gxlog_e(GXSE_LOG_MOD_SECURE, "send Scpu-Irdeto-KLM cmdbuf error.\n");
				return ret;
			}
		}
		pos += size;
	}

	if (ops->trx) {
		if ((ret = ops->trx(obj->reg, obj->priv, MB_OOB_ACPU_SET_IRDETO_CMD_END, NULL, 0,
						param->output.buf, param->output.length)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send Scpu-Irdeto-KLM end error.\n");
			return ret;
		}
	}

	return GXSE_SUCCESS;
}

int32_t gx_secure_object_get_resp(GxSeModuleHwObj *obj, GxTfmKlm *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);
	GxTfmScpuKlm klm = {0};

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, SECURE_GET_RESP)) < 0)
			return ret;
	}

	if (ops->firmware_is_alive) {
		if (ops->firmware_is_alive(obj->reg, obj->priv) == 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "firmware is dead.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	klm.module = param->module;
	klm.alg    = param->alg;
	klm.flags  = param->flags;
	memcpy(&klm.src, &param->src, sizeof(GxTfmSrcBuf));
	memcpy(&klm.dst, &param->dst, sizeof(GxTfmDstBuf));
	memcpy(&klm.key, &param->key, sizeof(GxTfmKeyBuf));
	memcpy(klm.input, param->input.buf, 16);
	memcpy(klm.nonce, param->nonce, 16);
	if (ops->trx) {
		if ((ret = ops->trx(obj->reg, obj->priv, MB_OOB_ACPU_GET_RESP,
						(void *)&klm, sizeof(GxTfmScpuKlm), param->output.buf, param->output.length)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send Scpu-KLM get-resp error.\n");
			return ret;
		}
	}

	return GXSE_SUCCESS;
}

int32_t gx_secure_object_send_user_key(GxSeModuleHwObj *obj, GxSecureUserKey *param)
{
	int32_t ret = GXSE_ERR_GENERIC, err = 0;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, SECURE_SEND_USER_KEY)) < 0)
			return ret;
	}

	if (ops->firmware_is_alive) {
		if (ops->firmware_is_alive(obj->reg, obj->priv) == 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "firmware is dead.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	if (ops->trx) {
		if ((ret = ops->trx(obj->reg, obj->priv, MB_OOB_USER_TX,
						(void *)param, sizeof(GxSecureUserKey), (void *)&err, 4)) < 0) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "send firmware user key error.\n");
			return ret;
		}
	}

	return err;
}

#else
int32_t gx_secure_object_direct_tx(GxSeModuleHwObj *obj, GxSecurePacket *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->tx) {
		if ((ret = ops->tx(obj->reg, obj->priv, param->oob, (void *)param->data, param->len)) < 0)
			return ret;
	}

	return ret;
}

int32_t gx_secure_object_direct_rx(GxSeModuleHwObj *obj, GxSecurePacket *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOps *ops = (GxSecureOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->rx) {
		if ((ret = ops->rx(obj->reg, obj->priv, &param->oob, (void *)param->data, param->len)) < 0)
			return ret;
	}
	param->len = ret;

	return ret;
}
#endif

GxSeModuleOps secure_dev_ops = {
	.ioctl  = gx_secure_ioctl,
	.init   = gxse_module_init,
#ifdef CPU_ACPU
	.deinit = gxse_module_deinit,
	.open   = gxse_module_open,
	.close  = gxse_module_close,
	.read   = gxse_module_read,
	.write  = gxse_module_write,
	.isr    = gxse_module_isr,
	.dsr    = gxse_module_dsr,
#endif
};

