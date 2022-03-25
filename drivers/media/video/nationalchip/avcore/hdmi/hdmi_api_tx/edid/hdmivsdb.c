/*
 * hdmivsdb.c
 *
 *  Created on: Jul 21, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#include "hdmivsdb.h"
#include "../util/bitOperation.h"
#include "../util/log.h"

void hdmivsdb_Reset(hdmivsdb_t *vsdb)
{
	int i, j = 0;
	vsdb->mPhysicalAddress = 0x1000;
	vsdb->mSupportsAi = FALSE;
	vsdb->mDeepColor30 = FALSE;
	vsdb->mDeepColor36 = FALSE;
	vsdb->mDeepColor48 = FALSE;
	vsdb->mDeepColorY444 = FALSE;
	vsdb->mDviDual = FALSE;
	vsdb->mMaxTmdsClk = 0;
	vsdb->mVideoLatency = 0;
	vsdb->mAudioLatency = 0;
	vsdb->mInterlacedVideoLatency = 0;
	vsdb->mInterlacedAudioLatency = 0;
	vsdb->mId = 0;
	vsdb->mContentTypeSupport = 0;
	vsdb->mHdmiVicCount = 0;
	for (i = 0; i < MAX_HDMI_VIC; i++)
	{
		vsdb->mHdmiVic[i] = 0;
	}
	vsdb->m3dPresent = FALSE;
	for (i = 0; i < MAX_VIC_WITH_3D; i++)
	{
		for (j = 0; j < MAX_HDMI_3DSTRUCT; j++)
		{
			vsdb->mVideo3dStruct[i][j] = 0;
		}
	}
	for (i = 0; i < MAX_VIC_WITH_3D; i++)
	{
		for (j = 0; j < MAX_HDMI_3DSTRUCT; j++)
		{
			vsdb->mDetail3d[i][j] = ~0;
		}
	}
	vsdb->mValid = FALSE;
}
int hdmivsdb_Parse(hdmivsdb_t *vsdb, u8 * data)
{
	u8 blockLength = 0;
	unsigned videoInfoStart = 0;
	unsigned hdmi3dStart = 0;
	unsigned hdmiVicLen = 0;
	unsigned hdmi3dLen = 0;
	unsigned spanned3d = 0;
	unsigned i = 0;
	unsigned j = 0;
	LOG_TRACE();
	hdmivsdb_Reset(vsdb);
	if (data == 0)
	{
		return FALSE;
	}
	if (bitOperation_BitField(data[0], 5, 3) != 0x3)
	{
		LOG_WARNING("Invalid datablock tag");
		return FALSE;
	}
	blockLength = bitOperation_BitField(data[0], 0, 5);
	if (blockLength < 5)
	{
		LOG_WARNING("Invalid minimum length");
		return FALSE;
	}
	if (bitOperation_Bytes2Dword(0x00, data[3], data[2], data[1]) != 0x000C03)
	{
		LOG_WARNING("HDMI IEEE registration identifier not valid");
		return FALSE;
	}
	hdmivsdb_Reset(vsdb);
	vsdb->mId = 0x000C03;
	vsdb->mPhysicalAddress = bitOperation_Bytes2Word(data[4], data[5]);
	/* parse extension fields if they exist */
	if (blockLength > 5)
	{
		vsdb->mSupportsAi = bitOperation_BitField(data[6], 7, 1) == 1;
		vsdb->mDeepColor48 = bitOperation_BitField(data[6], 6, 1) == 1;
		vsdb->mDeepColor36 = bitOperation_BitField(data[6], 5, 1) == 1;
		vsdb->mDeepColor30 = bitOperation_BitField(data[6], 4, 1) == 1;
		vsdb->mDeepColorY444 = bitOperation_BitField(data[6], 3, 1) == 1;
		vsdb->mDviDual = bitOperation_BitField(data[6], 0, 1) == 1;
	}
	else
	{
		vsdb->mSupportsAi = FALSE;
		vsdb->mDeepColor48 = FALSE;
		vsdb->mDeepColor36 = FALSE;
		vsdb->mDeepColor30 = FALSE;
		vsdb->mDeepColorY444 = FALSE;
		vsdb->mDviDual = FALSE;
	}
	vsdb->mMaxTmdsClk = (blockLength > 6) ? data[7] : 0;
	vsdb->mVideoLatency = 0;
	vsdb->mAudioLatency = 0;
	vsdb->mInterlacedVideoLatency = 0;
	vsdb->mInterlacedAudioLatency = 0;
	if (blockLength > 7)
	{
		if (bitOperation_BitField(data[8], 7, 1) == 1)
		{
			if (blockLength < 10)
			{
				LOG_WARNING("Invalid length - latencies are not valid");
				return FALSE;
			}
			if (bitOperation_BitField(data[8], 6, 1) == 1)
			{
				if (blockLength < 12)
				{
					LOG_WARNING("Invalid length - Interlaced latencies are not valid");
					return FALSE;
				}
				else
				{
					vsdb->mVideoLatency = data[9];
					vsdb->mAudioLatency = data[10];
					vsdb->mInterlacedVideoLatency = data[11];
					vsdb->mInterlacedAudioLatency = data[12];
					videoInfoStart = 13;
				}
			}
			else
			{
				vsdb->mVideoLatency = data[9];
				vsdb->mAudioLatency = data[10];
				vsdb->mInterlacedVideoLatency = 0;
				vsdb->mInterlacedAudioLatency = 0;
				videoInfoStart = 11;
			}
		}
		else
		{	/* no latency data */
			vsdb->mVideoLatency = 0;
			vsdb->mAudioLatency = 0;
			vsdb->mInterlacedVideoLatency = 0;
			vsdb->mInterlacedAudioLatency = 0;
			videoInfoStart = 9;
		}
		vsdb->mContentTypeSupport = bitOperation_BitField(data[8], 0, 4);
	}
	if (bitOperation_BitField(data[8], 5, 1) == 1)
	{	/* additional video format capabilities are described */
		vsdb->mImageSize = bitOperation_BitField(data[videoInfoStart], 3, 2);
		hdmiVicLen = bitOperation_BitField(data[videoInfoStart + 1], 5, 3);
		hdmi3dLen = bitOperation_BitField(data[videoInfoStart + 1], 0, 5);
		for (i = 0; i < hdmiVicLen; i++)
		{
			vsdb->mHdmiVic[i] = data[videoInfoStart + 2 + i];
		}
		vsdb->mHdmiVicCount = hdmiVicLen;
		if (bitOperation_BitField(data[videoInfoStart], 7, 1) == 1)
		{	/* 3d present */
			vsdb->m3dPresent = TRUE;
			hdmi3dStart = videoInfoStart + hdmiVicLen + 2;
			/* 3d multi 00 -> both 3d_structure_all and 3d_mask_15 are NOT present */
			/* 3d mutli 11 -> reserved */
			if (bitOperation_BitField(data[videoInfoStart], 5, 2) == 1)
			{	/* 3d multi 01 */
				/* 3d_structure_all is present but 3d_mask_15 not present */
				for (j = 0; j < 16; j++)
				{	/* j spans 3d structures */
					if (bitOperation_BitField(data[hdmi3dStart + (j / 8)], (j % 8), 1) == 1)
					{
						for (i = 0; i < 16; i++)
						{	/* go through 2 registers, [videoInfoStart + hdmiVicLen + 1] & [videoInfoStart + hdmiVicLen + 2]  */
							vsdb->mVideo3dStruct[i][(j < 8)? j + 8: j - 8] = 1;
						}
					}
				}
				spanned3d = 2;
				/*hdmi3dStart += 2;
				hdmi3dLen -= 2;*/
			}
			else if (bitOperation_BitField(data[videoInfoStart], 5, 2) == 2)
			{	/* 3d multi 10 */
				/* 3d_structure_all and 3d_mask_15 are present */
				for (j = 0; j < 16; j++)
				{
					for (i = 0; i < 16; i++)
					{	/* assign according to mask, through 2 registers, [videoInfoStart + hdmiVicLen + 3] & [videoInfoStart + hdmiVicLen + 4] */
						if (bitOperation_BitField(data[hdmi3dStart + 2 + (i / 8)], (i % 8), 1) == 1)
						{	/* go through 2 registers, [videoInfoStart + hdmiVicLen + 1] & [videoInfoStart + hdmiVicLen + 2]  */
							vsdb->mVideo3dStruct[(i < 8)? i + 8: i - 8][(j < 8)? j + 8: j - 8] = bitOperation_BitField(data[hdmi3dStart + (j / 8)], (j % 8), 1);
						}
					}
				}
				spanned3d = 4;
			}
			if (hdmi3dLen > spanned3d)
			{
				hdmi3dStart += spanned3d;
				for (i = 0, j = 0; i < (hdmi3dLen - spanned3d); i++)
				{
					vsdb->mVideo3dStruct[bitOperation_BitField(data[hdmi3dStart + i + j], 4, 4)][bitOperation_BitField(data[hdmi3dStart + i + j], 0, 4)] = 1;
					if (bitOperation_BitField(data[hdmi3dStart + i + j], 4, 4) > 7)
					{	/* bytes with 3D_Detail_X and Reserved(0) are present only when 3D_Strucutre_X > b'1000 - side-by-side(half) */
						j++;
						vsdb->mDetail3d[bitOperation_BitField(data[hdmi3dStart + i + j], 4, 4)][bitOperation_BitField(data[hdmi3dStart + i + j], 4, 4)] = bitOperation_BitField(data[hdmi3dStart + i + j], 4, 4);
					}
				}
			}
		}
		else
		{	/* 3d NOT present */
			vsdb->m3dPresent = FALSE;
		}
	}
	vsdb->mValid = TRUE;
	return TRUE;
}
int hdmivsdb_GetDeepColor30(hdmivsdb_t *vsdb)
{
	return vsdb->mDeepColor30;
}

