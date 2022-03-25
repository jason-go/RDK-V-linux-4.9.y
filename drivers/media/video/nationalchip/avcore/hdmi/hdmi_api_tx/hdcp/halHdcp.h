/*
 * halHdcp.h
 *
 *  Created on: Jul 19, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02 
 */
 /** @file
 *   HAL (hardware abstraction layer) of HDCP engine register block
 */

#ifndef HALHDCP_H_
#define HALHDCP_H_

#include "../util/types.h"
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_DeviceMode(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_EnableFeature11(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_RxDetected(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_EnableAvmute(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_RiCheck(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_BypassEncryption(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_EnableI2cFastMode(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_EnhancedLinkVerification(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
void halHdcp_SwReset(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_DisableEncryption(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_EncodingPacketHeader(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_DisableKsvListCheck(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
u8 halHdcp_HdcpEngaged(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
u8 halHdcp_AuthenticationState(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
u8 halHdcp_CipherState(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
u8 halHdcp_RevocationState(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
u8 halHdcp_OessState(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
u8 halHdcp_EessState(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
u8 halHdcp_DebugInfo(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param value
 */
void halHdcp_InterruptClear(u16 baseAddr, u8 value);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
u8 halHdcp_InterruptStatus(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param value mask of interrupt
 */
void halHdcp_InterruptMask(u16 baseAddr, u8 value);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @return value mask of interrupt
 */
u8 halHdcp_InterruptMaskStatus(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_HSyncPolarity(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_VSyncPolarity(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_DataEnablePolarity(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param value
 */
void halHdcp_UnencryptedVideoColor(u16 baseAddr, u8 value);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param value
 */
void halHdcp_OessWindowSize(u16 baseAddr, u8 value);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
u16 halHdcp_CoreVersion(u16 baseAddr);

u8 halHdcp_ControllerVersion(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_MemoryAccessRequest(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 */
u8 halHdcp_MemoryAccessGranted(u16 baseAddr);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param bit
 */
void halHdcp_UpdateKsvListState(u16 baseAddr, u8 bit);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param addr in list
 */
u8 halHdcp_KsvListRead(u16 baseAddr, u16 addr);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param addr in list
 * @param data
 */
void halHdcp_RevocListWrite(u16 baseAddr, u16 addr, u8 data);
/**
 * @param baseAddr Base address of the HDCP module registers
 * @param data to write: an 8 byte array of the 64-byte
 * pseudo-random value with the An to be written.
 * If null, the Random Number Generator Interface
 * (irndnum, orndnumgenena) of the DWC_hdmi_tx core will be used
 */
void halHdcp_AnWrite(u16 baseAddr, u8 * data);
/**
 * Read Sink BKSV (40-bit value)
 * @param baseAddr Base address of the HDCP module registers
 * @param bksv a 5-byte (40-bit) buffer to write the BKSV to
 * @return the number of bytes read from the HDCP engine (!=5 -> error)
 */
u8 halHdcp_BksvRead(u16 baseAddr, u8 * bksv);

void halHdcp_WaitMemAccess(u16 baseAddr);

void halHdcp_WriteAksv(u16 baseAddr, u8 aksv[7]);

void halHdcp_WriteSeed(u16 baseAddr, u8 encKey[2]);

void halHdcp_EnableEncrypt(u16 baseAddr, u8 enable);

void halHdcp_StoreEncryptKeys(u16 baseAddr, u8 keys[560]);

#endif /* HALHDCP_H_ */
