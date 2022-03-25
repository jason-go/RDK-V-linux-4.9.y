#include "kernelcalls.h"
#include "gx3211_regs.h"
#include "gx3211_demux.h"
#include "gxdemux.h"
#include "gxav_bitops.h"
#include "sdc_hal.h"
#include "profile.h"

void gx3211_set_tsr_avid(int dmxid, void *vreg, int avid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if (avid < 0)
		return;
	REG_SET_BIT(&(reg->rts_buf.rts_sdc_ctrl),( avid +16 ));
}

void gx3211_clr_tsr_avid(int dmxid, void *vreg, int avid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if (avid < 0)
		return;
	REG_SET_BIT(&(reg->rts_buf.rts_sdc_ctrl),( avid +16 ));
}

void gx3211_set_dvr_mode(int modid, void *vreg, int mode)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	if (modid < MAX_DMX_NORMAL) {
		if (mode == DVR_RECORD_ALL)
			REG_SET_BIT(&(reg->ts_write_ctrl), modid%2+2);
		else
			REG_CLR_BIT(&(reg->ts_write_ctrl), modid%2+2);
	}
}

void gx3211_set_dvr_source(int dmxid, int tswid, void *vreg, struct dvr_info *info)
{
	unsigned int temp;
	unsigned int source = info->source & 0x3;
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux *)vreg;
	struct dmx_demux *demux = get_subdemux(dmxid);

	if (source == DVR_INPUT_TSPORT)
		source = demux->source;
	else
		source = 3; /* from demux */

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

void gx3211_set_tsr_buf(int dmxid, void *vreg, unsigned int start_addr, unsigned int size, unsigned int gate)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	unsigned int temp;
	REG_SET_VAL(&(reg->rts_buf.rts_almost_empty_gate), gate);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_addr), start_addr);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_size), size);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_wptr), 0);

	temp = (1<<BIT_DEMUX_READ_EN)|(20);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_ctrl), temp);
}

void gx3211_clr_tsr_buf(int dmxid, void *vreg)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_ctrl), 0);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_addr), 0);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_size), 0);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_wptr), 0);
	REG_SET_VAL(&(reg->rts_buf.rts_almost_empty_gate), 0);
}

void gx3211_reset_tsr_buf(int dmxid, void *vreg)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_VAL(&(reg->rts_buf.rts_sdc_ctrl), 0);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_wptr), 0);
	REG_SET_VAL(&(reg->rts_buf.rts_sdc_ctrl), (1<<24)|20);
}

void gx3211_bind_slot_tsw(void *vreg,unsigned int slotid,unsigned int tswid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_FIELD(&(reg->wen_mask), DEMUX_PID_CFG_TSW_BUF_SEL, 0x3f, BIT_DEMUX_PID_CFG_TSW_BUF);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg_l), tswid);
	REG_SET_FIELD(&(reg->wen_mask), DEMUX_PID_CFG_TSW_BUF_SEL, 0x0, BIT_DEMUX_PID_CFG_TSW_BUF);
}

void gx3211_set_tsw_buf(int dmxid, int tswid, void *vreg,
		unsigned int start_addr, unsigned int size, unsigned int gate)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux *)vreg;

	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_buf_start_addr), start_addr);
	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_buf_len),size);
	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_write_addr),0);
	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_read_addr),0);
	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_almost_full_gate),gate);
}

void gx3211_clr_tsw_buf(int dmxid, int tswid, void *vreg)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux *)vreg;

	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_buf_start_addr), 0);
	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_buf_len),0);
	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_write_addr),0);
	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_read_addr),0);
	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_almost_full_gate),0);
}

void gx3211_set_tsw_enable(int dmxid, int tswid, void *vreg)
{
	int bit;
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux *)vreg;
	if (dmxid%2 == 0)
		REG_SET_BIT(&(reg->tsw_buf_en_0_l),tswid);
	else
		REG_SET_BIT(&(reg->tsw_buf_en_1_l),tswid);

	if (tswid < 32) {
		bit = tswid;
		REG_SET_VAL(&(reg->int_tsw_buf_alful_l),(0x1<<bit));
		REG_SET_BIT(&(reg->int_tsw_buf_alful_en_l),bit);
		REG_SET_VAL(&(reg->int_tsw_buf_ful_l),(0x1<<bit));
		REG_SET_BIT(&(reg->int_tsw_buf_ful_en_l),bit);

	} else {
		bit = tswid-32;
		REG_SET_VAL(&(reg->int_tsw_buf_alful_h),(0x1<<bit));
		REG_SET_BIT(&(reg->int_tsw_buf_alful_en_h),bit);
		REG_SET_VAL(&(reg->int_tsw_buf_ful_h),(0x1<<bit));
		REG_SET_BIT(&(reg->int_tsw_buf_ful_en_h),bit);
	}
}

