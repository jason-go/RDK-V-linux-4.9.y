/******************************************************************************
 *
 *  Copyright (C), 2008-2018, Nationalchip Tech. Co., Ltd.
 *
 *******************************************************************************
 *  File Name     : slot_hal.c
 *  Version       : Initial Draft
 *  Author        : Nationalchip multimedia software group
 *  Created       : 2008/03/18
 *  Last Modified :
 *  Description   : demux module (HAL) interface
 *  Function List :
 *  History       :
 *  1.Date        : 2008/03/18
 *    Author      : zz
 *    Modification: Created file
 *******************************************************************************/
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "sdc_hal.h"
#include "gx3211_hal_common.h"

void gx3211_disable_slot(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;

	int index = slot->id;
	if(index < 32) {
		REG_SET_BIT(&(reg->wen_mask), index);
		if (0 == slot->demux->id)
			REG_SET_VAL(&(reg->pid_en[0].pid_en_l), 0);
		else if (1 == slot->demux->id)
			REG_SET_VAL(&(reg->pid_en[1].pid_en_l), 0);

		REG_CLR_BIT(&(reg->wen_mask), index);
	}
	else {
		index -= 32;
		REG_SET_BIT(&(reg->wen_mask), index);
		if (0 == slot->demux->id) {
			REG_SET_VAL(&(reg->pid_en[0].pid_en_h), 0);
		} else if (1 == slot->demux->id) {
			REG_SET_VAL(&(reg->pid_en[1].pid_en_h), 0);
		}
		REG_CLR_BIT(&(reg->wen_mask), index);
	}
}

void gx3211_enable_slot(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;

	if(index < 32) {
		REG_SET_BIT(&(reg->wen_mask), index);
		if (0 == slot->demux->id)
			REG_SET_VAL(&(reg->pid_en[0].pid_en_l), (0x1 << index));
		else if (1 == slot->demux->id)
			REG_SET_VAL(&(reg->pid_en[1].pid_en_l), (0x1 << index));

		REG_CLR_BIT(&(reg->wen_mask), index);
	}else{
		index -= 32;
		REG_SET_BIT(&(reg->wen_mask), index);
		if (0 == slot->demux->id)
			REG_SET_VAL(&(reg->pid_en[0].pid_en_h), (0x1 << index));
		else if (1 == slot->demux->id)
			REG_SET_VAL(&(reg->pid_en[1].pid_en_h), (0x1 << index));

		REG_CLR_BIT(&(reg->wen_mask), index);
	}
}

void gx3211_reset_cc(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;

	if(index < 32) {
		REG_CLR_BIT(&(reg->cpu_pid_in_l), index);
	}else{
		index -= 32;
		REG_CLR_BIT(&(reg->cpu_pid_in_h), index);
	}
}

void gx3211_set_stop_unit(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	unsigned int val = 0;
	int repeat_flag = ((slot->flags & DMX_REPEAT_MODE) >> 1);
	if(repeat_flag)
		val = 0;
	else
		val =  DEMUX_PID_CFG_STOP_PER_UNIT;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STOP_PER_UNIT);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), val);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STOP_PER_UNIT);
}

void gx3211_set_interrupt_unit(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_INT_PER_UNIT);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_INT_PER_UNIT);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_INT_PER_UNIT);
}

void gx3211_set_err_discard(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	unsigned int val = 0;
	unsigned int discard_flag = ((slot->flags & DMX_ERR_DISCARD_EN) >> 6);
	if(discard_flag)
		val = DEMUX_PID_CFG_ERR_DISCARD;
	else
		val = 0;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ERR_DISCARD);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), val);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ERR_DISCARD);

}

void gx3211_set_dup_discard(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	unsigned int val = 0;
	unsigned int dup_flag = ((slot->flags & DMX_DUP_DISCARD_EN) >> 10);
	if(dup_flag)
		val = DEMUX_PID_CFG_DUP_DISCARD;
	else
		val = 0;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DUP_DISCARD);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), val);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_DUP_DISCARD);

}

