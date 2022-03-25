#include "kernelcalls.h"
#include "gx3211_regs.h"
#include "sirius_regs.h"
#include "taurus_regs.h"
#include "pegasus_regs.h"
#include "cygnus_regs.h"
#include "cygnus_demux.h"
#include "gx3211_demux.h"
#include "sirius_demux.h"
#include "taurus_demux.h"
#include "gxdemux.h"
#include "gxav_bitops.h"
#include "sdc_hal.h"
#include "profile.h"

void cygnus_set_reverse_pid(void *vreg, unsigned int dmxid, unsigned short pid)
{
	struct cygnus_reg_demux *reg = (struct cygnus_reg_demux*)vreg;
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

struct demux_ops cygnus_demux_ops = {

	.pf_disable_slot    = taurus_pf_disable,
	.pf_enable_slot     = taurus_pf_enable,
	.pf_set_pid         = taurus_pf_set_pid,
	.pf_clr_slot_cfg    = taurus_pf_clr_slot_cfg,
	.pf_set_ts_out      = taurus_pf_set_ts_out,
	.pf_set_err_discard = taurus_pf_set_err_discard,
	.pf_set_dup_discard = taurus_pf_set_dup_discard,

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
	.set_cas_descid     = taurus_set_cas_descid,
	.set_avid           = taurus_set_avid,
	.set_filterid       = taurus_set_filterid,
	.clr_filterid       = taurus_clr_filterid,
	.set_cas_key_valid  = taurus_set_key_valid,
	.set_cas_dsc_type   = taurus_set_dsc_type,
	.set_psi_type       = taurus_set_psi_type,
	.set_pes_type       = taurus_set_pes_type,
	.set_av_type        = taurus_set_av_type,
	.clr_slot_cfg       = taurus_clr_slot_cfg,
	.set_reverse_pid    = cygnus_set_reverse_pid,

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
	.set_input_source   = taurus_set_input_source,
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

	.set_cas_evenkey    = sirius_set_cas_evenkey,
	.set_cas_oddkey     = sirius_set_cas_oddkey,
	.set_cas_mode       = taurus_set_cas_mode,
	.set_cas_syskey     = taurus_set_cas_syskey,

	.df_isr             = gx3211_df_isr,
	.df_al_isr          = gx3211_df_al_isr,
	.av_gate_isr        = gx3211_av_gate_isr,
	.av_section_isr     = gx3211_av_section_isr,
	.ts_clk_err_isr     = gx3211_check_ts_clk_err_isr,

	.log_d_source       = gx3211_log_d_source,
};

struct cygnus_reg_demux *cygnus_dmxreg;

int cygnus_demux_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int i;
	void *cas_reg, *t2mi_reg;
	if (!gx_request_mem_region(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux))) {
		gxlog_e(LOG_DEMUX, "request DEMUX mem region failed!\n");
		return -1;
	}

	cygnus_dmxreg = gx_ioremap(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux));
	if (!cygnus_dmxreg) {
		gxlog_e(LOG_DEMUX, "ioremap DEMUX space failed!\n");
		return -1;
	}

	if (!gx_request_mem_region(TAURUS_DEMUX_BASE_CAS, sizeof(struct sirius_reg_cas))) {
		gxlog_e(LOG_DEMUX, "request DEMUX mem region failed!\n");
		return -1;
	}

	cas_reg = gx_ioremap(TAURUS_DEMUX_BASE_CAS, sizeof(struct sirius_reg_cas));
	if (!cas_reg) {
		gxlog_e(LOG_DEMUX, "ioremap DEMUX space failed!\n");
		return -1;
	}

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		struct dmxdev *dev = &gxdmxdev[i];
		struct dmx_demux *demux = NULL;
		gx_memset(dev, 0, sizeof(struct dmxdev));
		dev->id  = i;
		dev->reg = cygnus_dmxreg;
		dev->cas_reg = cas_reg;

		dev->ops = &cygnus_demux_ops;
		dev->max_dmx_unit = TAURUS_MAX_DEMUX_UNIT;
		dev->max_filter   = TAURUS_MAX_FILTER_NUM;
		dev->max_slot     = TAURUS_MAX_SLOT_NUM;
		dev->max_av       = TAURUS_MAX_AVBUF_NUM;
		dev->max_cas      = MAX_CAS_NUM;
		dev->max_tsw_slot = TAURUS_MAX_SLOT_NUM;
		dev->irqs[0] = DMX_IRQ0;
		demux = &(dev->demuxs[0]);
		demux->id = 0;
		demux->dev = dev;
		demux = &(dev->demuxs[1]);
		demux->id = 1;
		demux->dev = dev;
	}

	taurus_hw_init(cygnus_dmxreg);
	return demux_init(device,inode);
}

int cygnus_demux_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = &gxdmxdev[0];
	dev->ops = NULL;
	dev->max_filter = 0;
	dev->max_slot = 0;
	dev->max_cas = 0;
	dev->max_av = 0;
	dev->max_tsw_slot = 0;

	taurus_hw_init(cygnus_dmxreg);
	gx_iounmap(dev->reg);
	gx_release_mem_region(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux));
	gx_iounmap(dev->cas_reg);
	gx_release_mem_region(TAURUS_DEMUX_BASE_CAS, sizeof(struct sirius_reg_cas));

	return demux_cleanup(device,inode);
}

int cygnus_demux_open(struct gxav_module *module)
{
	if (module->sub == 2  || module->sub == 3  ||
		module->sub == 6  || module->sub == 7  ||
		module->sub == 9  || module->sub == 10 || module->sub == 11 ||
		module->sub == 13 || module->sub == 14 || module->sub == 15)
		return -1;
	return demux_open(module);
}

int cygnus_demux_close(struct gxav_module *module)
{
	if (module->sub == 2  || module->sub == 3  ||
		module->sub == 6  || module->sub == 7  ||
		module->sub == 9  || module->sub == 10 || module->sub == 11 ||
		module->sub == 13 || module->sub == 14 || module->sub == 15)
		return -1;
	return demux_close(module);
}

struct gxav_module_ops cygnus_demux_module = {
	.module_type  = GXAV_MOD_DEMUX,
	.count        = 16,
	.irqs         = {DMX_IRQ0, -1},
	.irq_names    = {"demux_incident0"},
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = cygnus_demux_open,
	.close        = cygnus_demux_close,
	.init         = cygnus_demux_init,
	.cleanup      = cygnus_demux_cleanup,
	.set_property = demux_set_entry,
	.get_property = demux_get_entry,
	.interrupts[DMX_IRQ0] = demux_irq,
};
