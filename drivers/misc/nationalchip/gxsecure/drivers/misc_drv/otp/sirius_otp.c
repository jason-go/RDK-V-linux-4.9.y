#include "../gxmisc_virdev.h"
#include "sirius_otp.h"
#include "sirius_otp_reg.h"

static int32_t _sirius_otp_read(void *vreg, uint32_t addr)
{
	uint32_t val = 0;
	volatile SiriusOTPReg *reg = (volatile SiriusOTPReg *)(vreg);

	while (reg->otp_status.bits.busy);
	reg->otp_ctrl.bits.address = (addr<<3)&0xffff;
	reg->otp_ctrl.bits.rd = 1;

	while (reg->otp_status.bits.busy);
	if (reg->otp_status.bits.rd_valid) {
#if defined (CPU_ACPU)
		val = reg->otp_status.bits.rd_data;
#else
		val = reg->otp_read_data;
#endif
	}
	reg->otp_ctrl.bits.rd = 0;

	return (!reg->otp_status.bits.rd_valid || reg->otp_status.bits.rw_fail) ? GXSE_ERR_GENERIC : val;
}

static int _sirius_otp_write(void *vreg, uint32_t addr, uint8_t val)
{
	volatile SiriusOTPReg *reg = (volatile SiriusOTPReg *)(vreg);

	reg->otp_ctrl.bits.address = (addr<<3)&0xffff;
#if defined (CPU_ACPU)
	reg->otp_ctrl.bits.wr_data = val;
#else
	reg->otp_write_data   = val;
#endif
	reg->otp_ctrl.bits.wr = 1;

	while (reg->otp_status.bits.busy);

	reg->otp_ctrl.bits.wr = 0;
	return reg->otp_status.bits.rw_fail ? GXSE_ERR_GENERIC : 0;
}

int32_t sirius_misc_otp_read(GxSeModuleHwObj *obj, uint32_t addr, uint8_t *buf, uint32_t size)
{
	int32_t i = 0, ret = 0;

	for (i = 0; i < size; i++) {
		if ((ret = _sirius_otp_read(obj->reg, addr+i)) == GXSE_ERR_GENERIC)
			return GXSE_ERR_GENERIC;
		else
			buf[i] = (uint8_t)(ret&0xff);
	}

	return GXSE_SUCCESS;
}

int32_t sirius_misc_otp_write(GxSeModuleHwObj *obj, uint32_t addr, uint8_t *buf, uint32_t size)
{
	int32_t i = 0, ret = 0;

	for (i = 0; i < size; i++) {
		if ((ret = _sirius_otp_write(obj->reg, addr+i, buf[i])) == GXSE_ERR_GENERIC)
			return GXSE_ERR_GENERIC;
	}

	return GXSE_SUCCESS;
}

#ifdef CPU_ACPU
static struct mutex_static_priv sirius_otp_priv;
static GxSeModuleDevOps sirius_otp_devops = {
	.init  = gxse_hwobj_mutex_static_init,
	.deinit= gxse_hwobj_mutex_static_deinit,
	.ioctl = gx_misc_otp_ioctl,
};

static GxSeModuleHwObjMiscOps sirius_otp_ops = {
	.devops = &sirius_otp_devops,
	.misc_otp = {
		.otp_read  = sirius_misc_otp_read,
		.otp_write = sirius_misc_otp_write,
	},
};

static GxSeModuleHwObj sirius_otp_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &sirius_otp_ops,
	.priv = &sirius_otp_priv,
};

GxSeModule sirius_otp_module = {
	.id   = GXSE_MOD_MISC_OTP,
	.ops  = &misc_dev_ops,
	.hwobj= &sirius_otp_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_OTP,
		.reg_len   = sizeof(SiriusOTPReg),
		.irqs      = {-1},
	},
};
#endif
