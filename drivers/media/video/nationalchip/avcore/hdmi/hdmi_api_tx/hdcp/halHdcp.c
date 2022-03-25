/*
 * halHdcp.c
 *
 *  Created on: Jul 19, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */
#include "halHdcp.h"
#include "../bsp/access.h"
#include "../util/log.h"

const u8 A_HDCPCFG0 = 0x00;
const u8 A_HDCPCFG1 = 0x01;
const u8 A_HDCPOBS0 = 0x02;
const u8 A_HDCPOBS1 = 0x03;
const u8 A_HDCPOBS2 = 0x04;
const u8 A_HDCPOBS3 = 0x05;
const u8 A_APIINTCLR = 0x06;
const u8 A_APIINTSTA = 0x07;
const u8 A_APIINTMSK = 0x08;
const u8 A_VIDPOLCFG = 0x09;
const u8 A_OESSWCFG = 0x0A;
const u8 A_COREVERLSB = 0x14;
const u8 A_COREVERMSB = 0x15;
const u8 A_KSVMEMCTRL = 0x16;
const u8 A_MEMORY = 0x20;

const u16 A_HDCPREG_RMLSTS = 0x780F;
const u16 A_HDCPREG_RMLCTL = 0x780E;
const u16 A_HDCPREG_DPK6	= 0x7818;
const u16 A_HDCPREG_DPK5	= 0x7817;
const u16 A_HDCPREG_DPK4	= 0x7816;
const u16 A_HDCPREG_DPK3	= 0x7815;
const u16 A_HDCPREG_DPK2	= 0x7814;
const u16 A_HDCPREG_DPK1	= 0x7813;
const u16 A_HDCPREG_DPK0	= 0x7812;
const u16 A_HDCPREG_SEED1 = 0x7811;
const u16 A_HDCPREG_SEED0 = 0x7810;

const u16 REVOCATION_OFFSET = 0x299;
const u16 HDCPREG_BKSV0 = 0x2800;
const u16 HDCPREG_BKSV4 = 0x2804;
const u16 HDCPREG_ANCONF = 0x2805;
const u16 HDCPREG_AN0 = 0x2806;
const u16 HDCPREG_AN7 = 0x280D;

void halHdcp_DeviceMode(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG0), 0, 1);
}

void halHdcp_EnableFeature11(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG0), 1, 1);
}

void halHdcp_RxDetected(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG0), 2, 1);
}

void halHdcp_EnableAvmute(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG0), 3, 1);
}

void halHdcp_RiCheck(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG0), 4, 1);
}

void halHdcp_BypassEncryption(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG0), 5, 1);
}

void halHdcp_EnableI2cFastMode(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG0), 6, 1);
}

void halHdcp_EnhancedLinkVerification(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG0), 7, 1);
}

void halHdcp_SwReset(u16 baseAddr)
{
	LOG_TRACE();
	/* active low */
	access_CoreWrite(0, (baseAddr + A_HDCPCFG1), 0, 1);
}

void halHdcp_DisableEncryption(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG1), 1, 1);
}

void halHdcp_EncodingPacketHeader(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG1), 2, 1);
}

void halHdcp_DisableKsvListCheck(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG1), 3, 1);
}

u8 halHdcp_HdcpEngaged(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreRead((baseAddr + A_HDCPOBS0), 0, 1);
}

u8 halHdcp_AuthenticationState(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreRead((baseAddr + A_HDCPOBS0), 1, 7);
}

u8 halHdcp_CipherState(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreRead((baseAddr + A_HDCPOBS2), 3, 3);
}

u8 halHdcp_RevocationState(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreRead((baseAddr + A_HDCPOBS1), 0, 3);
}

u8 halHdcp_OessState(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreRead((baseAddr + A_HDCPOBS1), 3, 3);
}

u8 halHdcp_EessState(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreRead((baseAddr + A_HDCPOBS2), 0, 3);
}

u8 halHdcp_DebugInfo(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreReadByte(baseAddr + A_HDCPOBS3);
}

void halHdcp_InterruptClear(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + A_APIINTCLR));
}

u8 halHdcp_InterruptStatus(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreReadByte(baseAddr + A_APIINTSTA);
}

void halHdcp_InterruptMask(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + A_APIINTMSK));
}

u8 halHdcp_InterruptMaskStatus(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreReadByte(baseAddr + A_APIINTMSK);
}

void halHdcp_HSyncPolarity(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_VIDPOLCFG), 1, 1);
}

void halHdcp_VSyncPolarity(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_VIDPOLCFG), 3, 1);
}

void halHdcp_DataEnablePolarity(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_VIDPOLCFG), 4, 1);
}

void halHdcp_UnencryptedVideoColor(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, (baseAddr + A_VIDPOLCFG), 5, 2);
}

void halHdcp_OessWindowSize(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + A_OESSWCFG));
}

u16 halHdcp_CoreVersion(u16 baseAddr)
{
	u16 version = 0;
	LOG_TRACE();
	version = access_CoreReadByte(baseAddr + A_COREVERLSB);
	version |= access_CoreReadByte(baseAddr + A_COREVERMSB) << 8;
	return version;
}

