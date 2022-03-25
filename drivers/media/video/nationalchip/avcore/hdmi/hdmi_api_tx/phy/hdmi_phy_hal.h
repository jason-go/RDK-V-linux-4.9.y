#ifndef __HDMI_PHY_HAL_H__
#define __HDMI_PHY_HAL_H__

#include "../util/types.h"
#include "../util/log.h"

/* phy pll lock timeout */
//#define PHY_TIMEOUT 1000
#define PHY_TIMEOUT (1000*50)

struct gxav_hdmi_phy_ops {
	int (*init)(u16 baseAddr, u8 dataEnablePolarity, u8 phy_model);
	int (*configure)(u16 baseAddr, u16 pClk, u8 cRes, u8 pRep, u8 phy_model);
	int (*standby)(u16 baseAddr);
	int (*enable_hpd_sense)(u16 baseAddr);
	int (*disable_hpd_sense)(u16 baseAddr);
	int (*hotplug_detected)(u16 baseAddr);
	int (*interrupt_enable)(u16 baseAddr, u8 value);
	int (*gen2pddq)(u16 baseAddr, u8 bit);
	int (*gen2tx_poweron)(u16 baseAddr, u8 bit);
};

void gxav_hdmi_phy_register(struct gxav_hdmi_phy_ops *ops);
void gxav_hdmi_phy_unregister(void);

/** Initialise phy and put into a known state
 * @param baseAddr of controller
 * @param dataEnablePolarity data enable polarity
 */
int phy_Initialize(u16 baseAddr, u8 dataEnablePolarity, u8 phy_model);
/** Bring up PHY and start sending media for a specified pixel clock, pixel
 * repetition and colour resolution (to calculate TMDS)
 * @param baseAddr of controller
 * @param pClk pixel clock
 * @param cRes colour depth or colour resolution (bits/component/pixel)
 * @param pRep pixel repetition
 */
int phy_Configure (u16 baseAddr, u16 pClk, u8 cRes, u8 pRep, u8 phy_model);
/** Set PHY to standby mode - turn off all interrupts
 * @param baseAddr of controller
 */
int phy_Standby(u16 baseAddr);
/** Enable HPD sensing ciruitry
 * @param baseAddr of controller
 */
int phy_EnableHpdSense(u16 baseAddr);
/** Disable HPD sensing ciruitry
 * @param baseAddr of controller
 */
int phy_DisableHpdSense(u16 baseAddr);
/**
 * Detects the signal on the HPD line and 
 * upon change, it inverts polarity of the interrupt
 * bit so that interrupt raises only on change
 * @param baseAddr of controller
 * @return TRUE the HPD line is asserted
 */
int phy_HotPlugDetected(u16 baseAddr);
/**
 * @param baseAddr of controller
 * @param value of mask of interrupt register
 */
int phy_InterruptEnable(u16 baseAddr, u8 value);
/**
 * @param baseAddr of controller
 * @param value
 */

int phy_Gen2PDDQ(u16 baseAddr, u8 bit);
int phy_Gen2TxPowerOn(u16 baseAddr, u8 bit);

#endif /* __HDMI_PHY_HAL_H__ */
