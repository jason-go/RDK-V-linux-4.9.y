#include "kernelcalls.h"
#include "gx3211_regs.h"
#include "gx3211_demux.h"
#include "gxdemux.h"
#include "gxav_bitops.h"
#include "sdc_hal.h"
#include "sdc_module.h"
#include "profile.h"

//=======================dmx===========================================
void gx3211_clr_ints(void *vreg)
{
	int i;
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	for(i=reg->int_ts_if; i<= reg->int_dmx_cc_en_l; i+=4){
		*(volatile int*)i = 0xffffffff;
	}
}

void gx3211_hw_init(void *vreg)
{
	struct dmxdev *dev = &gxdmxdev[0];
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	unsigned long *pid_sram_reg = NULL;
	unsigned int index = 0;

	/*clear PID and match CFGs */
	REG_SET_VAL(&(reg->wen_mask), 0xFFFFFFFF);

	/* 0x2000 ~ 0x3500 */
	pid_sram_reg = (unsigned long *)(&(reg->pid_en));
	for (index = 0; index < NUM_PID_SRAM; index++) {
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)) {
			if (((0x2000 + index * 4) >= 0x3400) && ((0x2000 + index * 4) < 0x3480))
				continue ;
		}
		REG_SET_VAL((pid_sram_reg + index), 0x00000000);
	}

	REG_SET_VAL(&(reg->wen_mask), 0x00000000);
	REG_SET_VAL(&(reg->demux_bigendian),0);

	/* for Pre-DEMUX */
	for (index = 0; index < dev->max_filter; index++) {
		REG_SET_VAL(&(reg->df_addr[index].df_start_addr), 0);
		REG_SET_VAL(&(reg->df_addr[index].df_buf_size), 0);
		REG_SET_VAL(&(reg->df_addr[index].df_write_addr), 0);
		REG_SET_VAL(&(reg->df_addr[index].df_read_addr), 0);
		REG_SET_VAL(&(reg->df_addr[index].df_almost_full_gate), 0);
	}

	//set clock
	REG_CLR_BITS(&(reg->stc_cnt), (0xFFFF << 0));
	REG_SET_BITS(&(reg->stc_cnt), 600);              // STC CNT
	REG_SET_BITS(&(reg->stc_cnt), (1 << 16));        // CNT_SET_EN

	REG_CLR_BITS(&(reg->stc_ctrl_1), (0xFFFF << 0));
	REG_SET_BITS(&(reg->stc_ctrl_1), 600);           // STC CNT
	REG_SET_BITS(&(reg->stc_ctrl_1), (1 << 16));     // CNT_SET_EN

	//start
	REG_CLR_BIT(&(reg->pts_mode), 0);
	REG_SET_VAL(&(reg->df_buf_full_stop_th), 0);
	REG_SET_VAL(&(reg->av_buf_full_stop_th), 0);
	REG_SET_BIT(&(reg->dmux_cfg), BIT_DEMUX_CFG_AUTO_START);
	REG_SET_BIT(&(reg->ts_if_cfg), 15);
	if (gxav_firewall_access_align()) {
		REG_SET_VAL(&(reg->axi_8to32_cfg), 1);
	}
}

void gx3211_set_sync_gate(void *vreg, unsigned char timing_gate,
		unsigned char byt_cnt_err_gate, unsigned char sync_loss_gate, unsigned char sync_lock_gate)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	unsigned long temp;

	temp = REG_GET_VAL(&(reg->dmux_cfg));
	temp &= (~(DEMUX_CFG_TIMING_GATE | DEMUX_CFG_COUNT_GATE | DEMUX_CFG_LOSS_GATE | DEMUX_CFG_LOCK_GATE));
	temp |= (((timing_gate << BIT_DEMUX_CFG_TIMING_GATE) | (byt_cnt_err_gate << BIT_DEMUX_CFG_COUNT_GATE)
				| (sync_loss_gate << BIT_DEMUX_CFG_LOSS_GATE) | (sync_lock_gate << BIT_DEMUX_CFG_LOCK_GATE))
			& (DEMUX_CFG_TIMING_GATE | DEMUX_CFG_COUNT_GATE | DEMUX_CFG_LOSS_GATE | DEMUX_CFG_LOCK_GATE));
	REG_SET_VAL(&(reg->dmux_cfg), temp);
}


