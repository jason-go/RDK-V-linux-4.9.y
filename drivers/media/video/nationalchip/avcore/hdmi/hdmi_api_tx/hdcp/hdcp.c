/*
 * hdcp.c
 *
 *  Created on: Jul 21, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */
#include "hdcp.h"
#include "hdcpVerify.h"
#include "halHdcp.h"
//#include "stdlib.h"
#include "../util/log.h"
#include "../util/error.h"
#include "../core/halInterrupt.h"

/* HDCP module registers offset */
static const u16 HDCP_BASE_ADDR = 0x5000;

/* local static variables */
static u8  hdcp_mNoOfDevices;
static u8* hdcp_mKsvListBuffer;

/* #define HW_WA_HDCP_DC0 */
/* #define HW_WA_HDCP_REP */

int hdcp_Initialize(u16 baseAddr, int dataEnablePolarity)
{
	LOG_TRACE();
	halHdcp_RxDetected(baseAddr + HDCP_BASE_ADDR, 0);
	halHdcp_DataEnablePolarity(baseAddr + HDCP_BASE_ADDR, (dataEnablePolarity
			== TRUE) ? 1 : 0);
	halHdcp_DisableEncryption(baseAddr + HDCP_BASE_ADDR, 1);
	return TRUE;
}

#ifdef HW_WA_HDCP_DC0
#include "../edid/halEdid.h"
#include "../core/halInterrupt.h"
#include "../bsp/system.h"
#define HDCP_TIMEOUT_5S 	32
/* Aksv = 0xE6CB130E59 */
static const u8 aksv[] = {0xE6, 0xCB, 0x13, 0x0E, 0x59}; /* TODO: to be replaced by vendor code */
static const u8 an[] = {0x5A, 0x84, 0x5A, 0xD9, 0x1F, 0x3C, 0xC3, 0x88};

u8 readI2c(u16 baseAddr, u8 address)
{
	int interrupt = 0;
	halEdid_RequestAddr(baseAddr + 0x7E00, address);
	halEdid_RequestRead(baseAddr + 0x7E00);
	do
	{
		interrupt = halInterrupt_I2cDdcState(baseAddr + 0x0100);
	}
	while(interrupt == 0);
	if (interrupt & BIT(0))
	{
		LOG_ERROR2("I2C error - READ", address);
	}
	halInterrupt_I2cDdcClear(baseAddr + 0x0100, interrupt); /* clear i2c interrupt */
	return halEdid_ReadData(baseAddr + 0x7E00);
}

u8 writeI2c(u16 baseAddr, u8 address, u8 data)
{
	int interrupt = 0;
	halEdid_RequestAddr(baseAddr + 0x7E00, address);
	halEdid_WriteData(baseAddr + 0x7E00, data);
	halEdid_RequestWrite(baseAddr + 0x7E00);
	do
	{
		interrupt = halInterrupt_I2cDdcState(baseAddr + 0x0100);
	}
	while(interrupt == 0);
	if (interrupt & BIT(0))
	{
		LOG_ERROR2("I2C error- WRITE", address);
	}
	halInterrupt_I2cDdcClear(baseAddr + 0x0100, interrupt); /* clear i2c interrupt */
	return TRUE;
}
#endif

int hdcp_Mask(u16 baseAddr)
{
	halInterrupt_MuteI2cClear(baseAddr + 0x0100, 0x3);
	return 0;
}

int hdcp_UnMask(u16 baseAddr)
{
	halInterrupt_MuteI2cClear(baseAddr + 0x0100, 0x0);
	return 0;
}

