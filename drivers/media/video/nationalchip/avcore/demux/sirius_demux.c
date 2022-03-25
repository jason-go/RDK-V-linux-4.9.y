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

void sirius_set_dsc_type(void *vreg, unsigned int slotid, unsigned int dsc_type)
{
	struct sirius_reg_demux *reg = (struct sirius_reg_demux *)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DSC_TYPE);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), dsc_type<<BIT_DEMUX_PID_CFG_DSC_TYPE);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DSC_TYPE);
}

void sirius_set_key_valid(void *vreg, unsigned int slotid)
{
	struct sirius_reg_demux *reg = (struct sirius_reg_demux *)vreg;

	REG_SET_BIT(&(reg->wen_mask), SIRIUS_BIT_PID_CFG_KEY_VALID);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0x1 << SIRIUS_BIT_PID_CFG_KEY_VALID);
	REG_CLR_BIT(&(reg->wen_mask), SIRIUS_BIT_PID_CFG_KEY_VALID);
}

void sirius_set_descid(void *vreg, unsigned int slotid, unsigned int caid)
{
	struct sirius_reg_demux *reg = (struct sirius_reg_demux *)vreg;

	REG_SET_BITS(&(reg->wen_mask), SIRIUS_PID_CFG_KEY_ID);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), caid << SIRIUS_BIT_PID_CFG_KEY_ID);
	REG_CLR_BITS(&(reg->wen_mask), SIRIUS_PID_CFG_KEY_ID);
}

void _set_cas_key(void *vreg, struct dmx_cas *cas, unsigned int *cw, unsigned int *iv, int even)
{
	int cas_id  = cas->id;
	int cw_addr = cas_id*2 + (even?0:1);
	int iv_addr = cas_id*2 + (even?0x40:0x41);
	struct sirius_reg_cas *reg = (struct sirius_reg_cas*)vreg;

	if (cas->ds_mode == DEMUX_DES_AES128_CBC ||
		cas->ds_mode == DEMUX_DES_SM4_CBC)
		DEMUX_SET_CW_IV(reg, iv, iv_addr);

	if (cas->ds_mode == DEMUX_DES_MULTI2) {
		int id = (cas_id >= 8) ? cas_id-8 : cas_id;
		DEMUX_SET_CW_IV(reg, iv, iv_addr);
		REG_SET_FIELD(&(reg->sys_sel[cas_id/8]), TAURUS_MSK_SYS_SEL((id*2)),
				cas->sys_sel_even, TAURUS_BIT_SYS_SEL((id*2)));
		REG_SET_FIELD(&(reg->sys_sel[cas_id/8]), TAURUS_MSK_SYS_SEL((id*2+1)),
				cas->sys_sel_odd, TAURUS_BIT_SYS_SEL((id*2+1)));
	}

	if (cas->flags & DMX_CAS_CW_FROM_CPU)
		DEMUX_SET_CW_IV(reg, cw, cw_addr);
	else
		REG_SET_VAL(&(reg->cw_wctrl), (((cw_addr)<<BIT_DEMUX_CW_WADDR) | 1));
}

void sirius_set_cas_oddkey(void *vreg, struct dmx_cas *cas)
{
	_set_cas_key(vreg, cas, cas->cw_odd, cas->iv_odd, 0);
}

void sirius_set_cas_evenkey(void *vreg, struct dmx_cas *cas)
{
	_set_cas_key(vreg, cas, cas->cw_even, cas->iv_even, 1);
}

int sirius_set_cas_mode(void *vreg, struct dmx_cas *cas)
{
	int cas_id = cas->id;
	struct sirius_reg_cas *reg = (struct sirius_reg_cas*)vreg;

	REG_SET_FIELD(&(reg->ds_mode[cas_id/8]), 0xf<<((cas_id%8)*4), cas->ds_mode, ((cas_id%8)*4));
	return 0;
}

int sirius_cas_enable(void *vreg, void *cas_reg)
{
//	struct sirius_reg_demux *reg = (struct sirius_reg_demux*)vreg;
	struct sirius_reg_cas *creg = (struct sirius_reg_cas*)cas_reg;
	REG_SET_BIT(&(creg->cas_mode), 0);

// 寄存器说明:
// 这个时钟分频寄存器设置会影响解复用和解扰器性能，默认值为8，
// 在默认情况下解复用和解扰器支持的最高瞬时码率均为120Mb，
// 这个值越大解复用支持的最高瞬时码率越低，解扰器支持的最高瞬时码率越高。
// 比如修改为16后，解复用仅支持60~70Mb，解扰器则支持170~180Mb
//
//	REG_SET_VAL(&(reg->cas_div), 8);
	return 0;
}

