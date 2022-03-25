#include "gxse_core.h"
#include "gxse_hwobj_secure.h"
#include "gxse_kapi_secure.h"
#include "gxmailbox_uapi.h"

#ifdef CPU_ACPU
int gxse_secure_load_firmware(GxSecureLoader *firm)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_SECURE_MBOX);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", GXSE_MOD_SECURE_MBOX);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_SEND_LOADER, firm, sizeof(GxSecureLoader));

	return ret;
}

int gxse_secure_get_firmware_status(void)
{
	int32_t ret = GXSE_ERR_GENERIC, status = 0;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_SECURE_MBOX);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", GXSE_MOD_SECURE_MBOX);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_GET_STATUS, &status, 4);

	return ret;
}

int gxse_secure_set_BIV(GxSecureImageVersion *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_SECURE_MBOX);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", GXSE_MOD_SECURE_MBOX);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_SET_BIV, param, sizeof(GxSecureImageVersion));

	return ret;
}

int gxse_secure_get_BIV(GxSecureImageVersion *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_SECURE_MBOX);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", GXSE_MOD_SECURE_MBOX);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_GET_BIV, param, sizeof(GxSecureImageVersion));

	return ret;
}

int gxse_secure_send_irdeto_cmd(GxTfmKlm *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_SECURE_MBOX);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", GXSE_MOD_SECURE_MBOX);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_SEND_IRDETO_CMD, param, sizeof(GxTfmKlm));

	return ret;
}

int gxse_secure_send_root_key(GxTfmKlm *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_SECURE_MBOX);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", GXSE_MOD_SECURE_MBOX);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_SEND_ROOTKEY, param, sizeof(GxTfmKlm));

	return ret;
}

int gxse_secure_send_kn(GxTfmKlm *param, int final)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_SECURE_MBOX);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", GXSE_MOD_SECURE_MBOX);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, final ? SECURE_SEND_CW : SECURE_SEND_KN, param, sizeof(GxTfmKlm));

	return ret;
}

int gxse_secure_get_resp(GxTfmKlm *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_SECURE_MBOX);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", GXSE_MOD_SECURE_MBOX);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_GET_RESP, param, sizeof(GxTfmKlm));

	return ret;
}

int gxse_secure_send_user_key(GxSecureUserKey *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_SECURE_MBOX);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", GXSE_MOD_SECURE_MBOX);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_SEND_USER_KEY, param, sizeof(GxSecureUserKey));

	return ret;
}

int gxse_secure_set_pvrk(int key_id)
{
	GxSecureUserKey key = {0};
	key.id = key_id;
	return gxse_secure_send_user_key(&key);
}

int gxse_secure_create_K3_by_vendorid(int vendorid)
{
	GxSecureUserKey key = {0};
	key.id = 0;
	key.value[0] = 0;
	key.value[1] = 0;
	key.value[2] = 0;
	key.value[3] = vendorid;

	gxlog_i(GXSE_LOG_MOD_SECURE, "[MBOX] send vendor id = [%d].\n", vendorid);
	return gxse_secure_send_user_key(&key);
}

int gx_mbox_load_firmware(GxSecureLoader *firm, int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_load_firmware(firm);
}

int gx_mbox_get_status(int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_get_firmware_status();
}

int gx_mbox_set_BIV(GxSecureImageVersion *param, int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_set_BIV(param);
}

int gx_mbox_get_BIV(GxSecureImageVersion *param, int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_get_BIV(param);
}

int gx_mbox_send_irdeto_cmd(GxTfmKlm *param, int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_send_irdeto_cmd(param);
}

int gx_mbox_send_root_key(GxTfmKlm *param, int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_send_root_key(param);
}

int gx_mbox_send_kn(GxTfmKlm *param, int final, int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_send_kn(param, final);
}

int gx_mbox_get_resp(GxTfmKlm *param, int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_get_resp(param);
}

int gx_mbox_send_user_key(GxSecureUserKey *param, int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_send_user_key(param);
}

int gx_mbox_set_pvrk(int key_id, int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_set_pvrk(key_id);
}

int gx_mbox_create_K3_by_vendorid(int vendorid, int mbox_is_tee)
{
	(void) mbox_is_tee;
	return gxse_secure_create_K3_by_vendorid(vendorid);
}

#else
int gx_mbox_read(GxSecurePacket *pkt, int mbox_is_tee)
{
	int32_t ret = GXSE_ERR_GENERIC;
	uint32_t id = mbox_is_tee ? GXSE_MOD_SECURE_MBOX_TEE : GXSE_MOD_SECURE_MBOX;
	GxSeModule *mod = gxse_module_find_by_id(id);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", id);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_DIRECT_RX, pkt, sizeof(GxSecurePacket));

	return ret;
}

int gx_mbox_write(GxSecurePacket *pkt, int mbox_is_tee)
{
	int32_t ret = GXSE_ERR_GENERIC;
	uint32_t id = mbox_is_tee ? GXSE_MOD_SECURE_MBOX_TEE : GXSE_MOD_SECURE_MBOX;
	GxSeModule *mod = gxse_module_find_by_id(id);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Can't find the module. id = %d\n", id);
		return GXSE_ERR_GENERIC;
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_DIRECT_TX, pkt, sizeof(GxSecurePacket));

	return ret;
}

void gx_mbox_notice_acpu_before_reset_scpu(void)
{
	GxSecurePacket pkt;
	pkt.oob = MB_OOB_APP_RESTART;
	pkt.len = 4;
	gx_mbox_write(&pkt, 0);
}

void gx_mbox_notice_acpu_result(GxSecureResult *result)
{
	GxSecurePacket pkt;
	pkt.oob = MB_OOB_SCPU_SEND_RESULT;
	pkt.len = sizeof(GxSecureResult);
	memcpy(pkt.data, result, sizeof(GxSecureResult));
	gx_mbox_write(&pkt, 0);
}

void gx_mbox_notice_acpu_chip_serialized(void)
{
	GxSecurePacket pkt;
	pkt.oob = MB_OOB_APP_START;
	pkt.len = 4;
	pkt.data[0] = FW_VAL_CHIP_SERIALIZED;
	gx_mbox_write(&pkt, 0);
}

void gx_mbox_notice_acpu_ack(void)
{
	GxSecurePacket pkt;
	pkt.oob = MB_OOB_ACK;
	pkt.len = 4;
	gx_mbox_write(&pkt, 0);
}

#endif

void gx_fw_debug_print(unsigned int value)
{
#ifdef CPU_SCPU
	GxSecurePacket pkt;
	pkt.len = 4;
	pkt.oob = MB_OOB_APP_START;
	pkt.data[0] = value;
	gx_mbox_write(&pkt, 0);
#endif
	(void) value;
}

