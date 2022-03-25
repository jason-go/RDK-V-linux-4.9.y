#include "kernelcalls.h"
#include "gx3211_regs.h"
#include "sirius_regs.h"
#include "taurus_regs.h"
#include "gx3211_demux.h"
#include "sirius_demux.h"
#include "taurus_demux.h"
#include "gxdemux.h"
#include "gxav_bitops.h"
#include "sdc_hal.h"
#include "profile.h"

void gx6605s_set_descid(void *vreg, unsigned int slotid,unsigned int caid, unsigned int flags)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BITS(&(reg->wen_mask), DEMUX_PID_CFG_KEY_ID);
	if (flags & DMX_CA_DES_MODE)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), caid << BIT_DEMUX_PID_CFG_KEY_ID);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), (caid*2+1) << BIT_DEMUX_PID_CFG_KEY_ID);
	REG_CLR_BITS(&(reg->wen_mask), DEMUX_PID_CFG_KEY_ID);
}

void gx6605s_set_even_valid(void *vreg, unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
}

void gx6605s_set_odd_valid(void *vreg, unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
}

void gx6605s_set_oddkey(void *vreg,unsigned int caid,unsigned int key_high,unsigned int key_low, unsigned int flags)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_VAL(&(reg->wen_mask), 0xFFFFFFFF);
	if (key_high !=0 ||key_low !=0) {
		if (flags & DMX_CA_DES_MODE) {
			if (flags & DMX_CA_PES_LEVEL) {
				REG_SET_VAL(&(reg->key_even_odd[caid*2].key_even_high), key_high);
				REG_SET_VAL(&(reg->key_even_odd[caid*2].key_even_low), key_low);
			} else {
				REG_SET_VAL(&(reg->key_even_odd[caid*2].key_odd_high), key_high);
				REG_SET_VAL(&(reg->key_even_odd[caid*2].key_odd_low), key_low);
			}
		} else {
			REG_SET_VAL((&(reg->key_even_odd[caid*2].key_even_high)), key_high);
			REG_SET_VAL((&(reg->key_even_odd[caid*2].key_even_low)), key_low);
		}
	}
	REG_SET_VAL(&(reg->wen_mask), 0x00000000);
}

void gx6605s_set_evenkey(void *vreg,unsigned int caid,unsigned int key_high,unsigned int key_low, unsigned int flags)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_VAL(&(reg->wen_mask), 0xFFFFFFFF);
	if (key_high !=0 ||key_low !=0) {
		if (flags & DMX_CA_DES_MODE) {
			REG_SET_VAL(&(reg->key_even_odd[caid*2+1].key_odd_high), key_high);
			REG_SET_VAL(&(reg->key_even_odd[caid*2+1].key_odd_low), key_low);
		} else {
			if (flags & DMX_CA_PES_LEVEL) {
				REG_SET_VAL(&(reg->key_even_odd[caid*2+1].key_odd_high), key_high);
				REG_SET_VAL(&(reg->key_even_odd[caid*2+1].key_odd_low), key_low);
				REG_SET_VAL(&(reg->key_even_odd[caid*2+1].key_even_high), key_high);
				REG_SET_VAL(&(reg->key_even_odd[caid*2+1].key_even_low), key_low);
			} else {
				REG_SET_VAL(&(reg->key_even_odd[caid*2+1].key_even_high), key_high);
				REG_SET_VAL(&(reg->key_even_odd[caid*2+1].key_even_low), key_low);
			}

		}
	}
	REG_SET_VAL(&(reg->wen_mask), 0x00000000);
}

void gx6605s_set_input_source(void *vreg, unsigned int dmxid,unsigned int source)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	struct dmx_demux *demux = get_subdemux(dmxid);
	unsigned int temp;

	if (dmxid == 0 || dmxid == 1) {
		temp = REG_GET_VAL(&(reg->dmux_cfg));
		if(dmxid == 0){
			temp = ((temp & ~(DEMUX_CFG_TS_SEL1)) | (source<<BIT_DEMUX_CFG_TS_SEL_1));
		}else if (dmxid == 1){
			temp = ((temp & ~(DEMUX_CFG_TS_SEL2)) | (source<<BIT_DEMUX_CFG_TS_SEL_2));
		}
		REG_SET_VAL(&(reg->dmux_cfg), temp);
	} else {
		temp = REG_GET_VAL(&(reg->ts_write_ctrl));
		if (dmxid == 4){
			temp &= ~(0x03 << 4);
			temp |= (source << 4);
		}else if (dmxid == 5){
			temp &= ~(0x03 << 6);
			temp |= (source << 6);
		}
		REG_SET_VAL(&(reg->ts_write_ctrl), temp);
	}


	gxlog_d(LOG_DEMUX, "Select demux(%d)'s source = %d\n", dmxid, source);
}

