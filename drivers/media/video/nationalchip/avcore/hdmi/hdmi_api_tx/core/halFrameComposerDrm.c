#include "halFrameComposerDrm.h"
#include "../bsp/access.h"
#include "../util/log.h"

/* register offsets */
static const u8 FC_PACKET_TX_EN = 0xE3;
static const u8 FC_DRM_UP   = 0x67;
static const u8 FC_DRM_HB0  = 0x68;
static const u8 FC_DRM_HB1  = 0x69;
static const u8 FC_DRM_PB0  = 0x6A;
static const u8 FC_DRM_PB1  = 0x6B;
static const u8 FC_DRM_PB2  = 0x6C;
static const u8 FC_DRM_PB3  = 0x6D;
static const u8 FC_DRM_PB4  = 0x6E;
static const u8 FC_DRM_PB5  = 0x6F;
static const u8 FC_DRM_PB6  = 0x70;
static const u8 FC_DRM_PB7  = 0x71;
static const u8 FC_DRM_PB8  = 0x72;
static const u8 FC_DRM_PB9  = 0x73;
static const u8 FC_DRM_PB10 = 0x74;
static const u8 FC_DRM_PB11 = 0x75;
static const u8 FC_DRM_PB12 = 0x76;
static const u8 FC_DRM_PB13 = 0x77;
static const u8 FC_DRM_PB14 = 0x78;
static const u8 FC_DRM_PB15 = 0x79;
static const u8 FC_DRM_PB16 = 0x7A;
static const u8 FC_DRM_PB17 = 0x7B;
static const u8 FC_DRM_PB18 = 0x7C;
static const u8 FC_DRM_PB19 = 0x7D;
static const u8 FC_DRM_PB20 = 0x7E;
static const u8 FC_DRM_PB21 = 0x7F;
static const u8 FC_DRM_PB22 = 0x80;
static const u8 FC_DRM_PB23 = 0x81;
static const u8 FC_DRM_PB24 = 0x82;
static const u8 FC_DRM_PB25 = 0x83;
static const u8 FC_DRM_PB26 = 0x84;
/* bit shifts */
static const u8 DRM_TX_EN = 7;
static const u8 DRM_PACKET_UPDATE = 0;

void halFrameComposerDrm_DrmTxEn(u16 baseAddr, u8 enable)
{
	LOG_TRACE1(enable);
	access_CoreWrite((enable ? 1 : 0), (baseAddr + FC_PACKET_TX_EN), DRM_TX_EN, 1);
}

void halFrameComposerDrm_DrmPacketUpdate(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + FC_DRM_UP), DRM_PACKET_UPDATE, 1);
}

void halFrameComposerDrm_DrmHb0(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_HB0));
}

void halFrameComposerDrm_DrmHb1(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_HB1));
}

void halFrameComposerDrm_DrmPb0(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB0));
}

void halFrameComposerDrm_DrmPb1(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB1));
}

void halFrameComposerDrm_DrmPb2(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB2));
}

void halFrameComposerDrm_DrmPb3(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB3));
}

void halFrameComposerDrm_DrmPb4(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB4));
}

void halFrameComposerDrm_DrmPb5(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB5));
}

void halFrameComposerDrm_DrmPb6(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB6));
}

void halFrameComposerDrm_DrmPb7(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB7));
}

void halFrameComposerDrm_DrmPb8(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB8));
}

void halFrameComposerDrm_DrmPb9(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB9));
}

void halFrameComposerDrm_DrmPb10(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB10));
}

void halFrameComposerDrm_DrmPb11(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB11));
}

void halFrameComposerDrm_DrmPb12(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB12));
}

void halFrameComposerDrm_DrmPb13(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB13));
}

void halFrameComposerDrm_DrmPb14(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB14));
}

void halFrameComposerDrm_DrmPb15(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB15));
}

void halFrameComposerDrm_DrmPb16(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB16));
}

void halFrameComposerDrm_DrmPb17(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB17));
}

void halFrameComposerDrm_DrmPb18(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB18));
}

void halFrameComposerDrm_DrmPb19(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB19));
}

void halFrameComposerDrm_DrmPb20(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB20));
}

void halFrameComposerDrm_DrmPb21(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB21));
}

