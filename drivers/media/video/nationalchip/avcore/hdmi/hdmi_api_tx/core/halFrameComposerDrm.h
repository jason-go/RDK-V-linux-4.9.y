#ifndef HALFRAMECOMPOSERDRM_H_
#define HALFRAMECOMPOSERDRM_H_

#include "../util/types.h"

#define HDR_VERSION_NUMBER (0x01)

typedef enum
{
	HDMI_HLG   = (0x3),
	HDMI_HDR10 = (0x2)
} HDRCurve_e;

typedef struct GxAVHDMIMasteringDisplayMetadata {
	/**
	* CIE 1931 xy chromaticity coords of color primaries (r, g, b order).
	*/
	unsigned display_primaries[3][2];

	/**
	* CIE 1931 xy chromaticity coords of white point.
	*/
	unsigned white_point[2];

	/**
	* Min luminance of mastering display (cd/m^2).
	*/
	unsigned min_luminance;

	/**
	* Max luminance of mastering display (cd/m^2).
	*/
	unsigned max_luminance;

	/**
	* Flag indicating whether the display primaries (and white point) are set.
	*/
	int has_primaries;

	/**
	* Flag indicating whether the luminance (min_ and max_) have been set.
	*/
	int has_luminance;
} GxAVHDMIMasteringDisplayMetadata;

typedef struct GxAVHDMIContentLightMetadata {
	/**
	* Max content light level (cd/m^2).
	*/
	unsigned MaxCLL;

	/**
	* Max average light level per frame (cd/m^2).
	*/
	unsigned MaxFALL;
} GxAVHDMIContentLightMetadata;

void halFrameComposerDrm_DrmTxEn(u16 baseAddr, u8 enable);

void halFrameComposerDrm_DrmPacketUpdate(u16 baseAddr, u8 bit);

void halFrameComposerDrm_DrmHb0(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmHb1(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb0(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb1(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb2(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb3(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb4(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb5(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb6(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb7(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb8(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb9(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb10(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb11(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb12(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb13(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb14(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb15(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb16(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb17(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb18(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb19(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb20(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb21(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb22(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb23(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb24(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb25(u16 baseAddr, u8 value);

void halFrameComposerDrm_DrmPb26(u16 baseAddr, u8 value);

int halFrameComposerDrm_Configure(u16 baseAddr, HDRCurve_e curve, GxAVHDMIMasteringDisplayMetadata mdmd, GxAVHDMIContentLightMetadata clmd);

#endif /* HALFRAMECOMPOSERDRM_H_ */