int gx3211_query_tslock(void *vreg, unsigned int dmxid, unsigned int source)
{
	int ts_lock_bit = 0;
	unsigned int status;
	volatile unsigned int *ts_lock_reg = NULL;
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	if (CHIP_IS_GX3211 || CHIP_IS_GX3201 || CHIP_IS_GX3113C || CHIP_IS_GX6605S) {
		ts_lock_reg = &(reg->int_ts_if);
		ts_lock_bit = BIT_DEMUX_LEVEL_LOCK_PORT1+source;
		REG_SET_VAL(ts_lock_reg, 1<<ts_lock_bit);

	} else {
		if (source < DEMUX_SDRAM) {
			ts_lock_reg = &(reg->int_ts_if);
			ts_lock_bit = BIT_DEMUX_LEVEL_LOCK_PORT1+source;
			REG_SET_VAL(ts_lock_reg, 1<<ts_lock_bit);

		} else {
			ts_lock_reg = (dmxid < MAX_DMX_NORMAL) ? &reg->int_dmx_tsr_buf : &reg->int_tsr23_buf;
			ts_lock_bit = (dmxid%2) ? BIT_DEMUX_LEVEL_LOCK_SRAM1 : BIT_DEMUX_LEVEL_LOCK_SRAM0;
		}
	}

	gxlog_d(LOG_DEMUX, "Current demux(%d)'s source = %d. tsif_reg = %x\n", dmxid, source,
			REG_GET_FIELD(&(reg->int_ts_if), 0x7<<BIT_DEMUX_LEVEL_LOCK_PORT1, BIT_DEMUX_LEVEL_LOCK_PORT1));

	status = REG_GET_VAL(ts_lock_reg);
	if ((status >> ts_lock_bit) & 0x1)
		return TS_SYNC_LOCKED;

	return TS_SYNC_UNLOCKED;
}

void gx3211_set_input_source(void *vreg, unsigned int dmxid,unsigned int source)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	unsigned int temp;

	temp = REG_GET_VAL(&(reg->dmux_cfg));
	if (dmxid%2 == 0) {
		temp = ((temp & ~(DEMUX_CFG_TS_SEL1)) | (source<<BIT_DEMUX_CFG_TS_SEL_1));
	} else{
		temp = ((temp & ~(DEMUX_CFG_TS_SEL2)) | (source<<BIT_DEMUX_CFG_TS_SEL_2));
	}
	REG_SET_VAL(&(reg->dmux_cfg), temp);
	gxlog_d(LOG_DEMUX, "Select demux(%d)'s source = %d\n", dmxid, source);
}

void gx3211_set_apts_sync(void *vreg, int avid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if (avid < 0)
		return;

	REG_SET_FIELD(&(reg->pts_cfg), DEMUX_PTS_CFG_RECOVER_SEL, avid,
			BIT_DEMUX_PTS_CFG_RECOVER_SEL);
	REG_SET_FIELD(&(reg->pts_cfg), DEMUX_PTS_CFG_VIEW_SEL, avid,
			BIT_DEMUX_PTS_CFG_VIEW_SEL);
}

void gx3211_set_pcr_sync(void *vreg, unsigned int dmxid,unsigned short pcr_pid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_FIELD(&(reg->pcr_cfg), DEMUX_PCR_CFG_PID, pcr_pid, BIT_DEMUX_PCR_CFG_PID);

	if (0 == dmxid) {
		REG_CLR_BIT(&(reg->pcr_cfg), BIT_DEMUX_PCR_CFG_SEL);
	} else if (1 == dmxid) {
		REG_SET_BIT(&(reg->pcr_cfg), BIT_DEMUX_PCR_CFG_SEL);
	}
}

void gx3211_set_oddkey(void *vreg,unsigned int caid,unsigned int key_high,unsigned int key_low, unsigned int alg)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_VAL(&(reg->wen_mask), 0xFFFFFFFF);

	if (key_high !=0 ||key_low !=0) {
		REG_SET_VAL(&(reg->key_even_odd[caid].key_odd_high), key_high);
		REG_SET_VAL(&(reg->key_even_odd[caid].key_odd_low), key_low);
	}

	REG_SET_VAL(&(reg->wen_mask), 0x00000000);
}

void gx3211_set_evenkey(void *vreg,unsigned int caid,unsigned int key_high,unsigned int key_low, unsigned int alg)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_VAL(&(reg->wen_mask), 0xFFFFFFFF);

	if (key_high !=0 ||key_low !=0) {
		REG_SET_VAL(&(reg->key_even_odd[caid].key_even_high), key_high);
		REG_SET_VAL(&(reg->key_even_odd[caid].key_even_low), key_low);
	}

	REG_SET_VAL(&(reg->wen_mask), 0x00000000);

}
void gx3211_link_avbuf(int dmxid, void *vreg, int avid,
		unsigned int start_addr, unsigned int size, unsigned int gate)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if (avid < 0)
		return;

	REG_SET_VAL(&(reg->av_addr[avid].av_start_addr), start_addr);
	REG_SET_VAL(&(reg->av_addr[avid].av_buf_size), size);
	REG_SET_VAL(&(reg->av_addr[avid].av_read_addr),0);
	REG_SET_VAL(&(reg->av_addr[avid].av_write_addr),0);
	REG_SET_BIT(&(reg->av_buf_wptr_clr),avid);
	REG_CLR_BIT(&(reg->av_buf_wptr_clr),avid);
	REG_SET_VAL(&(reg->av_addr[avid].av_almostfull_gate),gate);
}

