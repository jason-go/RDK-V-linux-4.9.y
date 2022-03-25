/******************************************************************************
 *
 *  Copyright (C), 2008-2018, Nationalchip Tech. Co., Ltd.
 *
 *******************************************************************************
 *  File Name     : dmx_hal.c
 *  Version       : Initial Draft
 *  Author        : Nationalchip multimedia software group
 *  Created       : 2009/03/18
 *  Last Modified :
 *  Description   : demux module (HAL) interface
 *  Function List :
 *  History       :
 *  1.Date        : 2009/03/18
 *    Author      : zz
 *    Modification: Created file
 *******************************************************************************/
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gx6605_hal_common.h"
#include "sdc_hal.h"
#include "profile.h"
#include "firewall.h"

extern struct reg_demux *gx6605_demux_reg;

void gx6605_clear_sram(void)
{
	unsigned long *pid_sram_reg = NULL;
	unsigned int index = 0;

	/*clear PID and match CFGs */
	REG_SET_VAL(&(gx6605_demux_reg->wen_mask), 0xFFFFFFFF);

	pid_sram_reg = (unsigned long *)(&(gx6605_demux_reg->pidf_en));
	for (index = 0; index < NUM_PID_SRAM; index++) {
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)) {
			if (((0x2000 + index * 4)  >=  0x3400) && ((0x2000 + index * 4)  <  0x3480 ))
				continue ;
		}

		REG_SET_VAL((pid_sram_reg + index), 0x00000000);
	}

	REG_SET_VAL(&(gx6605_demux_reg->wen_mask), 0x00000000);
	REG_SET_VAL(&(gx6605_demux_reg->demux_bigendian),0);
}

void gx6605_set_clock(void)
{
	REG_CLR_BITS(&(gx6605_demux_reg->stc_cnt), (0xFFFF << 0));
	REG_SET_BITS(&(gx6605_demux_reg->stc_cnt), 600);              // STC CNT
	REG_SET_BITS(&(gx6605_demux_reg->stc_cnt), (1 << 16));        // CNT_SET_EN

	REG_CLR_BITS(&(gx6605_demux_reg->stc_ctrl_1), (0xFFFF << 0));
	REG_SET_BITS(&(gx6605_demux_reg->stc_ctrl_1), 600);           // STC CNT
	REG_SET_BITS(&(gx6605_demux_reg->stc_ctrl_1), (1 << 16));     // CNT_SET_EN
}

void gx6605_set_gate(struct dmx_demux *dmx)
{
	unsigned long temp;
	unsigned char timing_gate;
	unsigned char byt_cnt_err_gate;
	unsigned char sync_loss_gate;
	unsigned char sync_lock_gate;

	timing_gate = dmx->time_gate;
	byt_cnt_err_gate = dmx->byt_cnt_err_gate;
	sync_loss_gate = dmx->sync_loss_gate;
	sync_lock_gate = dmx->sync_lock_gate;

	temp = REG_GET_VAL(&(gx6605_demux_reg->dmux_cfg));
	temp &= (~(DEMUX_CFG_TIMING_GATE | DEMUX_CFG_COUNT_GATE | DEMUX_CFG_LOSS_GATE | DEMUX_CFG_LOCK_GATE));
	temp |= (((timing_gate << BIT_DEMUX_CFG_TIMING_GATE) | (byt_cnt_err_gate << BIT_DEMUX_CFG_COUNT_GATE)
				| (sync_loss_gate << BIT_DEMUX_CFG_LOSS_GATE) | (sync_lock_gate << BIT_DEMUX_CFG_LOCK_GATE))
			& (DEMUX_CFG_TIMING_GATE | DEMUX_CFG_COUNT_GATE | DEMUX_CFG_LOSS_GATE | DEMUX_CFG_LOCK_GATE));
	REG_SET_VAL(&(gx6605_demux_reg->dmux_cfg), temp);
}


void gx6605_set_start(void)
{
	REG_SET_BIT(&(gx6605_demux_reg->dmux_cfg), BIT_DEMUX_CFG_AUTO_START);
	REG_SET_BIT(&(gx6605_demux_reg->ts_if_cfg), 12);
	REG_SET_BIT(&(gx6605_demux_reg->ts_write_ctrl), 0);
	REG_SET_BIT(&(gx6605_demux_reg->ts_write_ctrl), 1);
}

