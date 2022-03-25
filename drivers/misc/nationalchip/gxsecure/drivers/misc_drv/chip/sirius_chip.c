#include "../gxmisc_virdev.h"
#include "sirius_chip.h"
#include "sirius_chip_reg.h"

#ifdef CPU_SCPU
struct chip_priv {
	volatile SiriusChipReg *reg;
};

static struct chip_priv sirius_chip_priv;
int32_t sirius_misc_chip_init(GxSeModuleHwObj *obj)
{
	struct chip_priv *p = &sirius_chip_priv;

	p->reg = (volatile SiriusChipReg *)obj->reg;

	return GXSE_SUCCESS;
}

int32_t sirius_misc_scpu_reset(GxSeModuleHwObj *obj)
{
	volatile SiriusChipReg *reg = sirius_chip_priv.reg;
	reg->scpu_ctrl.bits.key_clr = 1;
	reg->scpu_ctrl.bits.reset_req = 1;

	return GXSE_SUCCESS;
}

int32_t sirius_misc_get_BIV(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size)
{
	int i;
	volatile SiriusChipReg *reg = sirius_chip_priv.reg;
	(void) size;

	if (!reg->BIV_status.bits.valid)
		return GXSE_ERR_GENERIC;

	for (i = 0; i < 4; i++) {
		gx_u32_set_val(buf+4*i, (void *)&reg->BIV[3-i], 1);
	}

	return GXSE_SUCCESS;
}

int32_t sirius_misc_send_m2m_soft_key(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size)
{
	int32_t i;
	volatile SiriusChipReg *reg = sirius_chip_priv.reg;

	for (i = 0; i < size/4; i++)
		reg->m2m_soft_key[size/4-i-1] = gx_u32_get_val(buf+4*i, 0);

	return GXSE_SUCCESS;
}

int32_t sirius_misc_send_secure_key(GxSeModuleHwObj *obj, GxSeScpuKeyType type, uint8_t *buf, uint32_t size)
{
	int i;
	volatile SiriusChipReg *reg = sirius_chip_priv.reg;

	size = (size+3)/4*4;
	if (size == 16)
		reg->key_ctrl.bits.len = 0;
	else if (size == 8)
		reg->key_ctrl.bits.len = 1;

	reg->key_ctrl.bits.mode = type;
	for (i = 0; i < size/4; i++)
		reg->secure_key[size/4-i-1] = gx_u32_get_val(buf+4*i, 0);

	reg->key_ctrl.bits.start = 1;
	while(!reg->key_ctrl.bits.done);
	reg->key_ctrl.bits.start = 0;

	return GXSE_SUCCESS;
}

int32_t gx_misc_isr(int irq, void *pdata)
{
	volatile SiriusChipReg *reg = sirius_chip_priv.reg;
	reg->key_err_int.value = 0xffff;

	return 0;
}

#else
static struct mutex_static_priv sirius_chip_priv;
int32_t sirius_misc_get_CSSN(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size)
{
	volatile SiriusChipReg *reg = (volatile SiriusChipReg *)(obj->reg);
	(void) size;

	memcpy(buf, (void *)reg->CSSN, GXSE_MISC_CSSN_LEN);
	gx_buf_reverse(buf, GXSE_MISC_CSSN_LEN, buf);

	return GXSE_SUCCESS;
}

int32_t sirius_misc_get_chipname(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size)
{
	volatile SiriusChipReg  *reg = (volatile SiriusChipReg *)(obj->reg);
	(void) size;

	memcpy(buf, (unsigned char *)reg->chipname, GXSE_MISC_CHIPNAME_LEN);
	gx_buf_reverse(buf, GXSE_MISC_CHIPNAME_LEN, buf);

	return GXSE_SUCCESS;
}

static GxSeModuleDevOps sirius_chip_devops = {
    .init  = gxse_hwobj_mutex_static_init,
    .deinit= gxse_hwobj_mutex_static_deinit,
    .ioctl = gx_misc_chip_ioctl,
};

static GxSeModuleHwObjMiscOps sirius_chip_ops = {
	.devops = &sirius_chip_devops,
	.misc_chip = {
		.get_CSSN     = sirius_misc_get_CSSN,
		.get_chipname = sirius_misc_get_chipname,
	},
};

static GxSeModuleHwObj sirius_chip_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &sirius_chip_ops,
	.priv = &sirius_chip_priv,
};

GxSeModule sirius_chip_module = {
	.id   = GXSE_MOD_MISC_CHIP_CFG,
	.ops  = &misc_dev_ops,
	.hwobj= &sirius_chip_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_CHIP_CFG,
		.reg_len   = SIRIUS_MISC_REG_LEN,
		.irqs      = {-1},
	},
};
#endif
