#include "../gxmisc_virdev.h"
#include "sirius_mpu.h"
#include "kernelcalls.h"

int gx_mpu_isr(void)
{
	int mode, auth;
	int hprot1 = readl(GXSCPU_BASE_ADDR_MPU + MPU_REG_FAULT_ATTR) & mMPU_FAULT_ATTR_HPROT1;
	int hprot0 = readl(GXSCPU_BASE_ADDR_MPU + MPU_REG_FAULT_ATTR) & mMPU_FAULT_ATTR_HPROT0;
	int hwrite = readl(GXSCPU_BASE_ADDR_MPU + MPU_REG_FAULT_ATTR) & mMPU_FAULT_ATTR_HWRITE;

	char mode_print[2][16] = {"user", "super"};
	char auth_print[3][16] = {"write", "read", "exec"};

	(void)mode_print;
	(void)auth_print;

	if (readl(GXSCPU_BASE_ADDR_MPU + MPU_REG_FAULT_ATTR) & mMPU_FAULT_ATTR_FLAG) {
		mode = (hprot1) ? 1 : 0;
		auth = (hprot0) ? ((hwrite) ? 0 : 1) : 2;

	//	gx_printf(MPU, ERROR, "mpu %s mode %s error, addr : 0x%x\n",
	//			mode_print[mode],
	//			auth_print[auth],
	//			readl(GXSCPU_BASE_ADDR_MPU + MPU_REG_FAULT_ADDR));
	}
	return 0;
}
