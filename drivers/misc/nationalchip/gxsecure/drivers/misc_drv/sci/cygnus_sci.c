#include "cygnus_sci.h"
#include "taurus_sci.h"

int32_t cygnus_sci_hw_init(void *vreg, void *priv)
{
	struct sci_priv *p = (struct sci_priv *)priv;
	volatile SCIReg *reg = (volatile SCIReg *)vreg;

	p->fifolen = SCI_FIFOLEN_V2;
	reg->ctrl2.bits.vcc_en = 0;
	reg->ctrl2.bits.auto_receive = 1;
	reg->ctrl2.bits.vcc_ctrl = 1;
	reg->ctrl3.bits.ETU = SCI_DEFAULT_ETU;
	reg->io_cfg.bits.din_hold_en = 1;
	reg->ctrl2.bits.sci_en = 1;

	return GXSE_SUCCESS;
}

int32_t cygnus_sci_set_clk_fb(void *vreg, void *priv, uint32_t enable)
{
	volatile SCIReg *reg = (volatile SCIReg *)vreg;

	if (enable) {
		reg->io_cfg.bits.clk_en = 0;
		reg->io_cfg.bits.clk_fb = 0xe;
	} else
		reg->io_cfg.bits.clk_fb = 0x0;

	return GXSE_SUCCESS;
}

int32_t cygnus_sci_set_rxfifo_gate(void *vreg, void *priv, uint32_t gate)
{
	volatile SCIReg *reg = (volatile SCIReg *)vreg;

	reg->fifo_gate.bits.rxfifo_level = gate;

	return GXSE_SUCCESS;
}

int32_t cygnus_sci_set_txfifo_gate(void *vreg, void *priv, uint32_t gate)
{
	volatile SCIReg *reg = (volatile SCIReg *)vreg;

	reg->fifo_gate.bits.txfifo_level = gate;

	return GXSE_SUCCESS;
}

static GxMiscSCIHWOps cygnus_sci_hwops = {
	.init            = cygnus_sci_hw_init,
	.set_clk_fb      = cygnus_sci_set_clk_fb,
	.set_rxfifo_gate = cygnus_sci_set_rxfifo_gate,
	.set_txfifo_gate = cygnus_sci_set_txfifo_gate,
};

static GxSeModuleHwObjMiscOps cygnus_sci_ops = {
	.devops = &taurus_sci_devops,
	.hwops  = &cygnus_sci_hwops,
	.misc_sci = {
		.set_param    = sci_set_param,
		.get_param    = sci_get_param,
		.ICC_reset    = sci_ICC_reset,
		.ICC_poweroff = sci_ICC_poweroff,
		.get_status   = sci_get_status,
		.print_reg    = sci_print_reg,
	},
};

GxSeModuleHwObj cygnus_sci_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &cygnus_sci_ops,
	.priv = &taurus_sci_priv,
};

GxSeModule cygnus_sci_module = {
	.id   = GXSE_MOD_MISC_SCI,
	.ops  = &misc_dev_ops,
	.hwobj= &cygnus_sci_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_SCI,
		.reg_len   = sizeof(SCIReg),
		.irqs      = {GXACPU_IRQ_TAURUS_SCI, -1},
		.irq_names = {"sci"},
		.clk       = 198000000UL,
	},
};

