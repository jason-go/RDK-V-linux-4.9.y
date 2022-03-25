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

void taurus_hw_init(void *vreg)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	unsigned long *tsw_slot_sram_reg = NULL;
	unsigned int index = 0;

	REG_SET_VAL(&(reg->wen_mask), 0xFFFFFFFF);

	tsw_slot_sram_reg = (unsigned long *)(&(reg->tsw_pid_en));
	for (index = 0; index < TAURUS_NUM_TSW_SLOT_SRAM; index++)
		REG_SET_VAL((tsw_slot_sram_reg + index), 0x00000000);

	REG_SET_VAL(&(reg->wen_mask), 0x0);
	gx3211_hw_init(reg);
}

void taurus_set_input_source(void *vreg, unsigned int dmxid,unsigned int source)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux *)vreg;
	unsigned int temp;

	if (dmxid == 0 || dmxid == 1) {
		temp = REG_GET_VAL(&(reg->dmux_cfg));
		if(dmxid == 0){
			temp = ((temp & ~(DEMUX_CFG_TS_SEL1)) | (source<<BIT_DEMUX_CFG_TS_SEL_1));
		}else if (dmxid == 1){
			temp = ((temp & ~(DEMUX_CFG_TS_SEL2)) | (source<<BIT_DEMUX_CFG_TS_SEL_2));
		}
		REG_SET_VAL(&(reg->dmux_cfg), temp);
	}else {
		temp = REG_GET_VAL(&(reg->ts_write_ctrl));
		if (source == DEMUX_SDRAM)
			source = TAURUS_TSW_SRC_TSR_FOR_TSWSLOT;
		if (dmxid == 4){
			temp &= ~(0x07 << 4);
			temp |= (source << 4);
		}else if (dmxid == 5){
			temp &= ~(0x07 << 7);
			temp |= (source << 7);
		}
		REG_SET_VAL(&(reg->ts_write_ctrl), temp);
	}

	gxlog_d(LOG_DEMUX, "Select demux(%d)'s source = %d\n", dmxid, source);
}

//=======================tsw slot===========================================
void taurus_pf_disable(void *vreg,unsigned int dmxid,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), slotid);
	REG_SET_VAL(&(reg->tsw_pid_en[dmxid]), 0);
	REG_CLR_BIT(&(reg->wen_mask), slotid);
}

void taurus_pf_enable(void *vreg,unsigned int dmxid,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), slotid);
	REG_SET_VAL(&(reg->tsw_pid_en[dmxid]), (0x1 << slotid));
	REG_CLR_BIT(&(reg->wen_mask), slotid);
}

void taurus_pf_set_pid(void *vreg,unsigned int slotid,unsigned short pid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	unsigned int *pid_reg;
	unsigned long pid_bit;
	int i, mask = slotid;

	REG_SET_BIT(&(reg->wen_mask),mask);
	for (i = 0; i < NUM_PID_BIT; i++) {
		pid_reg = &(reg->tsw_pid[i]);
		REG_SET_VAL(pid_reg, 0);
		pid_bit = (((pid >> (NUM_PID_BIT - i - 1)) & 0x01) << mask);
		REG_SET_VAL(pid_reg, pid_bit);
	}
	REG_CLR_BIT(&(reg->wen_mask), mask);
}

void taurus_pf_clr_slot_cfg(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_VAL(&(reg->wen_mask), 0xffffffff);
	REG_SET_VAL(&(reg->tsw_pid_cfg_sel[slotid].pid_cfg), 0);
	REG_SET_VAL(&(reg->wen_mask), 0);
}

void taurus_pf_set_ts_out(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_OUT_EN);
	REG_SET_VAL(&(reg->tsw_pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TS_OUT_EN);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_OUT_EN);
}

void taurus_pf_set_err_discard(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ERR_DISCARD);
	REG_SET_VAL(&(reg->tsw_pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_ERR_DISCARD);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ERR_DISCARD);
}

void taurus_pf_set_dup_discard(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DUP_DISCARD);
	REG_SET_VAL(&(reg->tsw_pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_DUP_DISCARD);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DUP_DISCARD);
}

//=======================slot===========================================
void taurus_disable_slot(void *vreg,unsigned int dmxid,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), slotid);
	REG_SET_VAL(&(reg->pid_en[dmxid]), 0);
	REG_CLR_BIT(&(reg->wen_mask), slotid);
}

void taurus_enable_slot(void *vreg,unsigned int dmxid,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), slotid);
	REG_SET_VAL(&(reg->pid_en[dmxid]), (0x1 << slotid));
	REG_CLR_BIT(&(reg->wen_mask), slotid);
}

