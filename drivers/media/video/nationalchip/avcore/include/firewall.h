#ifndef __gxav_firewall_h__
#define __gxav_firewall_h__

#include "external/gxfirewall.h"

typedef enum gxav_buffer_type {
	GXAV_BUFFER_DEMUX_ES       = GXFW_BUFFER_DEMUX_ES       ,
	GXAV_BUFFER_DEMUX_TSW      = GXFW_BUFFER_DEMUX_TSW      ,
	GXAV_BUFFER_DEMUX_TSR      = GXFW_BUFFER_DEMUX_TSR      ,
	GXAV_BUFFER_GP_ES          = GXFW_BUFFER_GP_ES          ,
	GXAV_BUFFER_AUDIO_FIRMWARE = GXFW_BUFFER_AUDIO_FIRMWARE ,
	GXAV_BUFFER_AUDIO_FRAME    = GXFW_BUFFER_AUDIO_FRAME    ,
	GXAV_BUFFER_VIDEO_FIRMWARE = GXFW_BUFFER_VIDEO_FIRMWARE ,
	GXAV_BUFFER_VIDEO_FRAME    = GXFW_BUFFER_VIDEO_FRAME    ,
	GXAV_BUFFER_VPU_CAP        = GXFW_BUFFER_VPU_CAP        ,
	GXAV_BUFFER_VPU_SVPU       = (GXFW_BUFFER_VPU_CAP << 1) ,
	GXAV_BUFFER_SOFT           = GXFW_BUFFER_SOFT,
} GxAvBufferType;

int gxav_firewall_init(void);
int gxav_firewall_buffer_protected(GxAvBufferType type);
int gxav_firewall_access_align(void);
int gxav_firewall_protect_align(void);

int gxav_firewall_register_buffer(GxAvBufferType buf_type, unsigned int buf_phy_addr, unsigned int buf_size);

#endif

