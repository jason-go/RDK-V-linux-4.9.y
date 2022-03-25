#include "secure_hal.h"
#include "kernelcalls.h"
#include "gxav_common.h"
#include "log_printf.h"

#ifdef CONFIG_AV_TEE_MODULE_SECURE
#include "hal/property.h"
int gxav_secure_encrypt(GxTfmCrypto *param)
{
	struct avhal_secure_property_encrypt data = { 0 };

	memcpy(&data.param, param, sizeof(data.param));
	gxav_tee_ioctl(AVHAL_SECURE_ENCRYPT, &data, sizeof(data));

	return data.ret;
}

int gxav_secure_decrypt(GxTfmCrypto *param)
{
	struct avhal_secure_property_decrypt data = { 0 };

	memcpy(&data.param, param, sizeof(data.param));
	gxav_tee_ioctl(AVHAL_SECURE_DECRYPT, &data, sizeof(data));

	return data.ret;
}

int gxav_secure_otp_read(unsigned int addr)
{
	struct avhal_secure_property_otp_read data = { .addr = addr };

	gxav_tee_ioctl(AVHAL_SECURE_OTP_READ, &data, sizeof(data));

	return data.ret;
}

int gxav_secure_otp_write(unsigned int addr, unsigned char byte)
{
	gx_printf("[error] %s() Only work in TEE!\n", __func__);
	return 0;
}

int gxav_secure_otp_read_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size)
{
	gx_printf("[error] %s() Only work in TEE!\n", __func__);
	return 0;
}

int gxav_secure_otp_write_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size)
{
	gx_printf("[error] %s() Only work in TEE!\n", __func__);
	return 0;
}

int gxav_secure_config_filter(int addr, int size, int master_rd_permission, int master_wr_permission)
{
	gx_printf("[error] %s() Only work in TEE!\n", __func__);
	return 0;
}

int gxav_secure_get_protect_buffer(void)
{
	struct avhal_secure_property_get_protect_buffer data = { 0 };

	gxav_tee_ioctl(AVHAL_SECURE_GET_PROTECT_BUFFER, &data, sizeof(data));

	return data.ret;
}

int gxav_secure_query_access_align(void)
{
	struct avhal_secure_property_query_access_align data = { 0 };

	gxav_tee_ioctl(AVHAL_SECURE_QUERY_ACCESS_ALIGN, &data, sizeof(data));

	return data.ret;
}

int gxav_secure_query_protect_align(void)
{
	struct avhal_secure_property_query_protect_align data = { 0 };

	gxav_tee_ioctl(AVHAL_SECURE_QUERY_PROTECT_ALIGN, &data, sizeof(data));

	return data.ret;
}

int gxav_secure_set_hdcp_state(int state)
{
	struct avhal_secure_property_hdcp_state data = { 0, .ret = state };

	gxav_tee_ioctl(AVHAL_SECURE_SET_HDCP_STATE, &data, sizeof(data));

	return 0;
}

int gxav_secure_register_read(unsigned int base, unsigned int offset)
{
	struct avhal_secure_property_register_read data = { 0, 0, base, offset};

	gxav_tee_ioctl(AVHAL_SECURE_REGISTER_READ, &data, sizeof(data));

	return data.ret;
}

int gxav_secure_register_write(unsigned int base, unsigned int offset, unsigned value)
{
	struct avhal_secure_property_register_write data = { 0, 0, base, offset, value};

	gxav_tee_ioctl(AVHAL_SECURE_REGISTER_WRITE, &data, sizeof(data));

	return 0;
}

int gxav_secure_get_macrovision_status(void)
{
	struct avhal_secure_property_get_macrovision data = { 0, 0};

	gxav_tee_ioctl(AVHAL_SECURE_GET_MACROVISION_STATUS, &data, sizeof(data));

	return data.ret;
}

int gxav_secure_get_hdr_status(void)
{
	struct avhal_secure_property_get_hdr data = { 0, 0};

	gxav_tee_ioctl(AVHAL_SECURE_GET_HDR_STATUS, &data, sizeof(data));

	return data.ret;
}

