#include "../gxmisc_virdev.h"
#include "taurus_otp_reg.h"
#include "taurus_otp.h"

#ifdef CPU_ACPU
static struct mutex_static_priv taurus_otp_priv;
static int32_t taurus_misc_otp_init(GxSeModuleHwObj *obj)
{
	struct mutex_static_priv *p = (struct mutex_static_priv *)obj->priv;
	volatile TaurusOTPReg *reg = (volatile TaurusOTPReg *)(obj->reg);

	reg->otp_delay_sw_ctrl.bits.enable = 1;
	// time(ns) = sw / 27MHz
	reg->otp_tsp_pgm_pgavdd.bits.sw0 = 28; //set time > 1000ns, sw > 1000ns/(1/27MHZ)
	reg->otp_tsp_pgm_pgavdd.bits.sw1 = 3;  //time > 100ns
	reg->otp_tpaen_tpgm.bits.sw0 = 62;     //time = (2000ns,2600ns)
	reg->otp_tpaen_tpgm.bits.sw1 = 52;     //time > 1900ns
	reg->otp_thp_pgavdd_pgm.bits.sw0 = 3;  //time > 100ns
	reg->otp_thp_pgavdd_pgm.bits.sw1 = 28; //time > 1000ns
	reg->otp_trd_tsr_rd.bits.sw0 = 3;      //time > 100ns
	reg->otp_trd_tsr_rd.bits.sw1 = 2;      //time > 40ns(Trd)
	reg->otp_tsq_tpaen.bits.sw0 = 3;       //time = Trd+35ns
	reg->otp_tsq_tpaen.bits.sw1 = 1;       //time < 45ns
	reg->otp_thr_rd.bits.sw0 = 1;          //time < 45ns

	gx_mutex_init(&p->mutex);
	obj->mutex = &p->mutex;

	return GXSE_SUCCESS;
}
#endif

static int32_t _taurus_otp_read(void *vreg, uint32_t addr)
{
	uint32_t val = 0;
	volatile TaurusOTPReg *reg = (volatile TaurusOTPReg *)(vreg);

	while (reg->otp_status.bits.busy);
	reg->otp_ctrl.bits.address = addr;
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

static int _taurus_otp_write(void *vreg, uint32_t addr, uint8_t val)
{
	volatile TaurusOTPReg *reg = (volatile TaurusOTPReg *)(vreg);

    while(!reg->otp_status.bits.ready);
    while (reg->otp_status.bits.busy);

    reg->otp_ctrl.bits.address = addr;
#ifdef CPU_ACPU
    reg->otp_ctrl.bits.wr_data = val;
#else
	reg->otp_write_data = val;
#endif
    reg->otp_ctrl.bits.wr = 1;

	while (reg->otp_status.bits.busy)
		if (reg->otp_status.bits.rw_fail)
			break;

	while (reg->otp_status.bits.busy);
	reg->otp_ctrl.bits.wr = 0;

	return reg->otp_status.bits.rw_fail ? GXSE_ERR_GENERIC : 0;
}

int32_t taurus_misc_otp_read(GxSeModuleHwObj *obj, uint32_t addr, uint8_t *buf, uint32_t size)
{
	int32_t i = 0, ret = 0;

	for (i = 0; i < size; i++) {
		if ((ret = _taurus_otp_read(obj->reg, addr+i)) == GXSE_ERR_GENERIC)
			return GXSE_ERR_GENERIC;
		else
			buf[i] = (uint8_t)(ret&0xff);
	}

	return GXSE_SUCCESS;
}

int32_t taurus_misc_otp_write(GxSeModuleHwObj *obj, uint32_t addr, uint8_t *buf, uint32_t size)
{
	int32_t i = 0, ret = 0;

	for (i = 0; i < size; i++) {
		if ((ret = _taurus_otp_write(obj->reg, addr+i, buf[i])) == GXSE_ERR_GENERIC)
			return GXSE_ERR_GENERIC;
	}

	return GXSE_SUCCESS;
}

#ifdef CPU_ACPU
static GxSeModuleDevOps taurus_otp_devops = {
	.init = taurus_misc_otp_init,
	.ioctl = gx_misc_otp_ioctl,
};

static GxSeModuleHwObjMiscOps taurus_otp_ops = {
	.devops = &taurus_otp_devops,
	.misc_otp = {
		.otp_read  = taurus_misc_otp_read,
		.otp_write = taurus_misc_otp_write,
	},
};

static GxSeModuleHwObj taurus_otp_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &taurus_otp_ops,
	.priv = &taurus_otp_priv,
};

GxSeModule taurus_otp_module = {
	.id   = GXSE_MOD_MISC_OTP,
	.ops  = &misc_dev_ops,
	.hwobj= &taurus_otp_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_OTP,
		.reg_len   = sizeof(TaurusOTPReg),
		.irqs      = {-1},
	},
};
#endif
