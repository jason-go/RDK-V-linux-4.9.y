/*
 * hdcp.h
 *
 *  Created on: Jul 21, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#ifndef HDCP_H_
#define HDCP_H_

#include "../util/types.h"
#include "hdcpParams.h"

typedef enum
{
	HDCP_IDLE = 0,
	HDCP_KSV_LIST_READY,
	HDCP_ERR_KSV_LIST_NOT_VALID,
	HDCP_KSV_LIST_ERR_DEPTH_EXCEEDED,
	HDCP_KSV_LIST_ERR_MEM_ACCESS,
	HDCP_ENGAGED,
	HDCP_FAILED
}
hdcp_status_t;

/* HDCP Interrupt bit fields */
#define INT_KSV_ACCESS   (0)
#define INT_KSV_SHA1     (1)
#define INT_HDCP_FAIL    (6)
#define INT_HDCP_ENGAGED (7)

int hdcp_Mask(u16 baseAddr);
int hdcp_UnMask(u16 baseAddr);
/**
 * @param baseAddr base address of controller
 * @return TRUE if successful
 */

/**
 * @param baseAddr Base address of the HDCP module registers
 * @param dataEnablePolarity
 * @return TRUE if successful
 */
int hdcp_Initialize(u16 baseAddr, int dataEnablePolarity);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param params HDCP parameters
 * @param hdmi TRUE if HDMI, FALSE if DVI
 * @param hsPol HSYNC polarity
 * @param vsPol VSYNC polarity
 * @return TRUE if successful
 */
int hdcp_Configure(u16 baseAddr, hdcpParams_t *params, int hdmi, int hsPol,
		int vsPol);
/**
 * The method handles DONE and ERROR events.
 * A DONE event will trigger the retrieving the read byte, and sending a request to read the following byte. The EDID is read until the block is done and then the reading moves to the next block.
 *  When the block is successfully read, it is sent to be parsed.
 * @param baseAddr Base address of the HDCP module registers
 * @param hpd on or off
 * @param state of the HDCP engine interrupts
 * @param param to be returned to application:
 * 		no of KSVs in KSV LIST if KSV_LIST_EVENT
 * 		1 (engaged) 0 (fail) if HDCP_EGNAGED_EVENT
 * @return the state of which the event was handled (FALSE for fail)
 */
/* @param ksvHandler Handler to call when KSV list is ready*/
u8 hdcp_EventHandler(u16 baseAddr, int hpd, u8 state, int * param);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param detected if RX is detected (TRUE or FALSE)
 */
void hdcp_RxDetected(u16 baseAddr, int detected);
/**
 * Enter or exit AV mute mode
 * @param baseAddr Base address of the HDCP module registers
 * @param enable the HDCP AV mute
 */
void hdcp_AvMute(u16 baseAddr, int enable);
/**
 * Bypass data encryption stage
 * @param baseAddr Base address of the HDCP module registers
 * @param bypass the HDCP AV mute
 */
void hdcp_BypassEncryption(u16 baseAddr, int bypass);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param disable the HDCP encrption
 */
void hdcp_DisableEncryption(u16 baseAddr, int disable);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @return TRUE if engaged
 */
int hdcp_Engaged(u16 baseAddr);
/**
 * The engine goes through the authentication
 * statesAs defined in the HDCP spec
 * @param baseAddr Base address of the HDCP module registers
 * @return a the state of the authentication machine
 * @note Used for debug purposes
 */
u8 hdcp_AuthenticationState(u16 baseAddr);
/**
 * The engine goes through several cipher states
 * @param baseAddr Base address of the HDCP module registers
 * @return a the state of the cipher machine
 * @note Used for debug purposes
 */
u8 hdcp_CipherState(u16 baseAddr);
/**
 * The engine goes through revocation states
 * @param baseAddr Base address of the HDCP module registers
 * @return a the state of the revocation the engine is going through
 * @note Used for debug purposes
 */
u8 hdcp_RevocationState(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @return debug info register - info from Bcaps and Bstatus
 * @note please refer to DWC_HDMI_TX_controller databook
 */
u8 hdcp_DebugInfo(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param data pointer to memory where SRM info is located
 * @return TRUE if successful
 */
int hdcp_SrmUpdate(u16 baseAddr, const u8 * data);
/**
 * @param baseAddr base address of HDCP module registers
 * @param value mask of interrupts to enable
 * @return TRUE if successful
 */
int hdcp_InterruptEnable(u16 baseAddr, u8 value);
/**
 * @param baseAddr base address of HDCP module registers
 * @param value mask of interrupts to disable
 * @return TRUE if successful
 */
int hdcp_InterruptDisable(u16 baseAddr, u8 value);
/**
 * @param baseAddr base address of HDCP module registers
 * @return HDCP interrupts state
 */
u8 hdcp_InterruptStatus(u16 baseAddr);
/**
 * Clear HDCP interrupts
 * @param baseAddr base address of controller
 * @param value mask of interrupts to clear
 * @return TRUE if successful
 */
int hdcp_InterruptClear(u16 baseAddr, u8 value);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param data to write: an 8 byte array of the 64-byte
 * pseudo-random value with the An to be written.
 * If null, the Random Number Generator Interface
 * (irndnum, orndnumgenena) of the DWC_hdmi_tx core will be used
 */
void hdcp_AnWrite(u16 baseAddr, u8 * data);
/**
 * Read Sink BKSV (40-bit value)
 * @param baseAddr Base address of the HDCP module registers
 * @param bksv a 5-byte (40-bit) buffer to write the BKSV to
 * @return the number of bytes read from the HDCP engine (!=5 -> error)
 */
u8 hdcp_BksvRead(u16 baseAddr, u8 * bksv);

void hdcp_WriteDpkKeys(u8 baseAddr, hdcpParams_t *params);

#endif /* HDCP_H_ */
