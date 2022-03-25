#include "gx_fw_hal.h"

//#define TDX_DBG
#define TAURUS_OTP_ADDR_SECTION3_LOCK 0x1df
#define TAURUS_OTP_ADDR_DSK1          0x160
#define TAURUS_OTP_ADDR_DSK2          0x170
#define TAURUS_OTP_ADDR_DSK3          0x180
#define TAURUS_OTP_ADDR_DSK4          0x190
#define TAURUS_OTP_ADDR_IK            0x1a0
#define TAURUS_OTP_ADDR_INFO          0x1d0

enum {
	TDXCAS_DSK1,
	TDXCAS_DSK2,
	TDXCAS_DSK3,
	TDXCAS_DSK4,
	TDXCAS_IK,
	TDXCAS_INFO,
	TDXCAS_MAX_KEY,
};

enum {
	TDXCAS_MODE_FORMAL,
	TDXCAS_MODE_CUSTOM,
};

typedef struct {
	unsigned int  key_init;
	unsigned char DSK1[16];
	unsigned char DSK2[16];
	unsigned char DSK3[16];
	unsigned char DSK4[16];
	unsigned char IK[16];
	unsigned char INFO[16];
	unsigned int  count;
} GxUserInfo;
static GxUserInfo s_info;

static unsigned int addr_array[TDXCAS_MAX_KEY] = {
	TAURUS_OTP_ADDR_DSK1,
	TAURUS_OTP_ADDR_DSK2,
	TAURUS_OTP_ADDR_DSK3,
	TAURUS_OTP_ADDR_DSK4,
	TAURUS_OTP_ADDR_IK,
	TAURUS_OTP_ADDR_INFO
};

static unsigned char *buf_array[TDXCAS_MAX_KEY] = {
	s_info.DSK1,
	s_info.DSK2,
	s_info.DSK3,
	s_info.DSK4,
	s_info.IK,
	s_info.INFO,
};

static int gx_fw_get_fw_mode(void)
{
#ifdef CFG_FW_CUSTOM
	return TDXCAS_MODE_CUSTOM; // Custom's FW requires Section 3 not to be locked
#else
	return TDXCAS_MODE_FORMAL; // The formal FW requires Section 3 to be locked
#endif
}

static int gx_fw_create_deckey(void)
{
	unsigned int temp[4];
	GxTfmCrypto param = {0};
	param.module            = TFM_MOD_CRYPTO;
	param.module_sub        = 1;
	param.alg               = TFM_ALG_AES128;
	param.even_key.id       = TFM_KEY_SCPU_SOFT;
	param.src.id            = TFM_SRC_MEM;

//	create key for decryption
//	07e993aa8c73a4c1 0d3b3b3f39179e6d
	unsigned char Edec_key[16] = {
		0x3E, 0x89, 0x92, 0x4C, 0x88, 0x98, 0x21, 0xE6,
		0x96, 0x41, 0xE0, 0x2D, 0x84, 0x2B, 0x9A, 0xBA,
	};

	memset(temp, 0, 16);

	param.input.buf = (unsigned char *)temp;
	param.input.length = 16;
	param.dst.id       = TFM_DST_REG;
	param.dst.sub      = 7;
	if (gx_tfm_decrypt(&param) < 0)
		return -1;

	param.input.buf = (unsigned char *)Edec_key;
	param.input.length = 16;
	param.even_key.id   = TFM_KEY_REG;
	param.even_key.sub  = 7;
	param.dst.id       = TFM_DST_REG;
	param.dst.sub      = 7;
	if (gx_tfm_decrypt(&param) < 0)
		return -1;

	return 0;

}