int hdcp_Configure(u16 baseAddr, hdcpParams_t *params, int hdmi, int hsPol,
		int vsPol)
{
#ifdef HW_WA_HDCP_DC0
	u8 bCaps = 0;
	u16 bStatus = 0;
	int isRepeater = FALSE;
	long long i = 0;
	u8 temp = halInterrupt_MuteI2cState(baseAddr + 0x0100);
	LOG_NOTICE("HW_WA_HDCP_DC0");
	halInterrupt_MuteI2cClear(baseAddr + 0x0100, 0x3);
	halEdid_MaskInterrupts(baseAddr + 0x7E00, FALSE); /* enable interrupts */
	halEdid_SlaveAddress(baseAddr + 0x7E00, 0x3A); /* HW deals with LSB */
	/* video related */
	halHdcp_DeviceMode(baseAddr + HDCP_BASE_ADDR, (hdmi == TRUE) ? 1 : 0);
	do
	{
		/* read Bcaps */
		bCaps = readI2c(baseAddr, 0x40);
		i = 0;
		do
		{	/* poll Bstatus until video mode(DVI/HDMI) is equal */
			bStatus = (readI2c(baseAddr, 0x42) << 8);
			bStatus |= readI2c(baseAddr, 0x41);
			if (i >= HDCP_TIMEOUT_5S)
			{
				halEdid_MaskInterrupts(baseAddr + 0x7E00, TRUE); /* disable interrupts */
				halInterrupt_MuteI2cClear(baseAddr + 0x0100, temp); /* return interrupts mute to original state */
				halInterrupt_I2cDdcClear(baseAddr + 0x0100, 0xff); /* clear i2c interrupt */
				error_Set(ERR_HDCP_FAIL);
				return FALSE;
			}
			i++;
		}
		while(hdmi != ((bStatus & BIT(12)) >> 12));
		bCaps = readI2c(baseAddr, 0x40);
		isRepeater = ((bCaps & BIT(6)) >> 6);
		if (!isRepeater)
		{	/* device is no repeater, controller takes over */
			LOG_NOTICE("Device NOT repeater");
			break;
		}
		/* write Ainfo - 0x00 */
		writeI2c(baseAddr, 0x15, 0x00);
		/* write An  */
		writeI2c(baseAddr, 0x18, an[7]);
		writeI2c(baseAddr, 0x19, an[6]);
		writeI2c(baseAddr, 0x1A, an[5]);
		writeI2c(baseAddr, 0x1B, an[4]);
		writeI2c(baseAddr, 0x1C, an[3]);
		writeI2c(baseAddr, 0x1D, an[2]);
		writeI2c(baseAddr, 0x1E, an[1]);
		writeI2c(baseAddr, 0x1F, an[0]);
		/* write Aksv */
		writeI2c(baseAddr, 0x10, aksv[4]);
		writeI2c(baseAddr, 0x11, aksv[3]);
		writeI2c(baseAddr, 0x12, aksv[2]);
		writeI2c(baseAddr, 0x13, aksv[1]);
		writeI2c(baseAddr, 0x14, aksv[0]);
		/* read Bksv */
		bCaps = readI2c(baseAddr, 0x00);
		bCaps = readI2c(baseAddr, 0x01);
		bCaps = readI2c(baseAddr, 0x02);
		bCaps = readI2c(baseAddr, 0x03);
		bCaps = readI2c(baseAddr, 0x04);
		/* wait 100ms min */
		system_SleepMs(110);
		/* Read r0' */
		bCaps = readI2c(baseAddr, 0x09);
		bCaps = readI2c(baseAddr, 0x08);
		/* see if device is ready - 5 seconds limit */
		i = 0;
		do
		{	/* read Bcaps */
			bCaps = readI2c(baseAddr, 0x40);
			system_SleepMs(100);
			i++;
		}
		while((((bCaps & BIT(5)) >> 5) == 0) && (i < HDCP_TIMEOUT_5S));
		if (i >= HDCP_TIMEOUT_5S)
		{
			break;
		}
		bStatus = readI2c(baseAddr, 0x41);
		bStatus |= (readI2c(baseAddr, 0x42) << 8);
		if ((bStatus & 0x007f) != 0)
		{	/* device count is no zero, controller takes over */
			LOG_NOTICE("device count > 0");
			break;
		}
		/* cannot recover - get out */
		halEdid_MaskInterrupts(baseAddr + 0x7E00, TRUE); /* disable interrupts */
		halInterrupt_MuteI2cClear(baseAddr + 0x0100, temp); /* return interrupts mute to original state */
		error_Set(ERR_HDCP_FAIL);
		LOG_ERROR("Repeater with Device Count Zero NOT supported");
		halInterrupt_I2cDdcClear(baseAddr + 0x0100, 0xff); /* clear i2c interrupt */
		return FALSE;
	}
	while(1);
	halEdid_MaskInterrupts(baseAddr + 0x7E00, TRUE); /* disable interrupts */
	halInterrupt_MuteI2cClear(baseAddr + 0x0100, temp); /* return interrupts mute to original state */
#endif
	LOG_TRACE();
	/* video related */
	halHdcp_DeviceMode(baseAddr + HDCP_BASE_ADDR, (hdmi == TRUE) ? 1 : 0);
	halHdcp_HSyncPolarity(baseAddr + HDCP_BASE_ADDR, (hsPol == TRUE) ? 1 : 0);
	halHdcp_VSyncPolarity(baseAddr + HDCP_BASE_ADDR, (vsPol == TRUE) ? 1 : 0);

	/* HDCP only */
	halHdcp_EnableFeature11(baseAddr + HDCP_BASE_ADDR,
			(hdcpParams_GetEnable11Feature(params) == TRUE) ? 1 : 0);
	halHdcp_RiCheck(baseAddr + HDCP_BASE_ADDR, (hdcpParams_GetRiCheck(params)
			== TRUE) ? 1 : 0);
	halHdcp_EnableI2cFastMode(baseAddr + HDCP_BASE_ADDR,
			(hdcpParams_GetI2cFastMode(params) == TRUE) ? 1 : 0);
	halHdcp_EnhancedLinkVerification(baseAddr + HDCP_BASE_ADDR,
			(hdcpParams_GetEnhancedLinkVerification(params) == TRUE) ? 1 : 0);

	hdcp_mNoOfDevices = hdcpParams_GetNoOfSupportedDevices(params);
	hdcp_mKsvListBuffer = hdcpParams_GetKsvListBuffer(params);
	/* fixed */
	halHdcp_EnableAvmute(baseAddr + HDCP_BASE_ADDR, 1);
	halHdcp_BypassEncryption(baseAddr + HDCP_BASE_ADDR, 0);
	halHdcp_DisableEncryption(baseAddr + HDCP_BASE_ADDR, 1);
	halHdcp_UnencryptedVideoColor(baseAddr + HDCP_BASE_ADDR, 0x03);
	halHdcp_OessWindowSize(baseAddr + HDCP_BASE_ADDR, 64);
	halHdcp_EncodingPacketHeader(baseAddr + HDCP_BASE_ADDR, 1);

	halHdcp_SwReset(baseAddr + HDCP_BASE_ADDR);
	/* check if controler is version 1.4a or higher to support DPK keys external storage */
	if(halHdcp_ControllerVersion(baseAddr) >= 0x14)
	{
		hdcp_WriteDpkKeys(baseAddr, params);
	}
	halHdcp_InterruptClear(baseAddr + HDCP_BASE_ADDR, ~0);
	/* enable KSV list SHA1 verification interrupt */
	halHdcp_InterruptMask(baseAddr + HDCP_BASE_ADDR, (~BIT(INT_KSV_SHA1)) &
			halHdcp_InterruptMaskStatus(baseAddr + HDCP_BASE_ADDR));
	return TRUE;
}