void sirius_set_ca_mode(void *vreg, int dmxid, int slot_id, enum cas_ds_mode ds_mode)
{
	int mode;
	struct sirius_reg_demux *reg = (struct sirius_reg_demux*)vreg;

	if (ds_mode < DEMUX_DES_DVB_CSA3)
		mode = ds_mode;
	else if (ds_mode == DEMUX_DES_DES_ECB)
		mode = 2;
	else
		return;

	REG_SET_VAL(&(reg->wen_mask), DEMUX_CA_MODE);
	REG_SET_FIELD(&(reg->pid_cfg_sel[slot_id].pid_cfg), DEMUX_CA_MODE, mode, BIT_DEMUX_CA_MODE);
	REG_SET_VAL(&(reg->wen_mask), 0);
}

struct demux_ops sirius_demux_ops = {
	.disable_slot       = gx3211_disable_slot,
	.enable_slot        = gx3211_enable_slot,
	.reset_cc           = gx3211_reset_cc,
	.set_repeat_mode    = gx3211_set_repeat_mode,
	.set_one_mode       = gx3211_set_one_mode,
	.set_interrupt_unit = gx3211_set_interrupt_unit,
	.set_crc_disable    = gx3211_set_crc_disable,
	.set_pts_to_sdram   = gx3211_set_pts_to_sdram,
	.set_pid            = gx3211_set_pid,
	.set_av_out         = gx3211_set_av_out,
	.set_des_out        = gx3211_set_des_out,
	.set_err_discard    = gx3211_set_err_discard,
	.set_dup_discard    = gx3211_set_dup_discard,
	.set_ts_out         = gx3211_set_ts_out,
	.set_pts_bypass     = gx3211_set_pts_bypass,
	.set_pts_insert     = gx3211_set_pts_insert,
	.set_descid         = gx3211_set_descid,
	.set_cas_descid     = sirius_set_descid,
	.set_ca_mode        = sirius_set_ca_mode,
	.set_avid           = gx3211_set_avid,
	.set_filterid       = gx3211_set_filterid,
	.clr_filterid       = gx3211_clr_filterid,
	.set_odd_valid      = gx3211_set_odd_valid,
	.set_even_valid     = gx3211_set_even_valid,
	.set_cas_key_valid  = sirius_set_key_valid,
	.set_cas_dsc_type   = sirius_set_dsc_type,
	.set_psi_type       = gx3211_set_psi_type,
	.set_pes_type       = gx3211_set_pes_type,
	.set_av_type        = gx3211_set_av_type,
	.clr_slot_cfg       = gx3211_clr_slot_cfg,

	.enable_filter      = gx3211_enable_filter,
	.disable_filter     = gx3211_disable_filter,
	.clr_dfrw           = gx3211_clr_dfrw,
	.set_dfbuf          = gx3211_set_dfbuf,
	.clr_int_df         = gx3211_clr_int_df,
	.clr_int_df_en      = gx3211_clr_int_df_en,
	.set_select         = gx3211_set_select,
	.set_sec_irqen      = gx3211_set_sec_irqen,
	.set_gate_irqen     = gx3211_set_gate_irqen,
	.set_match          = gx3211_set_match,

	.clr_ints           = gx3211_clr_ints,
	.hw_init            = gx3211_hw_init,
	.set_sync_gate      = gx3211_set_sync_gate,
	.set_input_source   = gx3211_set_input_source,
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

	.set_evenkey        = gx3211_set_evenkey,
	.set_oddkey         = gx3211_set_oddkey,

	.set_cas_evenkey    = sirius_set_cas_evenkey,
	.set_cas_oddkey     = sirius_set_cas_oddkey,
	.set_cas_mode       = sirius_set_cas_mode,
	.cas_enable         = sirius_cas_enable,

	.df_isr             = gx3211_df_isr,
	.df_al_isr          = gx3211_df_al_isr,
	.av_gate_isr        = gx3211_av_gate_isr,
	.av_section_isr     = gx3211_av_section_isr,
	.ts_clk_err_isr     = gx3211_check_ts_clk_err_isr,

	.log_d_source       = gx3211_log_d_source,
};

struct sirius_reg_demux *sirius_dmxreg[MAX_DEMUX_UNIT];
struct sirius_reg_cas   *cas_reg;

