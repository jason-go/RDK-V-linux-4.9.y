#include "gxse_core.h"
#include "gxse_hwobj_misc.h"

int gxsecure_otp_read(unsigned int addr)
{
	unsigned char buf[16] = {0};
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOtpBuf param = {0};
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_OTP);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_OTP);
		return GXSE_ERR_GENERIC;
	}

	param.addr = addr;
	param.buf = buf;
	param.size = 1;

	if (NULL == mod_ops(mod)->ioctl)
		return ret;

	ret = mod_ops(mod)->ioctl(mod, SECURE_OTP_READ, &param, sizeof(GxSecureOtpBuf));
	return ret < 0 ? ret : buf[0];
}

int gxsecure_otp_write(unsigned int addr, unsigned char byte)
{
	unsigned char buf[16] = {0};
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOtpBuf param = {0};
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_OTP);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_OTP);
		return GXSE_ERR_GENERIC;
	}

	buf[0] = byte;
	param.addr = addr;
	param.buf = buf;
	param.size = 1;

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_OTP_WRITE, &param, sizeof(GxSecureOtpBuf));

	return ret;
}

int gxsecure_otp_read_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOtpBuf param = {0};
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_OTP);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_OTP);
		return GXSE_ERR_GENERIC;
	}

	if (NULL == buf || buf_len < size) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter error\n");
		return GXSE_ERR_PARAM;
	}

	param.addr = addr;
	param.buf = buf;
	param.size = size;

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_OTP_READ, &param, sizeof(GxSecureOtpBuf));

	return ret;
}

int gxsecure_otp_write_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureOtpBuf param = {0};
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_OTP);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_OTP);
		return GXSE_ERR_GENERIC;
	}

	if (NULL == buf || buf_len < size) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter error\n");
		return GXSE_ERR_PARAM;
	}

	param.addr = addr;
	param.buf = buf;
	param.size = size;

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_OTP_WRITE, &param, sizeof(GxSecureOtpBuf));

	return ret;
}

int gxse_switch_multipin(GxSeMultipinStatus status)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_CHIP_CFG);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d\n", GXSE_MOD_MISC_CHIP_CFG);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, MISC_CHIP_SWITCH_MULTIPIN, &status, sizeof(GxSeMultipinStatus));

	return ret;
}

unsigned int gxse_rng_request(void)
{
	uint32_t param = 0;
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = NULL;

	if (NULL == (mod = gxse_module_find_by_id(GXSE_MOD_MISC_RNG))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_RNG);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_GET_RNG, &param, 4);

	return ret < 0 ? 0 : param;
}

#if defined (CPU_ACPU)
int gxsecure_otp_get_publicid(unsigned char *buf, unsigned int size)
{
#define GXOTP_GX3113C_CHIP_NAME_ADDR  (0x30c)
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureCSSN param = {.value={0}};
	GxSeModule *mod = NULL;

	if (NULL == (mod = gxse_module_find_by_id(GXSE_MOD_MISC_CHIP_CFG))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_CHIP_CFG);
		return GXSE_ERR_GENERIC;
	}

	if (NULL == buf || size < GXSE_MISC_CSSN_LEN) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter error %p %x\n", buf, size);
		return GXSE_ERR_PARAM;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_GET_CSSN, &param, sizeof(GxSecureCSSN));

	memcpy(buf, param.value, GXSE_MISC_CSSN_LEN);
	return ret;
}