u8 hdcp_EventHandler(u16 baseAddr, int hpd, u8 state, int * param)
{
	size_t list = 0;
	size_t size = 0;
	size_t attempt = 0;
	size_t i = 0;
	int memAccess = FALSE;
	int valid = HDCP_IDLE;
	LOG_TRACE();
	if (state == 0)
	{
		return HDCP_IDLE;
	}
	if ((state & BIT(INT_KSV_SHA1)) != 0)
	{
		valid = HDCP_IDLE;
		if (hdcp_mKsvListBuffer != 0)
		{
			halHdcp_MemoryAccessRequest(baseAddr + HDCP_BASE_ADDR, 1);
			for (attempt = 20; attempt > 0; attempt--)
			{
				if (halHdcp_MemoryAccessGranted(baseAddr + HDCP_BASE_ADDR) == 1)
				{
					memAccess = TRUE;
					list = (halHdcp_KsvListRead(baseAddr + HDCP_BASE_ADDR, 0)
							& KSV_MSK);/* * KSV_LEN; */
					if (list <= hdcp_mNoOfDevices)
					{
						size = (list * KSV_LEN) + HEADER + SHAMAX;
#if 0
						LOG_NOTICE2("size", size);
						int temp = 0;
#endif
						for (i = 0; i < size; i++)
						{
							if (i < HEADER)
							{ /* BSTATUS & M0 */
#if 1
								hdcp_mKsvListBuffer[(list * KSV_LEN) + i] = halHdcp_KsvListRead(baseAddr
										+ HDCP_BASE_ADDR, i);
#else
								temp = halHdcp_KsvListRead(baseAddr + HDCP_BASE_ADDR, i);
								hdcp_mKsvListBuffer[(list * KSV_LEN) + i] = temp;
								LOG_NOTICE3("", i, temp);
#endif
							}
							else if (i < (HEADER + (list * KSV_LEN)))
							{ /* KSV list */
#if 1
								hdcp_mKsvListBuffer[i - HEADER] = halHdcp_KsvListRead(baseAddr
										+ HDCP_BASE_ADDR, i);
#else
								temp = halHdcp_KsvListRead(baseAddr + HDCP_BASE_ADDR, i);
								hdcp_mKsvListBuffer[i - HEADER] = temp;
								LOG_NOTICE3("", i, temp);
#endif
							}
							else
							{ /* SHA */
#if 1
								hdcp_mKsvListBuffer[i] = halHdcp_KsvListRead(baseAddr
										+ HDCP_BASE_ADDR, i);
#else
								temp = halHdcp_KsvListRead(baseAddr + HDCP_BASE_ADDR, i);
								hdcp_mKsvListBuffer[i] = temp;
								LOG_NOTICE3("", i, temp);
#endif
							}
						}
#if 1
						valid = ((hdcpVerify_KSV(hdcp_mKsvListBuffer, size) == TRUE) ? HDCP_KSV_LIST_READY: HDCP_ERR_KSV_LIST_NOT_VALID);
#else
						hdcpVerify_KSV(hdcp_mKsvListBuffer, size);
						valid = HDCP_KSV_LIST_READY;
#endif
#ifdef HW_WA_HDCP_REP
						for (i = 0; !valid && i != (u8)(~0); i++)
						{
							hdcp_mKsvListBuffer[14] = i; /* M0 - LSB */
							valid = hdcpVerify_KSV(hdcp_mKsvListBuffer, size) == TRUE?
									HDCP_KSV_LIST_READY: HDCP_ERR_KSV_LIST_NOT_VALID;
						}
#endif
					}
					else
					{
						valid = HDCP_KSV_LIST_ERR_DEPTH_EXCEEDED;
						*param = 0;
						LOG_ERROR("depth exceeds KSV List memory");
					}
					halHdcp_MemoryAccessRequest(baseAddr + HDCP_BASE_ADDR, 0);
					halHdcp_UpdateKsvListState(baseAddr + HDCP_BASE_ADDR,
							(valid == HDCP_KSV_LIST_READY) ? 0 : 1);
#if 1
					*param = (valid == HDCP_KSV_LIST_READY)? list : 0;
#else
					*param = list;
#endif
				}
			}
			if (!memAccess)
			{
				LOG_ERROR("KSV List memory access denied");
				*param = 0;
				return HDCP_KSV_LIST_ERR_MEM_ACCESS;
			}
		}
		else
		{
			LOG_ERROR("KSV List memory not valid");
			*param = 0;
			valid = HDCP_KSV_LIST_ERR_DEPTH_EXCEEDED;
		}
	}
#if 0
	else if ((state & BIT(2)) != 0)
	{
		LOG_WARNING("keep out error interrupt");
	}
	else if ((state & BIT(3)) != 0)
	{
		LOG_WARNING("lost arbitration error interrupt");
	}
	else if ((state & BIT(4)) != 0)
	{
		LOG_WARNING("i2c nack error interrupt");
	}
#endif
	else if (((state & BIT(INT_HDCP_FAIL)) != 0) && ((state & BIT(INT_HDCP_ENGAGED)) != 0))
	{
		return HDCP_IDLE;
	}
	else if ((state & BIT(INT_HDCP_FAIL)) != 0)
	{
		*param = 0;
		return HDCP_FAILED;
	}
	else if ((state & BIT(INT_HDCP_ENGAGED)) != 0)
	{
		*param = 1;
		return HDCP_ENGAGED;
	}
	return valid;
}