int sirius_demux_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;
	unsigned int addr[MAX_DEMUX_UNIT];

	addr[0] = SIRIUS_DEMUX_BASE_SYS;
	addr[1] = SIRIUS_DEMUX_BASE_SYS1;

	if (!gx_request_mem_region(SIRIUS_DEMUX_BASE_CAS, sizeof(struct sirius_reg_cas))) {
		gxlog_e(LOG_DEMUX, "request DEMUX mem region failed!\n");
		return -1;
	}

	cas_reg = gx_ioremap(SIRIUS_DEMUX_BASE_CAS, sizeof(struct sirius_reg_cas));
	if (!cas_reg) {
	        gxlog_e(LOG_DEMUX, "ioremap DEMUX space failed!\n");
	        return -1;
	}

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		dev = &gxdmxdev[i];
		gx_memset(dev, 0, sizeof(struct dmxdev));
		dev->id = i;
		dev->cas_reg = cas_reg;
		if (!gx_request_mem_region(addr[dev->id], sizeof(struct gx3211_reg_demux))) {
			gxlog_e(LOG_DEMUX, "request DEMUX mem region failed!\n");
			return -1;
		}

		sirius_dmxreg[dev->id] = gx_ioremap(addr[dev->id], sizeof(struct gx3211_reg_demux));
		if (!sirius_dmxreg[dev->id]) {
			gxlog_e(LOG_DEMUX, "ioremap DEMUX space failed!\n");
			return -1;
		}

		dev->reg = sirius_dmxreg[dev->id];
		dev->ops = &sirius_demux_ops;
		dev->max_dmx_unit = MAX_DEMUX_UNIT;
		dev->max_filter = MAX_FILTER_NUM;
		dev->max_slot = MAX_SLOT_NUM;
		if (i == 0)
			dev->max_cas = MAX_CAS_NUM;
		else
			dev->max_cas = MAX_CA_NUM;
		dev->max_av = MAX_AVBUF_NUM;
		dev->dmx_mask = 0;

		gx3211_hw_init(dev->reg);
	}
	REG_SET_BIT(&sirius_dmxreg[0]->stream_out_ctrl, 4);
	if (dev->ops->cas_enable)
		dev->ops->cas_enable(sirius_dmxreg[0], cas_reg);
#ifdef CONFIG_AV_MODULE_GSE
	if (sirius_gse_init(device,inode) < 0)
		return -1;
#endif

	return demux_init(device,inode);
}

int sirius_demux_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;
	struct sirius_reg_cas *reg;
	unsigned int addr[MAX_DEMUX_UNIT];

	addr[0] = SIRIUS_DEMUX_BASE_SYS;
	addr[1] = SIRIUS_DEMUX_BASE_SYS1;

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		dev = &gxdmxdev[i];
		dev->ops = NULL;
		dev->max_filter = 0;
		dev->max_slot = 0;
		dev->max_cas = 0;
		dev->max_av = 0;
		reg = dev->cas_reg;
		dev->cas_reg = NULL;

		gx3211_hw_init(dev->reg);
		gx_iounmap(dev->reg);
		gx_release_mem_region(addr[dev->id], sizeof(struct gx3211_reg_demux));
	}
	gx_iounmap(reg);
	gx_release_mem_region(SIRIUS_DEMUX_BASE_CAS, sizeof(struct sirius_reg_cas));
#ifdef CONFIG_AV_MODULE_GSE
	sirius_gse_cleanup(device, inode);
#endif

	return demux_cleanup(device,inode);
}

int sirius_demux_open(struct gxav_module *module)
{
	int i;
	struct dmxdev *dev = NULL;

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		dev = &gxdmxdev[i];
		dev->irqs[0] = SIRIUS_IRQ0;
		dev->irqs[1] = SIRIUS_IRQ1;
#ifdef CONFIG_AV_MODULE_GSE
		dev->irqs[2] = SIRIUS_GSE_IRQ;
#endif
	}
	return demux_open(module);
}

struct gxav_module_ops sirius_demux_module = {
	.module_type  = GXAV_MOD_DEMUX,
	.count        = 16,
#ifdef CONFIG_AV_MODULE_GSE
	.irqs         = {SIRIUS_IRQ0, SIRIUS_IRQ1, SIRIUS_GSE_IRQ, -1},
	.irq_names    = {"demux_incident0", "demux_incident1", "gse"},
#else
	.irqs         = {SIRIUS_IRQ0, SIRIUS_IRQ1, -1},
	.irq_names    = {"demux_incident0", "demux_incident1"},
#endif
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = sirius_demux_open,
	.close        = demux_close,
	.init         = sirius_demux_init,
	.cleanup      = sirius_demux_cleanup,
	.set_property = demux_set_entry,
	.get_property = demux_get_entry,
	.interrupts[SIRIUS_IRQ0]    = demux_irq,
	.interrupts[SIRIUS_IRQ1]    = demux_irq,
#ifdef CONFIG_AV_MODULE_GSE
	.interrupts[SIRIUS_GSE_IRQ] = demux_irq
#endif
};