void gx3211_unlink_avbuf(int dmxid, void *vreg,int avid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if (avid < 0)
		return;
	REG_SET_VAL(&(reg->av_addr[avid].av_read_addr),0);
	REG_SET_VAL(&(reg->av_addr[avid].av_almostfull_gate),0);
}

void gx3211_link_ptsbuf(void *vreg, int avid,unsigned int start_addr, unsigned int size, unsigned int gate)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if (avid <0)
		return;
	REG_SET_VAL(&(reg->pts_addr[avid].pts_buf_addr), start_addr);
	REG_SET_VAL(&(reg->pts_addr[avid].pts_buf_len), size);
	REG_SET_BIT(&(reg->pts_buf_wptr_clr), avid);
	REG_CLR_BIT(&(reg->pts_buf_wptr_clr), avid);
	REG_SET_VAL(&(reg->pts_w_addr[avid]),0);
}

void gx3211_unlink_ptsbuf(void *vreg,int avid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if (avid < 0)
		return;
	REG_SET_BIT(&(reg->pts_buf_wptr_clr), avid);
	REG_CLR_BIT(&(reg->pts_buf_wptr_clr), avid);
}

void gx3211_enable_av_int(void *vreg, int avid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if (avid < 0)
		return;
	REG_SET_BIT(&(reg->int_dmx_av_buf),(avid));
	REG_SET_BIT(&(reg->int_dmx_av_buf),(avid+24));
	REG_SET_BIT(&(reg->int_dmx_av_buf_en),(avid+24));
	REG_SET_BIT(&(reg->int_dmx_av_buf_en),(avid));
}

void gx3211_disable_av_int(void *vreg, int avid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if (avid < 0)
		return;
	REG_CLR_BIT(&(reg->int_dmx_av_buf_en),(avid));
	REG_CLR_BIT(&(reg->int_dmx_av_buf_en),(avid+24));
	REG_SET_BIT(&(reg->int_dmx_av_buf),(avid));
	REG_SET_BIT(&(reg->int_dmx_av_buf),(avid+24));
}

//=======================slot===========================================
void gx3211_disable_slot(void *vreg,unsigned int dmxid,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	if(slotid < 32) {
		REG_SET_BIT(&(reg->wen_mask), slotid);
		if (0 == dmxid)
			REG_SET_VAL(&(reg->pid_en[0].pid_en_l), 0);
		else if (1 == dmxid)
			REG_SET_VAL(&(reg->pid_en[1].pid_en_l), 0);

		REG_CLR_BIT(&(reg->wen_mask), slotid);
	} else {
		slotid -= 32;
		REG_SET_BIT(&(reg->wen_mask), slotid);
		if (0 == dmxid)
			REG_SET_VAL(&(reg->pid_en[0].pid_en_h), 0);
		else if (1 == dmxid)
			REG_SET_VAL(&(reg->pid_en[1].pid_en_h), 0);

		REG_CLR_BIT(&(reg->wen_mask), slotid);
	}
}

void gx3211_enable_slot(void *vreg,unsigned int dmxid,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	if(slotid < 32) {
		REG_SET_BIT(&(reg->wen_mask), slotid);
		if (0 == dmxid)
			REG_SET_VAL(&(reg->pid_en[0].pid_en_l), (0x1 << slotid));
		else if (1 == dmxid)
			REG_SET_VAL(&(reg->pid_en[1].pid_en_l), (0x1 << slotid));

		REG_CLR_BIT(&(reg->wen_mask), slotid);
	}else{
		slotid -= 32;
		REG_SET_BIT(&(reg->wen_mask), slotid);
		if (0 == dmxid)
			REG_SET_VAL(&(reg->pid_en[0].pid_en_h), (0x1 << slotid));
		else if (1 == dmxid)
			REG_SET_VAL(&(reg->pid_en[1].pid_en_h), (0x1 << slotid));

		REG_CLR_BIT(&(reg->wen_mask), slotid);
	}
}

void gx3211_reset_cc(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	if(slotid < 32) {
		REG_CLR_BIT(&(reg->cpu_pid_in_l), slotid);
	}else{
		slotid -= 32;
		REG_CLR_BIT(&(reg->cpu_pid_in_h), slotid);
	}
}

void gx3211_set_repeat_mode(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STOP_PER_UNIT);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STOP_PER_UNIT);
}

void gx3211_set_one_mode(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STOP_PER_UNIT);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_STOP_PER_UNIT);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STOP_PER_UNIT);
}

