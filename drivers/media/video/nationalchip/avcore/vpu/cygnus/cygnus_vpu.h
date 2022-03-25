#ifndef __CYGNUS_VPU_INTERTNAL_H__
#define __CYGNUS_VPU_INTERTNAL_H__

#include "gxav_vpu_propertytypes.h"

#include "cygnus_vpu_reg.h"
#include "cygnus_vpu_internel.h"

extern unsigned int GA_INT_SRC;

#define OSD_FLICKER_TABLE_LEN     (64)

#define VPU_PRINTF(fmt, args...) \
	do { \
		gx_printf("\n[VPU][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)

extern int cygnus_vpp_get_actual_viewrect(GxAvRect *rect);
extern int cygnus_vpp_get_actual_cliprect(GxAvRect *rect);
extern int cygnus_vpp_set_stream_ratio(unsigned int ratio);
extern int cygnus_vpp_set_framesize(unsigned int width, unsigned int height);
extern int cygnus_vpp_set_playmode(GxVpuLayerPlayMode mode);
extern int cygnus_vpp_view_port(GxAvRect *clip_rect, GxAvRect *view_rect);
extern int cygnus_vpp_play_frame(struct frame *frame);
extern int cygnus_vpp_zoom_require(void);
extern unsigned int cygnus_vpu_get_scan_line(void);
extern unsigned int cygnus_vpu_check_layers_close(void);

extern int Cygnus_Vpu_SetVoutIsready(int ready);
extern int CygnusVpu_DACEnable(GxVpuDacID id, int enable);
extern int CygnusVpu_DACSource(GxVpuDacID dac_id, GxVpuDacSrc source);
#endif

