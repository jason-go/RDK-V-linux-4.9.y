#ifndef __GX3113C_VIDEOOUT_HAL_H__
#define __GX3113C_VIDEOOUT_HAL_H__

#include "gxav_vout_propertytypes.h"

#define GX3113C_IS_VPU_SRC(iface) \
	(iface == GXAV_VOUT_RCA ||\
	iface == GXAV_VOUT_RCA1 ||\
	iface == GXAV_VOUT_YUV ||\
	iface == GXAV_VOUT_SCART ||\
	iface == GXAV_VOUT_SVIDEO)

#define GX3113C_HAVE_VPU_SRC(select) \
	(select & GXAV_VOUT_RCA ||\
	select & GXAV_VOUT_RCA1 ||\
	select & GXAV_VOUT_YUV ||\
	select & GXAV_VOUT_SCART ||\
	select & GXAV_VOUT_SVIDEO)

#define GX3113C_CHECK_IFACE(iface) \
	do {\
		if (iface != GXAV_VOUT_RCA &&\
			iface != GXAV_VOUT_RCA1 &&\
			iface != GXAV_VOUT_YUV &&\
			iface != GXAV_VOUT_SCART &&\
			iface != GXAV_VOUT_SVIDEO) {\
			VIDEOOUT_PRINTF("iface is invalid!\n");\
			return -1;\
		}\
	} while(0)

#define GX3113C_CHECK_VOUT_IFACE(vout_select, iface) \
	do {\
		if (iface & (~vout_select)) {\
			VIDEOOUT_PRINTF("iface is invalid!\n");\
			return -1;\
		}\
	} while(0)

#define VIDEOOUT_INTERFACE_MIN_VALUE  (GXAV_VOUT_RCA)
#define VIDEOOUT_INTERFACE_MAX_VALUE  (GXAV_VOUT_RCA|GXAV_VOUT_YUV \
		|GXAV_VOUT_HDMI|GXAV_VOUT_SCART|GXAV_VOUT_SVIDEO)

#define GX3113C_BRIGHTNESS         161
#define GX3113C_SATURATION_U       151
#define GX3113C_SATURATION_V       214

#define GX3113c_BRIGHTNESS_HDMI    256
#define GX3113c_SATURATION_U_HDMI  256
#define GX3113c_SATURATION_V_HDMI  256

#define GX3113c_CVBS_BRIGHTNESS    160
#define GX3113c_CVBS_SATURATION_U  137
#define GX3113c_CVBS_SATURATION_V  194
#define GX3113c_YUV_BRIGHTNESS     160
#define GX3113c_YUV_SATURATION_U   157
#define GX3113c_YUV_SATURATION_V   157
#define GX3113c_HDMI_BRIGHTNESS    256
#define GX3113c_HDMI_SATURATION_U  256
#define GX3113c_HDMI_SATURATION_V  256

#define GX3113C_DVE_CHECK_RET(ret) \
	if(ret < 0) {\
		VIDEOOUT_PRINTF("####Error####\n");\
		return ret;	\
	}

#define IS_ID_INVALID(id)	\
	((id >= VIDEOOUT_MAX) || (id < 0))

#define IS_MODULE_NULL(module)	\
	(module == NULL)

#define IS_INTERFACE_UNITE_VALID(interface)	\
	((interface == (GXAV_VOUT_HDMI|GXAV_VOUT_RCA)) ||	\
	 (interface == (GXAV_VOUT_HDMI|GXAV_VOUT_YUV)) ||	\
	 (interface == (GXAV_VOUT_HDMI|GXAV_VOUT_YUV)))

#define IS_VOUT1_INTERFACE_INVALID(interface)	\
	!((interface == GXAV_VOUT_RCA) ||	\
			(interface == GXAV_VOUT_SCART) ||	\
			(interface == GXAV_VOUT_YUV))

#define IS_VIDEOOUT_MODE_VALID(mode)	\
	((mode >= GXAV_VOUT_PAL) && (mode < GXAV_VOUT_NULL_MAX))

#define IS_VIDEOOUT_INTERFACE_VALID(interface)	\
	((interface >= GXAV_VOUT_RCA) && (interface <= GXAV_VOUT_SVIDEO))
#define IS_SAME_RESOLUTION(resolution) \
	(resolution == 1)

#define GX3113C_VIDEOOUT0_CONFINE(videoout)	\
	do	\
{	\
	if ((videoout->resolution.iface == 0) ||	\
			(videoout->resolution.mode == 0)) {	\
		VIDEOOUT_PRINTF("VOUT0 select invalid!\n");	\
		return -1;	\
	}	\
} while(0)


#endif