void gx3211_set_interrupt_unit(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_INT_PER_UNIT);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_INT_PER_UNIT);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_INT_PER_UNIT);
}

void gx3211_set_err_discard(void *vreg,unsigned int slotid, int flag)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ERR_DISCARD);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_ERR_DISCARD);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ERR_DISCARD);
}

void gx3211_set_dup_discard(void *vreg,unsigned int slotid, int flag)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DUP_DISCARD);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_DUP_DISCARD);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DUP_DISCARD);
}

void gx3211_set_av_out(void *vreg,unsigned int slotid, int flag)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_AV_OUT_EN);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_AV_OUT_EN);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_AV_OUT_EN);
}

void gx3211_set_ts_out(void *vreg,unsigned int slotid, int flag)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_OUT_EN);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TS_OUT_EN);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_OUT_EN);
}

void gx3211_set_des_out(void *vreg,unsigned int slotid, int flag)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_DES_OUT_EN);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TS_DES_OUT_EN);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_DES_OUT_EN);
}

void gx3211_set_crc_disable(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_CRC_DISABLE);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_CRC_DISABLE);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_CRC_DISABLE);
}

void gx3211_set_psi_type(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_FILTER_PSI);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TYPE_SEL);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
}

void gx3211_set_pes_type(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TYPE_SEL);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
}
void gx3211_set_av_type(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);

	REG_SET_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_TYPE);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TYPE_AUDIO);
	REG_CLR_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_TYPE);
}

void gx3211_set_pts_bypass(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_BYPASS);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_PTS_BYPASS);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_BYPASS);
}

void gx3211_set_pts_insert(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_BYPASS);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_BYPASS);
}
void gx3211_set_pts_to_sdram(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_TO_SDRAM);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_PTS_TO_SDRAM);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_TO_SDRAM);
}

void gx3211_clr_slot_cfg(void *vreg,unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_VAL(&(reg->wen_mask), 0xffffffff);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_SET_VAL(&(reg->wen_mask), 0);
}

void gx3211_set_pid(void *vreg,unsigned int slotid,unsigned short pid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	unsigned int *pid_reg;
	unsigned long pid_bit;
	int i, mask = slotid;

	if (slotid >= 32)
		mask = slotid - 32;

	REG_SET_BIT(&(reg->wen_mask),mask);
	for (i = 0; i < NUM_PID_BIT; i++) {
		pid_reg = (slotid < 32) ? &(reg->pid[i].pid_l) : &(reg->pid[i].pid_h);
		REG_SET_VAL(pid_reg, 0);
		pid_bit = (((pid >> (NUM_PID_BIT - i - 1)) & 0x01) << mask);
		REG_SET_VAL(pid_reg, pid_bit);
	}
	REG_CLR_BIT(&(reg->wen_mask), mask);
}

void gx3211_get_pid(void *vreg,unsigned int slotid,unsigned short *pid)
{
#if (GX_DEBUG_PRINTF_LEVEL >= 4)
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	unsigned int *pid_reg;
	unsigned long pid_bit;
	int i, mask = slotid;

	*pid = 0;
	if (slotid >= 32)
		mask = slotid - 32;

	REG_SET_BIT(&(reg->dmux_cfg), 30);
	for (i = 0; i < NUM_PID_BIT; i++) {
		pid_reg = (slotid < 32) ? &(reg->pid[i].pid_l) : &(reg->pid[i].pid_h);
		pid_bit = (REG_GET_VAL(pid_reg) >> mask) & 0x01;
		*pid |= pid_bit << (NUM_PID_BIT - i - 1);
	}
	REG_CLR_BIT(&(reg->dmux_cfg), 30);
#endif
}

void gx3211_set_avid(void *vreg,unsigned int slotid,int avid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	unsigned int temp;
	if (avid < 0)
		return;
	REG_SET_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_NUM);
	temp = (avid << BIT_DEMUX_PID_CFG_AV_NUM);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), temp);
	REG_CLR_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_NUM);
}

void gx3211_set_descid(void *vreg, unsigned int slotid,unsigned int caid, unsigned int flags)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	REG_SET_BITS(&(reg->wen_mask), DEMUX_PID_CFG_KEY_ID);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), caid << BIT_DEMUX_PID_CFG_KEY_ID);
	REG_CLR_BITS(&(reg->wen_mask), DEMUX_PID_CFG_KEY_ID);
}

void gx3211_set_even_valid(void *vreg, unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
}

void gx3211_set_odd_valid(void *vreg, unsigned int slotid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
}

