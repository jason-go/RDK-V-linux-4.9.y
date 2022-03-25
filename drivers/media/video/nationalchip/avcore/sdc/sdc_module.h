#ifndef __GX_SDC_MOD__
#define __GX_SDC_MOD__

#include "kernelcalls.h"
#include "gxav_common.h"
#include "sdc_hal.h"

extern int gx_sdc_apply(struct gxav_channel *channel, GxAvChannelType type);
extern int gx_sdc_free(struct gxav_channel *channel, GxAvChannelType type);
extern int gx_sdc_reset(struct gxav_channel *channel);
extern int gx_sdc_flush(struct gxav_channel *channel);
extern int gx_sdc_gate_set(struct gxav_channel *channel, void *info);
extern int gx_sdc_pts_set(struct gxav_channel *channel, unsigned int start_addr, int pts);
extern int gx_sdc_pts_get(struct gxav_channel *channel, int *pts);
extern int gx_sdc_inc(struct gxav_channel *channel, int len);
extern int gx_sdc_dec(struct gxav_channel *channel, int len);
extern int gx_sdc_freesize_get(struct gxav_channel *channel);
extern int gx_sdc_length_get(struct gxav_channel *channel);
extern int gx_sdc_cap_get(struct gxav_channel *channel);
extern int gx_sdc_pts_cap(struct gxav_channel *channel);
extern int gx_sdc_pts_freesize(struct gxav_channel *channel);
extern int gx_sdc_pts_length(struct gxav_channel *channel);
extern int gx_sdc_pts_isfull(struct gxav_channel *channel);

extern int gx_sdc_init(struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_sdc_open(struct gxav_module *module);
extern int gx_sdc_close(struct gxav_module *module);
extern int gx_sdc_uninit(struct gxav_device *dev, struct gxav_module_inode *inode);
extern int gx_sdc_callback(int channel_id, unsigned int len,  unsigned int dataflow, GxAvSdcOPMode mode);
extern unsigned long gx_sdc_spin_lock_irqsave(int channel_id);
extern void gx_sdc_spin_unlock_irqrestore(int channel_id, unsigned long flags);
extern void* gx_sdc_get_outdata(int channel_id);
extern void* gx_sdc_get_indata(int channel_id);

#endif