int gxsecure_otp_get_chipname(unsigned char *buf, unsigned int size)
{
#define GXOTP_GX3113C_CHIP_NAME_ADDR  (0x30c)
	int32_t ret = GXSE_ERR_GENERIC;
	GxSecureChipName param = {.value={0}};
	GxSeModule *mod = NULL;

	if (CHIP_IS_GX3113C)
		return gxsecure_otp_read_buf(buf, size, GXOTP_GX3113C_CHIP_NAME_ADDR, 10);

	if (NULL == (mod = gxse_module_find_by_id(GXSE_MOD_MISC_CHIP_CFG))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_CHIP_CFG);
		return GXSE_ERR_GENERIC;
	}

	if (NULL == buf || size < GXSE_MISC_CHIPNAME_LEN) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter error\n");
		return GXSE_ERR_PARAM;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, SECURE_GET_CHIPNAME, &param, sizeof(GxSecureChipName));

	memcpy(buf, param.value, GXSE_MISC_CHIPNAME_LEN);
	return ret;
}

int gxsecure_otp_get_marketid(unsigned char *buf, unsigned int size)
{
#define GXOTP_SIRIUS_MARKETID_ADDR   (0xe0)
#define GXOTP_TAURUS_MARKETID_ADDR   (0x130)
#define GXOTP_GX3211_MARKETID_ADDR   (0x390)
#define GXOTP_GX6605S_MARKETID_ADDR  (0x17c)
#define GXOTP_GX3113C_MARKETID_ADDR  (0x348)

	unsigned int addr, len = 4;
	unsigned int chipid = gx_osdep_chip_probe();

	switch (chipid) {
	case GXSECURE_CHIPID_GX3211:
		addr = GXOTP_GX3211_MARKETID_ADDR;
		break;

	case GXSECURE_CHIPID_GX6605S:
		addr = GXOTP_GX6605S_MARKETID_ADDR;
		break;

	case GXSECURE_CHIPID_GX3113C:
		addr = GXOTP_GX3113C_MARKETID_ADDR;
		break;

	case GXSECURE_CHIPID_TAURUS:
	case GXSECURE_CHIPID_GEMINI:
		addr = GXOTP_TAURUS_MARKETID_ADDR;
		break;

	case GXSECURE_CHIPID_SIRIUS:
		addr = GXOTP_SIRIUS_MARKETID_ADDR;
		break;

	default:
		gxlog_d(GXSE_LOG_MOD_HAL, "Chip not supported\n");
		return GXSE_ERR_GENERIC;
	}

	return gxsecure_otp_read_buf(buf, size, addr, len);
}

int gxsecure_otp_get_macrovision_status(void)
{
	int ret = 0;
	int mv_en_1 = 0;
	int mv_en_2 = 0;

	unsigned int chipid = gx_osdep_chip_probe();
	switch (chipid) {
	case GXSECURE_CHIPID_GX3211:
		if ((ret = gxsecure_otp_read(0x500)) < 0)
			return ret;

		if ((ret & 0x80) == 0x80)
			mv_en_1 = 1;

		if ((ret = gxsecure_otp_read(0x312)) < 0)
			return ret;

		if ((ret & 0xFC) == 0xFC)
			mv_en_2 = 1;
		break;

	case GXSECURE_CHIPID_TAURUS:
	case GXSECURE_CHIPID_GEMINI:
		if ((ret = gxsecure_otp_read(0x12F)) < 0)
			return ret;

		if ((ret & 0xF0) == 0xF0)
			mv_en_1 = 1;

		if ((ret = gxsecure_otp_read(0x134)) < 0)
			return ret;

		if (ret == 0xFF)
			mv_en_2 = 1;
		break;

	case GXSECURE_CHIPID_SIRIUS:
		if ((ret = gxsecure_otp_read(0xCB)) < 0)
			return ret;

		if ((ret & 0xF0) == 0xF0)
			mv_en_1 = 1;

		if ((ret = gxsecure_otp_read(0xE4)) < 0)
			return ret;

		if (ret == 0xFF)
			mv_en_2 = 1;
		break;

	default:
		gxlog_d(GXSE_LOG_MOD_HAL, "Chip not supported\n");
		return GXSE_ERR_GENERIC;
	}
	if ((mv_en_1 == 1) && (mv_en_2 == 1))
		return 1;
	return 0;
}

