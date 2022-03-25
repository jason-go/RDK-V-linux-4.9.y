/*
 * phy.c
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */
#include "gx3211_phy.h"
#include "gx3211_halSourcePhy.h"
#include "gx3211_halI2cMasterPhy.h"
#include "../../core/halMainController.h"
#include "../../util/log.h"
#include "../../util/error.h"
#include "../../bsp/access.h"
#include "kernelcalls.h"
#include "gxav_common.h"
#include "secure_hal.h"
#include "../../core/control.h"

static const u16 PHY_BASE_ADDR = 0x3000;
static const u16 MC_BASE_ADDR = 0x4000;
static const u16 PHY_I2CM_BASE_ADDR = 0x3020;
//static const u8 PHY_I2C_SLAVE_ADDR = 0x54;
static const u8 PHY_I2C_SLAVE_ADDR = 0x69;
u16 phy_I2cReadSelf(u16 addr);

static int gx3211_phy_I2cWrite(u16 baseAddr, u16 data, u8 addr);

static int gx3211_phy_Initialize(u16 baseAddr, u8 dataEnablePolarity, u8 phy_model)
{
	LOG_TRACE1(dataEnablePolarity);
#ifndef PHY_THIRD_PARTY
	/* PHY_GEN2_GF_28SLP_1_8V*//* E108 */
	if(phy_model == 108)
	{
		LOG_NOTICE("GEN 2 GF 28SLP 1.8V build - E108");
		gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
		gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
	}
	/*PHY_GEN2_TSMC_40G_1_8V */ /* E102*/
	if(phy_model == 102)
	{
		LOG_NOTICE("GEN 2 TSMC 40G 1.8V build - E102");
		gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
		gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
	}
	/*PHY_GEN2_TSMC_65LP_2_5V */ /*E104*/
	if(phy_model == 104)
	{
		LOG_NOTICE("GEN 2 TSMC 65LP 2.5V build - E104");
		gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
		gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
	}
	/* PHY_GEN2_GF_40LP_2_5V */ /*E110*/
	if(phy_model == 110)
	{
		LOG_NOTICE("GEN 2 GF 40LP 2.5V build - E110");
		gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
		gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
	}
	/*PHY_GEN2_TSMC_40LP_2_5V*/ /* TQL */
	if(phy_model == 001)
	{
		LOG_NOTICE("GEN 2 TSMC 40LP 2.5V build - TQL");
		gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
		gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
	}
	/* PHY_GEN2_TSMC_28HPM_1_8V */ /* E112 */
	if(phy_model == 112)
	{
		LOG_NOTICE("GEN 2 TSMC 28HPM 1.8V build - E112");
		gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
		gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
	}
	/* PHY_TNP */
	if(phy_model == 002)
	{
		LOG_NOTICE("TNP build");
	}
	/* PHY_CNP */
	if(phy_model == 003)
	{
		LOG_NOTICE("CNP build");
	}
	gx3211_halSourcePhy_InterruptMask(baseAddr + PHY_BASE_ADDR, ~0); /* mask phy interrupts */
	gx3211_halSourcePhy_DataEnablePolarity(baseAddr + PHY_BASE_ADDR,
			dataEnablePolarity);
	gx3211_halSourcePhy_InterfaceControl(baseAddr + PHY_BASE_ADDR, 0);
	gx3211_halSourcePhy_EnableTMDS(baseAddr + PHY_BASE_ADDR, 0);
	gx3211_halSourcePhy_PowerDown(baseAddr + PHY_BASE_ADDR, 0); /* disable PHY */
#else
	LOG_NOTICE("Third Party PHY build");
#endif
	return TRUE;
}

