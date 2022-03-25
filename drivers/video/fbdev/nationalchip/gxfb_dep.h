
#ifndef _GXFB_AVDEV_DEP_H_
#define _GXFB_AVDEV_DEP_H_

/* depend on gx av.ko module struct */
#include "gxavdev.h"
#include "gxav_ioctl.h"
#include "gxav_vpu_propertytypes.h"
#include "gxav_vout_propertytypes.h"

#define GXAV_PACKET_PARA(para, id, addr, size)	\
	do {					\
		(para)->prop_id = (id);		\
		(para)->prop_val = (addr);	\
		(para)->prop_size = (size);	\
	} while (0)

/* FRAMEBUFFER / MODULE */

int fb_property_set(struct gxav_priv_dev *dev, void *arg);
int fb_property_get(struct gxav_priv_dev *dev, void *arg);

#endif