int gxsecure_otp_get_hdr_status(void)
{
	int ret = 0;
	int hdr_en = 0;

	unsigned int chipid = gx_osdep_chip_probe();
	switch (chipid) {
	case GXSECURE_CHIPID_SIRIUS:
		if ((ret = gxsecure_otp_read(0xCC)) < 0)
			return ret;

		if ((ret & 0x0F) == 0x0F)
			hdr_en = 1;
		break;

	case GXSECURE_CHIPID_TAURUS:
		if ((ret = gxsecure_otp_read(0x123)) < 0)
			return ret;

		if ((ret & 0x0F) == 0x0F)
			hdr_en = 1;
		break;

	case GXSECURE_CHIPID_GEMINI:
		if ((ret = gxsecure_otp_read(0x125)) < 0)
			return ret;

		if ((ret & 0x0F) == 0x0F)
			hdr_en = 1;
		break;

	default:
		gxlog_d(GXSE_LOG_MOD_HAL, "Chip not supported\n");
		return GXSE_ERR_GENERIC;
	}
	return hdr_en;
}

int gxsecure_otp_get_hevc_status(void)
{
	int ret = 0, hevc_status = 0;
	int hevc = 0, hevc_10bit = 0;

	unsigned int chipid = gx_osdep_chip_probe();
	switch (chipid) {
	case GXSECURE_CHIPID_SIRIUS:
		if ((ret = gxsecure_otp_read(0xDB)) < 0)
			return ret;

		if ((ret & 0xF0) == 0xF0)
			hevc = 1;

		if ((ret = gxsecure_otp_read(0xDC)) < 0)
			return ret;
		if ((ret & 0x0F) == 0x0F)
			hevc_10bit = 1;
		break;

	case GXSECURE_CHIPID_TAURUS:
		if ((ret = gxsecure_otp_read(0x10C)) < 0)
			return ret;

		hevc_10bit = 1;

		if ((ret & 0xF0) == 0xF0)
			hevc = 1;
		break;

	case GXSECURE_CHIPID_GEMINI:
		if ((ret = gxsecure_otp_read(0x10C)) < 0)
			return ret;
		if ((ret & 0xF0) == 0xF0)
			hevc = 1;

		if ((ret = gxsecure_otp_read(0x124)) < 0)
			return ret;
		if ((ret & 0x0F) == 0x0F)
			hevc_10bit = 1;
		break;

	default:
		gxlog_d(GXSE_LOG_MOD_HAL, "Chip not supported\n");
		return GXSE_ERR_GENERIC;
	}

	if ((hevc == 0) && (hevc_10bit == 0))
		hevc_status = GXSE_HEVC_SUPPORT_10BIT |GXSE_HEVC_SUPPORT_8BIT;
	if ((hevc == 0) && (hevc_10bit == 1))
		hevc_status = GXSE_HEVC_SUPPORT_8BIT;
	return hevc_status;
}

#else
int gxsecure_otp_get_publicid(unsigned char *buf, unsigned int size)
{
#define GXOTP_SIRIUS_PUBLICID_ADDR   (0xb0)
#define GXOTP_TAURUS_PUBLICID_ADDR   (0x11a)
#define GXOTP_GX3211_PUBLICID_ADDR   (0x348)
#define GXOTP_GX6605S_PUBLICID_ADDR  (0x148)
#define GXOTP_GX3113C_PUBLICID_ADDR  (0x368)

	unsigned int addr, len = 8;
	unsigned int chipid = gx_osdep_chip_probe();

	switch (chipid) {
	case GXSECURE_CHIPID_GX3211:
		addr = GXOTP_GX3211_PUBLICID_ADDR;
		break;

	case GXSECURE_CHIPID_GX6605S:
		addr = GXOTP_GX6605S_PUBLICID_ADDR;
		break;

	case GXSECURE_CHIPID_GX3113C:
		addr = GXOTP_GX3113C_PUBLICID_ADDR;
		break;

	case GXSECURE_CHIPID_TAURUS:
	case GXSECURE_CHIPID_GEMINI:
		addr = GXOTP_TAURUS_PUBLICID_ADDR;
		break;

	case GXSECURE_CHIPID_SIRIUS:
		addr = GXOTP_SIRIUS_PUBLICID_ADDR;
		break;

	default:
		gxlog_e(GXSE_MOD_MISC, "chipid error\n");
		return -2;
	}

	return gxsecure_otp_read_buf(buf, size, addr, len);
}

