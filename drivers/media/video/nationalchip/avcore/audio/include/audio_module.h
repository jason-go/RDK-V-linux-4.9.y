#ifndef __AUDIO_MOD_H__
#define __AUDIO_MOD_H__

extern int gx_audiodec_init  (struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_audiodec_uninit(struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_audiodec_open  (struct gxav_module *module);
extern int gx_audiodec_close (struct gxav_module *module);
extern int gx_audiodec_set_property  (struct gxav_module *module, int property_id, void *property, int size);
extern int gx_audiodec_get_property  (struct gxav_module *module, int property_id, void *property, int size);
extern int gx_audiodec_write_ctrlinfo(struct gxav_module *module, void *ctrl_info, int ctrl_size);
extern int gx_audiodec_callback(unsigned int event, void *priv);
extern struct gxav_module_inode *gx_audiodec_interrupt(struct gxav_module_inode *inode, int irq);

extern int gx_audioout_init  (struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_audioout_uninit(struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_audioout_open  (struct gxav_module *module);
extern int gx_audioout_close (struct gxav_module *module);
extern int gx_audioout_set_property(struct gxav_module *module, int property_id, void *property, int size);
extern int gx_audioout_get_property(struct gxav_module *module, int property_id, void *property, int size);
extern int gx_audioout_callback(unsigned int event, void *priv);
extern struct gxav_module_inode *gx_audioout_i2s_interrupt(struct gxav_module_inode *inode, int irq);
extern struct gxav_module_inode *gx_audioout_spdif_interrupt(struct gxav_module_inode *inode, int irq);

#endif

