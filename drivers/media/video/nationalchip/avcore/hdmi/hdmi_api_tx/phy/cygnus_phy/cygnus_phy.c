#include "cygnus_phy.h"
#include "cygnus_halPhy.h"
#include "../../util/error.h"

static const u16 PHY_BASE_ADDR = 0x3000;

static int cygnus_phy_Initialize(u16 baseAddr, u8 dataEnablePolarity, u8 phy_model)
{
	return TRUE;
}

static int cygnus_phy_Configure(u16 baseAddr, u16 pClk, u8 cRes, u8 pRep, u8 phy_model)
{
	u16 i = 0;
	LOG_TRACE();

	/* color resolution 0 is 8 bit color depth */
	if (cRes == 0)
		cRes = 8;

	if (pRep != 0) {
		error_Set(ERR_PIXEL_REPETITION_NOT_SUPPORTED);
		LOG_ERROR2("pixel repetition not supported", pRep);
		return FALSE;
	}

	/* reset PHY */

	switch (pClk) {
		case 2700:
			switch (cRes) {
				case 8:
					break;
				case 10:
					break;
				case 12:
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			break;
		case 7425:
			switch (cRes) {
				case 8:
					break;
				case 10:
					break;
				case 12:
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			break;
		case 14850:
			switch (cRes) {
				case 8:
					break;
				case 10:
					break;
				case 12:
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			break;
		default:
			error_Set(ERR_PIXEL_CLOCK_NOT_SUPPORTED);
			LOG_ERROR2("pixel clock not supported", pClk);
			return FALSE;
	}

	for (i = 0; i < PHY_TIMEOUT; i++) {
		if ((i % 100) == 0) {
			/* 获取PLL lock state */
			if (1) {
				gx_mdelay(10);
				LOG_NOTICE("PHY PLL locked");
				break;
			}
		}
	}

	if (1) {
		error_Set(ERR_PHY_NOT_LOCKED);
		LOG_ERROR("PHY PLL not locked");
		return FALSE;
	}

	return TRUE;
}

static int cygnus_phy_Standby(u16 baseAddr)
{
	return TRUE;
}

static int cygnus_phy_EnableHpdSense(u16 baseAddr)
{
	return TRUE;
}

static int cygnus_phy_DisableHpdSense(u16 baseAddr)
{
	return TRUE;
}

static int cygnus_phy_HotPlugDetected(u16 baseAddr)
{
	/* MASK		STATUS		POLARITY	INTERRUPT		HPD
	 *   0			0			0			1			0
	 *   0			1			0			0			1
	 *   0			0			1			0			0
	 *   0			1			1			1			1
	 *   1			x			x			0			x
	 */
	int polarity = 0;
	polarity = cygnus_halSourcePhy_InterruptPolarityStatus(baseAddr + PHY_BASE_ADDR, 0x02) >> 1;
	if (polarity == cygnus_halSourcePhy_HotPlugState(baseAddr + PHY_BASE_ADDR))
	{
		cygnus_halSourcePhy_InterruptPolarity(baseAddr + PHY_BASE_ADDR, 1, !polarity);
		return polarity;
	}
	return !polarity;
	/* return cygnus_halSourcePhy_HotPlugState(baseAddr + PHY_BASE_ADDR); */
}

static int cygnus_phy_InterruptEnable(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	cygnus_halSourcePhy_InterruptMask(baseAddr + PHY_BASE_ADDR, value);
	return TRUE;
}

static int cygnus_phy_Gen2PDDQ(u16 baseAddr, u8 bit)
{
	return TRUE;
}

static int cygnus_phy_Gen2TxPowerOn(u16 baseAddr, u8 bit)
{
	return TRUE;
}

static int cygnus_phy_TestControl(void)
{
	return TRUE;
}

static int cygnus_phy_ApbWrite(u16 baseAddr, u16 data, u8 addr)
{
	return TRUE;
}

static u16 cygnus_phy_ApbRead(u16 baseAddr)
{
	return TRUE;
}

struct gxav_hdmi_phy_ops cygnus_hdmi_phy_ops = {
	.init               = cygnus_phy_Initialize,
	.configure          = cygnus_phy_Configure,
	.standby            = cygnus_phy_Standby,
	.enable_hpd_sense   = cygnus_phy_EnableHpdSense,
	.disable_hpd_sense  = cygnus_phy_DisableHpdSense,
	.hotplug_detected   = cygnus_phy_HotPlugDetected,
	.interrupt_enable   = cygnus_phy_InterruptEnable,
	.gen2pddq           = cygnus_phy_Gen2PDDQ,
	.gen2tx_poweron     = cygnus_phy_Gen2TxPowerOn,
};
