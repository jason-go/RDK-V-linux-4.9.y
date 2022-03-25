#include "taurus_sci.h"

GxSeModule gx6605s_sci_module = {
	.id   = GXSE_MOD_MISC_SCI,
	.ops  = &misc_dev_ops,
	.hwobj= &taurus_sci_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_GX6605S_SCI,
		.reg_len   = sizeof(SCIReg),
		.irqs      = {GXACPU_IRQ_GX6605S_SCI, -1},
		.irq_names = {"sci"},
		.clk       = 27000000UL,
	},
};

GxSeModule pegasus_sci_module = {
	.id   = GXSE_MOD_MISC_SCI,
	.ops  = &misc_dev_ops,
	.hwobj= &taurus_sci_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_PEGASUS_SCI,
		.reg_len   = sizeof(SCIReg),
		.irqs      = {GXACPU_IRQ_GX6605S_SCI, -1},
		.irq_names = {"sci"},
		.clk       = 198000000UL,
	},
};