void gx3211_set_av_out(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	unsigned int val = 0;
	unsigned int avout_flag = ((slot->flags & DMX_AVOUT_EN) >> 5);
	if(avout_flag)
		val = DEMUX_PID_CFG_AV_OUT_EN;
	else
		val = 0;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_AV_OUT_EN);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), val);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_AV_OUT_EN);

}

void gx3211_set_ts_out(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int tsout_flag = ((slot->flags & DMX_TSOUT_EN) >> 4);
	int tsdesout_flag = ((slot->flags & DMX_DES_EN) >> 6);
	int index = slot->id;
	unsigned int val = 0 ;
	unsigned int temp;
	if(tsout_flag){
		val = (0x1 << BIT_DEMUX_PID_CFG_TS_OUT_EN);
		if(tsdesout_flag)
			val |= (0x1 << BIT_DEMUX_PID_CFG_TS_DES_OUT_EN);

		REG_SET_FIELD(&(reg->wen_mask), DEMUX_PID_CFG_TSW_BUF_SEL, 0x3f,
				BIT_DEMUX_PID_CFG_TSW_BUF);
		//REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg_l),(slot->tsw-64));
		REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg_l),(slot->tsw));
		REG_SET_FIELD(&(reg->wen_mask), DEMUX_PID_CFG_TSW_BUF_SEL, 0x0,
				BIT_DEMUX_PID_CFG_TSW_BUF);

		temp = REG_GET_VAL(&(reg->ts_write_ctrl));
		if(slot->demux->id == 0){
			temp &= ~(0x07 << 8);
			temp |= (0x3 << 8)|(1<<0);
		}else if (slot->demux->id == 1){
			temp &= ~(0x07 << 12);
			temp |= (0x3 << 12)|(1<<1);
		}
		REG_SET_VAL(&(reg->ts_write_ctrl), temp);


	}else{
		val = 0;
	}
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_OUT_EN);
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_DES_OUT_EN);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), val);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_DES_OUT_EN);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TS_OUT_EN);
}

void gx3211_set_filter_type(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	unsigned int val = 0;

	if(slot->type == DEMUX_SLOT_TS)
		val = (0x1 << BIT_DEMUX_PID_CFG_STORE_TS);
	else
		val = 0;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STORE_TS);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), val);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_STORE_TS);
}

void gx3211_set_crc_disable(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	unsigned int val = 0;
	unsigned int crc_disable_flag = ((slot->flags & DMX_CRC_DISABLE) >> 0);
	if (crc_disable_flag)
		val = (0x1 << BIT_DEMUX_PID_CFG_CRC_DISABLE);
	else
		val = 0;

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_CRC_DISABLE);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), val);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_CRC_DISABLE);

	//REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ERR_DISCARD);
	//REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), 0);
	//REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_ERR_DISCARD);

}

void gx3211_set_type_sel(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);
	if(slot->type == DEMUX_SLOT_PSI){
		REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_FILTER_PSI);
	}else if (slot->type == DEMUX_SLOT_PES || slot->type == DEMUX_SLOT_PES_AUDIO || slot->type == DEMUX_SLOT_PES_VIDEO){
		REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), 0);
	}
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_FILTER_PSI);

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
	switch (slot->type) {
		case DEMUX_SLOT_PSI:
		case DEMUX_SLOT_PES:
		case DEMUX_SLOT_PES_AUDIO:
		case DEMUX_SLOT_PES_VIDEO:
		case DEMUX_SLOT_TS:
			REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), 0x1 << BIT_DEMUX_PID_CFG_TYPE_SEL);
			break;
		case DEMUX_SLOT_AUDIO:
		case DEMUX_SLOT_SPDIF:
		case DEMUX_SLOT_AUDIO_SPDIF:
		case DEMUX_SLOT_VIDEO:
			REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), 0);
			break;
		default:
			break;
	}
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_TYPE_SEL);
}

