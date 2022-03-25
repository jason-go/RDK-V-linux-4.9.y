#include "kernelcalls.h"
#include "gx3211_regs.h"
#include "gx3211_demux.h"
#include "sirius_regs.h"
#include "sirius_demux.h"
#include "taurus_regs.h"
#include "taurus_demux.h"
#include "taurus_pfilter.h"
#include "cygnus_regs.h"
#include "cygnus_demux.h"
#include "sdc_hal.h"
#include "gxav_bitops.h"
#include "profile.h"

void cygnus_set_dvr_mode(int modid, void *vreg, int mode)
{
	struct cygnus_reg_demux *reg = (struct cygnus_reg_demux*)vreg;

	if (modid < MAX_DMX_NORMAL) {
		if (mode == DVR_RECORD_ALL)
			REG_SET_BIT(&(reg->ts_write_ctrl), modid%2+2);
		else
			REG_CLR_BIT(&(reg->ts_write_ctrl), modid%2+2);

		if (mode == DVR_RECORD_BY_REVERSE_SLOT)
			REG_SET_BIT(&(reg->discard_pkt_enable), (modid%2+2));
		else
			REG_CLR_BIT(&(reg->discard_pkt_enable), (modid%2+2));
	} else {
		if (mode == DVR_RECORD_ALL)
			REG_SET_BIT(&(reg->ts_write_ctrl_23), modid%2+2);
		else
			REG_CLR_BIT(&(reg->ts_write_ctrl_23), modid%2+2);

		if (mode == DVR_RECORD_BY_REVERSE_SLOT)
			REG_SET_BIT(&(reg->discard_pkt_enable), (modid%2));
		else
			REG_CLR_BIT(&(reg->discard_pkt_enable), (modid%2));
	}
}
//有疑问
void cygnus_set_dvr_source(int modid, int tswid, void *vreg, struct dvr_info *info)
{
	struct cygnus_reg_demux *reg = (struct cygnus_reg_demux *)vreg;
	unsigned int temp, source;
	unsigned int tsw_src = info->source;
	unsigned int dmxid   = (modid%4)%2; //dmxid为什么只取0, 1
	struct dmx_demux *demux = get_subdemux(dmxid);

	if (tsw_src == DVR_INPUT_TSPORT)
		source = demux->source;
	else {
		if (modid < MAX_DMX_NORMAL) {
			if (tsw_src == DVR_INPUT_MEM) {
				if (modid % 2)
					source = CYGNUS_TSW_SRC_TSR1;
				else
					source = CYGNUS_TSW_SRC_TSR0;
			} else {
				source = CYGNUS_TSW_SRC_DMXSLOT;
				REG_SET_VAL(&(reg->ts_des_flag_clean_enable), 1);
			}
		} else {
			if (tsw_src == DVR_INPUT_MEM) {
				if (modid % 2)
					source = CYGNUS_TSW_SRC_TSR3;
				else
					source = CYGNUS_TSW_SRC_TSR2;
			} else
				source = CYGNUS_TSW_SRC_DMX_FOR_TSWSLOT;
		}
	}

	if (modid < MAX_DMX_NORMAL) {
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
	} else {
		temp = REG_GET_VAL(&(reg->ts_write_ctrl_23));
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
		REG_SET_VAL(&(reg->ts_write_ctrl_23), temp);
	}
}

void cygnus_set_tsw_buf(int dmxid, int tswid, void *vreg,
		unsigned int start_addr, unsigned int size, unsigned int gate)
{
	struct cygnus_reg_demux *reg = (struct cygnus_reg_demux *)vreg;

	if (dmxid == 0 || dmxid == 1) {
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_buf_start_addr), start_addr);
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_buf_len),size);
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_write_addr),0);
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_read_addr),0);
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_almost_full_gate),gate);
	} else if (dmxid == 4 || dmxid == 5) {
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_buf_start_addr), start_addr);
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_buf_len),size);
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_write_addr),0);
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_read_addr),0);
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_almost_full_gate),gate);
	}
}