static int gx_fw_get_plain_key(int id, unsigned char *buffer, int len)
{
	int ret;
	unsigned int temp[4];
	GxTfmCrypto param = {0};
	param.module            = TFM_MOD_CRYPTO;
	param.module_sub        = 1;
	param.alg               = TFM_ALG_AES128;
	param.src.id            = TFM_SRC_MEM;
	param.output.buf       = (unsigned char *)temp;
	param.output.length    = 16;


#ifdef TDX_DBG
	gx_fw_debug_print(0x7770+id);
	gx_fw_debug_print((unsigned int)(buffer));
	gx_fw_debug_print(((unsigned int *)(buffer))[0]);
	gx_fw_debug_print(((unsigned int *)(buffer))[1]);
	gx_fw_debug_print(((unsigned int *)(buffer))[2]);
	gx_fw_debug_print(((unsigned int *)(buffer))[3]);
	gx_fw_debug_print(0x7777);
#endif

	param.input.buf        = (unsigned char *)buffer;
	param.input.length     = 16;
	param.even_key.id      = TFM_KEY_REG;
	param.even_key.sub     = 7;

	if (id == TDXCAS_IK) {
		param.dst.id       = TFM_DST_M2M;
		memcpy(temp, buffer, len);

	} else if (id == TDXCAS_INFO)
		param.dst.id  = TFM_DST_MEM;
	else {
		param.dst.id  = TFM_DST_REG;
		param.dst.sub = id + 1;
	}

	ret = gx_tfm_decrypt(&param);

	if (id == TDXCAS_INFO) {
		memcpy(buffer, temp, len);

	} else if (id == TDXCAS_IK) {
		GxSeScpuKey key = {0};

		memcpy(buffer, temp, len);
		param.dst.id  = TFM_DST_MEM;
		gx_tfm_decrypt(&param);

		key.type = MISC_KEY_SCPU_PVR;
		key.length = 16;
		key.value[0] = gx_endian_set(temp[0], 0);
		key.value[1] = gx_endian_set(temp[1], 0);
		key.value[2] = gx_endian_set(temp[2], 0);
		key.value[3] = gx_endian_set(temp[3], 0);
		gxse_misc_send_key(&key);
	}

#ifdef TDX_DBG
	if (id != TDXCAS_INFO &&
		id != TDXCAS_IK) {
		param.dst.id  = TFM_DST_MEM;
		ret = gx_tfm_decrypt(&param);
	}

	gx_fw_debug_print(0x9990+id);
	gx_fw_debug_print(temp[0]);
	gx_fw_debug_print(temp[1]);
	gx_fw_debug_print(temp[2]);
	gx_fw_debug_print(temp[3]);
	gx_fw_debug_print(0x9999);
#endif

	return ret;
}

static int gx_fw_write_user_key(int id, unsigned char *buffer, int len)
{
#ifdef TDX_DBG
	gx_fw_debug_print(0xcaf0+id);
	gx_fw_debug_print(((unsigned int *)buffer)[0]);
	gx_fw_debug_print(((unsigned int *)buffer)[1]);
	gx_fw_debug_print(((unsigned int *)buffer)[2]);
	gx_fw_debug_print(((unsigned int *)buffer)[3]);
	gx_fw_debug_print(0xcafe);
	memcpy(buf_array[id], buffer, len);
	return 0;
#endif

	if (id == TDXCAS_INFO) {
		memcpy(buf_array[id], buffer, len);
		return 0;
	}

	if (gxsecure_otp_write_buf(buffer, len, addr_array[id], len) < 0)
		return -1;

	return 0;
}

static int gx_fw_read_user_key(int id, unsigned char *buffer, int len)
{
#ifdef TDX_DBG
	memcpy(buffer, buf_array[id], len);
	return 0;
#endif
	if (id == TDXCAS_INFO) {
		memcpy(buffer, buf_array[id], len);
		return 0;
	}

	if (gxsecure_otp_read_buf(buffer, len, addr_array[id], len) < 0)
		return -1;

	return 0;
}
static int gx_fw_init_rootkey(void)
{
	int i;
	for (i = 0; i < TDXCAS_INFO; i++) {
		gx_fw_read_user_key(i, buf_array[i], 16);
		if (gx_fw_get_plain_key(i, buf_array[i], 16) < 0)
			return -1;
	}
	gx_mbox_notice_acpu_chip_serialized();
	return 0;
}