static int gx3211_phy_Configure(u16 baseAddr, u16 pClk, u8 cRes, u8 pRep, u8 phy_model)
{
#ifndef PHY_THIRD_PARTY
	//u16 clk = 0;
	//u16 rep = 0;
	u16 i = 0;
	u32 flag = 0;
	LOG_TRACE();
	/*  colour resolution 0 is 8 bit colour depth */
	if (cRes == 0)
		cRes = 8;

	if (pRep != 0)
	{
		error_Set(ERR_PIXEL_REPETITION_NOT_SUPPORTED);
		LOG_ERROR2("pixel repetition not supported", pRep);
		return FALSE;
	}

	/* The following is only for PHY_GEN1_CNP, and 1v0 NOT 1v1 */
	/* values are found in document HDMISRC1UHTCcnp_IPCS_DS_0v3.doc
	 * for the HDMISRCGPHIOcnp
	 */
	/* in the cnp PHY interface, the 3 most significant bits are ctrl (which
	 * part block to write) and the last 5 bits are data */
	/* for example 0x6A4a is writing to block  3 (ie. [14:10]) (5-bit blocks)
	 * the bits 0x0A, and  block 2 (ie. [9:5]) the bits 0x0A */
	/* configure PLL after core pixel repetition */
	/*PHY_GEN2_TSMC_40LP_2_5V*/ /* TQL */

	{
		/* PHY_GEN2_TSMC_40G_1_8V */ /*E102*/
		if(phy_model == 102)
		{
			gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
			gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
			halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 0);
			halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 1);
			halMainController_HeacPhyReset(baseAddr + MC_BASE_ADDR, 1);
			gx3211_halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 1);
			gx3211_halI2cMasterPhy_SlaveAddress(baseAddr + PHY_I2CM_BASE_ADDR, PHY_I2C_SLAVE_ADDR);
			gx3211_halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 0);

			if (CHIP_IS_TAURUS) {
				flag = gxav_secure_otp_read(0x15c);
				if ((flag&0xF) == 0)
					gx3211_phy_I2cWrite(baseAddr, 0x0070, 0x1e);// 数据NP反相
				else
					gx3211_phy_I2cWrite(baseAddr, 0x0000, 0x1e);// 数据NP不反相
			} else {
				gx3211_phy_I2cWrite(baseAddr, 0x0070, 0x1e);// 数据NP反相
			}

			if (CHIP_IS_GX3211) {
				gx3211_phy_I2cWrite(baseAddr, 0x0000, 0x13);
				gx3211_phy_I2cWrite(baseAddr, 0x0006, 0x19);
				gx3211_phy_I2cWrite(baseAddr, 0x0006, 0x17);
			} else {
				gx3211_phy_I2cWrite(baseAddr, 0x0800, 0x13);
				gx3211_phy_I2cWrite(baseAddr, 0x0006, 0x19);
				gx3211_phy_I2cWrite(baseAddr, 0x0006, 0x17);
			}
			gx3211_phy_I2cWrite(baseAddr, 0x8000, 0x05);
			switch (pClk)
			{
				case 2700:
					switch (cRes)
					{
						case 8:
							gx3211_phy_I2cWrite(baseAddr, 0x01e0, 0x06);
							if (CHIP_IS_GX6605S || CHIP_IS_GX3211)
								gx3211_phy_I2cWrite(baseAddr, 0x08da, 0x10);
							else
								gx3211_phy_I2cWrite(baseAddr, 0x0210, 0x10);

							if (CHIP_IS_SIRIUS || CHIP_IS_TAURUS || CHIP_IS_GEMINI)
								gx3211_phy_I2cWrite(baseAddr, 0x0005, 0x15);
							else
								gx3211_phy_I2cWrite(baseAddr, 0x0000, 0x15);
							break;
						case 10:
							gx3211_phy_I2cWrite(baseAddr, 0x21e1, 0x06);
							gx3211_phy_I2cWrite(baseAddr, 0x08da, 0x10);
							gx3211_phy_I2cWrite(baseAddr, 0x0000, 0x15);
							break;
						case 12:
							gx3211_phy_I2cWrite(baseAddr, 0x41e2, 0x06);
							gx3211_phy_I2cWrite(baseAddr, 0x065a, 0x10);
							gx3211_phy_I2cWrite(baseAddr, 0x0000, 0x15);
							break;
						default:
							error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
							LOG_ERROR2("color depth not supported", cRes);
							return FALSE;
					}
					gx3211_phy_I2cWrite(baseAddr, 0x8009, 0x09);
					//gx3211_phy_I2cWrite(baseAddr, 0x01ef, 0x0E);
					//gx3211_phy_I2cWrite(baseAddr, 0x0231, 0x0E);
					//gx3211_phy_I2cWrite(baseAddr, 0x0339, 0x0E);
					gx3211_phy_I2cWrite(baseAddr, 0x0273, 0x0E);
					break;
				case 5400:
					switch (cRes)
					{
						case 8:
							gx3211_phy_I2cWrite(baseAddr, 0x0140, 0x06);
							gx3211_phy_I2cWrite(baseAddr, 0x09da, 0x10);
							gx3211_phy_I2cWrite(baseAddr, 0x0005, 0x15);
							break;
						case 10:
							gx3211_phy_I2cWrite(baseAddr, 0x2141, 0x06);
							gx3211_phy_I2cWrite(baseAddr, 0x09da, 0x10);
							gx3211_phy_I2cWrite(baseAddr, 0x0005, 0x15);
							break;
						case 12:
							gx3211_phy_I2cWrite(baseAddr, 0x4142, 0x06);
							gx3211_phy_I2cWrite(baseAddr, 0x079a, 0x10);
							gx3211_phy_I2cWrite(baseAddr, 0x0005, 0x15);
							break;
						default:
							error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
							LOG_ERROR2("color depth not supported", cRes);
							return FALSE;
					}
					gx3211_phy_I2cWrite(baseAddr, 0x8009, 0x09);
					gx3211_phy_I2cWrite(baseAddr, 0x01ef, 0x0E);
					//gx3211_phy_I2cWrite(baseAddr, 0x0231, 0x0E);
					break;
				case 7425:
					switch (cRes)
					{
						case 8:
							gx3211_phy_I2cWrite(baseAddr, 0x0140, 0x06);
							if (CHIP_IS_GX6605S || CHIP_IS_GX3211)
							{
								gx3211_phy_I2cWrite(baseAddr, 0x079a, 0x10);
							    gx3211_phy_I2cWrite(baseAddr, 0x0005, 0x15);
							}
							else
							{
								gx3211_phy_I2cWrite(baseAddr, 0x0210, 0x10);
							    gx3211_phy_I2cWrite(baseAddr, 0x000a, 0x15);
							}
							break;
						case 10:
							gx3211_phy_I2cWrite(baseAddr, 0x20a1, 0x06);
							gx3211_phy_I2cWrite(baseAddr, 0x0bda, 0x10);
							gx3211_phy_I2cWrite(baseAddr, 0x000a, 0x15);
							break;
						case 12:
							gx3211_phy_I2cWrite(baseAddr, 0x40a2, 0x06);
							gx3211_phy_I2cWrite(baseAddr, 0x0a5a, 0x10);
							gx3211_phy_I2cWrite(baseAddr, 0x000a, 0x15);
							break;
						default:
							error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
							LOG_ERROR2("color depth not supported", cRes);
							return FALSE;
					}
					gx3211_phy_I2cWrite(baseAddr, 0x8009, 0x09);
					//gx3211_phy_I2cWrite(baseAddr, 0x0231, 0x0E);
					gx3211_phy_I2cWrite(baseAddr, 0x01ef, 0x0E);
					break;
				case 14850:
					switch (cRes)
					{
						case 8:
							gx3211_phy_I2cWrite(baseAddr, 0x00a0, 0x06);
							if (CHIP_IS_GX6605S || CHIP_IS_GX3211)
							{
								gx3211_phy_I2cWrite(baseAddr, 0x079a, 0x10);
							        gx3211_phy_I2cWrite(baseAddr, 0x000a, 0x15);
							}
							else
							{
								gx3211_phy_I2cWrite(baseAddr, 0x0010, 0x10);
							        gx3211_phy_I2cWrite(baseAddr, 0x000f, 0x15);
							}
							gx3211_phy_I2cWrite(baseAddr, 0x8009, 0x09);
							gx3211_phy_I2cWrite(baseAddr, 0x0108, 0x0E);
							//gx3211_phy_I2cWrite(baseAddr, 0x0231, 0x0E);
							break;
						case 10:
							gx3211_phy_I2cWrite(baseAddr, 0x2001, 0x06);
							gx3211_phy_I2cWrite(baseAddr, 0x0bda, 0x10);
							gx3211_phy_I2cWrite(baseAddr, 0x000f, 0x15);
							gx3211_phy_I2cWrite(baseAddr, 0x800b, 0x09);
							gx3211_phy_I2cWrite(baseAddr, 0x014a, 0x0E);
							break;
						case 12:
							gx3211_phy_I2cWrite(baseAddr, 0x4002, 0x06);
							gx3211_phy_I2cWrite(baseAddr, 0x0a5a, 0x10);
							gx3211_phy_I2cWrite(baseAddr, 0x000f, 0x15);
							gx3211_phy_I2cWrite(baseAddr, 0x800b, 0x09);
							gx3211_phy_I2cWrite(baseAddr, 0x014a, 0x0E);
							break;
						case 16:
							gx3211_phy_I2cWrite(baseAddr, 0x6003, 0x06);
							gx3211_phy_I2cWrite(baseAddr, 0x07da, 0x10);
							gx3211_phy_I2cWrite(baseAddr, 0x000f, 0x15);
							gx3211_phy_I2cWrite(baseAddr, 0x800b, 0x09);
							gx3211_phy_I2cWrite(baseAddr, 0x014a, 0x0E);
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
			gx3211_halSourcePhy_Gen2EnHpdRxSense(baseAddr + PHY_BASE_ADDR, 1);
			gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 1);
			gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 0);
		}
	}
	/* wait PHY_TIMEOUT no of cycles at most for the pll lock signal to raise ~around 20us max */
	for (i = 0; i < PHY_TIMEOUT; i++)
	{
		if ((i % 100) == 0)
		{
			if (gx3211_halSourcePhy_PhaseLockLoopState(baseAddr + PHY_BASE_ADDR) == TRUE)
			{
				gx_mdelay(10);
				LOG_NOTICE("PHY PLL locked");
				break;
			}
		}
	}
	if (gx3211_halSourcePhy_PhaseLockLoopState(baseAddr + PHY_BASE_ADDR) != TRUE)
	{
		error_Set(ERR_PHY_NOT_LOCKED);
		LOG_ERROR("PHY PLL not locked");
		return FALSE;
	}