void gx3211_set_tsw_disable(int dmxid, int tswid, void *vreg)
{
	int bit;
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux *)vreg;
	if (dmxid%2 == 0)
		REG_CLR_BIT(&(reg->tsw_buf_en_0_l),tswid);
	else
		REG_CLR_BIT(&(reg->tsw_buf_en_1_l),tswid);

	if (tswid < 32) {
		bit = tswid;
		REG_SET_VAL(&(reg->int_tsw_buf_alful_l),(0x1<<bit));
		REG_CLR_BIT(&(reg->int_tsw_buf_alful_en_l),bit);
		REG_SET_VAL(&(reg->int_tsw_buf_ful_l),(0x1<<bit));
		REG_CLR_BIT(&(reg->int_tsw_buf_ful_en_l),bit);

	} else {
		bit = tswid-32;
		REG_SET_VAL(&(reg->int_tsw_buf_alful_h),(0x1<<bit));
		REG_CLR_BIT(&(reg->int_tsw_buf_alful_en_h),bit);
		REG_SET_VAL(&(reg->int_tsw_buf_ful_h),(0x1<<bit));
		REG_CLR_BIT(&(reg->int_tsw_buf_ful_en_h),bit);
	}
}

void gx3211_tsw_module_enable(int dmxid, void *vreg)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux *)vreg;
	REG_SET_BIT(&(reg->ts_write_ctrl), dmxid%2);
}

void gx3211_tsw_module_disable(int dmxid, void *vreg)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux *)vreg;
	REG_CLR_BIT(&(reg->ts_write_ctrl), dmxid%2);
}

void gx3211_reset_tsw_buf(int dmxid, int tswid, void *vreg)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_write_addr),0);
	REG_SET_VAL(&(reg->tsw_buf[tswid].tsw_read_addr),0);
}

struct demux_dvr_ops gx3211_dvr_ops = {
	.set_tsr_avid        = gx3211_set_tsr_avid,
	.clr_tsr_avid        = gx3211_clr_tsr_avid,
	.set_dvr_mode        = gx3211_set_dvr_mode,
	.set_dvr_source      = gx3211_set_dvr_source,
	.set_tsw_buf         = gx3211_set_tsw_buf,
	.clr_tsw_buf         = gx3211_clr_tsw_buf,
	.reset_tsw_buf       = gx3211_reset_tsw_buf,
	.set_tsw_enable      = gx3211_set_tsw_enable,
	.set_tsw_disable     = gx3211_set_tsw_disable,
	.set_tsr_buf         = gx3211_set_tsr_buf,
	.clr_tsr_buf         = gx3211_clr_tsr_buf,
	.reset_tsr_buf       = gx3211_reset_tsr_buf,
	.bind_slot_tsw       = gx3211_bind_slot_tsw,

	.tsw_isr             = gx3211_tsw_isr,
	.tsw_full_isr        = gx3211_tsw_full_isr,
	.tsw_isr_en          = gx3211_tsw_isr_en,
	.tsw_module_enable   = gx3211_tsw_module_enable,
	.tsw_module_disable  = gx3211_tsw_module_disable,
	.tsr_isr             = gx3211_tsr_isr,
	.tsr_dealwith        = gx3211_tsr_dealwith,
};

int gx3211_dvr_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		dev = &gxdmxdev[i];
		dev->dvr_ops = &gx3211_dvr_ops;
		dev->max_dvr = MAX_DVR_UNIT;
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_TSW))
			dev->max_tsw = (i == 0) ? MAX_SECURE_TSW : 0;
		else
			dev->max_tsw = MAX_SLOT_NUM;

		dev->max_pfilter = 0;
		dev->max_pfilter_pid = 0;
		gx_memset(dev->dvr_handles, -1, sizeof(unsigned int)* MAX_SLOT_NUM);
	}

	return 0;
}

int gx3211_dvr_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		dev = &gxdmxdev[i];
		dev->dvr_ops = NULL;
		dev->max_dvr = 0;
	}

	return 0;
}

int gx3211_dvr_open(struct gxav_module *module)
{
	return dvr_open(module);
}

int gx3211_dvr_close(struct gxav_module *module)
{
	return dvr_close(module);
}

struct gxav_module_ops gx3211_dvr_module = {
	.module_type  = GXAV_MOD_DVR,
	.irqs         = {-1},
	.irq_flags    = {-1},
	.count        = MAX_DVR_UNIT,
	.event_mask   = 0xffffffff,
	.open         = gx3211_dvr_open,
	.close        = gx3211_dvr_close,
	.init         = gx3211_dvr_init,
	.cleanup      = gx3211_dvr_cleanup,
	.read         = dvr_read,
	.write        = dvr_write,
	.set_property = dvr_set_entry,
	.get_property = dvr_get_entry,
};

