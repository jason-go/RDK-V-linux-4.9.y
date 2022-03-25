#include "kernelcalls.h"
#include "gx3211_demux.h"
#include "sirius_demux.h"
#include "taurus_demux.h"
#include "gx6605s_demux.h"
#include "pegasus_regs.h"
#include "gxdemux.h"
#include "gxav_bitops.h"
#include "sdc_hal.h"
#include "profile.h"

void pegasus_pf_set_t2mi_out(void *vreg, unsigned int slotid, int flag)
{
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_PEGASUS_DEMUX_PID_CFG_T2MI_EN);
	if (flag)
		REG_SET_VAL(&(reg->tsw_pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_T2MI_EN);
	else
		REG_SET_VAL(&(reg->tsw_pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_PEGASUS_DEMUX_PID_CFG_T2MI_EN);
}

void pegasus_set_reverse_pid(void *vreg, unsigned int dmxid, unsigned short pid)
{
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_OUT_EN);
	if (dmxid < 2) {
		REG_SET_FIELD(&(reg->discard_pkt_cfg[2+dmxid]), PEGASUS_DEMUX_DISCARD_PID, pid, BIT_PEGASUS_DEMUX_DISCARD_PID);
		REG_SET_VAL(&(reg->pid_cfg_sel[31].pid_cfg), DEMUX_PID_CFG_TS_OUT_EN);
	} else {
		REG_SET_FIELD(&(reg->discard_pkt_cfg[dmxid%2]), PEGASUS_DEMUX_DISCARD_PID, pid, BIT_PEGASUS_DEMUX_DISCARD_PID);
		REG_SET_VAL(&(reg->tsw_pid_cfg_sel[31].pid_cfg), DEMUX_PID_CFG_TS_OUT_EN);
	}
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_OUT_EN);
}

void pegasus_set_t2mi_out(void *vreg, unsigned int slotid, int flag)
{
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_PEGASUS_DEMUX_PID_CFG_T2MI_EN);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_T2MI_EN);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_PEGASUS_DEMUX_PID_CFG_T2MI_EN);
}

void pegasus_set_ca_mode(void *vreg, int dmxid, int slotid, enum cas_ds_mode ds_mode)
{
	struct pegasus_reg_demux *reg = (struct pegasus_reg_demux*)vreg;

	if (ds_mode != DEMUX_DES_DVB_CSA2 &&
		ds_mode != DEMUX_DES_DVB_CSA2_CONFORMANCE &&
		ds_mode != DEMUX_DES_DES_ECB)
		return;

	REG_SET_BIT(&(reg->wen_mask), BIT_PEGASUS_DEMUX_PID_CFG_DES_SEL);
	if (ds_mode == DEMUX_DES_DES_ECB)
		REG_SET_BIT(&(reg->pid_cfg_sel[slotid].pid_cfg), BIT_PEGASUS_DEMUX_PID_CFG_DES_SEL);
	else {
		REG_CLR_BIT(&(reg->pid_cfg_sel[slotid].pid_cfg), BIT_PEGASUS_DEMUX_PID_CFG_DES_SEL);
		if (ds_mode == DEMUX_DES_DVB_CSA2_CONFORMANCE) {
			REG_SET_BIT(&(reg->cw_conformance[dmxid%2]), 0);
		} else {
			REG_CLR_BIT(&(reg->cw_conformance[dmxid%2]), 0);
		}
	}
	REG_CLR_BIT(&(reg->wen_mask), BIT_PEGASUS_DEMUX_PID_CFG_DES_SEL);

}

struct demux_ops pegasus_demux_ops = {

	.pf_disable_slot    = taurus_pf_disable,
	.pf_enable_slot     = taurus_pf_enable,
	.pf_set_pid         = taurus_pf_set_pid,
	.pf_clr_slot_cfg    = taurus_pf_clr_slot_cfg,
	.pf_set_ts_out      = taurus_pf_set_ts_out,
	.pf_set_err_discard = taurus_pf_set_err_discard,
	.pf_set_dup_discard = taurus_pf_set_dup_discard,
	.pf_set_t2mi_out    = pegasus_pf_set_t2mi_out,

	.disable_slot       = taurus_disable_slot,
	.enable_slot        = taurus_enable_slot,
	.reset_cc           = gx3211_reset_cc,
	.set_repeat_mode    = taurus_set_repeat_mode,
	.set_one_mode       = taurus_set_one_mode,
	.set_interrupt_unit = taurus_set_interrupt_unit,
	.set_crc_disable    = taurus_set_crc_disable,
	.set_pts_to_sdram   = taurus_set_pts_to_sdram,
	.set_pid            = taurus_set_pid,
	.set_ts_out         = taurus_set_ts_out,
	.set_av_out         = taurus_set_av_out,
	.set_des_out        = taurus_set_des_out,
	.set_err_discard    = taurus_set_err_discard,
	.set_dup_discard    = taurus_set_dup_discard,
	.set_err_mask       = taurus_set_err_mask,
	.set_pts_bypass     = taurus_set_pts_bypass,
	.set_pts_insert     = taurus_set_pts_insert,
	.set_avid           = taurus_set_avid,
	.set_filterid       = taurus_set_filterid,
	.clr_filterid       = taurus_clr_filterid,
	.set_psi_type       = taurus_set_psi_type,
	.set_pes_type       = taurus_set_pes_type,
	.set_av_type        = taurus_set_av_type,
	.clr_slot_cfg       = taurus_clr_slot_cfg,
	.set_reverse_pid    = pegasus_set_reverse_pid,

