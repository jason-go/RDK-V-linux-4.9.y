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
#include "gx3211_hal_common.h"
#include "sdc_hal.h"
#include "profile.h"

// clear all int status
void gx3211_clr_ints(struct reg_demux *reg)
{
	int i;
	for(i=reg->int_ts_if; i<= reg->int_dmx_cc_en_l; i+=4){
		*(volatile int*)i = 0xffffffff;
	}
}
void gx3211_clear_sram(struct reg_demux *reg)
{
	unsigned long *pid_sram_reg = NULL;
	unsigned int index = 0;

	/*clear PID and match CFGs */
	REG_SET_VAL(&(reg->wen_mask), 0xFFFFFFFF);

	pid_sram_reg = (unsigned long *)(&(reg->pid_en));
	for (index = 0; index < NUM_PID_SRAM; index++) {
		if (firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)) {
			if (((0x2000 + index * 4)  >=  0x3400) && ((0x2000 + index * 4)  <  0x3480 ))
				continue ;
		}
		REG_SET_VAL((pid_sram_reg + index), 0x00000000);
	}

	REG_SET_VAL(&(reg->wen_mask), 0x00000000);
	REG_SET_VAL(&(reg->demux_bigendian),0);
}

void gx3211_set_clock(struct reg_demux *reg)
{
	REG_CLR_BITS(&(reg->stc_cnt), (0xFFFF << 0));
	REG_SET_BITS(&(reg->stc_cnt), 600);              // STC CNT
	REG_SET_BITS(&(reg->stc_cnt), (1 << 16));        // CNT_SET_EN

	REG_CLR_BITS(&(reg->stc_ctrl_1), (0xFFFF << 0));
	REG_SET_BITS(&(reg->stc_ctrl_1), 600);           // STC CNT
	REG_SET_BITS(&(reg->stc_ctrl_1), (1 << 16));     // CNT_SET_EN
}

void gx3211_set_start(struct reg_demux *reg)
{
	REG_CLR_BIT(&(reg->pts_mode), 0);
	REG_SET_VAL(&(reg->df_buf_full_stop_th), 0);
	REG_SET_VAL(&(reg->av_buf_full_stop_th), 0);
	REG_SET_BIT(&(reg->dmux_cfg), BIT_DEMUX_CFG_AUTO_START);
	REG_SET_BIT(&(reg->ts_if_cfg), 12);
	REG_SET_BIT(&(reg->ts_write_ctrl), 0);
	REG_SET_BIT(&(reg->ts_write_ctrl), 1);
	if (firewall_access_align()) {
		REG_SET_VAL(&(reg->axi_8to32_cfg), 1);
	}
}

void gx3211_set_gate(struct dmx_demux *dmx)
{
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;
	unsigned long temp;
	unsigned char timing_gate;
	unsigned char byt_cnt_err_gate;
	unsigned char sync_loss_gate;
	unsigned char sync_lock_gate;

	timing_gate = dmx->time_gate;
	byt_cnt_err_gate = dmx->byt_cnt_err_gate;
	sync_loss_gate = dmx->sync_loss_gate;
	sync_lock_gate = dmx->sync_lock_gate;

	temp = REG_GET_VAL(&(reg->dmux_cfg));
	temp &= (~(DEMUX_CFG_TIMING_GATE | DEMUX_CFG_COUNT_GATE | DEMUX_CFG_LOSS_GATE | DEMUX_CFG_LOCK_GATE));
	temp |= (((timing_gate << BIT_DEMUX_CFG_TIMING_GATE) | (byt_cnt_err_gate << BIT_DEMUX_CFG_COUNT_GATE)
				| (sync_loss_gate << BIT_DEMUX_CFG_LOSS_GATE) | (sync_lock_gate << BIT_DEMUX_CFG_LOCK_GATE))
			& (DEMUX_CFG_TIMING_GATE | DEMUX_CFG_COUNT_GATE | DEMUX_CFG_LOSS_GATE | DEMUX_CFG_LOCK_GATE));
	REG_SET_VAL(&(reg->dmux_cfg), temp);
}


