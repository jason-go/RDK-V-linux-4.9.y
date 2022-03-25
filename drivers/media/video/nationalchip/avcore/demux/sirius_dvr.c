#include "kernelcalls.h"
#include "gx3211_regs.h"
#include "gx3211_demux.h"
#include "sirius_regs.h"
#include "taurus_regs.h"
#include "sirius_demux.h"
#include "gxav_bitops.h"
#include "sdc_hal.h"
#include "profile.h"
#include "gxdemux.h"
#include "sirius_pfilter.h"

void sirius_set_tsr_avid(int dmxid, void *vreg, int avid)
{
	struct sirius_reg_demux *reg = (struct sirius_reg_demux*)vreg;
	struct reg_demux_rts_buf *reg_rts_buf = (dmxid%2 == 0) ? &reg->rts_buf : &reg->rts_buf1;
	if (avid < 0)
		return;
	REG_SET_BIT(&(reg_rts_buf->rts_sdc_ctrl),( avid +16 ));
}

void sirius_clr_tsr_avid(int dmxid, void *vreg, int avid)
{
	struct sirius_reg_demux *reg = (struct sirius_reg_demux*)vreg;
	struct reg_demux_rts_buf *reg_rts_buf = (dmxid%2 == 0) ? &reg->rts_buf : &reg->rts_buf1;
	if (avid < 0)
		return;
	REG_CLR_BIT(&(reg_rts_buf->rts_sdc_ctrl),( avid +16 ));
}

void sirius_set_tsr_buf(int dmxid, void *vreg,unsigned int start_addr, unsigned int size, unsigned int gate)
{
	unsigned int temp;
	struct sirius_reg_demux *reg = (struct sirius_reg_demux*)vreg;
	struct reg_demux_rts_buf *reg_rts_buf = (dmxid%2 == 0) ? &reg->rts_buf : &reg->rts_buf1;

	REG_SET_VAL(&(reg_rts_buf->rts_almost_empty_gate), gate);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_addr), start_addr);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_size), size);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), 0);

	temp = (1<<BIT_DEMUX_READ_EN)|(12);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), temp);
}

void sirius_clr_tsr_buf(int dmxid, void *vreg)
{
	struct sirius_reg_demux *reg = (struct sirius_reg_demux*)vreg;
	struct reg_demux_rts_buf *reg_rts_buf = (dmxid%2 == 0) ? &reg->rts_buf : &reg->rts_buf1;
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_addr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_size), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_almost_empty_gate), 0);
}

void sirius_reset_tsr_buf(int dmxid, void *vreg)
{
	struct sirius_reg_demux *reg = (struct sirius_reg_demux*)vreg;
	struct reg_demux_rts_buf *reg_rts_buf = (dmxid%2 == 0) ? &reg->rts_buf : &reg->rts_buf1;

	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), (1<<25)|20);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), (1<<24)|20);
}

void sirius_set_dvr_source(int modid, int tswid, void *vreg, struct dvr_info *info)
{
	struct sirius_reg_demux *reg = (struct sirius_reg_demux *)vreg;
	unsigned int temp;
	unsigned int dmxid  = (modid%4)%2;
	unsigned int source = info->source;
	struct dmx_demux *demux = get_subdemux(dmxid);

	if (source == DVR_INPUT_TSPORT)
		source = demux->source;
	else if (source == DVR_INPUT_MEM)
		source = SIRIUS_TSW_SRC_TSR;
	else
		source = SIRIUS_TSW_SRC_DMXSLOT;

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

int sirius_dvr_get_tsr_id(struct gxav_module *module)
{
	if (module->sub >= MAX_DMX_TSW_SLOT && module->sub < MAX_DMX_PID_FILTER)
		return dvr_pfilter_get_tsr_id(module->sub%4);

	return 0;
}

int sirius_dvr_irq_entry(struct gxav_module_inode *inode, int irq)
{
	struct dmx_demux *demux = get_subdemux(0);

	if (irq != SIRIUS_PFILTER_IRQ)
		return -1;

	return dvr_pfilter_irq_entry(demux->dev->max_pfilter);
}

struct demux_dvr_ops sirius_dvr_ops = {
	.set_tsr_avid        = sirius_set_tsr_avid,
	.clr_tsr_avid        = sirius_clr_tsr_avid,
	.set_dvr_mode        = gx3211_set_dvr_mode,
	.set_dvr_source      = sirius_set_dvr_source,
	.set_tsw_buf         = gx3211_set_tsw_buf,
	.clr_tsw_buf         = gx3211_clr_tsw_buf,
	.reset_tsw_buf       = gx3211_reset_tsw_buf,
	.set_tsw_enable      = gx3211_set_tsw_enable,
	.set_tsw_disable     = gx3211_set_tsw_disable,
	.set_tsr_buf         = sirius_set_tsr_buf,
	.clr_tsr_buf         = sirius_clr_tsr_buf,
	.reset_tsr_buf       = sirius_reset_tsr_buf,
	.bind_slot_tsw       = gx3211_bind_slot_tsw,

	.tsw_isr             = gx3211_tsw_isr,
	.tsw_full_isr        = gx3211_tsw_full_isr,
	.tsw_update_rptr     = gx3211_tsw_update_rptr,
	.tsw_isr_en          = gx3211_tsw_isr_en,
	.tsw_module_enable   = gx3211_tsw_module_enable,
	.tsw_module_disable  = gx3211_tsw_module_disable,
	.tsr_isr             = sirius_tsr_isr,
	.tsr_dealwith        = sirius_tsr_dealwith,

#ifdef CONFIG_AV_MODULE_PIDFILTER
	.get_tsr_id          = sirius_dvr_get_tsr_id,
	.irq_entry           = sirius_dvr_irq_entry,
#endif
};

int sirius_dvr_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for (i=0; i<MAX_DEMUX_UNIT; i++) {
		dev = &gxdmxdev[i];
		dev->dvr_ops = &sirius_dvr_ops;
		dev->max_dvr = MAX_DVR_UNIT;
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_TSW))
			dev->max_tsw = (i == 0) ? MAX_SECURE_TSW : 0;
		else
			dev->max_tsw = MAX_SLOT_NUM;

		dev->dvr_mask = 0;
		dev->tsw_mask = 0;
		gx_memset(dev->dvr_handles, -1, sizeof(unsigned int)* MAX_SLOT_NUM);
	}

