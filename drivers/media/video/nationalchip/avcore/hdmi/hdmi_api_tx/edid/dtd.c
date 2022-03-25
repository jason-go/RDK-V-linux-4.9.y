/*
 * @file:dtd.c
 *
 *
 * Synopsys Inc.
 * SG DWC PT02
 */

#include "dtd.h"
#include "../util/log.h"
#include "../util/bitOperation.h"
#include "../util/error.h"

int dtd_Parse(dtd_t *dtd, u8 data[18])
{
	LOG_TRACE();

	dtd->mCode = -1;
	dtd->mPixelRepetitionInput = 0;

	dtd->mPixelClock = bitOperation_Bytes2Word(data[1], data[0]); /*  [10000Hz] */
	if (dtd->mPixelClock < 0x01)
	{ /* 0x0000 is defined as reserved */
		return FALSE;
	}

	dtd->mHActive = bitOperation_ConcatBits(data[4], 4, 4, data[2], 0, 8);
	dtd->mHBlanking = bitOperation_ConcatBits(data[4], 0, 4, data[3], 0, 8);
	dtd->mHSyncOffset = bitOperation_ConcatBits(data[11], 6, 2, data[8], 0, 8);
	dtd->mHSyncPulseWidth = bitOperation_ConcatBits(data[11], 4, 2, data[9], 0,
			8);
	dtd->mHImageSize = bitOperation_ConcatBits(data[14], 4, 4, data[12], 0, 8);
	dtd->mHBorder = data[15];

	dtd->mVActive = bitOperation_ConcatBits(data[7], 4, 4, data[5], 0, 8);
	dtd->mVBlanking = bitOperation_ConcatBits(data[7], 0, 4, data[6], 0, 8);
	dtd->mVSyncOffset = bitOperation_ConcatBits(data[11], 2, 2, data[10], 4, 4);
	dtd->mVSyncPulseWidth = bitOperation_ConcatBits(data[11], 0, 2, data[10],
			0, 4);
	dtd->mVImageSize = bitOperation_ConcatBits(data[14], 0, 4, data[13], 0, 8);
	dtd->mVBorder = data[16];

	if (bitOperation_BitField(data[17], 4, 1) != 1)
	{ /* if not DIGITAL SYNC SIGNAL DEF */
		LOG_WARNING("Invalid DTD Parameters");
		return FALSE;
	}
	if (bitOperation_BitField(data[17], 3, 1) != 1)
	{ /* if not DIGITAL SEPATATE SYNC */
		LOG_WARNING("Invalid DTD Parameters");
		return FALSE;
	}
	/* no stereo viewing support in HDMI */
	dtd->mInterlaced = bitOperation_BitField(data[17], 7, 1) == 1;
	dtd->mVSyncPolarity = bitOperation_BitField(data[17], 2, 1) == 1;
	dtd->mHSyncPolarity = bitOperation_BitField(data[17], 1, 1) == 1;
	return TRUE;
}
int dtd_Fill(dtd_t *dtd, u8 code, u32 refreshRate)
{
	LOG_TRACE();
	dtd->mCode = code;
	dtd->mHBorder = 0;
	dtd->mVBorder = 0;
	dtd->mPixelRepetitionInput = 0;
	dtd->mHImageSize = 16;
	dtd->mVImageSize = 9;

	switch (code)
	{
		case 2: /* 720x480p @ 59.94/60Hz 4:3 */
			dtd->mHImageSize = 4;
			dtd->mVImageSize = 3;
		case 3: /* 720x480p @ 59.94/60Hz 16:9 */
			dtd->mHActive = 720;
			dtd->mVActive = 480;
			dtd->mHBlanking = 138;
			dtd->mVBlanking = 45;
			dtd->mHSyncOffset = 16;
			dtd->mVSyncOffset = 9;
			dtd->mHSyncPulseWidth = 62;
			dtd->mVSyncPulseWidth = 6;
			dtd->mHSyncPolarity = dtd->mVSyncPolarity = 0;
			dtd->mInterlaced = 0;
			dtd->mPixelClock = (refreshRate == 59940) ? 2700 : 2702;
			break;
		case 4: /* 1280x720p @ 59.94/60Hz 16:9 */
			dtd->mHActive = 1280;
			dtd->mVActive = 720;
			dtd->mHBlanking = 370;
			dtd->mVBlanking = 30;
			dtd->mHSyncOffset = 110;
			dtd->mVSyncOffset = 5;
			dtd->mHSyncPulseWidth = 40;
			dtd->mVSyncPulseWidth = 5;
			dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
			dtd->mInterlaced = 0;
			dtd->mPixelClock = (refreshRate == 59940) ? 7417 : 7425;
			break;
		case 5: /* 1920x1080i @ 59.94/60Hz 16:9 */
			dtd->mHActive = 1920;
			dtd->mVActive = 540;
			dtd->mHBlanking = 280;
			dtd->mVBlanking = 22;
			dtd->mHSyncOffset = 88;
			dtd->mVSyncOffset = 2;
			dtd->mHSyncPulseWidth = 44;
			dtd->mVSyncPulseWidth = 5;
			dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
			dtd->mInterlaced = 1;
			dtd->mPixelClock = (refreshRate == 59940) ? 7417 : 7425;
			break;
		case 6: /* 720(1440)x480i @ 59.94/60Hz 4:3 */
			dtd->mHImageSize = 4;
			dtd->mVImageSize = 3;
		case 7: /* 720(1440)x480i @ 59.94/60Hz 16:9 */
			dtd->mHActive = 1440;
			dtd->mVActive = 240;
			dtd->mHBlanking = 276;
			dtd->mVBlanking = 22;
			dtd->mHSyncOffset = 38;
			dtd->mVSyncOffset = 4;
			dtd->mHSyncPulseWidth = 124;
			dtd->mVSyncPulseWidth = 3;
			dtd->mHSyncPolarity = dtd->mVSyncPolarity = 0;
			dtd->mInterlaced = 1;
			dtd->mPixelClock = (refreshRate == 59940) ? 2700 : 2702;
			dtd->mPixelRepetitionInput = 1;
			break;
		case 16: /* 1920x1080p @ 59.94/60Hz 16:9 */
			dtd->mHActive = 1920;
			dtd->mVActive = 1080;
			dtd->mHBlanking = 280;
			dtd->mVBlanking = 45;
			dtd->mHSyncOffset = 88;
			dtd->mVSyncOffset = 4;
			dtd->mHSyncPulseWidth = 44;
			dtd->mVSyncPulseWidth = 5;
			dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
			dtd->mInterlaced = 0;
			dtd->mPixelClock = (refreshRate == 59940) ? 14835 : 14850;
			break;
		case 17: /* 720x576p @ 50Hz 4:3 */
			dtd->mHImageSize = 4;
			dtd->mVImageSize = 3;
		case 18: /* 720x576p @ 50Hz 16:9 */
			dtd->mHActive = 720;
			dtd->mVActive = 576;
			dtd->mHBlanking = 144;
			dtd->mVBlanking = 49;
			dtd->mHSyncOffset = 12;
			dtd->mVSyncOffset = 5;
			dtd->mHSyncPulseWidth = 64;
			dtd->mVSyncPulseWidth = 5;
			dtd->mHSyncPolarity = dtd->mVSyncPolarity = 0;
			dtd->mInterlaced = 0;
			dtd->mPixelClock = 2700;
			break;
		case 19: /* 1280x720p @ 50Hz 16:9 */
			dtd->mHActive = 1280;
			dtd->mVActive = 720;
			dtd->mHBlanking = 700;
			dtd->mVBlanking = 30;
			dtd->mHSyncOffset = 440;
			dtd->mVSyncOffset = 5;
			dtd->mHSyncPulseWidth = 40;
			dtd->mVSyncPulseWidth = 5;
			dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
			dtd->mInterlaced = 0;
			dtd->mPixelClock = 7425;
			break;
		case 20: /* 1920x1080i @ 50Hz 16:9 */
			dtd->mHActive = 1920;
			dtd->mVActive = 540;
			dtd->mHBlanking = 720;
			dtd->mVBlanking = 22;
			dtd->mHSyncOffset = 528;
			dtd->mVSyncOffset = 2;
			dtd->mHSyncPulseWidth = 44;
			dtd->mVSyncPulseWidth = 5;
			dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
			dtd->mInterlaced = 1;
			dtd->mPixelClock = 7425;
			break;
		case 21: /* 720(1440)x576i @ 50Hz 4:3 */
			dtd->mHImageSize = 4;
			dtd->mVImageSize = 3;
		case 22: /* 720(1440)x576i @ 50Hz 16:9 */
			dtd->mHActive = 1440;
			dtd->mVActive = 288;
			dtd->mHBlanking = 288;
			dtd->mVBlanking = 24;
			dtd->mHSyncOffset = 24;
			dtd->mVSyncOffset = 2;
			dtd->mHSyncPulseWidth = 126;
			dtd->mVSyncPulseWidth = 3;
			dtd->mHSyncPolarity = dtd->mVSyncPolarity = 0;
			dtd->mInterlaced = 1;
			dtd->mPixelClock = 2700;
			dtd->mPixelRepetitionInput = 1;
			break;
		case 31: /* 1920x1080p @ 50Hz 16:9 */
			dtd->mHActive = 1920;
			dtd->mVActive = 1080;
			dtd->mHBlanking = 720;
			dtd->mVBlanking = 45;
			dtd->mHSyncOffset = 528;
			dtd->mVSyncOffset = 4;
			dtd->mHSyncPulseWidth = 44;
			dtd->mVSyncPulseWidth = 5;
			dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
			dtd->mInterlaced = 0;
			dtd->mPixelClock = 14850;
			break;
	default:
		dtd->mCode = -1;
		error_Set(ERR_DTD_INVALID_CODE);
		LOG_ERROR("invalid code");
		return FALSE;
	}
	return TRUE;
}

