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

void taurus_set_tsw_slot_buf(int dmxid, void *vreg,unsigned int start_addr, unsigned int size, unsigned int gate)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	unsigned int temp;
	struct reg_demux_rts_buf *reg_rts_buf = &reg->pfilter_rts_buf[dmxid%2];

	REG_SET_VAL(&(reg_rts_buf->rts_almost_empty_gate), gate);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_addr), start_addr);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_size), size);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), 0);

	temp = (1<<BIT_DEMUX_READ_EN)|(20);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), temp);
}

void taurus_clr_tsw_slot_buf(int dmxid, void *vreg)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	struct reg_demux_rts_buf *reg_rts_buf = &reg->pfilter_rts_buf[dmxid%2];
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_addr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_size), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_almost_empty_gate), 0);
}

void taurus_reset_tsw_slot_buf(int dmxid, void *vreg)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	struct reg_demux_rts_buf *reg_rts_buf = &reg->pfilter_rts_buf[dmxid%2];
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), (1<<25)|20);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), 0);
	REG_SET_VAL(&(reg_rts_buf->rts_sdc_ctrl), (1<<24)|20);
}

void taurus_set_dvr_source(int modid, int tswid, void *vreg, struct dvr_info *info)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux *)vreg;
	unsigned int temp, source;
	unsigned int tsw_src = info->source;
	unsigned int dmxid   = (modid%4)%2;
	struct dmx_demux *demux = get_subdemux(dmxid);

	if (tsw_src == DVR_INPUT_TSPORT)
		source = demux->source;
	else {
		if (modid < MAX_DMX_NORMAL) {
			if (tsw_src == DVR_INPUT_MEM)
				source = TAURUS_TSW_SRC_TSR;
			else
				source = TAURUS_TSW_SRC_DMXSLOT;
		} else {
			if (tsw_src == DVR_INPUT_MEM)
				source = TAURUS_TSW_SRC_TSR_FOR_TSWSLOT;
			else
				source = TAURUS_TSW_SRC_TSWSLOT;
		}
	}

	temp = REG_GET_VAL(&(reg->ts_write_ctrl));
	if (dmxid == 0) {
		temp &= ~(0x07 << 10);
		temp |= (source << 10);
		temp &= ~(0x3f << 16);
		temp |= (tswid << 16);
	} else {
		temp &= ~(0x07 << 13);
		temp |= (source << 13);
		temp &= ~(0x3f << 24);
		temp |= (tswid << 24);
	}
	REG_SET_VAL(&(reg->ts_write_ctrl), temp);
}

void taurus_bind_slot_tsw(void *vreg,unsigned int slotid,unsigned int tswid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_FIELD(&(reg->wen_mask), TAURUS_PID_CFG_TSW_BUF_SEL, 0x1f, TAURUS_BIT_PID_CFG_TSW_BUF);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg),tswid<<TAURUS_BIT_PID_CFG_TSW_BUF);
	REG_SET_FIELD(&(reg->wen_mask), TAURUS_PID_CFG_TSW_BUF_SEL, 0x0, TAURUS_BIT_PID_CFG_TSW_BUF);
}

void taurus_pf_bind_slot_tsw(void *vreg,unsigned int slotid,unsigned int tswid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_FIELD(&(reg->wen_mask), TAURUS_PID_CFG_TSW_BUF_SEL, 0x1f, TAURUS_BIT_PID_CFG_TSW_BUF);
	REG_SET_VAL(&(reg->tsw_pid_cfg_sel[slotid].pid_cfg),tswid<<TAURUS_BIT_PID_CFG_TSW_BUF);
	REG_SET_FIELD(&(reg->wen_mask), TAURUS_PID_CFG_TSW_BUF_SEL, 0x0, TAURUS_BIT_PID_CFG_TSW_BUF);
}

int taurus_dvr_irq_entry(struct gxav_module_inode *inode, int irq)
{
	struct dmx_demux *demux = get_subdemux(0);

	if (irq != TAURUS_PFILTER_IRQ)
		return -1;

	return dvr_pfilter_irq_entry(demux->dev->max_pfilter);
}

void taurus_t2mi_set_source(int dmxid, void *vreg)
{
	unsigned int source = 0;
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
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
		mask     = 0x07 << 4;
		offset   = 4;

	} else if (dmxid == 5) {
		temp_reg = &reg->ts_write_ctrl;
		mask     = 0x07 << 7;
		offset   = 7;
	}

	source = REG_GET_FIELD(temp_reg, mask, offset);
	if (source - 1 < DEMUX_TS2)
		REG_SET_BIT(&reg->ts_i_m2ts_sel, source-1);
	REG_SET_VAL(&(reg->internel_clk_div0), 0xc);
	REG_SET_VAL(&(reg->internel_clk_div1), 8);
}