void gx3211_set_filterid(void *vreg, unsigned int slotid,unsigned int filterid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if(filterid < 32 ){
		REG_SET_BIT(&(reg->wen_mask), filterid);
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_sel_l), (0x01 << filterid));
		REG_CLR_BIT(&(reg->wen_mask), filterid);
	} else {
		filterid = filterid - 32;
		REG_SET_BIT(&(reg->wen_mask), filterid);
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_sel_h), (0x01 << filterid));
		REG_CLR_BIT(&(reg->wen_mask), filterid);
	}

}

void gx3211_clr_filterid(void *vreg, unsigned int slotid,unsigned int filterid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if(filterid < 32 ){
		REG_SET_BIT(&(reg->wen_mask), filterid);
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_sel_l), 0);
		REG_CLR_BIT(&(reg->wen_mask), filterid);
	} else {
		filterid = filterid - 32;
		REG_SET_BIT(&(reg->wen_mask), filterid);
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_sel_h), 0);
		REG_CLR_BIT(&(reg->wen_mask), filterid);
	}

}

//=======================filter===========================================

void gx3211_clr_dfrw(void *vreg,unsigned int filterid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_VAL(&(reg->df_addr[filterid].df_read_addr), 0);
	REG_SET_VAL(&(reg->df_addr[filterid].df_write_addr), 0);
	REG_SET_VAL(&(reg->df_waddr[filterid]),0);
}

void gx3211_clr_int_df(void *vreg,unsigned int filterid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if(filterid <32 ) {
		REG_SET_VAL(&(reg->int_dmx_df_l), (1<<filterid));
		REG_SET_VAL(&(reg->int_df_buf_alfull_l), (1<<filterid));
	} else {
		filterid -=32;
		REG_SET_VAL(&(reg->int_dmx_df_h), (1<<filterid));
		REG_SET_VAL(&(reg->int_df_buf_alfull_h), (1<<filterid));
	}
}

void gx3211_clr_int_df_en(void *vreg,unsigned int filterid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if(filterid <32 ) {
		REG_CLR_BIT(&(reg->int_dmx_df_en_l), filterid);
		REG_CLR_BIT(&(reg->int_df_buf_alfull_en_l), filterid);
	} else {
		filterid -=32;
		REG_CLR_BIT(&(reg->int_dmx_df_en_h), filterid);
		REG_CLR_BIT(&(reg->int_df_buf_alfull_en_h), filterid);
	}
}

void gx3211_set_dfbuf(void *vreg,unsigned int filterid,unsigned int start,unsigned int size,unsigned int gate)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	REG_SET_VAL(&(reg->df_addr[filterid].df_start_addr), start);
	REG_SET_VAL(&(reg->df_addr[filterid].df_buf_size), size);
	REG_SET_VAL(&(reg->df_addr[filterid].df_almost_full_gate), gate);
	REG_SET_VAL(&(reg->df_addr[filterid].df_read_addr), 0);
	REG_SET_VAL(&(reg->df_addr[filterid].df_write_addr), 0);
	REG_SET_VAL(&(reg->df_waddr[filterid]),0);

}

void gx3211_set_select(void *vreg,unsigned int dmxid,unsigned int filterid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if (0 == dmxid) {
		if(filterid < 32 ){
			REG_CLR_BIT(&(reg->df_sel_l), filterid);
		}else{
			filterid = filterid - 32;
			REG_CLR_BIT(&(reg->df_sel_h), filterid);
		}
	} else if (1 == dmxid) {
		if(filterid <32 ){
			REG_SET_BIT(&(reg->df_sel_l), filterid);
		}else{
			filterid = filterid - 32;
			REG_SET_BIT(&(reg->df_sel_h), filterid);
		}
	}
}

