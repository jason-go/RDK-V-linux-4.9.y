#include "kernelcalls.h"
#include "gx3211_regs.h"
#include "gx3211_demux.h"
#include "sirius_regs.h"
#include "sirius_demux.h"
#include "taurus_regs.h"
#include "taurus_demux.h"
#include "taurus_pfilter.h"
#include "sdc_hal.h"
#include "gxav_bitops.h"
#include "profile.h"

void gx6605s_set_dvr_source(int modid, int tswid, void *vreg, struct dvr_info *info)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux *)vreg;
	unsigned int temp, source;
	unsigned int tsw_src = info->source;
	unsigned int dmxid   = (modid%4)%2;
	struct dmx_demux *demux = get_subdemux(dmxid);

	if (tsw_src == DVR_INPUT_TSPORT)
		source = demux->source;
	else {
		if (modid < MAX_DMX_NORMAL)
			source = 3; // from demux slot
		else
			source = 4; // from tsw slot
	}

	temp = REG_GET_VAL(&(reg->ts_write_ctrl));
	if(dmxid == 0){
		temp &= ~(0x07 << 8);
		temp |= (source << 8);
		temp &= ~(0x3f << 16);
		temp |= (tswid << 16);
	} else {
		temp &= ~(0x07 << 12);
		temp |= (source << 12);
		temp &= ~(0x3f << 24);
		temp |= (tswid << 24);
	}
	REG_SET_VAL(&(reg->ts_write_ctrl), temp);
}

struct demux_dvr_ops gx6605s_dvr_ops = {
	.set_tsr_avid        = gx3211_set_tsr_avid,
	.clr_tsr_avid        = gx3211_clr_tsr_avid,
	.set_dvr_mode        = gx3211_set_dvr_mode,
	.set_dvr_source      = gx6605s_set_dvr_source,
	.set_tsw_buf         = gx3211_set_tsw_buf,
	.clr_tsw_buf         = gx3211_clr_tsw_buf,
	.reset_tsw_buf       = gx3211_reset_tsw_buf,
	.set_tsw_enable      = gx3211_set_tsw_enable,
	.set_tsw_disable     = gx3211_set_tsw_disable,
	.set_tsr_buf         = gx3211_set_tsr_buf,
	.clr_tsr_buf         = gx3211_clr_tsr_buf,
	.reset_tsr_buf       = gx3211_reset_tsr_buf,
	.bind_slot_tsw       = taurus_bind_slot_tsw,
	.pf_bind_slot_tsw    = taurus_pf_bind_slot_tsw,

	.tsw_isr             = gx3211_tsw_isr,
	.tsw_full_isr        = gx3211_tsw_full_isr,
	.tsw_isr_en          = gx3211_tsw_isr_en,
	.tsw_module_enable   = gx3211_tsw_module_enable,
	.tsw_module_disable  = gx3211_tsw_module_disable,
	.tsr_isr             = gx3211_tsr_isr,
	.tsr_dealwith        = gx3211_tsr_dealwith,
};

int gx6605s_dvr_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for(i = 0; i < MAX_DEMUX_UNIT; i++){
		dev = &gxdmxdev[i];
		dev->dvr_ops = &gx6605s_dvr_ops;
		dev->max_dvr = TAURUS_MAX_DVR_UNIT;
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_TSW))
			dev->max_tsw = (i == 0) ? MAX_SECURE_TSW : 0;
		else
			dev->max_tsw = TAURUS_MAX_SLOT_NUM;

		dev->dvr_mask = 0;
		dev->tsw_mask = 0;
		gx_memset(dev->dvr_handles, -1, sizeof(unsigned int)* MAX_SLOT_NUM);
	}

	return 0;
}

int gx6605s_dvr_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for (i=0; i<MAX_DEMUX_UNIT; i++){
		dev = &gxdmxdev[i];
		dev->dvr_ops = NULL;
		dev->max_dvr = 0;
	}

	return 0;
}

int gx6605s_dvr_open(struct gxav_module *module)
{
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(0);
	struct dmxdev    *dev   = demux->dev;

	if (module->sub < MAX_DMX_TSW_SLOT) {
		if (sub >= dev->max_dvr)
			return -1;
		return gx3211_dvr_open(module);
	}

	return 0;
}

int gx6605s_dvr_close(struct gxav_module *module)
{
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(0);
	struct dmxdev    *dev   = demux->dev;

	if (module->sub < MAX_DMX_TSW_SLOT) {
		if (sub >= dev->max_dvr)
			return -1;
		return gx3211_dvr_close(module);
	}

	return 0;
}

struct gxav_module_ops gx6605s_dvr_module = {
	.module_type  = GXAV_MOD_DVR,
	.count        = 8,
	.irqs         = {-1},
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = gx6605s_dvr_open,
	.close        = gx6605s_dvr_close,
	.init         = gx6605s_dvr_init,
	.cleanup      = gx6605s_dvr_cleanup,
	.read         = dvr_read,
	.write        = dvr_write,
	.set_property = dvr_set_entry,
	.get_property = dvr_get_entry,
};