void hdcp_RxDetected(u16 baseAddr, int detected)
{
	LOG_TRACE1(detected);
	halHdcp_RxDetected(baseAddr + HDCP_BASE_ADDR, (detected == TRUE) ? 1 : 0);
}

void hdcp_AvMute(u16 baseAddr, int enable)
{
	LOG_TRACE1(enable);
	halHdcp_EnableAvmute(baseAddr + HDCP_BASE_ADDR, (enable == TRUE) ? 1 : 0);
}

void hdcp_BypassEncryption(u16 baseAddr, int bypass)
{
	LOG_TRACE1(bypass);
	halHdcp_BypassEncryption(baseAddr + HDCP_BASE_ADDR, (bypass == TRUE) ? 1
			: 0);
}

void hdcp_DisableEncryption(u16 baseAddr, int disable)
{
	LOG_TRACE1(disable);
	halHdcp_DisableEncryption(baseAddr + HDCP_BASE_ADDR, (disable == TRUE) ? 1
			: 0);
}

int hdcp_Engaged(u16 baseAddr)
{
	LOG_TRACE();
	return (halHdcp_HdcpEngaged(baseAddr + HDCP_BASE_ADDR) != 0);
}

u8 hdcp_AuthenticationState(u16 baseAddr)
{
	/* hardware recovers automatically from a lost authentication */
	LOG_TRACE();
	return halHdcp_AuthenticationState(baseAddr + HDCP_BASE_ADDR);
}

