#ifndef __GX3211_VPU_INTERTNAL_H__
#define __GX3211_VPU_INTERTNAL_H__

#include "gxav_vpu_propertytypes.h"

#include "gx3211_vpu_reg.h"
#include "gx3211_vpu_internel.h"

#define VPU_GA_INC_SRC      (62)

#define VPU_PRINTF(fmt, args...) \
	do { \
		gx_printf("\n[VPU][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)

extern int gx3211_vpp_get_actual_viewrect(GxAvRect *rect);
extern int gx3211_vpp_get_actual_cliprect(GxAvRect *rect);
extern int gx3211_vpp_set_actual_cliprect(GxAvRect *rect);
extern int gx3211_vpp_set_stream_ratio(unsigned int ratio);
extern int gx3211_vpp_set_framesize(unsigned int width, unsigned int height);
extern int gx3211_vpp_set_playmode(GxVpuLayerPlayMode mode);
extern int gx3211_vpp_view_port(GxAvRect *clip_rect, GxAvRect *view_rect);
extern int gx3211_vpp_play_frame(struct frame *frame);
extern int gx3211_vpp_zoom_require(void);
extern unsigned int gx3211_vpu_get_scan_line(void);
extern unsigned int gx3211_vpu_check_layers_close(void);

extern int Gx3211_Vpu_SetVoutIsready(int ready);
extern int Gx3211Vpu_DACEnable(GxVpuDacID id, int enable);
extern int Gx3211Vpu_DACSource(GxVpuDacID dac_id, GxVpuDacSrc source);
#endif

