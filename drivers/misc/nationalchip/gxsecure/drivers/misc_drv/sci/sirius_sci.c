#include "taurus_sci.h"

GxSeModule sirius_sci_module = {
	.id   = GXSE_MOD_MISC_SCI,
	.ops  = &misc_dev_ops,
	.hwobj= &taurus_sci_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_SCI,
		.reg_len   = sizeof(SCIReg),
		.irqs      = {GXACPU_IRQ_SCI, -1},
		.irq_names = {"sci"},
		.clk       = 180000000UL,
	},
};
