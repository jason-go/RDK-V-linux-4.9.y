#ifndef  __GX_MTC_MODULE_H_201506122015__
#define  __GX_MTC_MODULE_H_201506122015__

#include "avcore.h"


#ifdef __cplusplus
extern "C" {
#endif

int gxav_mtc_init(struct gxav_device *dev, struct gxav_module_inode *inode);
int gxav_mtc_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode);
int gxav_mtc_open(struct gxav_module *module);
int gxav_mtc_close(struct gxav_module *module);
int gxav_mtc_set_property(struct gxav_module *module, int property_id, void *property, int size);
int gxav_mtc_get_property(struct gxav_module *module, int property_id, void *property, int size);


#ifdef __cplusplus
}
#endif


#endif
