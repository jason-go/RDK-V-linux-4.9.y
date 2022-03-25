#ifndef  __GX_IEMM_MODULE_H_201506121825__
#define  __GX_IEMM_MODULE_H_201506121825__

#include "avcore.h"


#ifdef __cplusplus
extern "C" {
#endif

int gxav_iemm_init(struct gxav_device *dev, struct gxav_module_inode *inode);
int gxav_iemm_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode);
int gxav_iemm_open(struct gxav_module *module);
int gxav_iemm_close(struct gxav_module *module);
int gxav_iemm_set_property(struct gxav_module *module, int property_id, void *property, int size);
int gxav_iemm_get_property(struct gxav_module *module, int property_id, void *property, int size);


#ifdef __cplusplus
}
#endif


#endif