static int gx_fw_check_key_valid(GxSecureUserKey *key)
{
	unsigned int temp[4];
	GxTfmCrypto param = {0};
	param.module            = TFM_MOD_CRYPTO;
	param.module_sub        = 1;
	param.alg               = TFM_ALG_AES128;
	param.src.id            = TFM_SRC_MEM;
	param.output.buf       = (unsigned char *)temp;
	param.output.length    = 16;

	memset(temp, 0, 16);
	param.even_key.id       = TFM_KEY_REG;
	param.even_key.sub      = 7;
	param.dst.id            = TFM_DST_MEM;
	param.input.buf        = (unsigned char *)key->value;
	param.input.length     = 16;

	if (gx_tfm_decrypt(&param) < 0)
		return -1;

	if (key->check_val != temp[0])
		return -1;

	memcpy(key->value, temp, 16);
	return 0;
}

static void gx_fw_check_key_ready(void)
{
	if (s_info.count == TDXCAS_MAX_KEY) {
		gx_fw_init_rootkey();
#ifdef TDX_DBG
		gx_fw_debug_print(0x55aa);
#else
		gxsecure_otp_write(TAURUS_OTP_ADDR_SECTION3_LOCK, 0xf);
#endif
	}
}

static int gx_fw_check_otp_buf_writable(GxSecureUserKey *key, unsigned char *buffer, int len)
{
	int id = key->id;
	int unwritable = 0, i = 0;
	unsigned char empty_array[16];
	unsigned char *input = (unsigned char *)key->value;

	memset(empty_array, 0, len);
	if (id == TDXCAS_INFO) {
		if (memcmp(buffer, empty_array, len))
			return 0; // customid has been written

	} else {
		if (memcmp(buffer, empty_array, len)) {
			// already have some datas in otp area.
			unwritable = 1;
			if (memcmp(buffer, input, len)) {
				for (i = len-1; i >= 0; i--) {
					if (buffer[i] != 0 && buffer[i] != input[i])
						return FW_RET_DIFFERENT; // try to write different key
				}
			}
		}
	}

	if (unwritable) {
		memcpy(buf_array[id], buffer, len);
		s_info.count++;
		s_info.key_init |= 0x1<<id;
		gx_fw_check_key_ready();
		return FW_RET_SAME; // try to rewrite the same key
	}

	return 0;
}

static int gx_fw_TDX_set_customer_data(unsigned char *buffer, int len)
{
#define MAX_CUSTOMER_DATA_LEN 15
	int i = 0;
	unsigned char temp_buffer[16] = {0};
	unsigned char empty_array[16] = {0};

	len = buffer[1]+2;
	if (len > MAX_CUSTOMER_DATA_LEN)
		return FW_RET_CUSTOMID_ILLEGAL;

	if (gxsecure_otp_read_buf(temp_buffer, len, addr_array[TDXCAS_INFO], len) < 0)
		return -1;

	if (memcmp(temp_buffer, empty_array, len)) {
		if (memcmp(temp_buffer, buffer, len)) {
			for (i = len-1; i >= 0; i--) {
				if (temp_buffer[i] != 0 && temp_buffer[i] != buffer[i])
					return FW_RET_DIFFERENT; // try to write different key
			}
		}
		return 0; // customid has been written
	}
#ifdef TDX_DBG
	gx_fw_debug_print(0x66aa);
#else
	if (gxsecure_otp_write_buf(buffer, len, addr_array[TDXCAS_INFO], len) < 0)
		return -1;
#endif

	return 0;
}

static int gx_fw_TDX_get_customer_data(unsigned char *buffer, int len)
{
	int ret, addr = addr_array[TDXCAS_INFO];

	if ((ret = gxsecure_otp_read(addr)) < 0)
		return -1;
	buffer[0] = ret&0xff;

	if ((ret = gxsecure_otp_read(addr+1)) < 0)
		return -1;
	buffer[1] = ret&0xff;

	if (buffer[1] + 2 > len)
		return -1;

	if (gxsecure_otp_read_buf(buffer+2, buffer[1], addr+2, buffer[1]) < 0)
		return -1;

	return (buffer[1] == 0) ? 0 : buffer[1]+2;
}

