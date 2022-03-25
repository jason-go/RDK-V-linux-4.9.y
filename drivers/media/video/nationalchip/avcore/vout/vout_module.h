#ifndef __VOUT_MODULE_H__
#define __VOUT_MODULE_H__
#include "gxav.h"

#include "hdmi_hal.h"
#include "vout_hal.h"
#include "kernelcalls.h"

extern int gx_videoout_init(struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_videoout_uninit(struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_videoout_open(struct gxav_module *module);
extern int gx_videoout_close(struct gxav_module *module);
extern int gx_videoout_set_property(struct gxav_module *module, int property_id, void *property, int size);
extern int gx_videoout_get_property(struct gxav_module *module, int property_id, void *property, int size);
extern struct gxav_module_inode *gx_video_out_interrupt(struct gxav_module_inode *inode, int irq);

#endif