u8 halHdcp_ControllerVersion(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreReadByte(baseAddr);
}

void halHdcp_MemoryAccessRequest(u16 baseAddr, u8 bit)
{
	LOG_TRACE();
	access_CoreWrite(bit, (baseAddr + A_KSVMEMCTRL), 0, 1);
}

u8 halHdcp_MemoryAccessGranted(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreRead((baseAddr + A_KSVMEMCTRL), 1, 1);
}

void halHdcp_UpdateKsvListState(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_KSVMEMCTRL), 3, 1);
	access_CoreWrite(1, (baseAddr + A_KSVMEMCTRL), 2, 1);
	access_CoreWrite(0, (baseAddr + A_KSVMEMCTRL), 2, 1);
}

u8 halHdcp_KsvListRead(u16 baseAddr, u16 addr)
{
	LOG_TRACE1(addr);
	if (addr >= REVOCATION_OFFSET)
	{
		LOG_WARNING("Invalid address");
	}
	return access_CoreReadByte((baseAddr + A_MEMORY) + addr);
}

void halHdcp_RevocListWrite(u16 baseAddr, u16 addr, u8 data)
{
	LOG_TRACE2(addr, data);
	access_CoreWriteByte(data, (baseAddr + A_MEMORY) + REVOCATION_OFFSET + addr);
}

void halHdcp_AnWrite(u16 baseAddr, u8 * data)
{
	short i = 0;
	LOG_TRACE();
	if (data != 0)
	{
		LOG_TRACE1(data[0]);
		for (i = 0; i <= (HDCPREG_AN7 - HDCPREG_AN0); i++)
		{
			access_CoreWriteByte(data[i], (baseAddr + HDCPREG_AN0 + i));
		}
		access_CoreWrite(1, (baseAddr + HDCPREG_ANCONF), 0, 1);
	}
	else
	{
		access_CoreWrite(0, (baseAddr + HDCPREG_ANCONF), 0, 1);
	}
}

u8 halHdcp_BksvRead(u16 baseAddr, u8 * bksv)
{
	short i = 0;
	if (bksv != 0)
	{
		LOG_TRACE1(bksv[0]);
		for (i = 0; i <= (HDCPREG_BKSV4 - HDCPREG_BKSV0); i++)
		{
			bksv[i] = access_CoreReadByte(baseAddr + HDCPREG_BKSV0 + i);
		}
		return i;
	}
	else
	{
		return 0;
	}
}
void halHdcp_WriteAksv(u16 baseAddr, u8 aksv[7])
{
	access_CoreWrite(aksv[0], (baseAddr + A_HDCPREG_DPK6), 0, 8);
	access_CoreWrite(aksv[1], (baseAddr + A_HDCPREG_DPK5), 0, 8);
	access_CoreWrite(aksv[2], (baseAddr + A_HDCPREG_DPK4), 0, 8);
	access_CoreWrite(aksv[3], (baseAddr + A_HDCPREG_DPK3), 0, 8);
	access_CoreWrite(aksv[4], (baseAddr + A_HDCPREG_DPK2), 0, 8);
	access_CoreWrite(aksv[5], (baseAddr + A_HDCPREG_DPK1), 0, 8);
	access_CoreWrite(aksv[6], (baseAddr + A_HDCPREG_DPK0), 0, 8);
}
void halHdcp_WaitMemAccess(u16 baseAddr)
{
	while(!access_CoreRead((baseAddr + A_HDCPREG_RMLSTS), 6, 1));
}
void halHdcp_WriteSeed(u16 baseAddr, u8 encKey[2])
{
	access_CoreWrite(encKey[0], (baseAddr + A_HDCPREG_SEED1), 0, 8);
	access_CoreWrite(encKey[1], (baseAddr + A_HDCPREG_SEED0), 0, 8);
}
void halHdcp_EnableEncrypt(u16 baseAddr, u8 enable)
{
	access_CoreWrite(enable, (baseAddr + A_HDCPREG_RMLCTL), 0, 1);
}
void halHdcp_StoreEncryptKeys(u16 baseAddr, u8 keys[560])
{
	int key_nr = 0;
	for( key_nr = 0; key_nr < 280; key_nr = key_nr + 7 )
	{
		access_CoreWrite(keys[key_nr + 0], (baseAddr + A_HDCPREG_DPK6), 0, 8);
		access_CoreWrite(keys[key_nr + 1], (baseAddr + A_HDCPREG_DPK5), 0, 8);
		access_CoreWrite(keys[key_nr + 2], (baseAddr + A_HDCPREG_DPK4), 0, 8);
		access_CoreWrite(keys[key_nr + 3], (baseAddr + A_HDCPREG_DPK3), 0, 8);
		access_CoreWrite(keys[key_nr + 4], (baseAddr + A_HDCPREG_DPK2), 0, 8);
		access_CoreWrite(keys[key_nr + 5], (baseAddr + A_HDCPREG_DPK1), 0, 8);
		access_CoreWrite(keys[key_nr + 6], (baseAddr + A_HDCPREG_DPK0), 0, 8);
		halHdcp_WaitMemAccess(baseAddr);
	}
}