void gx3211_set_av_type(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	unsigned int avout_flag = ((slot->flags & DMX_AVOUT_EN) >> 5);
	int index = slot->id;

	REG_SET_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_TYPE);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), DEMUX_PID_CFG_TYPE_AUDIO);
	REG_CLR_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_TYPE);
	if((slot->type == DEMUX_SLOT_AUDIO) && (avout_flag == 1)){//音频slot使用对用av_num通道 选择apts恢复配置
		REG_SET_FIELD(&(reg->pts_cfg), DEMUX_PTS_CFG_RECOVER_SEL, slot->avbuf_id,
				BIT_DEMUX_PTS_CFG_RECOVER_SEL);
		REG_SET_FIELD(&(reg->pts_cfg), DEMUX_PTS_CFG_VIEW_SEL, slot->avbuf_id,
				BIT_DEMUX_PTS_CFG_VIEW_SEL);
	}
}

void gx3211_set_pts_bypass(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	unsigned int val = 0;
	unsigned int pts_bypass_flag = ((slot->flags & DMX_PTS_INSERT) >> 2);
	if (pts_bypass_flag) {
		val = 0;
	} else {
		val = (0x1 << BIT_DEMUX_PID_CFG_PTS_BYPASS);
	}

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_BYPASS);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), val);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_BYPASS);
}

void gx3211_set_pts_to_sdram(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	unsigned int val = 0;

	val = (0x1 << BIT_DEMUX_PID_CFG_PTS_TO_SDRAM);

	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_TO_SDRAM);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), val);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_TO_SDRAM);
}

void gx3211_clr_pts_to_sdram(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	REG_SET_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_TO_SDRAM);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), 0);
	REG_CLR_BIT(&(reg->wen_mask), BIT_DEMUX_PID_CFG_PTS_TO_SDRAM);
}

void gx3211_clr_slot_cfg(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	int index = slot->id;
	REG_SET_VAL(&(reg->wen_mask), 0xffffffff);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), 0);
	REG_SET_VAL(&(reg->wen_mask), 0);
}

void gx3211_set_audio_ac3(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	REG_SET_FIELD(&(reg->audio_head_ext), DEMUX_SPDIF_HEAD, SPDIF_HEAD, BIT_DEMUX_SPDIF_HEAD);
	REG_SET_BIT(&(reg->audio_head_ext), BIT_DEMUX_SPDIF_HEAD_FLAG);
}

void gx3211_clear_spdif(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	REG_SET_FIELD(&(reg->audio_head_ext), DEMUX_SPDIF_HEAD, 0, BIT_DEMUX_SPDIF_HEAD);
	REG_CLR_BIT(&(reg->audio_head_ext), BIT_DEMUX_SPDIF_HEAD_FLAG);
}

void gx3211_set_pid(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	unsigned long temp;
	int i;
	int index = slot->id;
	unsigned short pid = slot->pid;

	if(index < 32) {
		REG_SET_BIT(&(reg->wen_mask), index);
		for (i = 0; i < NUM_PID_BIT; i++) {
			temp = (((pid >> (NUM_PID_BIT - i - 1)) & 0x01) << index);
			REG_SET_VAL(&(reg->pid[i].pid_l), temp);
		}
		REG_CLR_BIT(&(reg->wen_mask), index);
	}
	else {
		index -= 32;
		REG_SET_BIT(&(reg->wen_mask), index);
		for (i = 0; i < NUM_PID_BIT; i++) {
			temp = (((pid >> (NUM_PID_BIT - i - 1)) & 0x01) << index);
			REG_SET_VAL(&(reg->pid[i].pid_h), temp);
		}
		REG_CLR_BIT(&(reg->wen_mask), index);
	}
}

void gx3211_set_avnum(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	unsigned long temp;
	int index = slot->id;
	REG_SET_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_NUM);
	temp = (slot->avbuf_id << BIT_DEMUX_PID_CFG_AV_NUM);
	REG_SET_VAL(&(reg->pid_cfg_sel[index].pid_cfg), temp);
	REG_CLR_BITS(&(reg->wen_mask), DEMUX_PID_CFG_AV_NUM);
	if(slot->demux->source == DEMUX_SDRAM)
		REG_SET_BIT(&(reg->rts_sdc_ctrl),( slot->avbuf_id +16 ));
	else
		REG_CLR_BIT(&(reg->rts_sdc_ctrl),( slot->avbuf_id +16 ));
}