void taurus_set_repeat_mode(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STOP_PER_UNIT);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STOP_PER_UNIT);
}

void taurus_set_one_mode(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STOP_PER_UNIT);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_STOP_PER_UNIT);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STOP_PER_UNIT);
}

void taurus_set_interrupt_unit(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_INT_PER_UNIT);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_INT_PER_UNIT);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_INT_PER_UNIT);
}

void taurus_set_err_discard(void *vreg,unsigned int slotid, int flag)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ERR_DISCARD);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_ERR_DISCARD);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ERR_DISCARD);
}

void taurus_set_dup_discard(void *vreg,unsigned int slotid, int flag)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DUP_DISCARD);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_DUP_DISCARD);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DUP_DISCARD);
}

void taurus_set_err_mask(void *vreg, unsigned int dmxid, unsigned int filterid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->ts_err_mask[dmxid]), filterid);
}

void taurus_set_av_out(void *vreg,unsigned int slotid, int flag)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_AV_OUT_EN);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_AV_OUT_EN);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_AV_OUT_EN);
}

void taurus_set_ts_out(void *vreg,unsigned int slotid, int flag)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_OUT_EN);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TS_OUT_EN);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_OUT_EN);
}

void taurus_set_des_out(void *vreg,unsigned int slotid, int flag)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_DES_OUT_EN);
	if (flag)
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TS_DES_OUT_EN);
	else
		REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_DES_OUT_EN);
}

void taurus_set_crc_disable(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_CRC_DISABLE);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_CRC_DISABLE);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_CRC_DISABLE);
}

void taurus_set_psi_type(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_FILTER_PSI);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TYPE_SEL);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
}

void taurus_set_pes_type(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TYPE_SEL);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
}
void taurus_set_av_type(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);

	REG_SET_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_TYPE);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_TYPE_AUDIO);
	REG_CLR_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_TYPE);

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_FILTER_PSI);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);
}

void taurus_set_pts_bypass(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_BYPASS);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_PTS_BYPASS);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_BYPASS);
}

void taurus_set_pts_insert(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_BYPASS);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_BYPASS);
}

void taurus_set_pts_to_sdram(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_TO_SDRAM);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), DEMUX_PID_CFG_PTS_TO_SDRAM);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_TO_SDRAM);
}

void taurus_clr_slot_cfg(void *vreg,unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_VAL(&(reg->wen_mask), 0xffffffff);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0);
	REG_SET_VAL(&(reg->wen_mask), 0);
	REG_CLR_BIT(&(reg->ts_err_mask[0]), slotid);
	REG_CLR_BIT(&(reg->ts_err_mask[1]), slotid);
}

void taurus_set_pid(void *vreg,unsigned int slotid,unsigned short pid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	unsigned int *pid_reg;
	unsigned long pid_bit;
	int i, mask = slotid;

	REG_SET_BIT(&(reg->wen_mask),mask);
	for (i = 0; i < NUM_PID_BIT; i++) {
		pid_reg = &(reg->pid[i]);
		REG_SET_VAL(pid_reg, 0);
		pid_bit = (((pid >> (NUM_PID_BIT - i - 1)) & 0x01) << mask);
		REG_SET_VAL(pid_reg, pid_bit);
	}
	REG_CLR_BIT(&(reg->wen_mask), mask);
}

void taurus_set_avid(void *vreg,unsigned int slotid, int avid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	unsigned int temp;
	if (avid < 0)
		return;
	REG_SET_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_NUM);
	temp = (avid << BIT_DEMUX_PID_CFG_AV_NUM);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), temp);
	REG_CLR_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_NUM);
}

void taurus_set_filterid(void *vreg, unsigned int slotid,unsigned int filterid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), filterid);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_sel_l), (0x01 << filterid));
	REG_CLR_BIT(&(reg->wen_mask), filterid);
}

void taurus_clr_filterid(void *vreg, unsigned int slotid,unsigned int filterid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	REG_SET_BIT(&(reg->wen_mask), filterid);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_sel_l), 0);
	REG_CLR_BIT(&(reg->wen_mask), filterid);
}

void taurus_set_key_valid(void *vreg, unsigned int slotid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux *)vreg;
	REG_SET_BIT(&(reg->wen_mask), SIRIUS_BIT_PID_CFG_KEY_VALID);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), 0x1 << SIRIUS_BIT_PID_CFG_KEY_VALID);
	REG_CLR_BIT(&(reg->wen_mask), SIRIUS_BIT_PID_CFG_KEY_VALID);
}