#define VERSION_BYTE_DEPTH       3
void gx3211_set_match(void *vreg,unsigned int filterid,unsigned int depth,
		struct dmx_filter_key *pkey ,unsigned int eq_flag)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	unsigned int i, j;
	unsigned long temp;
	struct dmx_filter_key *key = pkey;

	/*get filtrate depth */

	if ((depth < VERSION_BYTE_DEPTH) && (0 == eq_flag)) {
		depth = VERSION_BYTE_DEPTH;
	}

	if (filterid < 32) {
		REG_SET_BIT(&(reg->wen_mask), filterid);

		/*initialize all filtrate bytes */
		for (j = 0; j <= depth; j++) {
			if ((VERSION_BYTE_DEPTH == j) && (0 == eq_flag)) {
				for (i = 0; i < NUM_MATCH_BIT; i++) {
					if ((i == 0) || (i == 1) || (i == 7)) {
						REG_SET_VAL(&(reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m0_byte_l),
								0x01 << filterid);
						REG_SET_VAL(&(reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m1_byte_l),
								0x01 << filterid);
					} else {
						REG_SET_VAL(&(reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m0_byte_l),
								0);
						REG_SET_VAL(&(reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m1_byte_l),
								0);
					}
				}
			} else {
				for (i = 0; i < NUM_MATCH_BIT; i++) {
					REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m0_byte_l), 0x01 << filterid);
					REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m1_byte_l), 0x01 << filterid);
				}
			}
		}

		/*set filtrate byte(value) */
		key = pkey;
		for (j = 0; j <= depth; j++) {
			for (i = 0; i < NUM_MATCH_BIT; i++) {
				if (key->mask & (1 << (7 - i))) {
					if (key->value & (1 << (7 - i))) {
						REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m0_byte_l), 0);
						REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m1_byte_l), 0x01 << filterid);
					} else {
						REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m0_byte_l), 0x01 << filterid);
						REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m1_byte_l), 0);
					}
				}
			}

			if (j == 0) {
				key += 3;
			} else {
				key++;
			}
		}

		/*set filtrate:eq or neq */
		if (eq_flag) {
			REG_CLR_BIT(&(reg->df_nequal_on_l), filterid);
		} else {
			REG_SET_BIT(&(reg->df_nequal_on_l), filterid);
		}

		/*set filter depth */
		REG_CLR_BIT(&(reg->wen_mask), filterid);
	} else {
		filterid -= 32;
		REG_SET_BIT(&(reg->wen_mask), filterid);

		/*initialize all filtrate bytes */
		for (j = 0; j <= depth; j++) {
			if ((VERSION_BYTE_DEPTH == j) && (0 == eq_flag)) {
				for (i = 0; i < NUM_MATCH_BIT; i++) {
					if ((i == 0) || (i == 1) || (i == 7)) {
						REG_SET_VAL(&(reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m0_byte_h),
								0x01 << filterid);
						REG_SET_VAL(&(reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m1_byte_h),
								0x01 << filterid);
					} else {
						REG_SET_VAL(&(reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m0_byte_h),
								0);
						REG_SET_VAL(&(reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m1_byte_h),
								0);
					}
				}
			} else {
				for (i = 0; i < NUM_MATCH_BIT; i++) {
					REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m0_byte_h), 0x01 << filterid);
					REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m1_byte_h), 0x01 << filterid);
				}
			}
		}

		/*set filtrate byte(value) */
		key = pkey;
		for (j = 0; j <= depth; j++) {
			for (i = 0; i < NUM_MATCH_BIT; i++) {
				if (key->mask & (1 << (7 - i))) {
					if (key->value & (1 << (7 - i))) {
						REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m0_byte_h), 0);
						REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m1_byte_h), 0x01 << filterid);
					} else {
						REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m0_byte_h), 0x01 << filterid);
						REG_SET_VAL(&(reg->m_byte_n[j].m_byte[i].m1_byte_h), 0);
					}
				}
			}

			if (j == 0) {
				key += 3;
			} else {
				key++;
			}
		}

		/*set filtrate:eq or neq */
		if (eq_flag) {
			REG_CLR_BIT(&(reg->df_nequal_on_h), filterid);
		} else {
			REG_SET_BIT(&(reg->df_nequal_on_h), filterid);
		}

		/*set filter depth */
		REG_CLR_BIT(&(reg->wen_mask), filterid);
		filterid += 32;
	}

	temp = REG_GET_VAL(&(reg->df_depth[filterid >> 3]));
	temp &= ~(0x0f << ((filterid & 0x07) << 2));
	temp |= (depth << ((filterid & 0x07) << 2));
	REG_SET_VAL(&(reg->df_depth[filterid >> 3]), temp);
}

void gx3211_set_sec_irqen(void *vreg,unsigned int filterid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;

	if(filterid < 32)
		REG_SET_BIT(&(reg->int_dmx_df_en_l), filterid);
	else
		REG_SET_BIT(&(reg->int_dmx_df_en_h), filterid-32);
}

void gx3211_set_gate_irqen(void *vreg,unsigned int filterid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if(filterid < 32 )
		REG_SET_BIT(&(reg->int_df_buf_alfull_en_l), filterid);
	else
		REG_SET_BIT(&(reg->int_df_buf_alfull_en_h), (filterid-32));
}

void gx3211_enable_filter(void *vreg,unsigned int filterid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if(filterid < 32)
		REG_SET_BIT(&(reg->df_on_l), filterid);
	else
		REG_SET_BIT(&(reg->df_on_h), filterid-32);
}

void gx3211_disable_filter(void *vreg,unsigned int filterid)
{
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	if(filterid < 32 )
		REG_CLR_BIT(&(reg->df_on_l), filterid);
	else
		REG_CLR_BIT(&(reg->df_on_h), filterid-32);
}