int hdmivsdb_GetDeepColor36(hdmivsdb_t *vsdb)
{
	return vsdb->mDeepColor36;
}

int hdmivsdb_GetDeepColor48(hdmivsdb_t *vsdb)
{
	return vsdb->mDeepColor48;
}

int hdmivsdb_GetDeepColorY444(hdmivsdb_t *vsdb)
{
	return vsdb->mDeepColorY444;
}

int hdmivsdb_GetSupportsAi(hdmivsdb_t *vsdb)
{
	return vsdb->mSupportsAi;
}

int hdmivsdb_GetDviDual(hdmivsdb_t *vsdb)
{
	return vsdb->mDviDual;
}

u8 hdmivsdb_GetMaxTmdsClk(hdmivsdb_t *vsdb)
{
	return vsdb->mMaxTmdsClk;
}

u16 hdmivsdb_GetPVideoLatency(hdmivsdb_t *vsdb)
{
	return vsdb->mVideoLatency;
}

u16 hdmivsdb_GetPAudioLatency(hdmivsdb_t *vsdb)
{
	return vsdb->mAudioLatency;
}

u16 hdmivsdb_GetIAudioLatency(hdmivsdb_t *vsdb)
{
	return vsdb->mInterlacedAudioLatency;
}

u16 hdmivsdb_GetIVideoLatency(hdmivsdb_t *vsdb)
{
	return vsdb->mInterlacedVideoLatency;
}

