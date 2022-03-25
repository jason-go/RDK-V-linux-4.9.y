#ifndef __GXAV_DVR2DVR_PROTOCOL_H__
#define __GXAV_DVR2DVR_PROTOCOL_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "gxav_common.h"

typedef enum {
	GXAV_DVR2DVR_PROTOCOL_OPEN,
	GXAV_DVR2DVR_PROTOCOL_CLOSE,
	GXAV_DVR2DVR_PROTOCOL_CONFIG,
	GXAV_DVR2DVR_PROTOCOL_START,
	GXAV_DVR2DVR_PROTOCOL_STOP,
	GXAV_DVR2DVR_PROTOCOL_START_CACHE,
} GxAvDVR2DVRProtocolCMD;

typedef struct gxav_dvr2dvr_protocol {
	int dvrid;
	GxAvDVR2DVRProtocolCMD cmd;
	union {
		struct {
			int shared;
			int cachems;
		} open;
		struct {
			int handle;
		} config;
	} params;
} GxAvDVR2DVRProtocol;

int gxav_dvr2dvr_protocol_control(GxAvDVR2DVRProtocol *params);

#ifdef __cplusplus
}
#endif


#endif