static void check_portlock(struct dmx_demux *dmx,unsigned int port)
{
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;
    unsigned int status;
	switch(port)
	{
		case 0x00:
			REG_SET_VAL(&(reg->int_ts_if),
					1<<BIT_DEMUX_LEVEL_LOCK_PORT1);
			status = REG_GET_VAL(&(reg->int_ts_if));
			if((status >> BIT_DEMUX_LEVEL_LOCK_PORT1) & 0x1) {
				dmx->ts_lock = TS_SYNC_LOCKED;
			} else{
				dmx->ts_lock = TS_SYNC_UNLOCKED;
			}
			break;
		case 0x01:
			REG_SET_VAL(&(reg->int_ts_if),
					1<<BIT_DEMUX_LEVEL_LOCK_PORT2);
			status = REG_GET_VAL(&(reg->int_ts_if));
			if((status >> BIT_DEMUX_LEVEL_LOCK_PORT2) & 0x1) {
				dmx->ts_lock = TS_SYNC_LOCKED;
			} else{
				dmx->ts_lock = TS_SYNC_UNLOCKED;
			}
			break;
		case 0x02:
			REG_SET_VAL(&(reg->int_ts_if),
					1<<BIT_DEMUX_LEVEL_LOCK_PORT3);
			status = REG_GET_VAL(&(reg->int_ts_if));
			if((status >> BIT_DEMUX_LEVEL_LOCK_PORT3) & 0x1) {
				dmx->ts_lock = TS_SYNC_LOCKED;
			} else{
				dmx->ts_lock = TS_SYNC_UNLOCKED;
			}
			break;
		case 0x03:
			REG_SET_VAL(&(reg->int_ts_if),
					1<<BIT_DEMUX_LEVEL_LOCK_PORT4);
			status = REG_GET_VAL(&(reg->int_ts_if));
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

void gx3211_query_tslock(struct dmx_demux *dmx)
{
	unsigned int port = dmx->source;

	check_portlock(dmx,port);
}

void gx3211_link_rts_sdc(struct dmx_demux *dmx,unsigned int start_addr, unsigned int size, unsigned char id)
{
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;
	unsigned int temp;
	REG_SET_BIT(&(reg->int_dmx_tsr_buf_en), 0);
	REG_SET_VAL(&(reg->rts_almost_empty_gate), 128);
	REG_SET_VAL(&(reg->rts_sdc_addr), start_addr);
	REG_SET_VAL(&(reg->rts_sdc_size), size);
	REG_SET_VAL(&(reg->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg->rts_sdc_wptr), 0);

	temp = (1<<BIT_DEMUX_READ_EN)|(20);
	REG_SET_VAL(&(reg->rts_sdc_ctrl), temp);

	if (firewall_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
		REG_SET_VAL(&(reg->tsw_buf[1].tsw_buf_start_addr), start_addr);
		REG_SET_VAL(&(reg->tsw_buf[1].tsw_buf_len),size);
	}
}

void gx3211_unlink_rts_sdc(struct dmx_demux *dmx)
{
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;
	REG_SET_VAL(&(reg->rts_sdc_ctrl), 0);
	REG_SET_VAL(&(reg->rts_sdc_addr), 0);
	REG_SET_VAL(&(reg->rts_sdc_size), 0);
	REG_SET_VAL(&(reg->rts_sdc_rptr), 0);
	REG_SET_VAL(&(reg->rts_sdc_wptr), 0);
	REG_SET_VAL(&(reg->rts_almost_empty_gate), 0);
	REG_SET_VAL(&(reg->int_dmx_tsr_buf_en), 0);

}

int gx3211_wts_cb(unsigned int id, unsigned int len, unsigned int underflow,void *data)
{
	unsigned int tsw_id ;
	unsigned int rptr,wptr;
    struct dmx_demux *dmx = (struct dmx_demux *)data;
    //这里修改可能有问题
	struct demux_fifo *fifo = &(dmx->demux_device->fifo[id-PIN_ID_SDRAM_OUTPUT]);
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;

	tsw_id = fifo->pin_id - PIN_ID_SDRAM_OUTPUT;
	gxav_sdc_rwaddr_get(fifo->channel_id, &rptr,&wptr);
	REG_SET_VAL(&(reg->tsw_buf[tsw_id].tsw_read_addr), rptr);
	wptr = REG_GET_VAL(&(reg->tsw_buf[tsw_id].tsw_write_addr));
	gxav_sdc_Wptr_Set(fifo->channel_id,wptr);

	return 0;
}
int gx3211_rts_cb(unsigned int id, unsigned int len, unsigned int overflow, void *data)
{
	unsigned int sdc_rptr, sdc_wptr, sdc_len;
    struct dmx_demux *dmx = (struct dmx_demux *)data;
	struct demux_fifo *tsr_fifo = &(dmx->demux_device->rfifo);
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;

	gxav_sdc_rwaddr_get(tsr_fifo->channel_id, &sdc_rptr,&sdc_wptr);
	gxav_sdc_length_get(tsr_fifo->channel_id, &sdc_len);
	if(sdc_len >= 188){
		sdc_wptr = sdc_wptr/188*188;
		REG_SET_VAL(&(reg->rts_sdc_wptr), sdc_wptr);
	}

	sdc_rptr = REG_GET_VAL(&(reg->rts_sdc_rptr));
	gxav_sdc_Rptr_Set(tsr_fifo->channel_id, sdc_rptr);

	return 0;
}

void gx3211_link_wts_sdc(struct dmx_demux *dmx,unsigned int start_addr, unsigned int size, unsigned char id)
{
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;
	unsigned int tsw_id = id;
	struct dmx_slot *slot = dmxdev->muxslots[id];

	REG_SET_VAL(&(reg->tsw_buf[tsw_id].tsw_buf_start_addr), start_addr);
	REG_SET_VAL(&(reg->tsw_buf[tsw_id].tsw_buf_len),size);
	REG_SET_VAL(&(reg->tsw_buf[tsw_id].tsw_write_addr),0);
	REG_SET_VAL(&(reg->tsw_buf[tsw_id].tsw_read_addr),0);
	REG_SET_VAL(&(reg->tsw_buf[tsw_id].tsw_almost_full_gate),slot->almost_full_gate);
	if(tsw_id > 32){
		REG_SET_VAL(&(reg->int_tsw_buf_alful_h),1<<(tsw_id-32));
		REG_SET_VAL(&(reg->int_tsw_buf_ful_h),1<<(tsw_id-32));
		REG_SET_BIT(&(reg->int_tsw_buf_alful_en_h),tsw_id-32);
		REG_SET_BIT(&(reg->int_tsw_buf_ful_en_h),tsw_id-32);
	}else{
		REG_SET_VAL(&(reg->int_tsw_buf_alful_l),1<<tsw_id);
		REG_SET_VAL(&(reg->int_tsw_buf_ful_l),1<<tsw_id);
		REG_SET_BIT(&(reg->int_tsw_buf_alful_en_l),tsw_id);
		REG_SET_BIT(&(reg->int_tsw_buf_ful_en_l),tsw_id);

	}
	if(dmx->id == 0){
		if(tsw_id > 32)
			REG_SET_BIT(&(reg->tsw_buf_en_0_h),tsw_id-32);
		else
			REG_SET_BIT(&(reg->tsw_buf_en_0_l),tsw_id);
	}else if(dmx->id == 1){
		if(tsw_id > 32)
			REG_SET_BIT(&(reg->tsw_buf_en_1_h),tsw_id-32);
		else
			REG_SET_BIT(&(reg->tsw_buf_en_1_l),tsw_id);
	}else if(dmx->id == 2){
		if(tsw_id > 32)
			REG_SET_BIT(&(reg->tsw_buf_en_1_h),tsw_id-32);
		else
			REG_SET_BIT(&(reg->tsw_buf_en_1_l),tsw_id);
	}

}

void gx3211_unlink_wts_sdc(struct dmx_demux *dmx,unsigned char id)
{
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;
	unsigned int tsw_id = id;

	if(dmx->id == 0){
		if(tsw_id > 32){
			REG_CLR_BIT(&(reg->tsw_buf_en_0_h),tsw_id-32);
			REG_CLR_BIT(&(reg->int_tsw_buf_alful_en_h),tsw_id-32);
		}else{
			REG_CLR_BIT(&(reg->tsw_buf_en_0_l),tsw_id);
			REG_CLR_BIT(&(reg->int_tsw_buf_alful_en_l),tsw_id);
		}
	}else if(dmx->id == 1){
		if(tsw_id > 32){
			REG_CLR_BIT(&(reg->tsw_buf_en_1_h),tsw_id-32);
			REG_CLR_BIT(&(reg->int_tsw_buf_alful_en_h),tsw_id-32);
		}else{
			REG_CLR_BIT(&(reg->tsw_buf_en_1_l),tsw_id);
			REG_CLR_BIT(&(reg->int_tsw_buf_alful_en_l),tsw_id);
		}

	}else if(dmx->id == 2){
		if(tsw_id > 32){
			REG_CLR_BIT(&(reg->tsw_buf_en_1_h),tsw_id-32);
			REG_CLR_BIT(&(reg->int_tsw_buf_alful_en_h),tsw_id-32);
		}else{
			REG_CLR_BIT(&(reg->tsw_buf_en_1_l),tsw_id);
			REG_CLR_BIT(&(reg->int_tsw_buf_alful_en_l),tsw_id);
		}

	}

	REG_SET_VAL(&(reg->tsw_buf[tsw_id].tsw_read_addr),0);
	REG_SET_VAL(&(reg->tsw_buf[tsw_id].tsw_almost_full_gate),0);
}

void gx3211_set_tsw(struct dmx_demux *dmx,unsigned int slotid)
{
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;
    unsigned int temp;

    temp = REG_GET_VAL(&(reg->ts_write_ctrl));
    if(dmx->id == 0){
	    temp &= ~(0x07 << 8);
	    temp |= (dmx->source << 8);
	    temp &= ~(0x3f << 16);
	    temp |= (slotid << 16);
    } else{
	    temp &= ~(0x07 << 12);
	    temp |= (dmx->source << 12);
	    temp &= ~(0x3f << 24);
	    temp |= (slotid << 24);
    }
    REG_SET_VAL(&(reg->ts_write_ctrl), temp);
}

void gx3211_set_ts_source(struct dmx_demux *dmx)
{
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;
	unsigned int temp;

	temp = REG_GET_VAL(&(reg->dmux_cfg));
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
    REG_SET_VAL(&(reg->dmux_cfg), temp);
}

void gx3211_set_ts_mode(struct dmx_demux *dmx)
{
    struct dmxdev *dmxdev = dmx->demux_device;
    struct reg_demux *reg = dmxdev->reg;
	unsigned int temp;
	enum dmx_stream_mode mode = dmx->stream_mode;

	if (dmx->source == DEMUX_SDRAM) {
		mode = DEMUX_PARALLEL;
	}

	temp = REG_GET_VAL(&(reg->ts_if_cfg));
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
	REG_SET_VAL(&(reg->ts_if_cfg), temp);
}

void gx3211_set_pcr_sync(struct dmx_demux *demux)
{
    struct dmxdev *dmxdev = demux->demux_device;
    struct reg_demux *reg = dmxdev->reg;
	REG_SET_FIELD(&(reg->pcr_cfg), DEMUX_PCR_CFG_PID, demux->pcr_pid, BIT_DEMUX_PCR_CFG_PID);

	if (0 == demux->id) {
		REG_CLR_BIT(&(reg->pcr_cfg), BIT_DEMUX_PCR_CFG_SEL);
	} else if (1 == demux->id) {
		REG_SET_BIT(&(reg->pcr_cfg), BIT_DEMUX_PCR_CFG_SEL);
	}
}

void gx3211_set_descramble(struct dmx_ca *ca, struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
    struct reg_demux *reg = dmxdev->reg;
	int slot_index = slot->id;
	int even_flag = ((ca->flags & DMX_CA_KEY_EVEN) >> 0);
	int odd_flag = ((ca->flags & DMX_CA_KEY_ODD) >> 1);

	REG_SET_BITS(&(reg->wen_mask), DEMUX_PID_CFG_KEY_ID);
	REG_SET_VAL(&(reg->pid_cfg_sel[slot_index].pid_cfg), ca->id << BIT_DEMUX_PID_CFG_KEY_ID);
	REG_CLR_BITS(&(reg->wen_mask), DEMUX_PID_CFG_KEY_ID);

	REG_SET_VAL(&(reg->wen_mask), 0xFFFFFFFF);

	if (ca->even_key_high !=0 ||ca->even_key_low !=0) {
		REG_SET_VAL(&(reg->key_even_odd[ca->id].key_even_high), ca->even_key_high);
		REG_SET_VAL(&(reg->key_even_odd[ca->id].key_even_low), ca->even_key_low);
	}

	if (ca->odd_key_high !=0 ||ca->odd_key_low !=0) {
		REG_SET_VAL(&(reg->key_even_odd[ca->id].key_odd_high), ca->odd_key_high);
		REG_SET_VAL(&(reg->key_even_odd[ca->id].key_odd_low), ca->odd_key_low);
	}

	REG_SET_VAL(&(reg->wen_mask), 0x00000000);

	if (even_flag == 1) {
		REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
		REG_SET_VAL(&(reg->pid_cfg_sel[slot_index].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
		REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_EVEN_KEY_VALID);
	}

	if (odd_flag == 1) {
		REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		REG_SET_VAL(&(reg->pid_cfg_sel[slot_index].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
	}
}