u16 hdmivsdb_GetPhysicalAddress(hdmivsdb_t *vsdb)
{
	return vsdb->mPhysicalAddress;
}

u32 hdmivsdb_GetId(hdmivsdb_t *vsdb)
{
	return vsdb->mId;
}

int hdmivsdb_3dPresent(hdmivsdb_t *vsdb)
{
	return vsdb->m3dPresent;
}

int hdmivsdb_GetHdmiVicCount(hdmivsdb_t *vsdb)
{
	return vsdb->mHdmiVicCount;
}

int hdmivsdb_GetHdmiVic(hdmivsdb_t *vsdb, u8 index)
{
	return vsdb->mHdmiVic[index];
}

u16 hdmivsdb_GetIndexSupported3dStructs(hdmivsdb_t *vsdb, u8 index)
{
	u16 structs3d = 0;
	int i;
	for (i = 0; i < MAX_HDMI_3DSTRUCT; i++)
	{
		structs3d |= (vsdb->mVideo3dStruct[index][i] & 1) << i;
	}
	return structs3d;
}

u16 hdmivsdb_Get3dStructIndexes(hdmivsdb_t *vsdb, u8 struct3d)
{
	u16 indexes = 0;
	int i;
	for (i = 0; i < MAX_HDMI_3DSTRUCT; i++)
	{
		indexes |= (vsdb->mVideo3dStruct[i][struct3d] & 1) << i;
	}
	return indexes;
}
int hdmivsdb_IndexSupports3dStruct(hdmivsdb_t *vsdb, u8 index, u8 struct3d)
{
	return vsdb->mVideo3dStruct[index][struct3d];
}

u8 hdmivsdb_Get3dStructDetail(hdmivsdb_t *vsdb, u8 index, u8 struct3d)
{
	return vsdb->mDetail3d[index][struct3d];
}

int hdmivsdb_3dStructHasDetail(hdmivsdb_t *vsdb, u8 index, u8 struct3d)
{
	return (vsdb->mDetail3d[index][struct3d] != ~0)? TRUE: FALSE;
}

int hdmivsdb_GetImageSize(hdmivsdb_t *vsdb)
{
	return vsdb->mImageSize;
}