static void check_portlock(struct dmx_demux *dmx,unsigned int port)
{
	unsigned int status;
	switch(port)
	{
		case 0x00:
			REG_SET_VAL(&(gx6605_demux_reg->int_ts_if),
					1<<BIT_DEMUX_LEVEL_LOCK_PORT1);
			status = REG_GET_VAL(&(gx6605_demux_reg->int_ts_if));
			if((status >> BIT_DEMUX_LEVEL_LOCK_PORT1) & 0x1) {
				dmx->ts_lock = TS_SYNC_LOCKED;
			} else{
				dmx->ts_lock = TS_SYNC_UNLOCKED;
			}
			break;
		case 0x01:
			REG_SET_VAL(&(gx6605_demux_reg->int_ts_if),
					1<<BIT_DEMUX_LEVEL_LOCK_PORT2);
			status = REG_GET_VAL(&(gx6605_demux_reg->int_ts_if));
			if((status >> BIT_DEMUX_LEVEL_LOCK_PORT2) & 0x1) {
				dmx->ts_lock = TS_SYNC_LOCKED;
			} else{
				dmx->ts_lock = TS_SYNC_UNLOCKED;
			}
			break;
		case 0x02:
			REG_SET_VAL(&(gx6605_demux_reg->int_ts_if),
					1<<BIT_DEMUX_LEVEL_LOCK_PORT3);
			status = REG_GET_VAL(&(gx6605_demux_reg->int_ts_if));
			if((status >> BIT_DEMUX_LEVEL_LOCK_PORT3) & 0x1) {
				dmx->ts_lock = TS_SYNC_LOCKED;
			} else{
				dmx->ts_lock = TS_SYNC_UNLOCKED;
			}
			break;
		case 0x03:
			REG_SET_VAL(&(gx6605_demux_reg->int_ts_if),
					1<<BIT_DEMUX_LEVEL_LOCK_PORT4);
			status = REG_GET_VAL(&(gx6605_demux_reg->int_ts_if));
			if((status >> BIT_DEMUX_LEVEL_LOCK_PORT4) & 0x1) {
				dmx->ts_lock = TS_SYNC_LOCKED;
			} else{
				dmx->ts_lock = TS_SYNC_UNLOCKED;
			}
			break;
		default:
			dmx->ts_lock = TS_SYNC_UNLOCKED;
			break;
	}
}

void gx6605_query_tslock(struct dmx_demux *dmx)
{
	unsigned int port;

	port = dmx->source;
	check_portlock(dmx,port);
}

void gx6605_link_rts_sdc(unsigned int start_addr, unsigned int size, unsigned char id)
{
	unsigned int temp;
	REG_SET_BIT(&(gx6605_demux_reg->int_dmx_tsr_buf_en), 0);
	REG_SET_VAL(&(gx6605_demux_reg->rts_almost_empty_gate), 128);
	REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_addr), start_addr);
	REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_size), size);
	REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_rptr), 0);
	REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_wptr), 0);

	temp = (1<<BIT_DEMUX_READ_EN)|(20);
	REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_ctrl), temp);

}

void gx6605_unlink_rts_sdc(void)
{
	REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_ctrl), 0);
	REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_addr), 0);
	REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_size), 0);
	REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_rptr), 0);
	REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_wptr), 0);
	REG_SET_VAL(&(gx6605_demux_reg->rts_almost_empty_gate), 0);
	REG_SET_VAL(&(gx6605_demux_reg->int_dmx_tsr_buf_en), 0);

}

int gx6605_wts_cb(unsigned int id, unsigned int len, unsigned int underflow,void *data)
{
	unsigned int tsw_id ;
	unsigned int rptr,wptr;
	struct demux_fifo *fifo = (struct demux_fifo *)data;

	tsw_id = fifo->pin_id - PIN_ID_SDRAM_OUTPUT;
	gxav_sdc_rwaddr_get(fifo->channel_id, &rptr,&wptr);
	REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_read_addr), rptr);
	wptr = REG_GET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_write_addr));
	gxav_sdc_Wptr_Set(fifo->channel_id,wptr);

	return 0;
}
int gx6605_rts_cb(unsigned int id, unsigned int len, unsigned int overflow, void *data)
{
	unsigned int sdc_rptr, sdc_wptr, sdc_len;
	struct demux_fifo *tsr_fifo = (struct demux_fifo *)data;

	gxav_sdc_rwaddr_get(tsr_fifo->channel_id, &sdc_rptr,&sdc_wptr);
	gxav_sdc_length_get(tsr_fifo->channel_id, &sdc_len);
	if(sdc_len >= 188){
		sdc_wptr = sdc_wptr/188*188;
		REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_wptr), sdc_wptr);
	}

	//sdc_rptr = REG_GET_VAL(&(gx6605_demux_reg->rts_sdc_rptr));
	//gxav_sdc_Rptr_Set(tsr_fifo->channel_id, sdc_rptr);

	return 0;
}

