#include "gx_fw_hal.h"

static GxFWOps *s_ops = NULL;

static void probe_chip(void)
{
#ifdef CONFIG_SIRIUS
	PROBE_CHIP(sirius);
#endif
#ifdef CONFIG_TAURUS
	PROBE_CHIP(taurus);
#endif
}

void gx_fw_handle_is_alive(GxSecurePacket *pkt)
{
	gx_mbox_notice_acpu_ack();
}

#if defined (CFG_GXSE_MISC_SENSOR)
void gx_fw_check_sensor_status(void)
{
	GxSeSensorErrStatus p;

	gxse_sensor_get_err_status(&p);
	if (p.err_status) {

#if 0 // 获取sensor错误状态
		uint32_t i = 0, err_value = 0;
		for (i = 0; i < GXSE_SENSOR_TYPE_MAX; i++) {
			if (p.err_status & (0x1 << i))
				err_value = gxse_sensor_get_err_value(i);
		}
#endif
		gxse_misc_scpu_reset();
	}
}
#endif

#if defined (CFG_GXSE_FW_PASSWD)
#define TAURUS_FW_PASSWD_ADDR (0x1C0)
int gx_fw_passwd(GxSecurePacket *pkt)
{
	GxSecureResult result = {0};
	unsigned char fw_passwd_otp[16] = {0};
	unsigned int fw_passwd[4] = {0};
	unsigned int fw_passwd_zero[4] = {0};
	GxSecureUserKey *key = (GxSecureUserKey *)pkt->data;

	if (CHIP_IS_TAURUS || CHIP_IS_GEMINI) {
		memcpy(fw_passwd, key->value, 16);
		switch (pkt->oob) {
		case MB_OOB_USER_TX:

			if (gxse_fuse_read_buf(fw_passwd_otp, 16, TAURUS_FW_PASSWD_ADDR, 16) < 0)
				goto err;

			if (memcmp(fw_passwd_otp, fw_passwd_zero, 16) == 0)
				goto err;

			if (memcmp(fw_passwd_otp, fw_passwd, 16) != 0)
				goto err;

			result.type = FW_RET_RW_OK;
			gx_mbox_notice_acpu_result(&result);
			return FW_PASSWD_PASS;

		default:
			break;
		}
	} else
		return FW_PASSWD_NOT_SUPPORT;

err:
	result.type = FW_RET_RW_OK;
	gx_mbox_notice_acpu_result(&result);
	return FW_PASSWD_NOT_PASS;
}
#endif

#if defined (CFG_GXSE_FIRMWARE) && defined (CFG_MBOX_SET_DDR_INFO)
static struct {
	unsigned int addr;
	unsigned int size;
}fw_ddr_info;

static void gx_fw_set_ddr_info(GxSecurePacket *pkt)
{
	fw_ddr_info.addr = pkt->data[0];
	fw_ddr_info.size = pkt->data[1];
}

int gx_fw_get_ddr_info(unsigned int *addr, unsigned int *size)
{
	if (addr == NULL || size == NULL)
		return -2;

	if (fw_ddr_info.addr == 0 || fw_ddr_info.size == 0)
		return -1;

	*addr = fw_ddr_info.addr;
	*size = fw_ddr_info.size;
	return 0;
}
#endif

void gx_fw_send_cw_by_klm(GxSecurePacket *pkt)
{
	int ret = 0, fixed_stage = 0;
	GxSecureResult result = {0};
	GxTfmKeyLadder *param = (GxTfmKeyLadder *)pkt->data;

	switch (pkt->oob) {
	case MB_OOB_ACPU_SELECT_ROOT_KEY:
		if ((fixed_stage = gxse_fuse_get_fixed_klm_stage(&param->key, param->stage)) > 0)
			param->stage = fixed_stage;

		if (s_ops && s_ops->select_rootkey)
			ret = s_ops->select_rootkey(param);
		else
			ret = gx_tfm_klm_select_rootkey((void *)param);
		break;

	case MB_OOB_ACPU_SET_KN:
		if (s_ops && s_ops->set_kn)
			ret = s_ops->set_kn(param);
		else
			ret = gx_tfm_klm_set_kn((void *)param);
		break;

	case MB_OOB_ACPU_SET_CW:
		if (s_ops && s_ops->set_cw)
			ret = s_ops->set_cw(param);
		else
			ret = gx_tfm_klm_set_cw((void *)param);
		result.type = (ret == 0) ? FW_RET_RW_OK : FW_RET_RW_ERROR;
		gx_mbox_notice_acpu_result(&result);
		break;

	case MB_OOB_ACPU_GET_RESP:
		if ((fixed_stage = gxse_fuse_get_fixed_klm_stage(&param->key, param->stage)) > 0)
			param->stage = fixed_stage;

		if (s_ops && s_ops->get_resp)
			ret = s_ops->get_resp(param);
		else
			ret = gx_tfm_klm_get_resp((void *)param);
		if (ret == 0) {
			result.type = FW_RET_RW_RESP;
			result.len  = 16;
			memcpy(result.buffer, param->output, 16);
		} else
			result.type = FW_RET_RW_ERROR;
		gx_mbox_notice_acpu_result(&result);
		break;

	default:
		break;
	}
}

