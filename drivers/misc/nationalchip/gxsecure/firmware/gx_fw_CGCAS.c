#include "gx_fw_hal.h"

#define TAURUS_OTP_ADDR_DSK1          0x160
#define TAURUS_OTP_ADDR_DSK2          0x170
#define TAURUS_OTP_ADDR_DSK3          0x180
#define TAURUS_OTP_ADDR_DSK4          0x190
#define TAURUS_OTP_ADDR_RK1            0x1a0
#define TAURUS_OTP_ADDR_RK2            0x1b0

enum {
	CGCAS_DSK1,
	CGCAS_DSK2,
	CGCAS_DSK3,
	CGCAS_DSK4,
	CGCAS_RK1,
	CGCAS_RK2,
	CGCAS_MAX_KEY,
};

typedef struct {
	unsigned char DSK1[16];
	unsigned char DSK2[16];
	unsigned char DSK3[16];
	unsigned char DSK4[16];
	unsigned char RK1[16];
	unsigned char RK2[16];
} GxKeyInfo;

static GxKeyInfo s_info;
static unsigned int addr_array[CGCAS_MAX_KEY] = {
	TAURUS_OTP_ADDR_DSK1,
	TAURUS_OTP_ADDR_DSK2,
	TAURUS_OTP_ADDR_DSK3,
	TAURUS_OTP_ADDR_DSK4,
	TAURUS_OTP_ADDR_RK1,
	TAURUS_OTP_ADDR_RK2
};

static unsigned char *buf_array[CGCAS_MAX_KEY] = {
	s_info.DSK1,
	s_info.DSK2,
	s_info.DSK3,
	s_info.DSK4,
	s_info.RK1,
	s_info.RK2,
};

static int gx_fw_CGCAS_init_rootkey(void)
{
	int ret, i;
	unsigned int temp[4];
	GxTfmCrypto param = {0};
	param.module            = TFM_MOD_CRYPTO;
	param.module_sub         = 1;
	param.alg               = TFM_ALG_AES128;
	param.src.id            = TFM_SRC_MEM;
	param.output.buf        = (unsigned char *)temp;
	param.output.length     = 16;

	for (i = 0; i < CGCAS_MAX_KEY; i++) {
		gxsecure_otp_read_buf(buf_array[i], 16, addr_array[i], 16);
		param.input.buf = buf_array[i];
		param.input.length = 16;
		param.even_key.id  = TFM_KEY_TK;
		param.even_key.sub = 1;
		if (i < 4) {
			param.dst.id       = TFM_DST_REG;
			param.dst.sub      = i + 1;
		} else {
			param.dst.id       = TFM_DST_MEM;
			param.output.buf = buf_array[i];
			param.output.length = 16;
		}
		ret = gx_tfm_decrypt(&param);
	}

	return ret;
}


static int gx_fw_CGCAS_init(void)
{
	return gx_fw_CGCAS_init_rootkey();
}

static void gx_fw_CGCAS_start(void)
{
	gx_fw_debug_print(0x0);
}

static int gx_fw_CGCAS_console(GxSecurePacket *pkt)
{
	GxSeScpuKey key = {0};
	GxSecureResult result = {0};
	GxSecureUserKey *Userkey = (GxSecureUserKey *)pkt->data;

	switch (pkt->oob) {
	case MB_OOB_USER_TX:
		key.type = MISC_KEY_SCPU_PVR;
		key.length = 16;
		memcpy(key.value, buf_array[Userkey->id + 4], 16);
		gxse_misc_send_key(&key);
		result.type = FW_RET_RW_OK;
		gx_mbox_notice_acpu_result(&result);
		break;

	default:
		break;
	}
	return 0;
}

static int gx_fw_CGCAS_select_rootkey(GxTfmKeyLadder *param)
{
	if (param->key.id == TFM_KEY_PVRK)
		param->key.sub += 4;
	else if (param->key.id == TFM_KEY_CWUK)
		param->key.sub += 0;
	param->key.id = TFM_KEY_REG;
	param->key.sub = param->key.sub+1;
	return gx_tfm_klm_select_rootkey(param);
}

static int gx_fw_CGCAS_get_resp(GxTfmKeyLadder *param)
{
	if (param->key.id == TFM_KEY_PVRK)
		param->key.sub += 4;
	else if (param->key.id == TFM_KEY_CWUK)
		param->key.sub += 0;
	param->key.id = TFM_KEY_REG;
	param->key.sub = param->key.sub+1;
	return gx_tfm_klm_get_resp(param);
}
static GxFWOps CGCAS_fw_ops = {
	.init      = gx_fw_CGCAS_init,
	.start     = gx_fw_CGCAS_start,
	.console   = gx_fw_CGCAS_console,
	.select_rootkey = gx_fw_CGCAS_select_rootkey,
	.get_resp  = gx_fw_CGCAS_get_resp,
};

int gx_fw_register(GxFWOps **ops)
{
	*ops = &CGCAS_fw_ops;
	return 0;
}

