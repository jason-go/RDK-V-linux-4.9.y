#ifndef __vpu_region_h__
#define __vpu_region_h__

#include "vpu_hal.h"

typedef struct osd_region {
	unsigned int word1;
	unsigned int word2;
	unsigned int word3;
	unsigned int word4;
	unsigned int word5;
	unsigned int word6;
	unsigned int word7;

	unsigned pos_x, pos_y;
	unsigned width, height;
	VpuSurface *surface;
	struct osd_region *next;
} OsdRegion;

int gx_region_add   (VpuSurface *surface, unsigned pos_x, unsigned pos_y);
int gx_region_remove(VpuSurface *surface);
int gx_region_update(VpuSurface *surfac, unsigned pos_x, unsigned pos_y);

#endif