void taurus_set_dsc_type(void *vreg, unsigned int slotid, unsigned int dsc_type)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux *)vreg;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DSC_TYPE);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), dsc_type<<BIT_DEMUX_PID_CFG_DSC_TYPE);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DSC_TYPE);
}

void taurus_set_cas_descid(void *vreg, unsigned int slotid, unsigned int casid)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux *)vreg;
	REG_SET_BITS(&(reg->wen_mask), SIRIUS_PID_CFG_KEY_ID);
	REG_SET_VAL(&(reg->pid_cfg_sel[slotid].pid_cfg), casid << SIRIUS_BIT_PID_CFG_KEY_ID);
	REG_CLR_BITS(&(reg->wen_mask), SIRIUS_PID_CFG_KEY_ID);
}

#define VERSION_BYTE_DEPTH       3
void taurus_set_match(void *vreg,unsigned int filterid,unsigned int depth,
		struct dmx_filter_key *pkey ,unsigned int eq_flag)
{
	struct taurus_reg_demux *reg = (struct taurus_reg_demux*)vreg;
	unsigned int i, j;
	unsigned long temp;
	struct dmx_filter_key *key = pkey;

	/*get filtrate depth */
	if ((depth < VERSION_BYTE_DEPTH) && (0 == eq_flag))
		depth = VERSION_BYTE_DEPTH;

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
		if (eq_flag)
			REG_CLR_BIT(&(reg->df_nequal_on_l), filterid);
		else
			REG_SET_BIT(&(reg->df_nequal_on_l), filterid);

		/*set filter depth */
		REG_CLR_BIT(&(reg->wen_mask), filterid);
	}
	temp = REG_GET_VAL(&(reg->df_depth[filterid >> 3]));
	temp &= ~(0x0f << ((filterid & 0x07) << 2));
	temp |= (depth << ((filterid & 0x07) << 2));
	REG_SET_VAL(&(reg->df_depth[filterid >> 3]), temp);
}

int taurus_set_cas_mode(void *vreg, struct dmx_cas *cas)
{
	int cas_id = cas->id;
	struct sirius_reg_cas *reg = (struct sirius_reg_cas*)vreg;

	if (cas->ds_mode == DEMUX_DES_DVB_CSA3 ||
		(cas->ds_mode >= DEMUX_DES_SM4_ECB && cas->ds_mode != DEMUX_DES_MULTI2)) {
		gxlog_e(LOG_DEMUX, "cas don't support csa3 & sm4!\n");
		return -1;
	}

	REG_SET_FIELD(&(reg->ds_mode[cas_id/8]), 0xf<<((cas_id%8)*4), cas->ds_mode, ((cas_id%8)*4));
	REG_SET_BIT(&(reg->cas_mode), 0);
	return 0;
}

void taurus_set_cas_syskey(void *vreg, struct dmx_sys_key *sys)
{
	int sys_addr = sys->sys_sel*2+0xc0;
	struct sirius_reg_cas *reg = (struct sirius_reg_cas*)vreg;
	DEMUX_SET_CW_IV(reg, sys->sys_key_h, sys_addr);
	DEMUX_SET_CW_IV(reg, sys->sys_key_l, sys_addr+1);
}

void taurus_t2mi_config(void *vreg, int t2mi_id, int pid)
{
	struct taurus_reg_t2mi_unit *reg_t2mi;
	struct taurus_reg_t2mi *reg = (struct taurus_reg_t2mi*)vreg;
	reg_t2mi = (t2mi_id == 0) ? &reg->t2mi0[0] : &reg->t2mi1[0];

	REG_SET_VAL(&(reg_t2mi->t2mi_pid), pid);
	REG_SET_VAL(&(reg_t2mi->t2mi_cfg0), 0x3);
	REG_SET_VAL(&(reg_t2mi->t2mi_cfg1), 0x1);
	REG_SET_VAL(&(reg_t2mi->t2mi_cfg2), 0x8e);
}

struct demux_ops taurus_demux_ops = {

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

	.t2mi_config        = taurus_t2mi_config,

	.log_d_source       = gx3211_log_d_source,
};

struct taurus_reg_demux *taurus_dmxreg;

