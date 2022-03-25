#include "gx_fw_hal.h"

#define MKCAS_DBG
#define TAURUS_OTP_ADDR_SECTION3_LOCK 0x1df
#define TAURUS_OTP_ADDR_DSK1          0x160
#define TAURUS_OTP_ADDR_DSK2          0x170
#define TAURUS_OTP_ADDR_DSK3          0x180
#define TAURUS_OTP_ADDR_DSK4          0x190
#define TAURUS_OTP_ADDR_IK            0x1a0

enum {
	MKCAS_DSK1,
	MKCAS_DSK2,
	MKCAS_DSK3,
	MKCAS_DSK4,
	MKCAS_IK,
	MKCAS_MAX_KEY,
};

enum {
	MKCAS_MODE_FORMAL,
	MKCAS_MODE_CUSTOM,
};

typedef struct {
	unsigned int  key_init;
	unsigned char DSK1[16];
	unsigned char DSK2[16];
	unsigned char DSK3[16];
	unsigned char DSK4[16];
	unsigned char IK[16];
	unsigned int  count;
} GxUserInfo;
static GxUserInfo s_info;

static unsigned int addr_array[MKCAS_MAX_KEY] = {
	TAURUS_OTP_ADDR_DSK1,
	TAURUS_OTP_ADDR_DSK2,
	TAURUS_OTP_ADDR_DSK3,
	TAURUS_OTP_ADDR_DSK4,
	TAURUS_OTP_ADDR_IK
};

static unsigned char *buf_array[MKCAS_MAX_KEY] = {
	s_info.DSK1,
	s_info.DSK2,
	s_info.DSK3,
	s_info.DSK4,
	s_info.IK,
};

static int gx_fw_MKCAS_get_fw_mode(void)
{
#ifdef CFG_FW_CUSTOM
	return MKCAS_MODE_CUSTOM; // Custom's FW requires Section 3 not to be locked
#else
	return MKCAS_MODE_FORMAL; // The formal FW requires Section 3 to be locked
#endif
}

#ifdef MKCAS_DBG
static int gx_fw_write_user_key(int id, unsigned char *buffer, int len)
{
#ifdef MKCAS_DBG
	gx_fw_debug_print(0xcae0+id);
	gx_fw_debug_print(((unsigned int *)buffer)[0]);
	gx_fw_debug_print(((unsigned int *)buffer)[1]);
	gx_fw_debug_print(((unsigned int *)buffer)[2]);
	gx_fw_debug_print(((unsigned int *)buffer)[3]);
	gx_fw_debug_print(0xcaee);
	memcpy(buf_array[id], buffer, len);
	return 0;
#endif

	if (gxsecure_otp_write_buf(buffer, len, addr_array[id], len) < 0)
		return -1;

	return 0;
}

static int gx_fw_read_user_key(int id, unsigned char *buffer, int len)
{
#ifdef MKCAS_DBG
	memcpy(buffer, buf_array[id], len);
	return 0;
#endif
	if (gxsecure_otp_read_buf(buffer, len, addr_array[id], len) < 0)
		return -1;

	return 0;
}
#endif

