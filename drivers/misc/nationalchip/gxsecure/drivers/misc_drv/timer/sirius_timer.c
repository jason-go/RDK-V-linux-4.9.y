#include "../gxmisc_virdev.h"
#include "sirius_timer_reg.h"

#define GXSCPU_SYS_CLK                     148
struct timer_priv {
	volatile uint32_t time_up;
	volatile SiriusTimerReg *reg;
};

static struct timer_priv sirius_timer_priv;

int32_t sirius_timer_init(GxSeModuleHwObj *obj)
{
	volatile SiriusTimerReg *reg = (volatile SiriusTimerReg *)obj->reg;
	struct timer_priv *p = &sirius_timer_priv;
	memset(p, 0, sizeof(struct timer_priv));

	reg->counter_prediv.bits.pre_div = GXSCPU_SYS_CLK-1;
	reg->counter_config.bits.enable = 1;
	reg->counter_config.bits.int_en = 1;

	p->reg = reg;

	return GXSE_SUCCESS;
}

int32_t sirius_timer_udelay(GxSeModuleHwObj *obj, uint32_t us)
{
	struct timer_priv *p = &sirius_timer_priv;
	volatile SiriusTimerReg *reg = p->reg;

	reg->counter_ini = 0 - us;
	reg->counter_ctrl.bits.reset = 1;
	reg->counter_ctrl.bits.reset = 0;
	reg->counter_ctrl.bits.start = 1;

	while(!p->time_up);
	p->time_up = 0;

	return GXSE_SUCCESS;
}

void gx_timer_isr(void)
{
	sirius_timer_priv.time_up = 1;
	sirius_timer_priv.reg->counter_ctrl.bits.start = 0;
	sirius_timer_priv.reg->counter_status.bits.status = 1;
}

static GxSeModuleDevOps sirius_timer_devops = {
	.init = sirius_timer_init,
    .ioctl = gx_misc_timer_ioctl,
};

static GxSeModuleHwObjMiscOps sirius_timer_ops = {
	.devops = &sirius_timer_devops,
	.misc_timer = {
		.udelay  = sirius_timer_udelay,
	},
};

static GxSeModuleHwObj sirius_timer_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops    = &sirius_timer_ops,
	.priv   = &sirius_timer_priv,
};

GxSeModule sirius_timer_module = {
	.id   = GXSE_MOD_MISC_TIMER,
	.ops  = &misc_dev_ops,
	.hwobj= &sirius_timer_hwobj,
	.res  = {
		.reg_base  = GXSCPU_BASE_ADDR_TIMER,
		.reg_len   = sizeof(SiriusTimerReg),
	},
};
