#ifndef __GX3201_VOUT_H__
#define __GX3201_VOUT_H__

#define GX3201_YUV_BRIGHTNESS     160
#define GX3201_YUV_SATURATION_U   157
#define GX3201_YUV_SATURATION_V   157
#define GX3201_HDMI_BRIGHTNESS    256
#define GX3201_HDMI_SATURATION_U  256
#define GX3201_HDMI_SATURATION_V  256

#define GX3201_VIDEOOUT0_CONFINE(videoout)	\
	do	\
{	\
	if ((videoout->resolution.interface == 0) ||	\
			(videoout->resolution.mode == 0)) {	\
		VIDEOOUT_PRINTF("VOUT0 select invalid!\n");	\
		return -1;	\
	}	\
} while(0)

#define GX3201_VIDEOOUT1_CONFINE(videoout)	\
	do	\
{	\
	if ((videoout->resolution.mode < GXAV_VOUT_PAL) ||	\
			(videoout->resolution.mode > GXAV_VOUT_NTSC_443))	\
	{	\
		return -1;	\
	}	\
} while(0)

#define GX3201_IS_SVPU_SRC(iface) \
	(iface == GXAV_VOUT_RCA ||\
	iface == GXAV_VOUT_RCA1 ||\
	iface == GXAV_VOUT_SCART ||\
	iface == GXAV_VOUT_SVIDEO)

#define GX3201_IS_VPU_SRC(iface) \
	(iface == GXAV_VOUT_YUV ||\
	iface == GXAV_VOUT_HDMI)

#define GX3201_HAVE_SVPU_SRC(select) \
	(select & GXAV_VOUT_RCA ||\
	select & GXAV_VOUT_RCA1 ||\
	select & GXAV_VOUT_SCART ||\
	select & GXAV_VOUT_SVIDEO)

#define GX3201_HAVE_VPU_SRC(select) \
	(select & GXAV_VOUT_YUV ||\
	select & GXAV_VOUT_HDMI)

#define GX3201_CHECK_IFACE(iface) \
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

#define GX3201_CHECK_VOUT_IFACE(vout_select, iface) \
	do {\
		if (iface & (~vout_select)) {\
			VIDEOOUT_PRINTF("iface is invalid!\n");\
			return -1;\
		}\
	} while(0)

#define GX3201_DVE_CHECK_RET(ret) \
	if(ret < 0) {\
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

extern int  gx3201_videoout_pic_zoom_inquire(void);
extern int  gx3201_videoout_osd_zoom_inquire(void);
extern void Gx3201_enable_svpu(int mode);
extern void gx3201_videoout_out1_enable(unsigned int enable);
extern int  gx3201_videodec_disp_patch(int id);

#endif
