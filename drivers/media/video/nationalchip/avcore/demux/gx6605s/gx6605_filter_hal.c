/******************************************************************************

  Copyright (C), 2008-2018, Nationalchip Tech. Co., Ltd.

 ******************************************************************************
 File Name     : filter_hal.c
Version       : Initial
Author        : Nationalchip multimedia software group
Created       : 2011/10/13
Last Modified :
Description   : demux module filter(HAL) interface
Function List :
History       :
1.Date        : 2011/10/13
Author      : shenjch
Modification: Created file
 ******************************************************************************/
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gx6605_hal_common.h"
#include "fifo.h"

extern struct reg_demux *gx6605_demux_reg;

void gx6605_set_filter_reset(struct dmx_filter *filter)
{
	unsigned int temp;
	struct gxfifo *fifo = &filter->section_fifo;
	int i = filter->id;
	filter->pread_rd = 0;
	if (DEMUX_SLOT_MUXTS == filter->slot->type) {
#if 1
		if(filter->slot->demux->id == 0){
			if(i < 32)
				REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_0_l),i);
			else
				REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_0_h),i-32);


		}else if(filter->slot->demux->id == 1){
			if(i < 32)
				REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_1_l),i);
			else
				REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_1_h),i-32);

		}else if(filter->slot->demux->id == 2){
			if(i < 32)
				REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_1_l),i);
			else
				REG_CLR_BIT(&(gx6605_demux_reg->tsw_buf_en_1_h),i-32);

		}
#endif

		REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[i].tsw_write_addr),0);
		REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[i].tsw_read_addr),0);

		//if(fifo != NULL)
		//	gxfifo_reset(fifo);
#if 1
		if(filter->slot->demux->id == 0){
			if(i < 32)
				REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_0_l),i);
			else
				REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_0_h),i-32);

		}else if(filter->slot->demux->id == 1){
			if(i < 32)
				REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_1_l),i);
			else
				REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_1_h),i-32);
			
		}else if(filter->slot->demux->id == 2){
			if(i < 32)
				REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_1_l),i);
			else
				REG_SET_BIT(&(gx6605_demux_reg->tsw_buf_en_1_h),i-32);
			
		}
#endif
		return;
	}

	if(i < 32) {
		gx_memset(filter->virtual_dfbuf_addr, 0, filter->buffer_size);

		gx6605_clear_df_rw(i);

		temp = (0x01 << i);
		REG_SET_VAL(&(gx6605_demux_reg->int_df_crc_l), temp);
		REG_SET_VAL(&(gx6605_demux_reg->int_dmx_df_l), temp);
		REG_SET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_l), temp);
	}
	else {
		i -= 32;

		gx_memset(filter->virtual_dfbuf_addr, 0, filter->buffer_size);

		gx6605_clear_df_rw(i+32);

		temp = (0x01 << i);
		REG_SET_VAL(&(gx6605_demux_reg->int_df_crc_h), temp);
		REG_SET_VAL(&(gx6605_demux_reg->int_dmx_df_h), temp);
		REG_SET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_h), temp);
	}
	gx_dcache_clean_range(0,0);

	if(fifo != NULL)
		gxfifo_reset(fifo);
}

void gx6605_clear_df_rw(int index)
{
	REG_SET_VAL(&(gx6605_demux_reg->df_addr[index].df_read_addr), 0);
	REG_SET_VAL(&(gx6605_demux_reg->df_addr[index].df_write_addr), 0);
	REG_SET_VAL(&(gx6605_demux_reg->df_waddr[index]),0);
}

void gx6605_clear_df_buf(int index)
{
}

void gx6605_clear_int_df(int index)
{
	if(index <32 ) {
		REG_CLR_BIT(&(gx6605_demux_reg->int_dmx_df_en_l), index);
		REG_SET_VAL(&(gx6605_demux_reg->int_dmx_df_l), (1<<index));
		REG_SET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_l), (1<<index));
	}
	else {
		index -=32;
		REG_CLR_BIT(&(gx6605_demux_reg->int_dmx_df_en_h), index);
		REG_SET_VAL(&(gx6605_demux_reg->int_dmx_df_h), (1<<index));
		REG_SET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_h), (1<<index));
	}
}

void gx6605_disable_filter(int index)
{

	if(index < 32 ) {
		REG_CLR_BIT(&(gx6605_demux_reg->df_on_l), index);
	}
	else {
		index -=32;
		REG_CLR_BIT(&(gx6605_demux_reg->df_on_h), index);
	}

}

void gx6605_disable_pid_sel(int slot_index, int filter_index)
{
	if(filter_index <32 ) {
		REG_SET_BIT(&(gx6605_demux_reg->wen_mask), filter_index);
		REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_sel_l), 0);
		REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), filter_index);
	}
	else {
		filter_index -=32;
		//REG_SET_BIT(&(gx6605_demux_reg->wen_mask), filter_index);
		//REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_sel_h), 0);
		//REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), filter_index);
	}

}

