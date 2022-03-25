#include "cygnus_sci.h"
#include "taurus_sci.h"

static GxMiscSCIHWOps gemini_sci_hwops = {
	.init            = taurus_sci_hw_init,
	.set_clk_fb      = cygnus_sci_set_clk_fb,
	.set_rxfifo_gate = taurus_sci_set_rxfifo_gate,
	.set_txfifo_gate = taurus_sci_set_txfifo_gate,
};

static GxSeModuleHwObjMiscOps gemini_sci_ops = {
	.devops = &taurus_sci_devops,
	.hwops  = &gemini_sci_hwops,
	.misc_sci = {
		.set_param    = sci_set_param,
		.get_param    = sci_get_param,
		.ICC_reset    = sci_ICC_reset,
		.ICC_poweroff = sci_ICC_poweroff,
		.get_status   = sci_get_status,
		.print_reg    = sci_print_reg,
	},
};

GxSeModuleHwObj gemini_sci_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &gemini_sci_ops,
	.priv = &taurus_sci_priv,
};

GxSeModule gemini_sci_module = {
	.id   = GXSE_MOD_MISC_SCI,
	.ops  = &misc_dev_ops,
	.hwobj= &gemini_sci_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_SCI,
		.reg_len   = sizeof(SCIReg),
		.irqs      = {GXACPU_IRQ_TAURUS_SCI, -1},
		.irq_names = {"sci"},
		.clk       = 198000000UL,
	},
};