int gx3211_avbuf_callback(unsigned int channel_id,unsigned int length, unsigned int underflow,void *arg)
{
	unsigned long flags = gx_sdc_spin_lock_irqsave(channel_id);

	if ((arg = gx_sdc_get_outdata(channel_id)) && arg) {
		struct dmx_slot *slot = (struct dmx_slot *)arg;
		struct dmx_demux* demux = slot->demux;
		struct dmxdev* dmxdev = demux->dev;
		struct gx3211_reg_demux *reg = dmxdev->reg;

		int avid = slot->avid;
		unsigned int av_read_addr = 0;
		struct demux_fifo *avfifo = NULL;
		unsigned int av_write_addr = 0;
		unsigned int sdc_write_addr = 0;
		unsigned int pts_wptr = 0;

		if (avid >= 0) {
			avfifo = &dmxdev->avfifo[avid];
			gxav_sdc_rwaddr_get(avfifo->channel_id, &av_read_addr,&sdc_write_addr);
			REG_SET_VAL(&(reg->av_addr[avid].av_read_addr), av_read_addr);
			av_write_addr = REG_GET_VAL(&(reg->av_addr[avid].av_write_addr));
			if(av_write_addr != sdc_write_addr)
				gxav_sdc_Wptr_Set(avfifo->channel_id, av_write_addr);

			pts_wptr = REG_GET_VAL(&(reg->pts_w_addr[avid]));
			gxav_sdc_Wptr_Set(avfifo->pts_channel_id, pts_wptr);
			//gxlog_d(LOG_DEMUX, "demux callback av_wptr 0x%x \n",av_write_addr);
		}
	}

	gx_sdc_spin_unlock_irqrestore(channel_id, flags);
	return 0;
}

void gx3211_log_d_source(void *vreg)
{
#if (GX_DEBUG_PRINTF_LEVEL >= 4)
	struct gx3211_reg_demux *reg = (struct gx3211_reg_demux*)vreg;
	unsigned int ts_if_cfg = 0, i = 0;

	ts_if_cfg = REG_GET_VAL(&(reg->ts_if_cfg));
	for (i = 0; i < 3; i++) {
		gxlog_d(LOG_DEMUX, "DEMUX(%d) TS%d mode %s serial\n",
				i, ts_if_cfg&(0x1<<(8+i)) ? "is" : "isn't");
		gxlog_d(LOG_DEMUX, "\tnovalid=%d, nosync=%d, bigendian=%d, decmode=%d\n",
				ts_if_cfg&(0x1<<(i)) ? 1 : 0,
				ts_if_cfg&(0x1<<(12+i)) ? 1 : 0,
				ts_if_cfg&(0x1<<(4+i)) ? 1 : 0,
				ts_if_cfg&(0x1<<(16+i)) ? 1 : 0);
		if (i < 2)
			gxlog_d(LOG_DEMUX, "\tTS_PIN_MUX_SEL=0x%x\n", (ts_if_cfg>>(20+4*i)) & 0x7);
	}
#endif
}

struct demux_ops gx3211_demux_ops = {
	.disable_slot         = gx3211_disable_slot,
	.enable_slot          = gx3211_enable_slot,
	.reset_cc             = gx3211_reset_cc,
	.set_repeat_mode      = gx3211_set_repeat_mode,
	.set_one_mode         = gx3211_set_one_mode,
	.set_interrupt_unit   = gx3211_set_interrupt_unit,
	.set_crc_disable      = gx3211_set_crc_disable,
	.set_pts_to_sdram     = gx3211_set_pts_to_sdram,
	.set_pid              = gx3211_set_pid,
	.set_av_out           = gx3211_set_av_out,
	.set_des_out          = gx3211_set_des_out,
	.set_err_discard      = gx3211_set_err_discard,
	.set_dup_discard      = gx3211_set_dup_discard,
	.set_ts_out           = gx3211_set_ts_out,
	.set_pts_bypass       = gx3211_set_pts_bypass,
	.set_pts_insert       = gx3211_set_pts_insert,
	.set_descid           = gx3211_set_descid,
	.set_avid             = gx3211_set_avid,
	.set_filterid         = gx3211_set_filterid,
	.clr_filterid         = gx3211_clr_filterid,
	.set_even_valid       = gx3211_set_even_valid,
	.set_odd_valid        = gx3211_set_odd_valid,
	.set_psi_type         = gx3211_set_psi_type,
	.set_pes_type         = gx3211_set_pes_type,
	.set_av_type          = gx3211_set_av_type,
	.clr_slot_cfg         = gx3211_clr_slot_cfg,

	.enable_filter        = gx3211_enable_filter,
	.disable_filter       = gx3211_disable_filter,
	.clr_dfrw             = gx3211_clr_dfrw,
	.set_dfbuf            = gx3211_set_dfbuf,
	.clr_int_df           = gx3211_clr_int_df,
	.clr_int_df_en        = gx3211_clr_int_df_en,
	.set_select           = gx3211_set_select,
	.set_sec_irqen        = gx3211_set_sec_irqen,
	.set_gate_irqen       = gx3211_set_gate_irqen,
	.set_match            = gx3211_set_match,