void gx6605_set_dfbuf(struct dmx_filter *filter)
{
	int index = filter->id;

	REG_SET_VAL(&(gx6605_demux_reg->df_addr[index].df_start_addr), ((filter->phy_dfbuf_addr)));
	REG_SET_VAL(&(gx6605_demux_reg->df_addr[index].df_buf_size), filter->buffer_size);
	if(filter->slot->type == DEMUX_SLOT_PES || filter->slot->type == DEMUX_SLOT_PES_AUDIO
				|| filter->slot->type == DEMUX_SLOT_PES_VIDEO || filter->slot->type == DEMUX_SLOT_TS){
		REG_SET_VAL(&(gx6605_demux_reg->df_addr[index].df_almost_full_gate), (filter->slot->almost_full_gate));
		if(index < 32 )
			REG_SET_BIT(&(gx6605_demux_reg->int_df_buf_alfull_en_l), index);
		else
			REG_SET_BIT(&(gx6605_demux_reg->int_df_buf_alfull_en_h), (index-32));
	}
}

void gx6605_set_select(struct dmx_slot *slot, struct dmx_filter *filter)
{
	int slot_index = slot->id;
	int filter_index = filter->id;
	int i=0;

	if(filter_index < 32 ){
		REG_SET_BIT(&(gx6605_demux_reg->wen_mask), filter_index);
		REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_sel_l), (0x01 << filter_index));
		REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), filter_index);
	}
	else{
		i = filter_index - 32;
		//REG_SET_BIT(&(gx6605_demux_reg->wen_mask), i);
		//REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_sel_h), (0x01 << i));
		//REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), i);
	}

	if (0 == slot->demux->id) {
		if(filter_index < 32 )
			REG_CLR_BIT(&(gx6605_demux_reg->df_sel_l), filter_index);
		else{
			i = filter_index - 32;
			REG_CLR_BIT(&(gx6605_demux_reg->df_sel_h), i);
		}
	} else if (1 == slot->demux->id) {
		if(filter_index <32 )
			REG_SET_BIT(&(gx6605_demux_reg->df_sel_l), filter_index);
		else{
			i = filter_index - 32;
			REG_SET_BIT(&(gx6605_demux_reg->df_sel_h), i);
		}
	}
}

