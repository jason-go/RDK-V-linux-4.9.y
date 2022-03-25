#include "gx_fw_hal.h"
static int key_init = 0;

//#define DEBUG_K3

static int gx_fw_DCAS_create_K3(GxSecurePacket *pkt)
{
	int ret;
	unsigned int vendor_array[4];
	GxTfmCrypto param = {
		.module = TFM_MOD_CRYPTO,
		.module_sub = 1,
		.alg = TFM_ALG_SM4,
		.opt = TFM_OPT_ECB,
		.src.id = TFM_SRC_MEM,
		.dst.id = TFM_DST_REG,
		.dst.sub = 0,
		.even_key.id = TFM_KEY_CWUK,
		.even_key.sub = 0,
		.output.buf = (unsigned char *)vendor_array,
		.output.length = sizeof(vendor_array),
		.flags = TFM_FLAG_BIG_ENDIAN,
	};

	memcpy(&vendor_array, pkt->data+1, 16);

	param.input.buf = (unsigned char *)vendor_array;
	param.input.length = sizeof(vendor_array);
	param.dst.id = TFM_DST_REG;
	param.dst.sub = 0;
	ret = gx_tfm_decrypt(&param);

	param.even_key.id = TFM_KEY_SMK;
	param.even_key.sub = 0;
	param.dst.id = TFM_DST_REG;
	param.dst.sub = 1;
	ret = gx_tfm_decrypt(&param);

	param.even_key.id = TFM_KEY_REG;
	param.even_key.sub = 0;
	param.dst.id = TFM_DST_REG;
	param.dst.sub = 7;
	param.src.id = TFM_SRC_REG;
	param.src.sub = 1;
	ret = gx_tfm_decrypt(&param);
	if (ret == 0)
		key_init = 1;


#ifdef DEBUG_K3
	param.even_key.id = TFM_KEY_REG;
	param.even_key.sub = 0;
	param.dst.id = TFM_DST_MEM;
	param.dst.sub = 0;
	param.src.id = TFM_SRC_REG;
	param.src.sub = 1;
	ret = gx_tfm_decrypt(&param);
	gx_fw_debug_print(0xff1100);
	gx_fw_debug_print(vendor_array[0]);
	gx_fw_debug_print(vendor_array[1]);
	gx_fw_debug_print(vendor_array[2]);
	gx_fw_debug_print(vendor_array[3]);
	gx_fw_debug_print(0xff1100);
#endif

	return ret;
}

static int gx_fw_DCAS_console(GxSecurePacket *pkt)
{
	int ret = 0;
	GxSecureResult result = {0};

	switch (pkt->oob) {
		case MB_OOB_USER_TX:
			ret = gx_fw_DCAS_create_K3(pkt);
			result.type = (ret == 0) ? FW_RET_RW_OK : FW_RET_RW_ERROR;
			gx_mbox_notice_acpu_result(&result);
			break;

		default:
			break;
	}
	return ret;
}

static int gx_fw_DCAS_select_rootkey(GxTfmKeyLadder *param)
{
	if (key_init) {
		param->key.id = TFM_KEY_REG;
		param->key.sub = 7;
	}
	return gx_tfm_klm_select_rootkey(param);
}

static int gx_fw_DCAS_get_resp(GxTfmKeyLadder *param)
{
	if (key_init) {
		param->key.id = TFM_KEY_REG;
		param->key.sub = 7;
	}
	return gx_tfm_klm_get_resp(param);
}

static int gx_fw_DCAS_init(void)
{
	return 0;
}

static void gx_fw_DCAS_start(void)
{
	gx_fw_debug_print(0x0);
}

static GxFWOps DCAS_fw_ops = {
	.init      = gx_fw_DCAS_init,
	.start     = gx_fw_DCAS_start,
	.console   = gx_fw_DCAS_console,
	.select_rootkey = gx_fw_DCAS_select_rootkey,
	.get_resp  = gx_fw_DCAS_get_resp,
};

int gx_fw_register(GxFWOps **ops)
{
	*ops = &DCAS_fw_ops;
	return 0;
}

