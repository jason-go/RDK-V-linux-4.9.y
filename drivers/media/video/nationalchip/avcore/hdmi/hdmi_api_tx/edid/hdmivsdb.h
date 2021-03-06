/*
 * hdmivsdb.h
 *
 *  Created on: Jul 21, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#ifndef HDMIVSDB_H_
#define HDMIVSDB_H_

#include "../util/types.h"

#define MAX_HDMI_VIC		16
#define MAX_HDMI_3DSTRUCT	16
#define MAX_VIC_WITH_3D		16

/** For detailed handling of this structure, refer to documentation of the functions */
typedef struct
{
	u16 mPhysicalAddress;

	int mSupportsAi;

	int mDeepColor30;

	int mDeepColor36;

	int mDeepColor48;

	int mDeepColorY444;

	int mDviDual;

	u8 mMaxTmdsClk;

	u16 mVideoLatency;

	u16 mAudioLatency;

	u16 mInterlacedVideoLatency;

	u16 mInterlacedAudioLatency;

	u32 mId;

	u8 mContentTypeSupport;
	
	u8 mImageSize;

	int mHdmiVicCount;

	u8 mHdmiVic[MAX_HDMI_VIC];

	int m3dPresent;

	int mVideo3dStruct[MAX_VIC_WITH_3D][MAX_HDMI_3DSTRUCT]; /* row index is the VIC number */
	
	int mDetail3d[MAX_VIC_WITH_3D][MAX_HDMI_3DSTRUCT]; /* index is the VIC number */

	int mValid;

} hdmivsdb_t;

void hdmivsdb_Reset(hdmivsdb_t *vsdb);

/**
 * Parse an array of data to fill the hdmivsdb_t data strucutre
 * @param *vsdb pointer to the structure to be filled
 * @param *data pointer to the 8-bit data type array to be parsed
 * @return Success, or error code:
 * @return 1 - array pointer invalid
 * @return 2 - Invalid datablock tag
 * @return 3 - Invalid minimum length
 * @return 4 - HDMI IEEE registration identifier not valid
 * @return 5 - Invalid length - latencies are not valid
 * @return 6 - Invalid length - Interlaced latencies are not valid
 */
int hdmivsdb_Parse(hdmivsdb_t *vsdb, u8 * data);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return TRUE if sink supports 10bit/pixel deep color mode
 */
int hdmivsdb_GetDeepColor30(hdmivsdb_t *vsdb);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return TRUE if sink supports 12bit/pixel deep color mode
 */
int hdmivsdb_GetDeepColor36(hdmivsdb_t *vsdb);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return TRUE if sink supports 16bit/pixel deep color mode
 */
int hdmivsdb_GetDeepColor48(hdmivsdb_t *vsdb);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return TRUE if sink supports YCC 4:4:4 in deep color modes
 */
int hdmivsdb_GetDeepColorY444(hdmivsdb_t *vsdb);

/**
 * @param *vsdb pointer to the structure holding the information
 * @return TRUE if sink supports at least one function that uses
 * information carried by the ACP, ISRC1, or ISRC2 packets
 */
int hdmivsdb_GetSupportsAi(hdmivsdb_t *vsdb);

/**
 * @param *vsdb pointer to the structure holding the information
 * @return TRUE if Sink supports DVI dual-link operation
 */
int hdmivsdb_GetDviDual(hdmivsdb_t *vsdb);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return maximum TMDS clock rate supported [divided by 5MHz]
 */
u8 hdmivsdb_GetMaxTmdsClk(hdmivsdb_t *vsdb);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return the amount of video latency when receiving progressive video formats
 */
u16 hdmivsdb_GetPVideoLatency(hdmivsdb_t *vsdb);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return the amount of audio latency when receiving progressive video formats
 */
u16 hdmivsdb_GetPAudioLatency(hdmivsdb_t *vsdb);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return the amount of audio latency when receiving interlaced video formats
 */
u16 hdmivsdb_GetIAudioLatency(hdmivsdb_t *vsdb);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return the amount of video latency when receiving interlaced video formats
 */
u16 hdmivsdb_GetIVideoLatency(hdmivsdb_t *vsdb);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return Physical Address extracted from the VSDB
 */
u16 hdmivsdb_GetPhysicalAddress(hdmivsdb_t *vsdb);
/**
 * @param *vsdb pointer to the structure holding the information
 * @return the 24-bit IEEE Registration Identifier of 0x000C03, a value belonging to HDMI Licensing
 */
u32 hdmivsdb_GetId(hdmivsdb_t *vsdb);

int hdmivsdb_3dPresent(hdmivsdb_t *vsdb);

int hdmivsdb_GetHdmiVicCount(hdmivsdb_t *vsdb);

int hdmivsdb_GetHdmiVic(hdmivsdb_t *vsdb, u8 index);

u16 hdmivsdb_GetIndexSupported3dStructs(hdmivsdb_t *vsdb, u8 index);

u16 hdmivsdb_Get3dStructIndexes(hdmivsdb_t *vsdb, u8 struct3d);

int hdmivsdb_IndexSupports3dStruct(hdmivsdb_t *vsdb, u8 index, u8 struct3d);

u8 hdmivsdb_Get3dStructDetail(hdmivsdb_t *vsdb, u8 index, u8 struct3d);

int hdmivsdb_3dStructHasDetail(hdmivsdb_t *vsdb, u8 index, u8 struct3d);

int hdmivsdb_GetImageSize(hdmivsdb_t *vsdb);

#endif /* HDMIVSDB_H_ */
