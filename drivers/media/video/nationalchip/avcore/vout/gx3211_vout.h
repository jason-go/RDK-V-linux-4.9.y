#ifndef __GX3211_VIDEOOUT_H__
#define __GX3211_VIDEOOUT_H__

#include "gxav_vout_propertytypes.h"

#define GX3211_IS_VPU_SRC(iface) \
	(iface == GXAV_VOUT_YUV ||\
	iface == GXAV_VOUT_HDMI)

#define GX3211_CHECK_IFACE(iface) \
	do {\
		if (iface != GXAV_VOUT_RCA &&\
			iface != GXAV_VOUT_RCA1 &&\
			iface != GXAV_VOUT_YUV &&\
			iface != GXAV_VOUT_SCART &&\
			iface != GXAV_VOUT_SVIDEO &&\
			iface != GXAV_VOUT_HDMI) {\
			VIDEOOUT_PRINTF("iface is invalid!\n");\
			return -1;\
		}\
	} while(0)

#define GX3211_CHECK_VOUT_IFACE(vout_select, iface) \
	do {\
		if (iface & (~vout_select)) {\
			VIDEOOUT_PRINTF("iface is invalid!\n");\
			return -1;\
		}\
	} while(0)

#define VIDEOOUT_INTERFACE_MIN_VALUE  (GXAV_VOUT_RCA)
#define VIDEOOUT_INTERFACE_MAX_VALUE  (GXAV_VOUT_RCA|GXAV_VOUT_YUV \
		|GXAV_VOUT_HDMI|GXAV_VOUT_SCART|GXAV_VOUT_SVIDEO)

#define GX3211_YUV_BRIGHTNESS     160
#define GX3211_YUV_SATURATION_U   157
#define GX3211_YUV_SATURATION_V   157
#define GX3211_HDMI_BRIGHTNESS    256
#define GX3211_HDMI_SATURATION_U  256
#define GX3211_HDMI_SATURATION_V  256

#define GX3211_DVE_CHECK_RET(ret) \
	if(ret < 0) {\
		VIDEOOUT_PRINTF("####Error####\n");\
		return ret;	\
	}

#define IS_ID_INVALID(id)	\
	((id >= VIDEOOUT_MAX) || (id < 0))

#define IS_MODULE_NULL(module)	\
	(module == NULL)

#define IS_INTERFACE_UNITE_VALID(interface)	\
	((interface == (GXAV_VOUT_HDMI|GXAV_VOUT_RCA)) || \
	 (interface == (GXAV_VOUT_HDMI|GXAV_VOUT_YUV)) || \
	 (interface == (GXAV_VOUT_RCA|GXAV_VOUT_YUV))  || \
	 (interface == GXAV_VOUT_RCA)  || \
	 (interface == GXAV_VOUT_YUV)  || \
	 (interface == GXAV_VOUT_HDMI))

#define IS_VPU_MODE_VALID(mode)	\
	((mode >= GXAV_VOUT_PAL) && (mode < GXAV_VOUT_NULL_MAX))
#define IS_SVPU_MODE_VALID(mode)	\
	((mode >= GXAV_VOUT_PAL) && (mode <= GXAV_VOUT_NTSC_443))

#define IS_VIDEOOUT_INTERFACE_VALID(interface)	\
	((interface >= GXAV_VOUT_RCA) && (interface <= GXAV_VOUT_SVIDEO))
#define IS_VIDEOOUT1_INTERFACE_VALID(interface)	\
	(interface == GXAV_VOUT_RCA)

#define GX3211_VIDEOOUT0_CONFINE(videoout)	\
	do	\
{	\
	if ((videoout->resolution.interface == 0) ||	\
			(videoout->resolution.mode == 0)) {	\
		VIDEOOUT_PRINTF("VOUT0 select invalid!\n");	\
		return -1;	\
	}	\
} while(0)

#define GX3211_VIDEOOUT1_CONFINE(videoout)	\
	do	\
{	\
	if ((videoout->resolution.mode < GXAV_VOUT_PAL) ||	\
			(videoout->resolution.mode > GXAV_VOUT_NTSC_443))	\
	{	\
		VIDEOOUT_PRINTF("VOUT1 select invalid!\n");	\
		return -1;	\
	}	\
} while(0)

#endif