void halFrameComposerDrm_DrmPb22(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB22));
}

void halFrameComposerDrm_DrmPb23(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB23));
}

void halFrameComposerDrm_DrmPb24(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB24));
}

void halFrameComposerDrm_DrmPb25(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB25));
}

void halFrameComposerDrm_DrmPb26(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + FC_DRM_PB26));
}

int halFrameComposerDrm_Configure(u16 baseAddr, HDRCurve_e curve, GxAVHDMIMasteringDisplayMetadata mdmd, GxAVHDMIContentLightMetadata clmd)
{
	int hdr_data_len = 0;

	switch (curve) {
		case HDMI_HLG:
			hdr_data_len = 0x02;
			break;
		case HDMI_HDR10:
			hdr_data_len = 0x1A;
			break;
		default:
			LOG_ERROR("hdr curve not supported");
			return FALSE;
	}

	halFrameComposerDrm_DrmTxEn(baseAddr, 0);

	halFrameComposerDrm_DrmHb0(baseAddr + 0x100, HDR_VERSION_NUMBER);
	halFrameComposerDrm_DrmHb1(baseAddr + 0x100, hdr_data_len);

	halFrameComposerDrm_DrmPb0(baseAddr + 0x100, curve);
	halFrameComposerDrm_DrmPb1(baseAddr + 0x100, 0x00);

	if (curve == HDMI_HDR10) {
		halFrameComposerDrm_DrmPb2(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[0][0] & 0xFF);
		halFrameComposerDrm_DrmPb3(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[0][0] >> 8);
		halFrameComposerDrm_DrmPb4(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[0][1] & 0xFF);
		halFrameComposerDrm_DrmPb5(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[0][1] >> 8);
		halFrameComposerDrm_DrmPb6(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[1][0] & 0xFF);
		halFrameComposerDrm_DrmPb7(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[1][0] >> 8);
		halFrameComposerDrm_DrmPb8(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[1][1] & 0xFF);
		halFrameComposerDrm_DrmPb9(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[1][1] >> 8);
		halFrameComposerDrm_DrmPb10(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[2][0] & 0xFF);
		halFrameComposerDrm_DrmPb11(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[2][0] >> 8);
		halFrameComposerDrm_DrmPb12(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[2][1] & 0xFF);
		halFrameComposerDrm_DrmPb13(baseAddr + 0x100, (unsigned short)mdmd.display_primaries[2][1] >> 8);
		halFrameComposerDrm_DrmPb14(baseAddr + 0x100, (unsigned short)mdmd.white_point[0] & 0xFF);
		halFrameComposerDrm_DrmPb15(baseAddr + 0x100, (unsigned short)mdmd.white_point[0] >> 8);
		halFrameComposerDrm_DrmPb16(baseAddr + 0x100, (unsigned short)mdmd.white_point[1] & 0xFF);
		halFrameComposerDrm_DrmPb17(baseAddr + 0x100, (unsigned short)mdmd.white_point[1] >> 8);
		halFrameComposerDrm_DrmPb18(baseAddr + 0x100, (unsigned short)mdmd.max_luminance & 0xFF);
		halFrameComposerDrm_DrmPb19(baseAddr + 0x100, (unsigned short)mdmd.max_luminance >> 8);
		halFrameComposerDrm_DrmPb20(baseAddr + 0x100, (unsigned short)mdmd.min_luminance & 0xFF);
		halFrameComposerDrm_DrmPb21(baseAddr + 0x100, (unsigned short)mdmd.min_luminance >> 8);
		halFrameComposerDrm_DrmPb22(baseAddr + 0x100, (unsigned short)clmd.MaxCLL & 0xFF);
		halFrameComposerDrm_DrmPb23(baseAddr + 0x100, (unsigned short)clmd.MaxCLL >> 8);
		halFrameComposerDrm_DrmPb24(baseAddr + 0x100, (unsigned short)clmd.MaxFALL & 0xFF);
		halFrameComposerDrm_DrmPb25(baseAddr + 0x100, (unsigned short)clmd.MaxFALL >> 8);
	}

	halFrameComposerDrm_DrmPacketUpdate(baseAddr + 0x100, 1);
	halFrameComposerDrm_DrmTxEn(baseAddr, 1);

	return TRUE;
}