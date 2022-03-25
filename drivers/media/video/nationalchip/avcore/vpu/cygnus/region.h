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

int cygnus_region_add   (GxLayerID layer_id, VpuSurface *surface, unsigned pos_x, unsigned pos_y);
OsdRegion *cygnus_region_revoke(GxLayerID layer_id, VpuSurface *surface);
int cygnus_region_reconfig(GxLayerID layer_id, OsdRegion *region, VpuSurface *surface, unsigned pos_x, unsigned pos_y);
int cygnus_region_remove(GxLayerID layer_id, VpuSurface *surface);
int cygnus_region_update(GxLayerID layer_id, VpuSurface *surfac, unsigned pos_x, unsigned pos_y);

#endif