int taurus_demux_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int i;
	void *cas_reg, *t2mi_reg;
	if (!gx_request_mem_region(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux))) {
		gxlog_e(LOG_DEMUX, "request DEMUX mem region failed!\n");
		return -1;
	}

	taurus_dmxreg = gx_ioremap(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux));
	if (!taurus_dmxreg) {
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

	if (!gx_request_mem_region(T2MI_BASE_ADDR, sizeof(struct taurus_reg_t2mi))) {
		gxlog_e(LOG_DEMUX, "request DEMUX mem region failed!\n");
		return -1;
	}

	t2mi_reg = gx_ioremap(T2MI_BASE_ADDR, sizeof(struct taurus_reg_t2mi));
	if (!t2mi_reg) {
		gxlog_e(LOG_DEMUX, "ioremap DEMUX space failed!\n");
		return -1;
	}

	for(i=0;i<MAX_DEMUX_UNIT;i++){
		struct dmxdev *dev = &gxdmxdev[i];
		struct dmx_demux *demux = NULL;
		gx_memset(dev, 0, sizeof(struct dmxdev));
		dev->id  = i;
		dev->reg = taurus_dmxreg;
		dev->cas_reg = cas_reg;
		dev->t2mi_reg = t2mi_reg;

		dev->ops = &taurus_demux_ops;
		dev->max_dmx_unit = TAURUS_MAX_DEMUX_UNIT;
		dev->max_filter   = TAURUS_MAX_FILTER_NUM;
		dev->max_slot     = TAURUS_MAX_SLOT_NUM;
		dev->max_av       = TAURUS_MAX_AVBUF_NUM;
		dev->max_cas      = MAX_CAS_NUM;
		dev->max_t2mi     = TAURUS_MAX_T2MI_PID;
		dev->max_tsw_slot = TAURUS_MAX_SLOT_NUM;
		dev->irqs[0] = DMX_IRQ0;
#ifdef CONFIG_AV_MODULE_GSE
		dev->irqs[2] = TAURUS_GSE_IRQ;
#endif
		demux = &(dev->demuxs[0]);
		demux->id = 0;
		demux->dev = dev;
		demux = &(dev->demuxs[1]);
		demux->id = 1;
		demux->dev = dev;
	}

#ifdef CONFIG_AV_MODULE_GSE
	if (taurus_gse_init(device,inode) < 0)
		return -1;
#endif
	taurus_hw_init(taurus_dmxreg);
	return demux_init(device,inode);
}

int taurus_demux_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = &gxdmxdev[0];
	dev->ops = NULL;
	dev->max_filter = 0;
	dev->max_slot = 0;
	dev->max_cas = 0;
	dev->max_av = 0;
	dev->max_tsw_slot = 0;

	taurus_hw_init(taurus_dmxreg);
	gx_iounmap(dev->reg);
	gx_release_mem_region(GXAV_DEMUX_BASE_SYS, sizeof(struct taurus_reg_demux));
	gx_iounmap(dev->cas_reg);
	gx_release_mem_region(TAURUS_DEMUX_BASE_CAS, sizeof(struct sirius_reg_cas));
	gx_iounmap(dev->t2mi_reg);
	gx_release_mem_region(T2MI_BASE_ADDR, sizeof(struct taurus_reg_t2mi));

#ifdef CONFIG_AV_MODULE_GSE
	taurus_gse_cleanup(device, inode);
#endif
	return demux_cleanup(device,inode);
}

int taurus_demux_open(struct gxav_module *module)
{
	if (module->sub == 2  || module->sub == 3  ||
		module->sub == 6  || module->sub == 7  ||
		module->sub == 9  || module->sub == 10 || module->sub == 11 ||
		module->sub == 13 || module->sub == 14 || module->sub == 15)
		return -1;
	return demux_open(module);
}

int taurus_demux_close(struct gxav_module *module)
{
	if (module->sub == 2  || module->sub == 3  ||
		module->sub == 6  || module->sub == 7  ||
		module->sub == 9  || module->sub == 10 || module->sub == 11 ||
		module->sub == 13 || module->sub == 14 || module->sub == 15)
		return -1;
	return demux_close(module);
}

struct gxav_module_ops taurus_demux_module = {
	.module_type  = GXAV_MOD_DEMUX,
	.count        = 16,
#ifdef CONFIG_AV_MODULE_GSE
	.irqs         = {DMX_IRQ0, TAURUS_GSE_IRQ, -1},
	.irq_names    = {"demux_incident0", "gse"},
#else
	.irqs         = {DMX_IRQ0, -1},
	.irq_names    = {"demux_incident0"},
#endif
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = taurus_demux_open,
	.close        = taurus_demux_close,
	.init         = taurus_demux_init,
	.cleanup      = taurus_demux_cleanup,
	.set_property = demux_set_entry,
	.get_property = demux_get_entry,
	.interrupts[DMX_IRQ0] = demux_irq,
	.interrupts[TAURUS_GSE_IRQ] = demux_irq,
};