void gx6605_link_wts_sdc(struct dmx_demux *dmx,unsigned int start_addr, unsigned int size, unsigned char id)
{
	unsigned int tsw_id = id;
	struct dmx_slot *slot = gx6605_dmx.muxslots[id];

	REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_buf_start_addr), start_addr);
	REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_buf_len),size);
	REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_write_addr),0);
	REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_read_addr),0);
	REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_almost_full_gate),slot->almost_full_gate);
	if(tsw_id > 32){
		REG_SET_VAL(&(gx6605_demux_reg->int_tsw_buf_alful_h),1<<(tsw_id-32));
		REG_SET_VAL(&(gx6605_demux_reg->int_tsw_buf_ful_h),1<<(tsw_id-32));
		REG_SET_BIT(&(gx6605_demux_reg->int_tsw_buf_alful_en_h),tsw_id-32);
		REG_SET_BIT(&(gx6605_demux_reg->int_tsw_buf_ful_en_h),tsw_id-32);
	}else{
		REG_SET_VAL(&(gx6605_demux_reg->int_tsw_buf_alful_l),1<<tsw_id);
		REG_SET_VAL(&(gx6605_demux_reg->int_tsw_buf_ful_l),1<<tsw_id);
		REG_SET_BIT(&(gx6605_demux_reg->int_tsw_buf_alful_en_l),tsw_id);
		REG_SET_BIT(&(gx6605_demux_reg->int_tsw_buf_ful_en_l),tsw_id);

	}
	if(dmx->id == 0){
		if(tsw_id > 32)
			REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_0_h),tsw_id-32);
		else
			REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_0_l),tsw_id);
	}else if(dmx->id == 1){
		if(tsw_id > 32)
			REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_1_h),tsw_id-32);
		else
			REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_1_l),tsw_id);
	}else if(dmx->id == 2){
		if(tsw_id > 32)
			REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_1_h),tsw_id-32);
		else
			REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_1_l),tsw_id);
	}

}

void gx6605_unlink_wts_sdc(struct dmx_demux *dmx,unsigned char id)
{
	unsigned int tsw_id = id;

	if(dmx->id == 0){
		if(tsw_id > 32){
			REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_0_h),tsw_id-32);
			REG_CLR_BIT(&(gx6605_demux_reg->int_tsw_buf_alful_en_h),tsw_id-32);
		}else{
			REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_0_l),tsw_id);
			REG_CLR_BIT(&(gx6605_demux_reg->int_tsw_buf_alful_en_l),tsw_id);
		}
	}else if(dmx->id == 1){
		if(tsw_id > 32){
			REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_1_h),tsw_id-32);
			REG_CLR_BIT(&(gx6605_demux_reg->int_tsw_buf_alful_en_h),tsw_id-32);
		}else{
			REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_1_l),tsw_id);
			REG_CLR_BIT(&(gx6605_demux_reg->int_tsw_buf_alful_en_l),tsw_id);
		}

	}else if(dmx->id == 2){
		if(tsw_id > 32){
			REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_1_h),tsw_id-32);
			REG_CLR_BIT(&(gx6605_demux_reg->int_tsw_buf_alful_en_h),tsw_id-32);
		}else{
			REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_1_l),tsw_id);
			REG_CLR_BIT(&(gx6605_demux_reg->int_tsw_buf_alful_en_l),tsw_id);
		}

	}

	REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_read_addr),0);
	REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_almost_full_gate),0);
}

