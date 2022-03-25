#include "gxav_common.h"
#include "hdmi_phy_hal.h"

static struct gxav_hdmi_phy_ops *_hdmi_phy_ops = NULL;

void gxav_hdmi_phy_register(struct gxav_hdmi_phy_ops *ops)
{
	_hdmi_phy_ops = ops;

	return ;
}

void gxav_hdmi_phy_unregister(void)
{
	_hdmi_phy_ops = NULL;

	return ;
}

int phy_Initialize(u16 baseAddr, u8 dataEnablePolarity, u8 phy_model)
{
	if (_hdmi_phy_ops && _hdmi_phy_ops->init) {
		return _hdmi_phy_ops->init(baseAddr, dataEnablePolarity, phy_model);
	}

	return FALSE;
}

int phy_Configure(u16 baseAddr, u16 pClk, u8 cRes, u8 pRep, u8 phy_model)
{
	if (_hdmi_phy_ops && _hdmi_phy_ops->configure) {
		return _hdmi_phy_ops->configure(baseAddr, pClk, cRes, pRep, phy_model);
	}

	return FALSE;
}

int phy_Standby(u16 baseAddr)
{
	if (_hdmi_phy_ops && _hdmi_phy_ops->standby) {
		return _hdmi_phy_ops->standby(baseAddr);
	}

	return FALSE;
}

int phy_EnableHpdSense(u16 baseAddr)
{
	if (_hdmi_phy_ops && _hdmi_phy_ops->enable_hpd_sense) {
		return _hdmi_phy_ops->enable_hpd_sense(baseAddr);
	}

	return FALSE;
}

int phy_DisableHpdSense(u16 baseAddr)
{
	if (_hdmi_phy_ops && _hdmi_phy_ops->disable_hpd_sense) {
		return _hdmi_phy_ops->disable_hpd_sense(baseAddr);
	}

	return FALSE;
}

int phy_HotPlugDetected(u16 baseAddr)
{
	if (_hdmi_phy_ops && _hdmi_phy_ops->hotplug_detected) {
		return _hdmi_phy_ops->hotplug_detected(baseAddr);
	}

	return FALSE;
}

int phy_InterruptEnable(u16 baseAddr, u8 value)
{
	if (_hdmi_phy_ops && _hdmi_phy_ops->interrupt_enable) {
		return _hdmi_phy_ops->interrupt_enable(baseAddr, value);
	}

	return FALSE;
}

int phy_Gen2PDDQ(u16 baseAddr, u8 bit)
{
	if (_hdmi_phy_ops && _hdmi_phy_ops->gen2pddq) {
		return _hdmi_phy_ops->gen2pddq(baseAddr, bit);
	}

	return FALSE;
}

int phy_Gen2TxPowerOn(u16 baseAddr, u8 bit)
{
	if (_hdmi_phy_ops && _hdmi_phy_ops->gen2tx_poweron) {
		return _hdmi_phy_ops->gen2tx_poweron(baseAddr, bit);
	}

	return FALSE;
}