#endif
	return TRUE;
}

static int gx3211_phy_Standby(u16 baseAddr)
{
#ifndef PHY_THIRD_PARTY
	gx3211_halSourcePhy_InterruptMask(baseAddr + PHY_BASE_ADDR, ~0); /* mask phy interrupts */
	gx3211_halSourcePhy_EnableTMDS(baseAddr + PHY_BASE_ADDR, 0);
	gx3211_halSourcePhy_PowerDown(baseAddr + PHY_BASE_ADDR, 0); /*  disable PHY */
	gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr, 0);
	gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
#endif
	return TRUE;
}

static int gx3211_phy_EnableHpdSense(u16 baseAddr)
{
#ifndef PHY_THIRD_PARTY
	gx3211_halSourcePhy_Gen2EnHpdRxSense(baseAddr + PHY_BASE_ADDR, 1);
#endif
	return TRUE;
}

static int gx3211_phy_DisableHpdSense(u16 baseAddr)
{
#ifndef PHY_THIRD_PARTY
	gx3211_halSourcePhy_Gen2EnHpdRxSense(baseAddr + PHY_BASE_ADDR, 0);
#endif
	return TRUE;
}

static int gx3211_phy_HotPlugDetected(u16 baseAddr)
{
	/* MASK		STATUS		POLARITY	INTERRUPT		HPD
	 *   0			0			0			1			0
	 *   0			1			0			0			1
	 *   0			0			1			0			0
	 *   0			1   		1			1			1
	 *   1			x			x			0			x
	 */
	int polarity = 0;
	polarity = gx3211_halSourcePhy_InterruptPolarityStatus(baseAddr + PHY_BASE_ADDR, 0x02) >> 1;
	if (polarity == gx3211_halSourcePhy_HotPlugState(baseAddr + PHY_BASE_ADDR))
	{
		gx3211_halSourcePhy_InterruptPolarity(baseAddr + PHY_BASE_ADDR, 1, !polarity);
		return polarity;
	}
	return !polarity;
	/* return gx3211_halSourcePhy_HotPlugState(baseAddr + PHY_BASE_ADDR); */
}

