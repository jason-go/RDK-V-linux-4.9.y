#ifndef  __GX_DESCRAMBLER_MODULE_H_201506121950__
#define  __GX_DESCRAMBLER_MODULE_H_201506121950__

#include "avcore.h"


#ifdef __cplusplus
extern "C" {
#endif

int gxav_descrambler_init(struct gxav_device *dev, struct gxav_module_inode *inode);
int gxav_descrambler_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode);
int gxav_descrambler_open(struct gxav_module *module);
int gxav_descrambler_close(struct gxav_module *module);
int gxav_descrambler_set_property(struct gxav_module *module, int property_id, void *property, int size);
int gxav_descrambler_get_property(struct gxav_module *module, int property_id, void *property, int size);


#ifdef __cplusplus
}
#endif


#endif