#ifdef CONFIG_AV_MODULE_PIDFILTER
	return sirius_pfilter_init();
#else
	return 0;
#endif
}

int sirius_dvr_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for (i=0; i<MAX_DEMUX_UNIT; i++) {
		dev = &gxdmxdev[i];
		dev->dvr_ops = NULL;
		dev->max_dvr = 0;
		dev->dvr_mask = 0;
	}

#ifdef CONFIG_AV_MODULE_PIDFILTER
	return sirius_pfilter_deinit();
#else
	return 0;
#endif
}

int sirius_dvr_open(struct gxav_module *module)
{
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(0);
	struct dmxdev    *dev   = demux->dev;

	(void) dev;
	(void) sub;

	if (module->sub < MAX_DMX_NORMAL) {
		return gx3211_dvr_open(module);

	} else if (module->sub < MAX_DMX_TSW_SLOT) {
		return -1;

#ifdef CONFIG_AV_MODULE_PIDFILTER
	} else if (module->sub < MAX_DMX_PID_FILTER) {
		if (sub >= dev->max_pfilter)
			return -1;
		return dvr_pfilter_open(sub);
#endif

#ifdef CONFIG_AV_MODULE_GSE
	} else if (module->sub < MAX_DMX_GSE) {
		if (sub >= dev->max_gse)
			return -1;
		return dvr_gse_open(sub);
#endif
	}

	return 0;
}

int sirius_dvr_close(struct gxav_module *module)
{
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(0);
	struct dmxdev    *dev   = demux->dev;

	(void) dev;
	(void) sub;

	if (module->sub < MAX_DMX_NORMAL) {
		return gx3211_dvr_close(module);

	} else if (module->sub < MAX_DMX_TSW_SLOT) {
		return -1;

#ifdef CONFIG_AV_MODULE_PIDFILTER
	} else if (module->sub < MAX_DMX_PID_FILTER) {
		if (sub >= dev->max_pfilter)
			return -1;
		return dvr_pfilter_close(sub);
#endif

#ifdef CONFIG_AV_MODULE_GSE
	} else if (module->sub < MAX_DMX_GSE) {
		if (sub >= dev->max_gse)
			return -1;
		return dvr_gse_close(sub);
#endif
	}

	return 0;
}

struct gxav_module_ops sirius_dvr_module = {
	.module_type  = GXAV_MOD_DVR,
	.count        = 16,
#ifdef CONFIG_AV_MODULE_PIDFILTER
	.irqs         = {SIRIUS_PFILTER_IRQ, -1},
	.irq_names    = {"ts_filter"},
#else
	.irqs         = {-1},
#endif
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = sirius_dvr_open,
	.close        = sirius_dvr_close,
	.init         = sirius_dvr_init,
	.cleanup      = sirius_dvr_cleanup,
	.read         = dvr_read,
	.write        = dvr_write,
	.set_property = dvr_set_entry,
	.get_property = dvr_get_entry,
#ifdef CONFIG_AV_MODULE_PIDFILTER
	.interrupts[SIRIUS_PFILTER_IRQ] = dvr_irq,
#endif
};