static int gx_fw_TDX_set_user_key(GxSecurePacket *pkt)
{
	unsigned char temp_array[16];
	GxSecureUserKey *key = (GxSecureUserKey *)pkt->data;
	int id = key->id;
	int len = 16, ret = 0;

	if (gx_fw_get_fw_mode() == TDXCAS_MODE_FORMAL)
		return FW_RET_FUNC_DISABLE;

	if (gxsecure_otp_read(TAURUS_OTP_ADDR_SECTION3_LOCK))
		return FW_RET_SERIALIZED;

	if (gx_fw_check_key_valid(key) < 0)
		return FW_RET_ILLEGAL;

	if (s_info.key_init & (0x1<<id) || id >= TDXCAS_MAX_KEY)
		return FW_RET_SAME;

	if (gx_fw_read_user_key(id, temp_array, len) < 0)
		return -1;

	if ((ret = gx_fw_check_otp_buf_writable(key, temp_array, len)) != 0)
		return ret;

	if (gx_fw_write_user_key(id, (void *)key->value, len) < 0)
		return -1;

	if (gx_fw_read_user_key(id, temp_array, len) < 0)
		return -1;

	if (memcmp(temp_array, key->value, len))
		return -1;

	if (id == TDXCAS_INFO) {
		gx_fw_read_user_key(id, buf_array[id], 16);
		if (gx_fw_get_plain_key(id, buf_array[id], 16) < 0)
			return -1;

		if ((ret = gx_fw_TDX_set_customer_data(buf_array[id], 16)) != 0)
			return ret;
	}
	s_info.count++;
	s_info.key_init |= 0x1<<id;
	gx_fw_check_key_ready();

	return 0;
}

static int gx_fw_TDXCAS_init(void)
{
	if (gx_fw_create_deckey() < 0)
		return -1;

	if (gxsecure_otp_read(TAURUS_OTP_ADDR_SECTION3_LOCK) == 0) {
		if (gx_fw_get_fw_mode() == TDXCAS_MODE_CUSTOM)
			return 0;
		return -1;
	}

	return gx_fw_init_rootkey();
}

static void gx_fw_TDXCAS_start(void)
{
	if (gx_fw_get_fw_mode() == TDXCAS_MODE_CUSTOM)
		gx_fw_debug_print(FW_VAL_APP_CUSTOM);
	else
		gx_fw_debug_print(FW_VAL_APP_FORMAL);
}

static int gx_fw_TDXCAS_console(GxSecurePacket *pkt)
{
	int ret = 0;
	GxSecureResult result = {0};

	switch (pkt->oob) {
	case MB_OOB_USER_TX:
		ret = gx_fw_TDX_set_user_key(pkt);
		result.type = (ret == 0) ? FW_RET_RW_OK : FW_RET_RW_ERROR;
		result.len = 1;
		result.buffer[0] = ret;
		gx_mbox_notice_acpu_result(&result);
		break;

	case MB_OOB_USER_RX:
		ret = gx_fw_TDX_get_customer_data(result.buffer, 32);
		result.type = (ret >= 0) ? FW_RET_RW_OK : FW_RET_RW_ERROR;
		result.len = ret;
		gx_mbox_notice_acpu_result(&result);
		break;

	default:
		break;
	}
	return ret;
}

static int gx_fw_TDXCAS_select_rootkey(GxTfmKeyLadder *param)
{
	if (param->key.id != TFM_KEY_CWUK)
		param->key.sub = 0;
	param->key.id = TFM_KEY_REG;
	param->key.sub = param->key.sub+1;
	return gx_tfm_klm_select_rootkey(param);
}

static int gx_fw_TDXCAS_get_resp(GxTfmKeyLadder *param)
{
	if (param->key.id != TFM_KEY_CWUK)
		param->key.sub = 0;
	param->key.id = TFM_KEY_REG;
	param->key.sub = param->key.sub+1;
	return gx_tfm_klm_get_resp(param);
}

static GxFWOps TDXCAS_fw_ops = {
	.init      = gx_fw_TDXCAS_init,
	.start     = gx_fw_TDXCAS_start,
	.console   = gx_fw_TDXCAS_console,
	.select_rootkey = gx_fw_TDXCAS_select_rootkey,
	.get_resp  = gx_fw_TDXCAS_get_resp,
};

int gx_fw_register(GxFWOps **ops)
{
	*ops = &TDXCAS_fw_ops;
	memset(&s_info, 0, sizeof(GxUserInfo));
	return 0;
}

