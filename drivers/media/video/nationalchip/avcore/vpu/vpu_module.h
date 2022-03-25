#ifndef __GX_VPU_MOD_H__
#define __GX_VPU_MOD_H__

#include "avcore.h"

extern int gx_vpu_open(struct gxav_module *module);
extern int gx_vpu_close(struct gxav_module *module);
extern int gx_vpu_init(struct gxav_device *device, struct gxav_module_inode *inode);
extern int gx_vpu_cleanup(struct gxav_device *device, struct gxav_module_inode *inode);
extern int gx_vpu_set_property(struct gxav_module *module, int property_id, void *property, int size);
extern int gx_vpu_get_property(struct gxav_module *module, int property_id, void *property, int size);
extern struct gxav_module_inode *gx_vpu_ga_interrupt(struct gxav_module_inode *inode, int irq);

#endif