void gx6605_set_ts_source(struct dmx_demux *dmx)
{
	unsigned int temp;

	temp = REG_GET_VAL(&(gx6605_demux_reg->dmux_cfg));
	if(dmx->id == 0){
		switch (dmx->source) {
			case DEMUX_TS1:
				temp = ((temp & ~(DEMUX_CFG_TS_SEL1)) | DEMUX_CFG_DMUX1_SEL_TS1);
				break;
			case DEMUX_TS2:
				temp = ((temp & ~(DEMUX_CFG_TS_SEL1)) | DEMUX_CFG_DMUX1_SEL_TS2);
				break;
			case DEMUX_TS3:
				temp = ((temp & ~(DEMUX_CFG_TS_SEL1)) | DEMUX_CFG_DMUX1_SEL_TS3);
				break;
			case DEMUX_SDRAM:
				temp = ((temp & ~(DEMUX_CFG_TS_SEL1)) | DEMUX_CFG_DMUX1_SEL_SDRAM);
				break;
			default:
				temp = ((temp & ~(DEMUX_CFG_TS_SEL1)) | DEMUX_CFG_DMUX1_SEL_TS1);
				break;
		}
	} else if (dmx->id == 1){
		switch (dmx->source) {
			case DEMUX_TS1:
				temp = ((temp & ~(DEMUX_CFG_TS_SEL2)) | DEMUX_CFG_DMUX2_SEL_TS1);
				break;
			case DEMUX_TS2:
				temp = ((temp & ~(DEMUX_CFG_TS_SEL2)) | DEMUX_CFG_DMUX2_SEL_TS2);
				break;
			case DEMUX_TS3:
				temp = ((temp & ~(DEMUX_CFG_TS_SEL2)) | DEMUX_CFG_DMUX2_SEL_TS3);
				break;
			case DEMUX_SDRAM:
				temp = ((temp & ~(DEMUX_CFG_TS_SEL2)) | DEMUX_CFG_DMUX2_SEL_SDRAM);
				break;
			default:
				temp = ((temp & ~(DEMUX_CFG_TS_SEL2)) | DEMUX_CFG_DMUX2_SEL_TS1);
				break;
		}

	}
	REG_SET_VAL(&(gx6605_demux_reg->dmux_cfg), temp);
	if (dmx->id == 2){
		temp = REG_GET_VAL(&(gx6605_demux_reg->ts_write_ctrl));
		switch (dmx->source) {
			case DEMUX_TS1:
				temp = ((temp & ~(0xf<<4)) | 0x00<<4 | 0x00<<6);
				break;
			case DEMUX_TS2:
				temp = ((temp & ~(0xf<<4)) | 0x01<<4 | 0x01<<6);
				break;
			case DEMUX_TS3:
				temp = ((temp & ~(0xf<<4)) | 0x02<<4 | 0x02<<6);
				break;
			default:
				temp = ((temp & ~(0xf<<4)) | 0x00<<4 | 0x00<<6);
				break;
		}
		REG_SET_VAL(&(gx6605_demux_reg->ts_write_ctrl), temp);
	}
}

void gx6605_set_ts_mode(struct dmx_demux *dmx)
{
	unsigned int temp;
	enum dmx_stream_mode mode = dmx->stream_mode;

	if (dmx->source == DEMUX_SDRAM) {
		mode = DEMUX_PARALLEL;
	}

	temp = REG_GET_VAL(&(gx6605_demux_reg->ts_if_cfg));
	switch (dmx->source) {
		case DEMUX_TS1:
			break;
		case DEMUX_TS2:
			break;
		case DEMUX_TS3:
			break;
		default:
			break;
	}
	REG_SET_VAL(&(gx6605_demux_reg->ts_if_cfg), temp);
}

void gx6605_set_pcr_sync(struct dmx_demux *demux)
{
	REG_SET_FIELD(&(gx6605_demux_reg->pcr_cfg), DEMUX_PCR_CFG_PID, demux->pcr_pid, BIT_DEMUX_PCR_CFG_PID);

	if (0 == demux->id) {
		REG_CLR_BIT(&(gx6605_demux_reg->pcr_cfg), BIT_DEMUX_PCR_CFG_SEL);
	} else if (1 == demux->id) {
		REG_SET_BIT(&(gx6605_demux_reg->pcr_cfg), BIT_DEMUX_PCR_CFG_SEL);
	}
}

void gx6605_mtc_decrypt(gx_mtc_info *mtc_info_cfg,struct dmx_ca *ca)
{

	
}

