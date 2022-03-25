#include "../gxmisc_virdev.h"
#include "sirius_chip.h"
#include "taurus_chip.h"
#include "sirius_chip_reg.h"
#include "taurus_chip_reg.h"

#ifdef CPU_ACPU
static struct mutex_static_priv taurus_chip_priv;
int32_t taurus_misc_get_CSSN(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size)
{
	volatile TaurusChipReg *reg = (volatile TaurusChipReg *)(obj->reg);
	(void) size;

	memcpy(buf, (void *)reg->CSSN, GXSE_MISC_CSSN_LEN);
	if (CHIP_IS_TAURUS || CHIP_IS_GEMINI) {
		gx_buf_reverse(buf, GXSE_MISC_CSSN_LEN, buf);
	}

	return GXSE_SUCCESS;
}

int32_t taurus_misc_get_chipname(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size)
{
	volatile TaurusChipReg  *reg = (volatile TaurusChipReg *)(obj->reg);
	(void) size;

	if (CHIP_IS_GX3113C || CHIP_IS_GX3201) {
		gxlog_e(GXSE_LOG_MOD_MISC_CHIP, "GX3201/GX3113C unsupport now!\n");
		return GXSE_ERR_GENERIC;
	}

	memcpy(buf, (unsigned char *)reg->chipname, GXSE_MISC_CHIPNAME_LEN);
	if (CHIP_IS_TAURUS || CHIP_IS_GEMINI)
		gx_buf_reverse(buf, GXSE_MISC_CHIPNAME_LEN, buf);

	return GXSE_SUCCESS;
}
#endif

static GxSeModuleDevOps taurus_chip_devops = {
    .init  = gxse_hwobj_mutex_static_init,
    .deinit= gxse_hwobj_mutex_static_deinit,
    .ioctl = gx_misc_chip_ioctl,
};

static GxSeModuleHwObjMiscOps taurus_chip_ops = {
	.devops = &taurus_chip_devops,
	.misc_chip = {
		.get_CSSN     = taurus_misc_get_CSSN,
		.get_chipname = taurus_misc_get_chipname,
	},
};

static GxSeModuleHwObj taurus_chip_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &taurus_chip_ops,
	.priv = &taurus_chip_priv,
};

GxSeModule taurus_chip_module = {
	.id   = GXSE_MOD_MISC_CHIP_CFG,
	.ops  = &misc_dev_ops,
	.hwobj= &taurus_chip_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_CHIP_CFG,
		.reg_len   = SIRIUS_MISC_REG_LEN,
		.irqs      = {-1},
	},
};
