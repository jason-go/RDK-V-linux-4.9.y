#ifndef __SIRIUS_VPU_INTERTNAL_H__
#define __SIRIUS_VPU_INTERTNAL_H__

#include "gxav_vpu_propertytypes.h"

#include "sirius_vpu_reg.h"
#include "sirius_vpu_internel.h"

extern unsigned int GA_INT_SRC;

#define OSD_FLICKER_TABLE_LEN     (64)

#define VPU_PRINTF(fmt, args...) \
	do { \
		gx_printf("\n[VPU][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)

extern int sirius_vpp_get_actual_viewrect(GxAvRect *rect);
extern int sirius_vpp_get_actual_cliprect(GxAvRect *rect);
extern int sirius_vpp_set_stream_ratio(unsigned int ratio);
extern int sirius_vpp_set_framesize(unsigned int width, unsigned int height);
extern int sirius_vpp_set_playmode(GxVpuLayerPlayMode mode);
extern int sirius_vpp_view_port(GxAvRect *clip_rect, GxAvRect *view_rect);
extern int sirius_vpp_play_frame(struct frame *frame);
extern int sirius_vpp_zoom_require(void);
extern unsigned int sirius_vpu_get_scan_line(void);
extern unsigned int sirius_vpu_check_layers_close(void);

extern int Sirius_Vpu_SetVoutIsready(int ready);
extern int SiriusVpu_DACEnable(GxVpuDacID id, int enable);
extern int SiriusVpu_DACSource(GxVpuDacID dac_id, GxVpuDacSrc source);
#endif