void gx3211_link_avbuf(struct dmx_slot *slot, unsigned int start_addr, unsigned int size, unsigned char id)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	unsigned int almostfull;
	unsigned int almostempty;
	int channel_id = (id - 1);

	DMX_DBG("avbuf_size = 0x%x\n", size);

	if ((slot->type < DEMUX_SLOT_AUDIO) || (slot->type > DEMUX_SLOT_VIDEO)){
		DMX_PRINTF("link av buffer buf slot type is other!\n");
		return;
	}
	gxav_sdc_algate_get(channel_id,&almostempty,&almostfull);
	REG_SET_VAL(&(reg->av_addr[slot->avbuf_id].av_start_addr), start_addr);
	REG_SET_VAL(&(reg->av_addr[slot->avbuf_id].av_buf_size), size);
	REG_SET_VAL(&(reg->av_addr[slot->avbuf_id].av_read_addr),0);
	REG_SET_VAL(&(reg->av_addr[slot->avbuf_id].av_write_addr),0);
	REG_SET_BIT(&(reg->av_buf_wptr_clr),slot->avbuf_id);
	REG_CLR_BIT(&(reg->av_buf_wptr_clr),slot->avbuf_id);

	REG_SET_VAL(&(reg->av_addr[slot->avbuf_id].av_almostfull_gate),almostfull);
	REG_SET_BIT(&(reg->int_dmx_av_buf_en),(slot->avbuf_id+24));
	REG_SET_VAL(&(reg->int_dmx_av_buf),1<<(slot->avbuf_id));
	REG_SET_BIT(&(reg->int_dmx_av_buf_en),(slot->avbuf_id));

	DMX_DBG("[demux av link] av_start_addr : 0x%x ,av_end_addr : 0x%x,av_size : 0x%x ,al_full : 0x%x ,buffer id=0x%x\n",
			start_addr,(start_addr + size -1),size,almostfull,id);
}

void gx3211_unlink_avbuf(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	if ((slot->type < DEMUX_SLOT_AUDIO) || (slot->type > DEMUX_SLOT_VIDEO)){
		DMX_PRINTF("link av buffer buf slot type is other!\n");
		return;
	}

	REG_CLR_BIT(&(reg->int_dmx_av_buf_en),(slot->avbuf_id));
	REG_CLR_BIT(&(reg->int_dmx_av_buf_en),(slot->avbuf_id+24));
	REG_CLR_BIT(&(reg->int_dmx_av_buf),(slot->avbuf_id));
	REG_CLR_BIT(&(reg->int_dmx_av_buf),(slot->avbuf_id+24));
	REG_SET_VAL(&(reg->av_addr[slot->avbuf_id].av_read_addr),0);
	REG_SET_VAL(&(reg->av_addr[slot->avbuf_id].av_almostfull_gate),0);
	REG_CLR_BIT(&(reg->rts_sdc_ctrl),( slot->avbuf_id +16 ));
}

void gx3211_link_ptsbuf(struct dmx_slot *slot, unsigned int start_addr, unsigned int size, unsigned char id)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;

	REG_SET_VAL(&(reg->pts_addr[slot->avbuf_id].pts_buf_addr), start_addr);
	REG_SET_VAL(&(reg->pts_addr[slot->avbuf_id].pts_buf_len), size);
	REG_SET_BIT(&(reg->pts_buf_wptr_clr), slot->avbuf_id);
	REG_CLR_BIT(&(reg->pts_buf_wptr_clr), slot->avbuf_id);
	REG_SET_VAL(&(reg->pts_w_addr[slot->avbuf_id]),0);
	DMX_DBG("[demux pts link] pts_start_addr : 0x%x ,pts_end_addr : 0x%x,pts_size : 0x%x ,buffer id=0x%x\n",
			start_addr,(start_addr + size -1),size,id);
}

void gx3211_unlink_ptsbuf(struct dmx_slot *slot)
{
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;

	REG_SET_BIT(&(reg->pts_buf_wptr_clr), slot->avbuf_id);
	REG_CLR_BIT(&(reg->pts_buf_wptr_clr), slot->avbuf_id);
}