int gxsecure_otp_get_fixed_klm_stage(GxTfmKeyBuf *key, int stage)
{
#ifdef CONFIG_TAURUS
	int addr = 0, pos = 0, ret = 0, val = 0, sub = 0;
	switch(key->id) {
	case TFM_KEY_CWUK:
		sub = key->sub & 0x3;
		addr = 0x145;
		break;
	case TFM_KEY_PVRK:
		sub = key->sub & 0x1;
		addr = 0x147;
		break;
	default:
		return 0;
	}

	addr += sub/2 + sub%2;
	pos  = (sub%2) ? 0 : 4;
	if ((ret = gxsecure_otp_read(addr)) < 0)
		return 0;

	val = (ret >> pos) & 0xf;
	switch (val) {
	case 0x0:
			return 0;
	case 0x2:
			return 2;
	case 0x4:
			return 3;
	case 0x7:
			return 4;
	case 0x8:
			return 5;
	case 0xb:
			return 6;
	case 0xd:
			return 7;
	case 0xe:
			if (stage == 5 || stage == 3)
				return stage;
			return 3;
	default:  return 8;
	}
#endif

	return 0;
}

void gxse_sensor_get_err_status (GxSeSensorErrStatus *param)
{
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_SENSOR);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_SENSOR);
		return;
	}

	if (mod_ops(mod)->ioctl)
		mod_ops(mod)->ioctl(mod, MISC_SENSOR_GET_STATUS, param, sizeof(GxSeSensorErrStatus));
}

unsigned int gxse_sensor_get_err_value (unsigned int status)
{
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_SENSOR);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_SENSOR);
		return 0;
	}

	if (mod_ops(mod)->ioctl)
		mod_ops(mod)->ioctl(mod, MISC_SENSOR_GET_VALUE, &status, 4);

	return status;
}

void gxse_misc_scpu_reset(void)
{
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_CHIP_CFG);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_CHIP_CFG);
		return;
	}

	if (mod_ops(mod)->ioctl)
		mod_ops(mod)->ioctl(mod, MISC_CHIP_SCPU_RESET, NULL, 0);

	return;
}

void gxse_misc_send_key(GxSeScpuKey *key)
{
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_CHIP_CFG);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_CHIP_CFG);
		return;
	}

	if (mod_ops(mod)->ioctl)
		mod_ops(mod)->ioctl(mod, MISC_CHIP_SEND_KEY, key, sizeof(GxSeScpuKey));

	return;
}

int gxse_misc_get_BIV(GxSecureImageVersion *biv)
{
	int ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_CHIP_CFG);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_CHIP_CFG);
		return GXSE_SUCCESS;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, MISC_CHIP_GET_BIV, biv, sizeof(GxSecureImageVersion));

	return GXSE_SUCCESS;
}

#endif // end of CPU_ACPU

#if defined (LINUX_OS)
EXPORT_SYMBOL(gxsecure_otp_read);
EXPORT_SYMBOL(gxsecure_otp_write);
EXPORT_SYMBOL(gxsecure_otp_read_buf);
EXPORT_SYMBOL(gxsecure_otp_write_buf);
EXPORT_SYMBOL(gxsecure_otp_get_macrovision_status);
EXPORT_SYMBOL(gxsecure_otp_get_hdr_status);
EXPORT_SYMBOL(gxsecure_otp_get_hevc_status);
#endif