static int gx3211_phy_InterruptEnable(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	gx3211_halSourcePhy_InterruptMask(baseAddr + PHY_BASE_ADDR, value);
	return TRUE;
}

static int gx3211_phy_Gen2PDDQ(u16 baseAddr, u8 bit)
{
	gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, bit);

	return TRUE;
}

static int gx3211_phy_Gen2TxPowerOn(u16 baseAddr, u8 bit)
{
	gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, bit);

	return TRUE;
}

#ifndef PHY_THIRD_PARTY
int phy_TestControl(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	gx3211_halSourcePhy_TestDataIn(baseAddr + PHY_BASE_ADDR, value);
	gx3211_halSourcePhy_TestEnable(baseAddr + PHY_BASE_ADDR, 1);
	gx3211_halSourcePhy_TestClock(baseAddr + PHY_BASE_ADDR, 1);
	gx3211_halSourcePhy_TestClock(baseAddr + PHY_BASE_ADDR, 0);
	gx3211_halSourcePhy_TestEnable(baseAddr + PHY_BASE_ADDR, 0);
	return TRUE;
}

int gx3211_phy_TestData(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	gx3211_halSourcePhy_TestDataIn(baseAddr + PHY_BASE_ADDR, value);
	gx3211_halSourcePhy_TestEnable(baseAddr + PHY_BASE_ADDR, 0);
	gx3211_halSourcePhy_TestClock(baseAddr + PHY_BASE_ADDR, 1);
	gx3211_halSourcePhy_TestClock(baseAddr + PHY_BASE_ADDR, 0);
	return TRUE;
}

