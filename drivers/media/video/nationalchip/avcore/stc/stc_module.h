#ifndef __GX_STC_MOD_H__
#define __GX_STC_MOD_H__

#include "avcore.h"

extern int gx_stc_init(struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_stc_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_stc_open(struct gxav_module *module);
extern int gx_stc_close(struct gxav_module *module);
extern int gx_stc_set_property(struct gxav_module *module, int property_id, void *property, int size);
extern int gx_stc_get_property(struct gxav_module *module, int property_id, void *property, int size);

#endif