void cygnus_clr_tsw_buf(int dmxid, int tswid, void *vreg)
{
	struct cygnus_reg_demux *reg = (struct cygnus_reg_demux *)vreg;

	if (dmxid == 0 || dmxid == 1) {
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_buf_start_addr), 0);
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_buf_len),0);
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_write_addr),0);
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_read_addr),0);
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_almost_full_gate),0);
	} else if (dmxid == 4 || dmxid == 5) {
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_buf_start_addr), 0);
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_buf_len),0);
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_write_addr),0);
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_read_addr),0);
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_almost_full_gate),0);
	}
}


void cygnus_set_tsw_enable(int dmxid, int tswid, void *vreg)
{
	int bit;
	struct cygnus_reg_demux *reg = (struct cygnus_reg_demux *)vreg;
	if (dmxid == 0)
		REG_SET_BIT(&(reg->tsw_buf_en_0_l),tswid);
	else if (dmxid == 1)
		REG_SET_BIT(&(reg->tsw_buf_en_1_l),tswid);
	else if (dmxid == 4)
		REG_SET_BIT(&(reg->tsw_buf_en_2_l),tswid);
	else if (dmxid == 5)
		REG_SET_BIT(&(reg->tsw_buf_en_3_l),tswid);

	if (dmxid == 0 || dmxid == 1) {
		bit = tswid;
		REG_SET_VAL(&(reg->int_tsw_buf_alful_l),(0x1<<bit));
		REG_SET_BIT(&(reg->int_tsw_buf_alful_en_l),bit);
		REG_SET_VAL(&(reg->int_tsw_buf_ful_l),(0x1<<bit));
		REG_SET_BIT(&(reg->int_tsw_buf_ful_en_l),bit);
	}else if (dmxid == 4 || dmxid == 5) {
		bit = tswid + 16;
		REG_SET_VAL(&(reg->int_tsw_buf_alful_l),(0x1<<bit));
		REG_SET_BIT(&(reg->int_tsw_buf_alful_en_l),bit);
		REG_SET_VAL(&(reg->int_tsw_buf_ful_l),(0x1<<bit));
		REG_SET_BIT(&(reg->int_tsw_buf_ful_en_l),bit);
	}
}


void cygnus_set_tsw_disable(int dmxid, int tswid, void *vreg)
{
	int bit;
	struct cygnus_reg_demux *reg = (struct cygnus_reg_demux *)vreg;

	if (dmxid == 0)
		REG_CLR_BIT(&(reg->tsw_buf_en_0_l),tswid);
	else if (dmxid == 1)
		REG_CLR_BIT(&(reg->tsw_buf_en_1_l),tswid);
	else if (dmxid == 4)
		REG_CLR_BIT(&(reg->tsw_buf_en_2_l),tswid);
	else if (dmxid == 5)
		REG_CLR_BIT(&(reg->tsw_buf_en_3_l),tswid);

	if (dmxid == 0 || dmxid == 1) {
		bit = tswid;
		REG_SET_VAL(&(reg->int_tsw_buf_alful_l),(0x1<<bit));
		REG_CLR_BIT(&(reg->int_tsw_buf_alful_en_l),bit);
		REG_SET_VAL(&(reg->int_tsw_buf_ful_l),(0x1<<bit));
		REG_CLR_BIT(&(reg->int_tsw_buf_ful_en_l),bit);
	}else if (dmxid == 4 || dmxid == 5) {
		bit = tswid + 16;
		REG_SET_VAL(&(reg->int_tsw_buf_alful_l),(0x1<<bit));
		REG_CLR_BIT(&(reg->int_tsw_buf_alful_en_l),bit);
		REG_SET_VAL(&(reg->int_tsw_buf_ful_l),(0x1<<bit));
		REG_CLR_BIT(&(reg->int_tsw_buf_ful_en_l),bit);
	}
}

void cygnus_tsw_module_enable(int dmxid, void *vreg)
{
	struct cygnus_reg_demux *reg = (struct cygnus_reg_demux *)vreg;
	if (dmxid == 0 || dmxid == 1) {
		REG_SET_BIT(&(reg->ts_write_ctrl), dmxid%2);
	} else if (dmxid == 4 || dmxid == 5) {
		REG_SET_BIT(&(reg->ts_write_ctrl_23), dmxid%2);
	}
}