#if defined (CFG_GXSE_FIRMWARE) && defined (CFG_MBOX_PROTOCOL_GENERIC)
int gx_fw_handle_user_data(GxSecurePacket *pkt)
{
	int ret = 0;
	GxSecureResult result = {0};
	GxSecureUserMsgHead *head = NULL;
	static int protocol_start = 0;
	static unsigned int total_len = 0;
	static unsigned int cur_len   = 0;

	if (protocol_start == 0) {
		head = (void *)pkt->data;
		if (head->sync_byte != SECURE_SYNC_BYTE)
			goto err;

		if (pkt->oob == MB_OOB_USER_RX) {
			if (s_ops && s_ops->mbox_send_check) {
				if (pkt->len < sizeof(GxSecureUserMsgHead))
					goto err;
				if ((ret = s_ops->mbox_send_check(head->size, (unsigned char *)pkt->data+sizeof(GxSecureUserMsgHead),
												  pkt->len-sizeof(GxSecureUserMsgHead))) < 0)
					goto err;
			} else
				goto err;
		}

		result.type = FW_RET_RW_OK;
		gx_mbox_notice_acpu_result(&result);
		cur_len = 0;
		total_len = head->size;
		protocol_start = 1;

	} else {
		if (pkt->oob == MB_OOB_USER_RX) {
			head = (void *)pkt->data;
			result.len = head->size;
			if (cur_len + head->size > total_len)
				goto err;

			if (s_ops && s_ops->mbox_send) {
				if ((ret = s_ops->mbox_send((void *)result.buffer, &result.len)) < 0)
					goto err;
			} else
				goto err;

			result.type = FW_RET_RW_RESP;
			gx_mbox_notice_acpu_result(&result);
			cur_len += result.len;
			if (cur_len == total_len)
				protocol_start = 0;

		} else {
			if (cur_len + pkt->len > total_len) {
				if (s_ops && s_ops->mbox_terminate) {
					if ((ret = s_ops->mbox_terminate()) < 0)
						goto err;
				} else
					goto err;
			}

			if (s_ops && s_ops->mbox_peek) {
				if ((ret = s_ops->mbox_peek((void *)pkt->data, pkt->len)) < 0)
					goto err;
			} else
				goto err;

			cur_len += pkt->len;
			result.type = FW_RET_RW_OK;
			result.len  = 0;

			if (cur_len == total_len) {
				if (s_ops && s_ops->mbox_peek_final) {
					if ((ret = s_ops->mbox_peek_final()) < 0)
						goto err;
				} else
					goto err;

				if (ret > 0) {
					result.type = FW_RET_RW_RESP;
					memcpy(result.buffer, &ret, 4);
					result.len  = 4;
				}
				protocol_start = 0;
			}
			gx_mbox_notice_acpu_result(&result);
		}
	}
	return 0;

err:
	result.type = FW_RET_RW_ERROR;
	gx_mbox_notice_acpu_result(&result);
	protocol_start = 0;
	return -1;
}
#endif

int main(int argc, char **argv)
{
	int ret = 0;
	GxSecurePacket pkt;

	probe_chip();
	gx_fw_register(&s_ops);

	if (s_ops->init) {
		if ((ret = s_ops->init()) < 0) {
			gx_fw_debug_print(1);
			while(1);
		}
	}
	if (s_ops->start)
		s_ops->start();

	while(1) {
#if defined (CFG_GXSE_MISC_SENSOR)
		gx_fw_check_sensor_status();
#endif
		/* receive pkt */
		if (gx_mbox_read(&pkt, 0) < 0) {
			mdelay(5);
			continue;
		}

#if defined (CFG_FW_PASSWD)
		if (ret == MB_SCPU_PASSWD) {
			int passwd_status = gx_fw_passwd(&pkt);
			if (passwd_status == FW_PASSWD_NOT_SUPPORT || passwd_status == FW_PASSWD_PASS)
				ret = 0;

			if (passwd_status == FW_PASSWD_NOT_PASS || passwd_status == FW_PASSWD_PASS)
				continue;
		}
#endif

		switch (pkt.oob) {
		case MB_OOB_APP_IS_ALIVE:
			gx_fw_handle_is_alive(&pkt);
			break;

		case MB_OOB_ACPU_SELECT_ROOT_KEY:
		case MB_OOB_ACPU_SET_KN:
		case MB_OOB_ACPU_SET_CW:
		case MB_OOB_ACPU_GET_RESP:
			gx_fw_send_cw_by_klm(&pkt);
			break;

#if defined (CFG_GXSE_FIRMWARE) && defined (CFG_MBOX_PROTOCOL_GENERIC)
		case MB_OOB_USER_TX:
		case MB_OOB_USER_RX:
			gx_fw_handle_user_data(&pkt);
			break;
#endif

#if defined (CFG_GXSE_FIRMWARE) && defined (CFG_MBOX_SET_DDR_INFO)
		case MB_OOB_USER_CFG_DDR_INFO:
			gx_fw_set_ddr_info(&pkt);
			break;
#endif

		default:
			if (s_ops->console)
				s_ops->console(&pkt);
			break;
		}
	}
	return 0;
}

