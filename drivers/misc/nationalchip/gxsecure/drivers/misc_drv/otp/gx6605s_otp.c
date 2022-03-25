#include "../gxmisc_virdev.h"
#include "taurus_otp_reg.h"


static struct mutex_static_priv gx6605s_otp_priv;
static void sec_write(unsigned int addr, unsigned int value, unsigned int delay)
{
#ifdef ARCH_ARM
	(void)delay;
	*(volatile unsigned int *)addr = value;
#else
	__asm__ __volatile("  \
			sync                \n\
			br      2f          \n\
			1:                  \n\
			subi    %2, %2, 1   \n\
			2:                  \n\
			cmpnei  %2, 0       \n\
			bt      1b          \n\
			sync                \n\
			st      %1, (%0, 0) \n\
			sync                \n\
			":
			:"r"(addr),"r"(value),"r"(delay)
			:
			);
#endif
}

static unsigned int sec_read(unsigned int addr, unsigned int delay)
{
	unsigned int rdata;
#ifdef ARCH_ARM
	(void)delay;
	rdata = *(volatile unsigned int *)addr;
#else
	__asm__ __volatile("  \
			sync                \n\
			br      2f          \n\
			1:                  \n\
			subi    %2, %2, 1   \n\
			2:                  \n\
			cmpnei  %2, 0       \n\
			bt      1b          \n\
			sync                \n\
			ld      %0, (%1, 0) \n\
			sync                \n\
			":"=r"(rdata)
			:"r"(addr),"r"(delay)
			:
			);
#endif
	return(rdata);
}

static int _gx6605s_otp_read(void *vreg, unsigned int addr)
{
	unsigned int val = 0;
	unsigned int reg_data = 0;
	unsigned int otp_con_reg = 0, rd_valid = 0, rw_fail = 0;
	volatile TaurusOTPReg *reg = (volatile TaurusOTPReg *)(vreg);

	while (1) {
		reg_data = sec_read((unsigned int)(&reg->otp_status), 65535);
		if ((reg_data & 0x100) == 0)
			break;
	}

	otp_con_reg = (0x7ff & (addr))<<3;  // start address
	sec_write((unsigned int)(&reg->otp_ctrl),otp_con_reg | (0x1 << 14),65535); // set READEN
	gx_usleep(64);

	while (1) {
		reg_data = sec_read((unsigned int)(&reg->otp_status), 65535);
		if ((reg_data & 0x100) == 0)
			break;
	}

	reg_data = sec_read((unsigned int)(&reg->otp_status), 65535);
	rd_valid = reg_data & 0x200;

	if (rd_valid != 0) {
		reg_data = sec_read((unsigned int)(&reg->otp_status), 65535);
		val = reg_data & 0xFF;
	}

	sec_write((unsigned int)(&reg->otp_ctrl), 0x0, 65535);

	reg_data = sec_read((unsigned int)(&reg->otp_status), 65535);
	rd_valid = reg_data & 0x200;
	rw_fail = reg_data & (1 << 12);
	return (rd_valid == 0 || rw_fail) ?  GXSE_ERR_GENERIC : val;
}

static int _gx6605s_otp_write(void *vreg, unsigned int addr, unsigned char val)
{
	volatile TaurusOTPReg *reg = (volatile TaurusOTPReg *)(vreg);
	unsigned int otp_con_reg, reg_data, rw_fail;

	gxse_switch_multipin(GXSE_MULTIPIN_SWITCH);
	while (1)
	{
		reg_data = sec_read((unsigned int)(&reg->otp_status), 65535);
		if ((reg_data & (1 << 10)) != 0)
			break;
	}
	while (1)
	{
		reg_data = sec_read((unsigned int)(&reg->otp_status), 65535);
		if ((reg_data & 0x100) == 0)
			break;
	}

	otp_con_reg = (0x7ff & (addr))<<3;
	otp_con_reg &= ~(0xff<<16);
	otp_con_reg |= val << 16;
	sec_write((unsigned int)(&reg->otp_ctrl),otp_con_reg | (0x1 << 15),65535);
	gx_usleep(64);

	while (1)
	{
		reg_data = sec_read((unsigned int)(&reg->otp_status), 65535);
		if ((reg_data & 0x100) == 0)
			break;
		if ((reg_data & (1 << 12)) == 1)
			break;
	}
	sec_write((unsigned int)(&reg->otp_ctrl), otp_con_reg, 65535);
	gx_usleep(64);

	while (1)
	{
		reg_data = sec_read((unsigned int)(&reg->otp_status), 65535);
		if ((reg_data & 0x100) == 0)
			break;
	}

	reg_data = sec_read((unsigned int)(&reg->otp_status), 65535);
	rw_fail = reg_data & (1 << 12);

	gxse_switch_multipin(GXSE_MULTIPIN_RECOVER);
	return rw_fail ? GXSE_ERR_GENERIC : 0;
}

static int32_t gx6605s_misc_otp_read(GxSeModuleHwObj *obj, uint32_t addr, uint8_t *buf, uint32_t size)
{
	int32_t i = 0, ret = 0;

	for (i = 0; i < size; i++) {
		if ((ret = _gx6605s_otp_read(obj->reg, addr+i)) == GXSE_ERR_GENERIC)
			return GXSE_ERR_GENERIC;
		else
			buf[i] = (uint8_t)(ret&0xff);
	}

	return GXSE_SUCCESS;
}

static int32_t gx6605s_misc_otp_write(GxSeModuleHwObj *obj, uint32_t addr, uint8_t *buf, uint32_t size)
{
	int32_t i = 0, ret = 0;

	for (i = 0; i < size; i++) {
		if ((ret = _gx6605s_otp_write(obj->reg, addr+i, buf[i])) == GXSE_ERR_GENERIC)
			return GXSE_ERR_GENERIC;
	}

	return GXSE_SUCCESS;
}

static GxSeModuleDevOps gx6605s_otp_devops = {
	.init  = gxse_hwobj_mutex_static_init,
	.deinit= gxse_hwobj_mutex_static_deinit,
	.ioctl = gx_misc_otp_ioctl,
};

static GxSeModuleHwObjMiscOps gx6605s_otp_ops = {
	.devops = &gx6605s_otp_devops,
	.misc_otp = {
		.otp_read  = gx6605s_misc_otp_read,
		.otp_write = gx6605s_misc_otp_write,
	},
};

static GxSeModuleHwObj s_otp_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &gx6605s_otp_ops,
	.priv = &gx6605s_otp_priv,
};

GxSeModule gx6605s_otp_module = {
	.id   = GXSE_MOD_MISC_OTP,
	.ops  = &misc_dev_ops,
	.hwobj= &s_otp_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_OTP - 0x80,
		.reg_len   = sizeof(TaurusOTPReg),
		.irqs      = {-1},
	},
};