u8 dtd_GetCode(const dtd_t *dtd)
{
	return dtd->mCode;
}

u16 dtd_GetPixelRepetitionInput(const dtd_t *dtd)
{
	return dtd->mPixelRepetitionInput;
}

u16 dtd_GetPixelClock(const dtd_t *dtd)
{
	return dtd->mPixelClock;
}

u8 dtd_GetInterlaced(const dtd_t *dtd)
{
	return dtd->mInterlaced;
}

u16 dtd_GetHActive(const dtd_t *dtd)
{
	return dtd->mHActive;
}

u16 dtd_GetHBlanking(const dtd_t *dtd)
{
	return dtd->mHBlanking;
}

u16 dtd_GetHBorder(const dtd_t *dtd)
{
	return dtd->mHBorder;
}

u16 dtd_GetHImageSize(const dtd_t *dtd)
{
	return dtd->mHImageSize;
}

u16 dtd_GetHSyncOffset(const dtd_t *dtd)
{
	return dtd->mHSyncOffset;
}

u8 dtd_GetHSyncPolarity(const dtd_t *dtd)
{
	return dtd->mHSyncPolarity;
}

u16 dtd_GetHSyncPulseWidth(const dtd_t *dtd)
{
	return dtd->mHSyncPulseWidth;
}