	.enable_filter      = gx3211_enable_filter,
	.disable_filter     = gx3211_disable_filter,
	.clr_dfrw           = gx3211_clr_dfrw,
	.set_dfbuf          = gx3211_set_dfbuf,
	.clr_int_df         = gx3211_clr_int_df,
	.clr_int_df_en      = gx3211_clr_int_df_en,
	.set_select         = gx3211_set_select,
	.set_sec_irqen      = gx3211_set_sec_irqen,
	.set_gate_irqen     = gx3211_set_gate_irqen,
	.set_match          = taurus_set_match,

	.clr_ints           = gx3211_clr_ints,
	.hw_init            = taurus_hw_init,
	.set_sync_gate      = gx3211_set_sync_gate,
	.set_input_source   = gx6605s_set_input_source,
	.set_pcr_sync       = gx3211_set_pcr_sync,
	.set_apts_sync      = gx3211_set_apts_sync,
	.query_tslock       = gx3211_query_tslock,

	.enable_av_int      = gx3211_enable_av_int,
	.disable_av_int     = gx3211_disable_av_int,
	.link_avbuf         = gx3211_link_avbuf,
	.unlink_avbuf       = gx3211_unlink_avbuf,
	.link_ptsbuf        = gx3211_link_ptsbuf,
	.unlink_ptsbuf      = gx3211_unlink_ptsbuf,
	.avbuf_cb           = gx3211_avbuf_callback,

	.set_cas_dsc_type   = taurus_set_dsc_type,
	.set_descid         = gx6605s_set_descid,
	.set_even_valid     = gx6605s_set_even_valid,
	.set_odd_valid      = gx6605s_set_odd_valid,
	.set_evenkey        = gx6605s_set_evenkey,
	.set_oddkey         = gx6605s_set_oddkey,
	.set_ca_mode        = pegasus_set_ca_mode,

	.df_isr             = gx3211_df_isr,
	.df_al_isr          = gx3211_df_al_isr,
	.av_gate_isr        = gx3211_av_gate_isr,
	.av_section_isr     = gx3211_av_section_isr,
	.ts_clk_err_isr     = gx3211_check_ts_clk_err_isr,

	.set_t2mi_out       = pegasus_set_t2mi_out,
//	.t2mi_config        = taurus_t2mi_config,
};

struct pegasus_reg_demux *pegasus_dmxreg;

int pegasus_demux_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int i;

	if (!gx_request_mem_region(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux))) {
		gxlog_e(LOG_DEMUX, "request DEMUX mem region failed!\n");
		return -1;
	}

	pegasus_dmxreg = gx_ioremap(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux));
	if (!pegasus_dmxreg) {
		gxlog_e(LOG_DEMUX, "ioremap DEMUX space failed!\n");
		return -1;
	}

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		struct dmxdev *dev = &gxdmxdev[i];
		struct dmx_demux *demux = NULL;
		gx_memset(dev, 0, sizeof(struct dmxdev));
		dev->id  = i;
		dev->reg = pegasus_dmxreg;

		dev->ops = &pegasus_demux_ops;
		dev->max_dmx_unit = TAURUS_MAX_DEMUX_UNIT;
		dev->max_filter   = TAURUS_MAX_FILTER_NUM;
		dev->max_slot     = TAURUS_MAX_SLOT_NUM;
		dev->max_av       = TAURUS_MAX_AVBUF_NUM;
		dev->max_cas      = MAX_CA_NUM;
		dev->max_tsw_slot = TAURUS_MAX_SLOT_NUM;
		dev->irqs[0] = DMX_IRQ0;
		demux = &(dev->demuxs[0]);
		demux->id = 0;
		demux->dev = dev;
		demux = &(dev->demuxs[1]);
		demux->id = 1;
		demux->dev = dev;
	}

	taurus_hw_init(pegasus_dmxreg);
	return demux_init(device,inode);
}

int pegasus_demux_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = &gxdmxdev[0];
	dev->ops = NULL;
	dev->max_filter = 0;
	dev->max_slot = 0;
	dev->max_cas = 0;
	dev->max_av = 0;
	dev->max_tsw_slot = 0;

	taurus_hw_init(pegasus_dmxreg);
	gx_iounmap(dev->reg);
	gx_release_mem_region(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux));

	return demux_cleanup(device,inode);
}

int pegasus_demux_open(struct gxav_module *module)
{
	if (module->sub == 2  || module->sub == 3  ||
		module->sub == 6  || module->sub == 7)
		return -1;
	return demux_open(module);
}

int pegasus_demux_close(struct gxav_module *module)
{
	if (module->sub == 2  || module->sub == 3  ||
		module->sub == 6  || module->sub == 7)
		return -1;
	return demux_close(module);
}

struct gxav_module_ops pegasus_demux_module = {
	.module_type  = GXAV_MOD_DEMUX,
	.count        = 16,
	.irqs         = {DMX_IRQ0, -1},
	.irq_names    = {"demux_incident0", },
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = pegasus_demux_open,
	.close        = pegasus_demux_close,
	.init         = pegasus_demux_init,
	.cleanup      = pegasus_demux_cleanup,
	.set_property = demux_set_entry,
	.get_property = demux_get_entry,
	.interrupts[DMX_IRQ0] = demux_irq,
};