static int gx_fw_MKCAS_create_deckey(void)
{
	unsigned int temp[4];
	GxTfmCrypto param = {0};
	param.module            = TFM_MOD_CRYPTO;
	param.module_sub        = 1;
	param.alg               = TFM_ALG_AES128;
	param.even_key.id       = TFM_KEY_SCPU_SOFT;
	param.src.id            = TFM_SRC_MEM;

//	create key for decryption
//	b8ca99d42a5609a1 bb9fe15d50f63767
	unsigned char Edec_key[16] = {
		0x86, 0xF1, 0xA5, 0xDC, 0x8C, 0x48, 0xE1, 0x7D,
		0x96, 0xBA, 0x64, 0x5C, 0x40, 0x21, 0x41, 0x46,
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

static int gx_fw_MKCAS_init_rootkey(void)
{
	int ret, i;
	unsigned int temp[4];
	GxTfmCrypto param = {0};
	param.module            = TFM_MOD_CRYPTO;
	param.module_sub        = 1;
	param.alg               = TFM_ALG_AES128;
	param.even_key.id       = TFM_KEY_SCPU_SOFT;
	param.src.id            = TFM_SRC_MEM;
	param.output.buf       = (unsigned char *)temp;
	param.output.length    = 16;

	for (i = 0; i < MKCAS_MAX_KEY; i++) {
#ifdef MKCAS_DBG
		gx_fw_debug_print(0x7770+i);
		gx_fw_debug_print((unsigned int)(buf_array[i]));
		gx_fw_debug_print(((unsigned int *)(buf_array[i]))[0]);
		gx_fw_debug_print(((unsigned int *)(buf_array[i]))[1]);
		gx_fw_debug_print(((unsigned int *)(buf_array[i]))[2]);
		gx_fw_debug_print(((unsigned int *)(buf_array[i]))[3]);
		gx_fw_debug_print(0x7777);
#else
		gxsecure_otp_read_buf(buf_array[i], 16, addr_array[i], 16);
#endif
		param.even_key.id   = TFM_KEY_REG;
		param.even_key.sub  = 7;
		param.input.buf = buf_array[i];
		param.input.length = 16;

		if (i == MKCAS_IK)
			param.dst.id  = TFM_DST_M2M;
		else {
			param.dst.id  = TFM_DST_REG;
			param.dst.sub = i + 1;
		}
		ret = gx_tfm_decrypt(&param);
#ifdef MKCAS_DBG
		param.dst.id  = TFM_DST_MEM;
		ret = gx_tfm_decrypt(&param);
		gx_fw_debug_print(0x9990+i);
		gx_fw_debug_print(temp[0]);
		gx_fw_debug_print(temp[1]);
		gx_fw_debug_print(temp[2]);
		gx_fw_debug_print(temp[3]);
		gx_fw_debug_print(0x9999);
#endif
	}
	return ret;
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
#if 0
	gx_fw_debug_print(0x201902);
	gx_fw_debug_print(temp[0]);
	gx_fw_debug_print(temp[1]);
	gx_fw_debug_print(temp[2]);
	gx_fw_debug_print(temp[3]);
	gx_fw_debug_print(0x201902);
#endif
	memcpy(key->value, temp, 16);
	return 0;
}

static void gx_fw_check_key_ready(void)
{
	if (s_info.count == MKCAS_MAX_KEY) {
#ifdef MKCAS_DBG
		gx_fw_debug_print(0xaaff);
#else
		gxsecure_otp_write(TAURUS_OTP_ADDR_SECTION3_LOCK, 0xf);
#endif
		gx_fw_MKCAS_init_rootkey();
	}
}

static int gx_fw_MKCAS_recv_key(GxSecurePacket *pkt)
{
	unsigned char temp_array[16];
	unsigned char empty_array[16];
	GxSecureUserKey *key = (GxSecureUserKey *)pkt->data;
	int id = key->id;

	if (gx_fw_MKCAS_get_fw_mode() == MKCAS_MODE_FORMAL)
		return -1;

	if (gxsecure_otp_read(TAURUS_OTP_ADDR_SECTION3_LOCK))
		return -1;

	if (gx_fw_check_key_valid(key) < 0)
		return -1;

	if (s_info.key_init & (0x1<<id) || id >= MKCAS_MAX_KEY)
		return -1;

#ifdef MKCAS_DBG
	gx_fw_read_user_key(id, temp_array, 16);
#else
	if (gxsecure_otp_read_buf((unsigned char *)temp_array, 16, addr_array[id], 16) < 0)
		return -1;
#endif

	memset(empty_array, 0, 16);
	if (memcmp(temp_array, empty_array, 16) &&
		memcmp(temp_array, key->value, 16) == 0) {
		memcpy(buf_array[id], temp_array, 16);
		s_info.count++;
		s_info.key_init |= 0x1<<id;
		gx_fw_check_key_ready();
		return -1;
	}

#ifdef MKCAS_DBG
	gx_fw_write_user_key(id, (void *)key->value, 16);
#else
	if (gxsecure_otp_write_buf((unsigned char *)key->value, 16, addr_array[id], 16) < 0)
		return -1;
#endif


#ifdef MKCAS_DBG
	gx_fw_read_user_key(id, temp_array, 16);
#else
	if (gxsecure_otp_read_buf((unsigned char *)temp_array, 16, addr_array[id], 16) < 0)
		return -1;
#endif

	if (memcmp(temp_array, key->value, 16))
		return -1;

	memcpy(buf_array[id], key->value, 16);
#if 0
	gx_fw_debug_print(0xcaf0+key->id);
	gx_fw_debug_print((unsigned int)(buf_array[key->id]));
	gx_fw_debug_print(((unsigned int *)(buf_array[key->id]))[0]);
	gx_fw_debug_print(((unsigned int *)(buf_array[key->id]))[1]);
	gx_fw_debug_print(((unsigned int *)(buf_array[key->id]))[2]);
	gx_fw_debug_print(((unsigned int *)(buf_array[key->id]))[3]);
	gx_fw_debug_print(0xcafe);
#endif
	s_info.count++;
	s_info.key_init |= 0x1<<id;
	gx_fw_check_key_ready();

	return 0;
}

static int gx_fw_MKCAS_init(void)
{
	if (gx_fw_MKCAS_create_deckey() < 0)
		return -1;

	if (gxsecure_otp_read(TAURUS_OTP_ADDR_SECTION3_LOCK) == 0) {
		if (gx_fw_MKCAS_get_fw_mode() == MKCAS_MODE_CUSTOM)
			return 0;
		return -1;
	}

	return gx_fw_MKCAS_init_rootkey();
}

static void gx_fw_MKCAS_start(void)
{
	if (gx_fw_MKCAS_get_fw_mode() == MKCAS_MODE_CUSTOM)
		gx_fw_debug_print(0xc);
	else
		gx_fw_debug_print(0x0);
}

static int gx_fw_MKCAS_console(GxSecurePacket *pkt)
{
	int ret;
	GxSecureResult result = {0};

	switch (pkt->oob) {
	case MB_OOB_USER_TX:
		ret = gx_fw_MKCAS_recv_key(pkt);
		result.type = (ret == 0) ? FW_RET_RW_OK : FW_RET_RW_ERROR;
		gx_mbox_notice_acpu_result(&result);
		break;

	default:
		break;
	}
	return 0;
}

static int gx_fw_MKCAS_select_rootkey(GxTfmKeyLadder *param)
{
	if (param->key.id != TFM_KEY_CWUK)
		param->key.sub = 0;
	param->key.id = TFM_KEY_REG;
	param->key.sub = param->key.sub+1;
	return gx_tfm_klm_select_rootkey(param);
}

static int gx_fw_MKCAS_get_resp(GxTfmKeyLadder *param)
{
	if (param->key.id != TFM_KEY_CWUK)
		param->key.sub = 0;
	param->key.id = TFM_KEY_REG;
	param->key.sub = param->key.sub+1;
	return gx_tfm_klm_get_resp(param);
}

static GxFWOps MKCAS_fw_ops = {
	.init      = gx_fw_MKCAS_init,
	.start     = gx_fw_MKCAS_start,
	.console   = gx_fw_MKCAS_console,
	.select_rootkey = gx_fw_MKCAS_select_rootkey,
	.get_resp  = gx_fw_MKCAS_get_resp,
};

int gx_fw_register(GxFWOps **ops)
{
	*ops = &MKCAS_fw_ops;
	memset(&s_info, 0, sizeof(GxUserInfo));
	return 0;
}