void cygnus_tsw_module_disable(int dmxid, void *vreg)
{
	struct cygnus_reg_demux *reg = (struct cygnus_reg_demux *)vreg;
	if (dmxid == 0 || dmxid == 1) {
		REG_CLR_BIT(&(reg->ts_write_ctrl), dmxid%2);
	} else if (dmxid == 4 || dmxid == 5) {
		REG_CLR_BIT(&(reg->ts_write_ctrl_23), dmxid%2);
	}
}

void cygnus_reset_tsw_buf(int dmxid, int tswid, void *vreg)
{
	struct cygnus_reg_demux *reg = (struct cygnus_reg_demux*)vreg;

	if (dmxid == 0 || dmxid == 1) {
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_write_addr),0);
		REG_SET_VAL(&(reg->tsw01_buf[tswid].tsw_read_addr),0);
	} else if (dmxid == 4 || dmxid == 5) {
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_write_addr),0);
		REG_SET_VAL(&(reg->tsw23_buf[tswid].tsw_read_addr),0);
	}
}

struct demux_dvr_ops cygnus_dvr_ops = {
	.set_tsr_avid        = sirius_set_tsr_avid,
	.clr_tsr_avid        = sirius_clr_tsr_avid,
	.set_dvr_mode        = cygnus_set_dvr_mode,
	.set_dvr_source      = cygnus_set_dvr_source,
	.set_tsw_buf         = cygnus_set_tsw_buf,
	.clr_tsw_buf         = cygnus_clr_tsw_buf,
	.reset_tsw_buf       = cygnus_reset_tsw_buf,
	.set_tsw_enable      = cygnus_set_tsw_enable,
	.set_tsw_disable     = cygnus_set_tsw_disable,
	.set_tsr_buf         = sirius_set_tsr_buf,
	.clr_tsr_buf         = sirius_clr_tsr_buf,
	.reset_tsr_buf       = sirius_reset_tsr_buf,
	.set_tsw_slot_buf    = taurus_set_tsw_slot_buf,
	.clr_tsw_slot_buf    = taurus_clr_tsw_slot_buf,
	.reset_tsw_slot_buf  = taurus_reset_tsw_slot_buf,
	.bind_slot_tsw       = taurus_bind_slot_tsw,
	.pf_bind_slot_tsw    = taurus_pf_bind_slot_tsw,

	.tsw_isr             = cygnus_tsw_isr,
	.tsw_full_isr        = cygnus_tsw_full_isr,
	.tsw_isr_en          = cygnus_tsw_isr_en,
	.tsw_module_enable   = cygnus_tsw_module_enable,
	.tsw_module_disable  = cygnus_tsw_module_disable,
	.tsw_update_rptr     = cygnus_tsw_update_rptr,
	.tsr_isr             = taurus_tsr_isr,
	.tsr_dealwith        = taurus_tsr_dealwith,

#ifdef CONFIG_AV_MODULE_PIDFILTER
	.get_tsr_id          = sirius_dvr_get_tsr_id,
	.irq_entry           = taurus_dvr_irq_entry,
#endif
};

int cygnus_dvr_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		dev = &gxdmxdev[i];
		dev->dvr_ops = &cygnus_dvr_ops;
		dev->max_dvr = TAURUS_MAX_DVR_UNIT;
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_TSW))
			dev->max_tsw = (i == 0) ? MAX_SECURE_TSW : 0;
		else
			dev->max_tsw = CYGNUS_MAX_TSW_NUM;

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

int cygnus_dvr_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
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

int cygnus_dvr_open(struct gxav_module *module)
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
	}

	return 0;
}

int cygnus_dvr_close(struct gxav_module *module)
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

	}

	return 0;
}

struct gxav_module_ops cygnus_dvr_module = {
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
	.open         = cygnus_dvr_open,
	.close        = cygnus_dvr_close,
	.init         = cygnus_dvr_init,
	.cleanup      = cygnus_dvr_cleanup,
	.read         = dvr_read,
	.write        = dvr_write,
	.set_property = dvr_set_entry,
	.get_property = dvr_get_entry,
#ifdef CONFIG_AV_MODULE_PIDFILTER
	.interrupts[TAURUS_PFILTER_IRQ] = dvr_irq,
#endif
};