u16 dtd_GetVActive(const dtd_t *dtd)
{
	return dtd->mVActive;
}

u16 dtd_GetVBlanking(const dtd_t *dtd)
{
	return dtd->mVBlanking;
}

u16 dtd_GetVBorder(const dtd_t *dtd)
{
	return dtd->mVBorder;
}

u16 dtd_GetVImageSize(const dtd_t *dtd)
{
	return dtd->mVImageSize;
}

u16 dtd_GetVSyncOffset(const dtd_t *dtd)
{
	return dtd->mVSyncOffset;
}

u8 dtd_GetVSyncPolarity(const dtd_t *dtd)
{
	return dtd->mVSyncPolarity;
}

u16 dtd_GetVSyncPulseWidth(const dtd_t *dtd)
{
	return dtd->mVSyncPulseWidth;
}

int dtd_IsEqual(const dtd_t *dtd1, const dtd_t *dtd2)
{
	return (dtd1->mInterlaced == dtd2->mInterlaced && dtd1->mHActive
			== dtd2->mHActive && dtd1->mHBlanking == dtd2->mHBlanking
			&& dtd1->mHBorder == dtd2->mHBorder && dtd1->mHSyncOffset
			== dtd2->mHSyncOffset && dtd1->mHSyncPolarity
			== dtd2->mHSyncPolarity && dtd1->mHSyncPulseWidth
			== dtd2->mHSyncPulseWidth && dtd1->mVActive == dtd2->mVActive
			&& dtd1->mVBlanking == dtd2->mVBlanking && dtd1->mVBorder
			== dtd2->mVBorder && dtd1->mVSyncOffset == dtd2->mVSyncOffset
			&& dtd1->mVSyncPolarity == dtd2->mVSyncPolarity
			&& dtd1->mVSyncPulseWidth == dtd2->mVSyncPulseWidth
			&& ((dtd1->mHImageSize * 10) / dtd1->mVImageSize)
					== ((dtd2->mHImageSize * 10) / dtd2->mVImageSize));
}

int dtd_SetPixelRepetitionInput(dtd_t *dtd, u16 value)
{
	LOG_TRACE();

	switch (dtd->mCode)
	{
	case (u8)-1:
	case 10:
	case 11:
	case 12:
	case 13:
	case 25:
	case 26:
	case 27:
	case 28:
		if (value < 10)
		{
			dtd->mPixelRepetitionInput = value;
			return TRUE;
		}
		break;
	case 14:
	case 15:
	case 29:
	case 30:
		if (value < 2)
		{
			dtd->mPixelRepetitionInput = value;
			return TRUE;
		}
		break;
	case 35:
	case 36:
	case 37:
	case 38:
		if (value < 2 || value == 3)
		{
			dtd->mPixelRepetitionInput = value;
			return TRUE;
		}
		break;
	default:
		if (value == dtd->mPixelRepetitionInput)
		{
			return TRUE;
		}
		break;
	}
	error_Set(ERR_PIXEL_REPETITION_FOR_VIDEO_MODE);
	LOG_ERROR3("Invalid pixel repetition input for video mode", value, dtd->mCode);
	return FALSE;
}