struct demux_ops gx6605s_demux_ops = {

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
	.set_avid           = taurus_set_avid,
	.set_filterid       = taurus_set_filterid,
	.clr_filterid       = taurus_clr_filterid,
	.set_cas_key_valid  = taurus_set_key_valid,
	.set_cas_dsc_type   = taurus_set_dsc_type,
	.set_psi_type       = taurus_set_psi_type,
	.set_pes_type       = taurus_set_pes_type,
	.set_av_type        = taurus_set_av_type,
	.clr_slot_cfg       = taurus_clr_slot_cfg,

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

	.set_descid         = gx6605s_set_descid,
	.set_even_valid     = gx6605s_set_even_valid,
	.set_odd_valid      = gx6605s_set_odd_valid,
	.set_evenkey        = gx6605s_set_evenkey,
	.set_oddkey         = gx6605s_set_oddkey,

	.df_isr             = gx3211_df_isr,
	.df_al_isr          = gx3211_df_al_isr,
	.av_gate_isr        = gx3211_av_gate_isr,
	.av_section_isr     = gx3211_av_section_isr,
	.ts_clk_err_isr     = gx3211_check_ts_clk_err_isr,

	.log_d_source       = gx3211_log_d_source,
};

struct taurus_reg_demux *gx6605s_dmxreg;

int gx6605s_demux_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int i;
	if (!gx_request_mem_region(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux))) {
		gxlog_e(LOG_DEMUX, "request DEMUX mem region failed!\n");
		return -1;
	}

	gx6605s_dmxreg = gx_ioremap(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux));
	if (!gx6605s_dmxreg) {
		gxlog_e(LOG_DEMUX, "ioremap DEMUX space failed!\n");
		return -1;
	}

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		struct dmxdev *dev = &gxdmxdev[i];
		struct dmx_demux *demux = NULL;
		gx_memset(dev, 0, sizeof(struct dmxdev));
		dev->id  = i;
		dev->reg = gx6605s_dmxreg;

		dev->ops = &gx6605s_demux_ops;
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

	taurus_hw_init(gx6605s_dmxreg);
	return demux_init(device,inode);
}

int gx6605s_demux_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = &gxdmxdev[0];
	dev->ops = NULL;
	dev->max_filter = 0;
	dev->max_slot = 0;
	dev->max_cas = 0;
	dev->max_av = 0;
	dev->max_tsw_slot = 0;

	taurus_hw_init(gx6605s_dmxreg);
	gx_iounmap(dev->reg);
	gx_release_mem_region(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux));

	return demux_cleanup(device,inode);
}

int gx6605s_demux_open(struct gxav_module *module)
{
	if (module->sub == 2  || module->sub == 3  ||
		module->sub == 6  || module->sub == 7)
		return -1;
	return demux_open(module);
}

int gx6605s_demux_close(struct gxav_module *module)
{
	if (module->sub == 2  || module->sub == 3  ||
		module->sub == 6  || module->sub == 7)
		return -1;
	return demux_close(module);
}

struct gxav_module_ops gx6605s_demux_module = {
	.module_type  = GXAV_MOD_DEMUX,
	.count        = 16,
	.irqs         = {DMX_IRQ0, -1},
	.irq_names    = {"demux_incident0", },
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = gx6605s_demux_open,
	.close        = gx6605s_demux_close,
	.init         = gx6605s_demux_init,
	.cleanup      = gx6605s_demux_cleanup,
	.set_property = demux_set_entry,
	.get_property = demux_get_entry,
	.interrupts[DMX_IRQ0] = demux_irq,
};

