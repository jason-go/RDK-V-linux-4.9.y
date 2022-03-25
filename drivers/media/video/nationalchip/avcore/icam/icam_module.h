#ifndef  __GX_ICAM_MODULE_H_201506111025__
#define  __GX_ICAM_MODULE_H_201506111025__

#include "avcore.h"


#ifdef __cplusplus
extern "C" {
#endif

int gxav_icam_init(struct gxav_device *dev, struct gxav_module_inode *inode);
int gxav_icam_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode);
int gxav_icam_open(struct gxav_module *module);
int gxav_icam_close(struct gxav_module *module);
int gxav_icam_set_property(struct gxav_module *module, int property_id, void *property, int size);
int gxav_icam_get_property(struct gxav_module *module, int property_id, void *property, int size);
struct gxav_module_inode *gxav_icam_interrupt(struct gxav_module_inode *inode, int irq);

#ifdef __cplusplus
}
#endif


#endif