int gxav_secure_get_hevc_status(void)
{
	struct avhal_secure_property_get_hevc data = { 0, 0};

	gxav_tee_ioctl(AVHAL_SECURE_GET_HEVC_STATUS, &data, sizeof(data));

	return data.ret;
}

#else

int gxav_secure_encrypt(GxTfmCrypto *param)
{
	return gx_tfm_encrypt(param);
}

int gxav_secure_decrypt(GxTfmCrypto *param)
{
	return gx_tfm_decrypt(param);
}

int gxav_secure_otp_read(unsigned int addr)
{
	int ret = gxsecure_otp_read(addr);

	if (ret != -1)
		return ret;

	gxlog_e(LOG_SCU, "(%s) error \n", __func__);
	return 0;
}

int gxav_secure_otp_write(unsigned int addr, unsigned char byte)
{
	int ret = gxsecure_otp_write(addr, byte);

	if (ret != -1)
		return ret;

	gxlog_e(LOG_SCU, "(%s) error \n", __func__);
	return 0;
}

int gxav_secure_otp_read_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size)
{
	int ret = gxsecure_otp_read_buf(buf, buf_len, addr, size);

	if (ret != -1)
		return ret;

	gxlog_e(LOG_SCU, "(%s) error \n", __func__);
	return 0;
}

int gxav_secure_otp_write_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size)
{
	int ret = gxsecure_otp_write_buf(buf, buf_len, addr, size);

	if (ret != -1)
		return ret;

	gxlog_e(LOG_SCU, "(%s) error \n", __func__);
	return 0;
}

int gxav_secure_get_macrovision_status(void)
{
	static int macrovision_status = 0xFF;
	if (macrovision_status == 0xFF) {
		macrovision_status = gxsecure_otp_get_macrovision_status();
		if (macrovision_status == -1) {
			macrovision_status = 0;
			gxlog_e(LOG_SCU, "(%s) error \n", __func__);
		}
	}
	return macrovision_status;
}

int gxav_secure_get_hdr_status(void)
{
	static int hdr_status = 0xFF;
	if (hdr_status == 0xFF) {
		hdr_status = gxsecure_otp_get_hdr_status();
		if (hdr_status == -1) {
			hdr_status = 0;
			gxlog_e(LOG_SCU, "(%s) error \n", __func__);
		}
	}
	return hdr_status;
}

int gxav_secure_get_hevc_status(void)
{
	static int hevc_status = 0xFF;
	if (hevc_status == 0xFF) {
		hevc_status = gxsecure_otp_get_hevc_status();
		if (hevc_status == -1) {
			if ((CHIP_IS_SIRIUS || CHIP_IS_GEMINI || CHIP_IS_CYGNUS))
				hevc_status = GXAV_HEVC_SUPPORT_8BIT | GXAV_HEVC_SUPPORT_10BIT;
			if ((CHIP_IS_TAURUS || CHIP_IS_GX3211))
				hevc_status = GXAV_HEVC_SUPPORT_8BIT;
			gxlog_e(LOG_SCU, "(%s) error \n", __func__);
		}
	}
	return hevc_status;
}

int gxav_secure_config_filter(int addr, int size, int master_rd_permission, int master_wr_permission)
{
	return gx_firewall_config_filter(addr, size, master_rd_permission, master_wr_permission, 0);
}
int gxav_secure_get_protect_buffer(void)
{
	return gx_firewall_get_protect_buffer();
}
int gxav_secure_query_access_align(void)
{
	return gx_firewall_query_access_align();
}
int gxav_secure_query_protect_align(void)
{
	return gx_firewall_query_protect_align();
}

static int _hdcp_state = AV_HDCP_IDLE;
int gxav_secure_set_hdcp_state(int state)
{
	_hdcp_state = state;

	return 0;
}

int gxav_secure_get_hdcp_state(void)
{
	return _hdcp_state;
}

int gxav_secure_register_read(unsigned int base, unsigned int offset)
{
	return 0;
}

int gxav_secure_register_write(unsigned int base, unsigned int offset, unsigned value)
{
#ifdef TEE_OS
	if (base == 0x8a200000) {
		extern int gx_demux_register_write(unsigned int offset, unsigned int value);
		return gx_demux_register_write(offset, value);
	}
#endif
	return 0;
}

#endif