u8 hdcp_CipherState(u16 baseAddr)
{
	LOG_TRACE();
	return halHdcp_CipherState(baseAddr + HDCP_BASE_ADDR);
}

u8 hdcp_RevocationState(u16 baseAddr)
{
	LOG_TRACE();
	return halHdcp_RevocationState(baseAddr + HDCP_BASE_ADDR);
}
u8 hdcp_DebugInfo(u16 baseAddr)
{
	LOG_TRACE();
	return halHdcp_DebugInfo(baseAddr + HDCP_BASE_ADDR);
}

int hdcp_SrmUpdate(u16 baseAddr, const u8 * data)
{
	size_t size = 0;
	size_t i = 0;
	size_t j = 0;
	size_t attempt = 10;
	int success = FALSE;
	LOG_TRACE();

	for (i = 0; i < VRL_NUMBER; i++)
	{
		size <<= i * 8;
		size |= data[VRL_LENGTH + i];
	}
	size += VRL_HEADER;

	if (hdcpVerify_SRM(data, size) == TRUE)
	{
		halHdcp_MemoryAccessRequest(baseAddr + HDCP_BASE_ADDR, 1);
		for (attempt = 20; attempt > 0; attempt--)
		{
			if (halHdcp_MemoryAccessGranted(baseAddr + HDCP_BASE_ADDR) == 1)
			{
				/* TODO fix only first-generation is handled */
				size -= (VRL_HEADER + VRL_NUMBER + 2 * DSAMAX);
				/* write number of KSVs */
				halHdcp_RevocListWrite(baseAddr + HDCP_BASE_ADDR, 0,
						(size == 0)? 0 : data[VRL_LENGTH + VRL_NUMBER]);
				halHdcp_RevocListWrite(baseAddr + HDCP_BASE_ADDR, 1, 0);
				/* write KSVs */
				for (i = 1; i < size; i += KSV_LEN)
				{
					for (j = 1; j <= KSV_LEN; j++)
					{
						halHdcp_RevocListWrite(baseAddr + HDCP_BASE_ADDR,
								i + j, data[VRL_LENGTH + VRL_NUMBER + i
								            + (KSV_LEN - j)]);
					}
				}
				success = TRUE;
				LOG_NOTICE("SRM successfully updated");
				break;
			}
		}
		halHdcp_MemoryAccessRequest(baseAddr + HDCP_BASE_ADDR, 0);
		if (!success)
		{
			error_Set(ERR_HDCP_MEM_ACCESS);
			LOG_ERROR("cannot access memory");
		}
	}
	else
	{
		error_Set(ERR_SRM_INVALID);
		LOG_ERROR("SRM invalid");
	}
	return success;
}

