#ifndef __video_hal_h__
#define __video_hal_h__

#include "gxav.h"
#include "avcore.h"

extern unsigned int VIDEO_PP_INC_SRC   ;
extern unsigned int VIDEO_VPU_INC_SRC  ;
extern unsigned int VIDEO_BODA_INIT_SRC;

int video_hal_init(struct gxav_device *dev, struct gxav_module_inode *inode);

int video_hal_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode);

int video_hal_open(struct gxav_module *module);

int video_hal_close(struct gxav_module *module);

int video_hal_set_property(struct gxav_module *module, int property_id, void *property, int size);

int video_hal_get_property(struct gxav_module *module, int property_id, void *property, int size);

struct gxav_module_inode *video_hal_interrupt(struct gxav_module_inode *inode, int irq);

#endif