void taurus_t2mi_clr_source(int id, void *vreg)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_VAL(&(reg->ts_i_m2ts_sel), 0);
}

struct demux_dvr_ops taurus_dvr_ops = {
	.set_tsr_avid        = sirius_set_tsr_avid,
	.clr_tsr_avid        = sirius_clr_tsr_avid,
	.set_dvr_mode        = gx3211_set_dvr_mode,
	.set_dvr_source      = taurus_set_dvr_source,
	.set_tsw_buf         = gx3211_set_tsw_buf,
	.clr_tsw_buf         = gx3211_clr_tsw_buf,
	.reset_tsw_buf       = gx3211_reset_tsw_buf,
	.set_tsw_enable      = gx3211_set_tsw_enable,
	.set_tsw_disable     = gx3211_set_tsw_disable,
	.set_tsr_buf         = sirius_set_tsr_buf,
	.clr_tsr_buf         = sirius_clr_tsr_buf,
	.reset_tsr_buf       = sirius_reset_tsr_buf,
	.set_tsw_slot_buf    = taurus_set_tsw_slot_buf,
	.clr_tsw_slot_buf    = taurus_clr_tsw_slot_buf,
	.reset_tsw_slot_buf  = taurus_reset_tsw_slot_buf,
	.bind_slot_tsw       = taurus_bind_slot_tsw,
	.pf_bind_slot_tsw    = taurus_pf_bind_slot_tsw,

	.tsw_isr             = gx3211_tsw_isr,
	.tsw_full_isr        = gx3211_tsw_full_isr,
	.tsw_isr_en          = gx3211_tsw_isr_en,
	.tsw_module_enable   = gx3211_tsw_module_enable,
	.tsw_module_disable  = gx3211_tsw_module_disable,
	.tsw_update_rptr     = gx3211_tsw_update_rptr,
	.tsr_isr             = taurus_tsr_isr,
	.tsr_dealwith        = taurus_tsr_dealwith,

	.t2mi_set_source     = taurus_t2mi_set_source,
	.t2mi_clr_source     = taurus_t2mi_clr_source,

#ifdef CONFIG_AV_MODULE_PIDFILTER
	.get_tsr_id          = sirius_dvr_get_tsr_id,
	.irq_entry           = taurus_dvr_irq_entry,
#endif
};

int taurus_dvr_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		dev = &gxdmxdev[i];
		dev->dvr_ops = &taurus_dvr_ops;
		dev->max_dvr = TAURUS_MAX_DVR_UNIT;
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_TSW))
			dev->max_tsw = (i == 0) ? MAX_SECURE_TSW : 0;
		else
			dev->max_tsw = TAURUS_MAX_SLOT_NUM;

		dev->dvr_mask = 0;
		dev->tsw_mask = 0;
		gx_memset(dev->dvr_handles, -1, sizeof(unsigned int)* MAX_SLOT_NUM);
	}

#ifdef CONFIG_AV_MODULE_PIDFILTER
	return taurus_pfilter_init();
#else
	return 0;
#endif
}

int taurus_dvr_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for (i=0; i<MAX_DEMUX_UNIT; i++){
		dev = &gxdmxdev[i];
		dev->dvr_ops = NULL;
		dev->max_dvr = 0;
	}

#ifdef CONFIG_AV_MODULE_PIDFILTER
	return taurus_pfilter_deinit();
#else
	return 0;
#endif
}

int taurus_dvr_open(struct gxav_module *module)
{
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(0);
	struct dmxdev    *dev   = demux->dev;

	if (module->sub < MAX_DMX_TSW_SLOT) {
		if (sub >= dev->max_dvr)
			return -1;
		return gx3211_dvr_open(module);

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

int taurus_dvr_close(struct gxav_module *module)
{
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(0);
	struct dmxdev    *dev   = demux->dev;

	if (module->sub < MAX_DMX_TSW_SLOT) {
		if (sub >= dev->max_dvr)
			return -1;
		return gx3211_dvr_close(module);

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

struct gxav_module_ops taurus_dvr_module = {
	.module_type  = GXAV_MOD_DVR,
	.count        = 16,
#ifdef CONFIG_AV_MODULE_PIDFILTER
	.irqs         = {TAURUS_PFILTER_IRQ, -1},
	.irq_names    = {"ts_pfilter"},
#else
	.irqs         = {-1},
#endif
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = taurus_dvr_open,
	.close        = taurus_dvr_close,
	.init         = taurus_dvr_init,
	.cleanup      = taurus_dvr_cleanup,
	.read         = dvr_read,
	.write        = dvr_write,
	.set_property = dvr_set_entry,
	.get_property = dvr_get_entry,
#ifdef CONFIG_AV_MODULE_PIDFILTER
	.interrupts[TAURUS_PFILTER_IRQ] = dvr_irq,
#endif
};