int hdcp_InterruptEnable(u16 baseAddr, u8 value)
{
	if ((value == INT_HDCP_ENGAGED) || (value == INT_HDCP_FAIL))
	{
		halHdcp_InterruptMask(baseAddr + HDCP_BASE_ADDR,
				(u8)(~(BIT(INT_HDCP_ENGAGED) | BIT(INT_HDCP_FAIL))) &
				halHdcp_InterruptMaskStatus(baseAddr + HDCP_BASE_ADDR));
	}
	return TRUE;
}

int hdcp_InterruptDisable(u16 baseAddr, u8 value)
{
	if ((value == INT_HDCP_ENGAGED) || (value == INT_HDCP_FAIL))
	{
		halHdcp_InterruptMask(baseAddr + HDCP_BASE_ADDR,
				(u8)((BIT(INT_HDCP_ENGAGED) | BIT(INT_HDCP_FAIL))) |
				halHdcp_InterruptMaskStatus(baseAddr + HDCP_BASE_ADDR));
	}
	return TRUE;
}

u8 hdcp_InterruptStatus(u16 baseAddr)
{
	return halHdcp_InterruptStatus(baseAddr + HDCP_BASE_ADDR);
}

int hdcp_InterruptClear(u16 baseAddr, u8 value)
{
	halHdcp_InterruptClear(baseAddr + HDCP_BASE_ADDR, value);
	return TRUE;
}

void hdcp_AnWrite(u16 baseAddr, u8 * data)
{
	return halHdcp_AnWrite(baseAddr + HDCP_BASE_ADDR, data);
}

u8 hdcp_BksvRead(u16 baseAddr, u8 * bksv)
{
	return halHdcp_BksvRead(baseAddr + HDCP_BASE_ADDR, bksv);
}

void hdcp_WriteDpkKeys(u8 baseAddr, hdcpParams_t *params)
{
	halHdcp_WaitMemAccess(baseAddr);
	halHdcp_WriteAksv(baseAddr, params->mAksv);
	halHdcp_WaitMemAccess(baseAddr);
	halHdcp_EnableEncrypt(baseAddr, 1);
	halHdcp_WriteSeed(baseAddr, params->mSwEncKey);
	halHdcp_StoreEncryptKeys(baseAddr, params->mKeys);
}
