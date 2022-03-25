#include "kernelcalls.h"
#include "gx3211_demux.h"
#include "sirius_demux.h"
#include "taurus_demux.h"
#include "gx6605s_demux.h"
#include "pegasus_regs.h"
#include "sdc_hal.h"
#include "gxav_bitops.h"
#include "profile.h"

void pegasus_set_dvr_mode(int modid, void *vreg, int mode)
{
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;

	if (modid < MAX_DMX_NORMAL) {
		if (mode == DVR_RECORD_ALL)
			REG_SET_BIT(&(reg->ts_write_ctrl), (modid%2+2));
		else
			REG_CLR_BIT(&(reg->ts_write_ctrl), (modid%2+2));

		if (mode == DVR_RECORD_BY_REVERSE_SLOT)
			REG_SET_BIT(&(reg->discard_pkt_enable), (modid%2+2));
		else
			REG_CLR_BIT(&(reg->discard_pkt_enable), (modid%2+2));

	} else {
		if (mode == DVR_RECORD_BY_REVERSE_SLOT)
			REG_SET_BIT(&(reg->discard_pkt_enable), (modid%2));
		else
			REG_CLR_BIT(&(reg->discard_pkt_enable), (modid%2));
	}
}

void pegasus_set_tsr_buf(int dmxid, void *vreg,unsigned int start_addr, unsigned int size, unsigned int gate)
{
	unsigned int temp;
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;
	struct reg_demux_rts_buf *reg_rts_buf = &reg->rts_buf;

	REG_SET_VAL(&(reg_rts_buf->rts_almost_empty_gate), gate);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_addr), start_addr);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_size), size);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), 0);

	temp = (1<<BIT_DEMUX_READ_EN)|(20);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), temp);
}

void pegasus_clr_tsr_buf(int dmxid, void *vreg)
{
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;
	struct reg_demux_rts_buf *reg_rts_buf = &reg->rts_buf;
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_addr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_size), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_almost_empty_gate), 0);
}

void pegasus_reset_tsr_buf(int dmxid, void *vreg)
{
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;
	struct reg_demux_rts_buf *reg_rts_buf = &reg->rts_buf;

	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), (1<<25)|20);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), (1<<24)|20);
}

void pegasus_t2mi_set_source(int dmxid, void *vreg)
{
	unsigned int source = 0;
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;
	volatile unsigned int *temp_reg = &reg->dmux_cfg;
	unsigned int mask = DEMUX_CFG_TS_SEL1, offset = BIT_DEMUX_CFG_TS_SEL_1;

	if (dmxid == 0) {
		temp_reg = &reg->dmux_cfg;
		mask     = DEMUX_CFG_TS_SEL1;
		offset   = BIT_DEMUX_CFG_TS_SEL_1;

	} else if (dmxid == 1) {
		temp_reg = &reg->dmux_cfg;
		mask     = DEMUX_CFG_TS_SEL2;
		offset   = BIT_DEMUX_CFG_TS_SEL_2;

	} else if (dmxid == 4) {
		temp_reg = &reg->ts_write_ctrl;
		mask     = 0x03 << 4;
		offset   = 4;

	} else if (dmxid == 5) {
		temp_reg = &reg->ts_write_ctrl;
		mask     = 0x03 << 6;
		offset   = 6;
	}

	if ((source = REG_GET_FIELD(temp_reg, mask, offset)) < DEMUX_TS3)
		REG_SET_BIT(&(reg->t2mi_out_sel), source);
}

void pegasus_t2mi_clr_source(int dmxid, void *vreg)
{
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;
	REG_SET_VAL(&(reg->t2mi_out_sel), 0);
}

void pegasus_t2mi_set_dest(int dmxid, void *vreg)
{
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;

	if (dmxid < 2)
		REG_SET_VAL(&(reg->t2mi_in_sel), dmxid);
	else
		REG_SET_VAL(&(reg->t2mi_in_sel), dmxid%2+2);
}

void pegasus_t2mi_clr_dest(int dmxid, void *vreg)
{
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;
	REG_SET_VAL(&(reg->t2mi_in_sel), 0);
}

struct demux_dvr_ops pegasus_dvr_ops = {
	.set_tsr_avid        = gx3211_set_tsr_avid,
	.clr_tsr_avid        = gx3211_clr_tsr_avid,
	.set_dvr_mode        = pegasus_set_dvr_mode,
	.set_dvr_source      = gx6605s_set_dvr_source,
	.set_tsw_buf         = gx3211_set_tsw_buf,
	.clr_tsw_buf         = gx3211_clr_tsw_buf,
	.reset_tsw_buf       = gx3211_reset_tsw_buf,
	.set_tsw_enable      = gx3211_set_tsw_enable,
	.set_tsw_disable     = gx3211_set_tsw_disable,
	.set_tsr_buf         = pegasus_set_tsr_buf,
	.clr_tsr_buf         = pegasus_clr_tsr_buf,
	.reset_tsr_buf       = pegasus_reset_tsr_buf,
	.set_tsw_slot_buf    = pegasus_set_tsr_buf,
	.clr_tsw_slot_buf    = pegasus_clr_tsr_buf,
	.reset_tsw_slot_buf  = pegasus_reset_tsr_buf,
	.bind_slot_tsw       = taurus_bind_slot_tsw,
	.pf_bind_slot_tsw    = taurus_pf_bind_slot_tsw,

	.tsw_isr             = gx3211_tsw_isr,
	.tsw_full_isr        = gx3211_tsw_full_isr,
	.tsw_module_enable   = gx3211_tsw_module_enable,
	.tsw_isr_en          = gx3211_tsw_isr_en,
	.tsr_isr             = gx3211_tsr_isr,
	.tsr_dealwith        = gx3211_tsr_dealwith,

	.t2mi_set_source     = pegasus_t2mi_set_source,
	.t2mi_clr_source     = pegasus_t2mi_clr_source,
	.t2mi_set_dest       = pegasus_t2mi_set_dest,
	.t2mi_clr_dest       = pegasus_t2mi_clr_dest,
};

int pegasus_dvr_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for(i = 0; i < MAX_DEMUX_UNIT; i++){
		dev = &gxdmxdev[i];
		dev->dvr_ops = &pegasus_dvr_ops;
		dev->max_dvr = TAURUS_MAX_DVR_UNIT;
		dev->max_tsw = TAURUS_MAX_SLOT_NUM;
		dev->dvr_mask = 0;
		dev->tsw_mask = 0;
		gx_memset(dev->dvr_handles, -1, sizeof(unsigned int)* MAX_SLOT_NUM);
	}

	return 0;
}

int pegasus_dvr_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
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

int pegasus_dvr_open(struct gxav_module *module)
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

int pegasus_dvr_close(struct gxav_module *module)
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

struct gxav_module_ops pegasus_dvr_module = {
	.module_type  = GXAV_MOD_DVR,
	.count        = 8,
	.irqs         = {-1},
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = pegasus_dvr_open,
	.close        = pegasus_dvr_close,
	.init         = pegasus_dvr_init,
	.cleanup      = pegasus_dvr_cleanup,
	.read         = dvr_read,
	.write        = dvr_write,
	.set_property = dvr_set_entry,
	.get_property = dvr_get_entry,
};