void gx6605_set_descramble(struct dmx_ca *ca, struct dmx_slot *slot)
{
	int slot_index = slot->id;
	int even_flag = ((ca->flags & DMX_CA_KEY_EVEN) >> 0);
	int odd_flag = ((ca->flags & DMX_CA_KEY_ODD) >> 1);
	int des_flag = ((ca->flags & DMX_CA_DES_MODE) >> 7);
	unsigned int chip = 0;



	if(des_flag == 1){
		REG_SET_BITS(&(gx6605_demux_reg->wen_mask), DEMUX_PID_CFG_KEY_ID);
		REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg), (ca->id) << BIT_DEMUX_PID_CFG_KEY_ID);
		REG_CLR_BITS(&(gx6605_demux_reg->wen_mask), DEMUX_PID_CFG_KEY_ID);
		REG_SET_VAL(&(gx6605_demux_reg->wen_mask), 0xFFFFFFFF);
		REG_SET_VAL(&gx6605_demux_reg->descramble_sel, 1);
		if (ca->even_key_high !=0 || ca->even_key_low !=0) {
			REG_SET_VAL(&(gx6605_demux_reg->key_even_odd[(ca->id)*2+1].key_odd_high), ca->even_key_high);
			REG_SET_VAL(&(gx6605_demux_reg->key_even_odd[(ca->id)*2+1].key_odd_low), ca->even_key_low);
		}

		if (ca->odd_key_high !=0 || ca->odd_key_low !=0) {
			REG_SET_VAL(&(gx6605_demux_reg->key_even_odd[(ca->id)*2].key_odd_high), ca->odd_key_high);
			REG_SET_VAL(&(gx6605_demux_reg->key_even_odd[(ca->id)*2].key_odd_low), ca->odd_key_low);
		}
		REG_SET_VAL(&(gx6605_demux_reg->wen_mask), 0x00000000);
	}else{
		REG_SET_BITS(&(gx6605_demux_reg->wen_mask), DEMUX_PID_CFG_KEY_ID);
		REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg), ((ca->id *2)+1) << BIT_DEMUX_PID_CFG_KEY_ID);
		REG_CLR_BITS(&(gx6605_demux_reg->wen_mask), DEMUX_PID_CFG_KEY_ID);
		REG_SET_VAL(&(gx6605_demux_reg->wen_mask), 0xFFFFFFFF);
		REG_SET_VAL(&gx6605_demux_reg->descramble_sel, 0);
		if (ca->even_key_high !=0 || ca->even_key_low !=0) {
			REG_SET_VAL(&(gx6605_demux_reg->key_even_odd[(ca->id)*2+1].key_even_high), ca->even_key_high);
			REG_SET_VAL(&(gx6605_demux_reg->key_even_odd[(ca->id)*2+1].key_even_low), ca->even_key_low);
		}

		if (ca->odd_key_high !=0 || ca->odd_key_low !=0) {
			REG_SET_VAL((&(gx6605_demux_reg->key_even_odd[(ca->id)*2+1].key_odd_high)-2), ca->odd_key_high);
			REG_SET_VAL((&(gx6605_demux_reg->key_even_odd[(ca->id)*2+1].key_odd_low)-2), ca->odd_key_low);
		}
		REG_SET_VAL(&(gx6605_demux_reg->wen_mask), 0x00000000);
	}

	chip = gxcore_chip_probe();
	if(chip ==  GXAV_ID_GX3201 || chip ==  GXAV_ID_GX6605S){
		if (even_flag == 1) {
			REG_SET_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
			REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
			REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
		}

		if (odd_flag == 1) {
			REG_SET_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
			REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
			REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		}
	}else if (chip == GXAV_ID_GX3113C){
		if (odd_flag == 1 && even_flag == 1) {
			REG_SET_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
			REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
			REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		}else if (odd_flag == 0 && even_flag == 0) {
			REG_SET_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
			REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg), 0);
			REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		}
	}

}

void gx6605_set_mtc_descramble(struct dmx_ca *ca, struct dmx_slot *slot)
{
	int slot_index = slot->id;
	int even_flag = ((ca->flags & MTC_KEY_EVEN));
	int odd_flag = ((ca->flags & MTC_KEY_ODD));

	REG_SET_BITS(&(gx6605_demux_reg->wen_mask), DEMUX_PID_CFG_KEY_ID);
	REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg), ca->id << BIT_DEMUX_PID_CFG_KEY_ID);
	REG_CLR_BITS(&(gx6605_demux_reg->wen_mask), DEMUX_PID_CFG_KEY_ID);

	if (even_flag) {
		REG_SET_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
		REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
		REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
	}

	if (odd_flag) {
		REG_SET_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
	}
}