static int gx3211_phy_I2cWrite(u16 baseAddr, u16 data, u8 addr)
{
	LOG_TRACE2(data, addr);
	gx_udelay(1000);
	gx3211_halI2cMasterPhy_RegisterAddress(baseAddr + PHY_I2CM_BASE_ADDR, addr);
	gx_mdelay(1);
	gx3211_halI2cMasterPhy_WriteData(baseAddr + PHY_I2CM_BASE_ADDR, data);
	gx_mdelay(1);
	gx3211_halI2cMasterPhy_WriteRequest(baseAddr + PHY_I2CM_BASE_ADDR);
	gx_mdelay(1);
	return TRUE;
}

void gx3211_phy_I2cReadRequest(u16 baseAddr, u8 addr)
{
	LOG_TRACE1(addr);
	gx3211_halI2cMasterPhy_MaskInterrupts(baseAddr + PHY_I2CM_BASE_ADDR, 0);
	gx3211_halI2cMasterPhy_RegisterAddress(baseAddr + PHY_I2CM_BASE_ADDR, addr);
	gx3211_halI2cMasterPhy_ReadRequest(baseAddr + PHY_I2CM_BASE_ADDR);
}

static u16 gx3211_phy_I2cRead(u16 baseAddr)
{
	LOG_TRACE();
	gx3211_halI2cMasterPhy_MaskInterrupts(baseAddr + PHY_I2CM_BASE_ADDR, 1);
	return gx3211_halI2cMasterPhy_ReadData(baseAddr + PHY_I2CM_BASE_ADDR);
}

#if 0
u16 gx3211_phy_I2cReadSelf(u16 addr)
{
	int tmp = 0;
	u16 tmp1 = 0;
	gx3211_halI2cMasterPhy_RegisterAddress(PHY_I2CM_BASE_ADDR, addr);
	gx3211_halI2cMasterPhy_ReadRequest(PHY_I2CM_BASE_ADDR);
	LOG_NOTICE("77211111");
	for (tmp = 0; (access_CoreReadByte(0x108) == 0) && (tmp < 1000); tmp++)
	{
		LOG_NOTICE("111111");
	}
	//log_notice("api_CoreRead(0x108) = 0x%x", access_CoreReadByte(0x108));
	if (access_CoreReadByte(0x108) == 0x2)
	{	/* i2c done */
		//tmp1 = gx3211_phy_I2cRead(addr);
		tmp1 = gx3211_halI2cMasterPhy_ReadData(PHY_I2CM_BASE_ADDR);
	}else{
		if (tmp >= 1000)
		{
			LOG_ERROR("reading PHY register time out");
		}
		else
		{
			LOG_ERROR("reading PHY register");
		}
	}
	return tmp1;
}
#else

u16 gx3211_phy_I2cReadSelf(u16 addr)
{
	int tmp = 0;
	u16 tmp1 = 0;
	u8 value;
	gx3211_phy_I2cReadRequest(0, addr);
	LOG_NOTICE("77211111");
	gx_mdelay(10);
	//for (tmp = 0; (access_CoreReadByte(0x108) == 0) && (tmp < 1000); tmp++)
	//{
	//		LOG_NOTICE("111111");
	//}
	//log_notice("api_CoreRead(0x108) = 0x%x", access_CoreReadByte(0x108));
	value = access_CoreReadByte(0x108);
	//if (access_CoreReadByte(0x108) == 0x2)
	gx_printf("----value = 0x%x\n", value);
	if (value == 0x2)
	{	/* i2c done */
		tmp1 = gx3211_phy_I2cRead(0);
	}else{
		if (tmp >= 1000)
		{
			LOG_ERROR("reading PHY register time out");
		}
		else
		{
			LOG_ERROR("reading PHY register");
		}
	}
	return tmp1;
}

#endif
#endif

struct gxav_hdmi_phy_ops gx3211_hdmi_phy_ops = {
	.init               = gx3211_phy_Initialize,
	.configure          = gx3211_phy_Configure,
	.standby            = gx3211_phy_Standby,
	.enable_hpd_sense   = gx3211_phy_EnableHpdSense,
	.disable_hpd_sense  = gx3211_phy_DisableHpdSense,
	.hotplug_detected   = gx3211_phy_HotPlugDetected,
	.interrupt_enable   = gx3211_phy_InterruptEnable,
	.gen2pddq           = gx3211_phy_Gen2PDDQ,
	.gen2tx_poweron     = gx3211_phy_Gen2TxPowerOn,
};
