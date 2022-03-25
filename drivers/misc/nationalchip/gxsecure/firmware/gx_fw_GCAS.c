#include "gx_fw_hal.h"
#define SECURE_BOOT_FEATURE_ENABLE (0x10c)
#define ACPU_SECURE_BOOT_ENABLE (0x139)

#ifdef CFG_FW_PASSWD
static int secure_boot_flag = 1;
#endif
static int gx_fw_GCAS_init(void)
{
	// 永新需要固件和安全启动位绑定
#ifdef CFG_SECURE_BOOT
	int secure_boot_feature_enable = 0;
	int acpu_secure_boot_enable = 0;
	secure_boot_feature_enable = gxsecure_otp_read(SECURE_BOOT_FEATURE_ENABLE);
	acpu_secure_boot_enable = gxsecure_otp_read(ACPU_SECURE_BOOT_ENABLE);
	if (secure_boot_feature_enable == 0 || acpu_secure_boot_enable == 0) {
#ifdef CFG_FW_PASSWD
		secure_boot_flag = 0;
		return MB_SCPU_PASSWD;
#else
		return -1;
#endif
	}
#endif
	return 0;
}

static void gx_fw_GCAS_start(void)
{
#ifdef CFG_FW_PASSWD
	if (secure_boot_flag == 0)
		gx_fw_debug_print(FW_VAL_APP_PASSWD);
	else
#endif
		gx_fw_debug_print(0x0);
}

static GxFWOps GCAS_fw_ops = {
	.init      = gx_fw_GCAS_init,
	.start     = gx_fw_GCAS_start,
	.console   = NULL,
};

int gx_fw_register(GxFWOps **ops)
{
	*ops = &GCAS_fw_ops;
	return 0;
}

