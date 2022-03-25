#include "gx_fw_hal.h"

static int key_init = 0;
unsigned int total_size;
unsigned int  send_pos = 0;
unsigned char *p_send = NULL;

//#define DEBUG_K3
static int gx_fw_TCAS_create_K3(GxSecurePacket *pkt)
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

static int gx_fw_TCAS_console(GxSecurePacket *pkt)
{
	int ret = 0;
	GxSecureResult result = {0};

	switch (pkt->oob) {
		case MB_OOB_USER_TX:
			ret = gx_fw_TCAS_create_K3(pkt);
			result.type = (ret == 0) ? FW_RET_RW_OK: FW_RET_RW_ERROR;
			gx_mbox_notice_acpu_result(&result);
			gx_mbox_notice_acpu_result(&result);
			break;

		default:
			break;
	}
	return ret;
}


static int gx_fw_TCAS_init(void)
{
	GxSeScpuKey soft_key = {
		.type = MISC_KEY_SCPU_SOFT,
		.length = 32,
		.value = {0x12, 0x34, 0x56, 0x78, 0x0, 0x0, 0x0, 0x0, 0x22, 0x34, 0x56, 0x78, 0x0, 0x0, 0x0, 0x0, 0x32, 0x34, 0x56, 0x78},
	};
	gxse_misc_send_key(&soft_key);

	GxSeScpuKey misc_key = {
		.type = MISC_KEY_SCPU_PVR,
		.length = 16,
		.value = {0x12, 0x34, 0x56, 0x78, 0x0, 0x0, 0x0, 0x0, 0x22, 0x34, 0x56, 0x78},
	};
	gxse_misc_send_key(&misc_key);
	return 0;
}

static void gx_fw_TCAS_start(void)
{
	gx_fw_debug_print(0x0);
	gx_fw_debug_print(0x1);
}


static unsigned char peek_data[1024];

static unsigned int peek_pos = 0;
static int gx_fw_TCAS_peek(unsigned char *buf, unsigned int size)
{
	if (buf == NULL || size <= 0 || size > 512)
		return -1;
	memcpy(((unsigned char *)&peek_data)+peek_pos, buf, size);
	peek_pos += size;
	return 0;
}

static int gx_fw_TCAS_peek_final()
{
	unsigned char uCrc = 0;
	unsigned int ret = 0;
	int i = 0;
	for (i = 0; i < peek_pos - 1; i++)
		uCrc ^= peek_data[i];
	gx_fw_debug_print(0x111000);
	gx_fw_debug_print(uCrc);
	gx_fw_debug_print(0x111000);
	if(uCrc != peek_data[peek_pos - 1])
		ret = -1;
	peek_pos = 0;
	return ret;
}

static int gx_fw_TCAS_send(unsigned char *buf, unsigned int *size)
{
	if (*size <= total_size - send_pos) {
		memcpy(buf, p_send + send_pos, *size);
		send_pos += *size;
	}else {
		return -1;
	}

	return 0;
}

static struct {
	int offset;
	int len;
} info_data;

static int gx_fw_TCAS_send_check(unsigned int size, unsigned char *data_info, unsigned int data_len)
{
	if (data_info == NULL || size >= 1024)
		return -1;
	total_size = size;
	send_pos = 0;
	memcpy(&info_data, data_info, data_len);
	if (size != info_data.len)
		return -2;
	p_send = peek_data + info_data.offset;
	return 0;
}
static GxFWOps TCAS_fw_ops = {
	.init      = gx_fw_TCAS_init,
	.start     = gx_fw_TCAS_start,
	.console   = gx_fw_TCAS_console,
	.mbox_peek           = gx_fw_TCAS_peek,
	.mbox_peek_final     = gx_fw_TCAS_peek_final,
	.mbox_send           = gx_fw_TCAS_send,
	.mbox_send_check     = gx_fw_TCAS_send_check,
};

int gx_fw_register(GxFWOps **ops)
{
	*ops = &TCAS_fw_ops;
	return 0;
}