	.set_evenkey          = gx3211_set_evenkey,
	.set_oddkey           = gx3211_set_oddkey,

	.clr_ints             = gx3211_clr_ints,
	.hw_init              = gx3211_hw_init,
	.set_sync_gate        = gx3211_set_sync_gate,
	.set_input_source     = gx3211_set_input_source,
	.set_pcr_sync         = gx3211_set_pcr_sync,
	.set_apts_sync        = gx3211_set_apts_sync,
	.query_tslock         = gx3211_query_tslock,

	.enable_av_int        = gx3211_enable_av_int,
	.disable_av_int       = gx3211_disable_av_int,
	.link_avbuf           = gx3211_link_avbuf,
	.unlink_avbuf         = gx3211_unlink_avbuf,
	.link_ptsbuf          = gx3211_link_ptsbuf,
	.unlink_ptsbuf        = gx3211_unlink_ptsbuf,
	.avbuf_cb             = gx3211_avbuf_callback,

	.df_isr               = gx3211_df_isr,
	.df_al_isr            = gx3211_df_al_isr,
	.av_gate_isr          = gx3211_av_gate_isr,
	.av_section_isr       = gx3211_av_section_isr,
	.ts_clk_err_isr       = gx3211_check_ts_clk_err_isr,

	.log_d_source         = gx3211_log_d_source,
};

struct gx3211_reg_demux *gx3211_dmxreg[MAX_DEMUX_UNIT];

int gx3211_demux_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int ret = 0;
	unsigned int i=0;
	struct dmxdev *dev = NULL;
	unsigned int addr[MAX_DEMUX_UNIT];

	addr[0] = GXAV_DEMUX_BASE_SYS;
	addr[1] = GXAV_DEMUX_BASE_SYS1;
	for(i=0;i<MAX_DEMUX_UNIT;i++){
		dev = &gxdmxdev[i];
		gx_memset(dev, 0, sizeof(struct dmxdev));
		dev->id = i;
		if (!gx_request_mem_region(addr[dev->id], sizeof(struct gx3211_reg_demux))) {
			gxlog_e(LOG_DEMUX, "request DEMUX mem region failed!\n");
			return -1;
		}

		gx3211_dmxreg[dev->id] = gx_ioremap(addr[dev->id], sizeof(struct gx3211_reg_demux));
		if (!gx3211_dmxreg[dev->id]) {
			gxlog_e(LOG_DEMUX, "ioremap DEMUX space failed!\n");
			return -1;
		}

		dev->reg = gx3211_dmxreg[dev->id];
		dev->ops = &gx3211_demux_ops;
		dev->max_dmx_unit = MAX_DEMUX_UNIT;
		dev->max_filter = MAX_FILTER_NUM;
		dev->max_slot = MAX_SLOT_NUM;
		dev->max_cas = MAX_CA_NUM;
		dev->max_av = MAX_AVBUF_NUM;
		dev->irqs[0] = DMX_IRQ0;
		dev->irqs[1] = DMX_IRQ1;

		gx3211_hw_init(dev->reg);
	}
	ret =demux_init(device,inode);
	if(ret < 0)
		return -1;

	return 0;
}

int gx3211_demux_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int ret = 0;
	unsigned int i=0;
	struct dmxdev *dev = NULL;
	unsigned int addr[MAX_DEMUX_UNIT];

	addr[0] = GXAV_DEMUX_BASE_SYS;
	addr[1] = GXAV_DEMUX_BASE_SYS1;

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		dev = &gxdmxdev[i];
		dev->ops = NULL;
		dev->max_filter = 0;
		dev->max_slot = 0;
		dev->max_cas = 0;
		dev->max_av = 0;
		gx3211_hw_init(dev->reg);
		gx_iounmap(dev->reg);
		gx_release_mem_region(addr[dev->id], sizeof(struct gx3211_reg_demux));
	}

	ret = demux_cleanup(device,inode);
	if(ret < 0)
		return -1;

	return 0;
}

struct gxav_module_ops gx3211_demux_module = {
	.module_type  = GXAV_MOD_DEMUX,
	.count        = MAX_DEMUX_UNIT*2,
	.irqs         = {DMX_IRQ0,DMX_IRQ1, -1},
	.irq_names    = {"demux_incident0", "demux_incident1"},
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = demux_open,
	.close        = demux_close,
	.init         = gx3211_demux_init,
	.cleanup      = gx3211_demux_cleanup,
	.set_property = demux_set_entry,
	.get_property = demux_get_entry,
	.interrupts[DMX_IRQ0]    = demux_irq,
	.interrupts[DMX_IRQ1]    = demux_irq
};