void gx6605_set_match(struct dmx_filter *filter)
{
	unsigned int i, j;
	unsigned long temp;
	int index = filter->id;
	unsigned short filter_depth = filter->depth;
	struct dmx_filter_key *key = filter->key;
	int eq_flag = ((filter->flags & DMX_EQ) >> 1);
	int sw_flag = ((filter->flags & DMX_SW_FILTER) >> 7);

	/*get filtrate depth */
	if (filter_depth > 3)
		filter_depth -= 3;
	if(sw_flag == 1)
		filter_depth = 0;

	if ((filter_depth < VERSION_BYTE_DEPTH) && (0 == eq_flag)) {
		filter_depth = VERSION_BYTE_DEPTH;
	}

	if (index < 32) {
		REG_SET_BIT(&(gx6605_demux_reg->wen_mask), index);

		/*initialize all filtrate bytes */
		for (j = 0; j <= filter_depth; j++) {
			if ((VERSION_BYTE_DEPTH == j) && (0 == eq_flag)) {
				for (i = 0; i < NUM_MATCH_BIT; i++) {
					if ((i == 0) || (i == 1) || (i == 7)) {
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m0_byte_l),
								0x01 << index);
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m1_byte_l),
								0x01 << index);
					} else {
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m0_byte_l),
								0);
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m1_byte_l),
								0);
					}
				}
			} else {
				for (i = 0; i < NUM_MATCH_BIT; i++) {
					REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m0_byte_l), 0x01 << index);
					REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m1_byte_l), 0x01 << index);
				}
			}
		}

		/*set filtrate byte(value) */
		key = filter->key;
		for (j = 0; j <= filter_depth; j++) {
			for (i = 0; i < NUM_MATCH_BIT; i++) {
				if (key->mask & (1 << (7 - i))) {
					if (key->value & (1 << (7 - i))) {
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m0_byte_l), 0);
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m1_byte_l), 0x01 << index);
					} else {
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m0_byte_l), 0x01 << index);
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m1_byte_l), 0);
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
			REG_CLR_BIT(&(gx6605_demux_reg->df_nequal_on_l), index);
		} else {
			REG_SET_BIT(&(gx6605_demux_reg->df_nequal_on_l), index);
		}

		/*set filter depth */
		temp = REG_GET_VAL(&(gx6605_demux_reg->df_depth[index >> 3]));
		temp &= ~(0x0f << ((index & 0x07) << 2));
		temp |= (filter_depth << ((index & 0x07) << 2));
		REG_SET_VAL(&(gx6605_demux_reg->df_depth[index >> 3]), temp);

		REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), index);
	}
	else {
		index -= 32;
#if 0
		REG_SET_BIT(&(gx6605_demux_reg->wen_mask), index);

		/*initialize all filtrate bytes */
		for (j = 0; j <= filter_depth; j++) {
			if ((VERSION_BYTE_DEPTH == j) && (0 == eq_flag)) {
				for (i = 0; i < NUM_MATCH_BIT; i++) {
					if ((i == 0) || (i == 1) || (i == 7)) {
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m0_byte_h),
								0x01 << index);
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m1_byte_h),
								0x01 << index);
					} else {
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m0_byte_h),
								0);
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[VERSION_BYTE_DEPTH].m_byte[i].m1_byte_h),
								0);
					}
				}
			} else {
				for (i = 0; i < NUM_MATCH_BIT; i++) {
					REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m0_byte_h), 0x01 << index);
					REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m1_byte_h), 0x01 << index);
				}
			}
		}

		/*set filtrate byte(value) */
		key = filter->key;
		for (j = 0; j <= filter_depth; j++) {
			for (i = 0; i < NUM_MATCH_BIT; i++) {
				if (key->mask & (1 << (7 - i))) {
					if (key->value & (1 << (7 - i))) {
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m0_byte_h), 0);
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m1_byte_h), 0x01 << index);
					} else {
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m0_byte_h), 0x01 << index);
						REG_SET_VAL(&(gx6605_demux_reg->m_byte_n[j].m_byte[i].m1_byte_h), 0);
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
			REG_CLR_BIT(&(gx6605_demux_reg->df_nequal_on_h), index);
		} else {
			REG_SET_BIT(&(gx6605_demux_reg->df_nequal_on_h), index);
		}

		/*set filter depth */
		temp = REG_GET_VAL(&(gx6605_demux_reg->df_depth[(filter->id) >> 3]));
		temp &= ~(0x0f << (((filter->id) & 0x07) << 2));
		temp |= (filter_depth << (((filter->id) & 0x07) << 2));
		REG_SET_VAL(&(gx6605_demux_reg->df_depth[(filter->id) >> 3]), temp);

		REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), index);
#endif
	}
}

void gx6605_set_int_en(struct dmx_slot *slot, struct dmx_filter *filter)
{
	int slot_index = slot->id;
	int filter_index = filter->id;

	if(filter_index < 32) {
		REG_SET_BIT(&(gx6605_demux_reg->int_dmx_df_en_l), filter_index);
		REG_SET_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_INT_PER_UNIT);
		REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg), DEMUX_PID_CFG_INT_PER_UNIT);
		REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_INT_PER_UNIT);
	}
	else {
		filter_index -= 32;
		//REG_SET_BIT(&(gx6605_demux_reg->int_dmx_df_en_h), filter_index);
		//REG_SET_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_INT_PER_UNIT);
		//REG_SET_VAL(&(gx6605_demux_reg->pid_cfg_sel[slot_index].pid_cfg_h), DEMUX_PID_CFG_INT_PER_UNIT);
		//REG_CLR_BIT(&(gx6605_demux_reg->wen_mask), BIT_DEMUX_PID_CFG_INT_PER_UNIT);
	}
}

void gx6605_set_filtrate(struct dmx_filter *filter)
{
	struct dmx_slot *slot = filter->slot;

	if ((filter->key[1].mask != 0) || (filter->key[2].mask != 0)) {
		return;
	}

	gx6605_set_match(filter);
	gx6605_set_int_en(slot, filter);
}

void gx6605_enable_filter(struct dmx_filter *filter)
{
	int index = filter->id;
	if(index < 32)
		REG_SET_BIT(&(gx6605_demux_reg->df_on_l), index);
	else {
		index -= 32;
		REG_SET_BIT(&(gx6605_demux_reg->df_on_h), index);
	}
}

int gx6605_filter_ts_status(struct dmx_filter *filter)
{
	unsigned int buffer_write_addr;
	int index = filter->id;

	buffer_write_addr = REG_GET_VAL(&(gx6605_demux_reg->df_waddr[index]));
	if (buffer_write_addr)
		return 0;
	else
		return -1;
}

unsigned int gx6605_filter_ts_get(struct dmx_filter *filter, void *data_buf, unsigned int size)
{
	unsigned int actual_size;

	if (size >= filter->buffer_size)
		actual_size = filter->buffer_size;
	else
		return 0;

	gx_copy_to_user(data_buf, filter->virtual_dfbuf_addr, actual_size);

	return actual_size;
}
