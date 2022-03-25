#include "taurus_sci.h"
#include "sci_reg.h"
#include "sci_chiptest.h"

#ifdef CFG_GXSE_CHIP_TEST

uint32_t g_sci_flag = 0;
static uint32_t g_sci_reg_base = 0x0;

void gxse_chiptest_sci_init(uint32_t reg_base)
{
	g_sci_reg_base = reg_base;
}

void gxse_chiptest_sci_change_card_in_sw(void)
{
	volatile SCIReg *reg = (volatile SCIReg *)g_sci_reg_base;

	reg->io_cfg.bits.card_in_sw = reg->io_cfg.bits.card_in_sw ? 0 : 1;
}

void gxse_chiptest_sci_enable_din_hold(void)
{
	volatile SCIReg *reg = (volatile SCIReg *)g_sci_reg_base;

	reg->io_cfg.bits.din_hold_en = 1;
}

void gxse_chiptest_sci_disable_din_hold(void)
{
	volatile SCIReg *reg = (volatile SCIReg *)g_sci_reg_base;

	reg->io_cfg.bits.din_hold_en = 0;
}

void gxse_chiptest_sci_set_clk_fb(uint8_t value)
{
	volatile SCIReg *reg = (volatile SCIReg *)g_sci_reg_base;

	reg->io_cfg.bits.clk_en = 0;
	reg->io_cfg.bits.clk_fb = value;
}

void gxse_chiptest_sci_set_output_mode(uint8_t push_pull)
{
	volatile SCIReg *reg = (volatile SCIReg *)g_sci_reg_base;

	reg->io_cfg.bits.clk_en = push_pull;
}

void gxse_chiptest_sci_set_card_hot_reset(void)
{
	volatile SCIReg *reg = (volatile SCIReg *)g_sci_reg_base;

	reg->ctrl1.bits.hotrst = 1;
}

void gxse_chiptest_sci_set_card_deact(void)
{
	volatile SCIReg *reg = (volatile SCIReg *)g_sci_reg_base;

	reg->ctrl1.bits.deact = 1;
}

void gxse_chiptest_sci_set_not_calc_parity(void)
{
	volatile SCIReg *reg = (volatile SCIReg *)g_sci_reg_base;

	reg->parity_cfg.bits.no_calc_parity = 1;
}

void gxse_chiptest_sci_enable_error_parity(void)
{
	g_sci_flag |= SCI_TEST_FLAG_ERROR_PARITY;
}

void gxse_chiptest_sci_enable_rx_gate(void)
{
	g_sci_flag |= SCI_TEST_FLAG_RX_GATE;
}

void gxse_chiptest_sci_disable_rx_gate(void)
{
	g_sci_flag &= ~SCI_TEST_FLAG_RX_GATE;
}

void gxse_chiptest_sci_enable_rx_overflow(void)
{
	g_sci_flag |= SCI_TEST_FLAG_RX_OVERFLOW;
}

void gxse_chiptest_sci_disable_rx_overflow(void)
{
	g_sci_flag &= ~SCI_TEST_FLAG_RX_OVERFLOW;
}

void gxse_chiptest_sci_enable_rx_overtime(void)
{
	g_sci_flag |= SCI_TEST_FLAG_RX_OVERTIME;
}

void gxse_chiptest_sci_disable_rx_overtime(void)
{
	g_sci_flag &= ~SCI_TEST_FLAG_RX_OVERTIME;
}
#endif
