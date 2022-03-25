#ifndef __HDMI_MODULE_H__
#define __HDMI_MODULE_H__
#include "gxav.h"

#include "hdmi_hal.h"
#include "vout_hal.h"
#include "kernelcalls.h"

extern int gx_hdmi_init(struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_hdmi_uninit(struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_hdmi_open(struct gxav_module *module);
extern int gx_hdmi_close(struct gxav_module *module);
extern int gx_hdmi_set_property(struct gxav_module *module, int property_id, void *property, int size);
extern int gx_hdmi_get_property(struct gxav_module *module, int property_id, void *property, int size);
extern struct gxav_module_inode *gx_hdmi_interrupt(struct gxav_module_inode *inode, int irq);
extern int gxav_hdmi_wake_event(unsigned int event);

#endif
