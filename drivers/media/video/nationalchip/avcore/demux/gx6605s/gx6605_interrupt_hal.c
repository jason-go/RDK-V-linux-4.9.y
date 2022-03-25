/******************************************************************************
 *
 *   Copyright (C), 2008-2018, Nationalchip Tech. Co., Ltd.
 *
 ******************************************************************************
 *  File Name     : interrupt_hal.c
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
#include "fifo.h"

#include "gx6605_hal_common.h"
#include "sdc_hal.h"
#include "gx6605_demux.h"
#include "kernelcalls.h"
#include "gxav_bitops.h"

#define MAX_DNUM   20
#define MIN_SECTION_LEN   13
#define MAX_SECTION_LEN   4096

extern struct reg_demux *gx6605_demux_reg;
static volatile long long thread_tsw_l,thread_tsw_full_l;
static unsigned long flag ;

static void gx6605_dmx_stc_isr(unsigned int int_dmx_ts)
{
	if(int_dmx_ts & (1<<24)){
		REG_SET_VAL(&(gx6605_demux_reg->int_dmx_ts), (1<<24));
		gx_printf("enter stc0 lock int\n");
	}
	if(int_dmx_ts & (1<<26)){
		REG_SET_VAL(&(gx6605_demux_reg->int_dmx_ts), (1<<26));
		gx_printf("enter stc0 loss int \n");
	}
	if(int_dmx_ts & (1<<25)){
		REG_SET_VAL(&(gx6605_demux_reg->int_dmx_ts), (1<<25));
		gx_printf("enter stc1 lock int\n");
	}
	if(int_dmx_ts & (1<<27)){
		REG_SET_VAL(&(gx6605_demux_reg->int_dmx_ts), (1<<27));
		gx_printf("enter stc1 loss int \n");
	}
}

static int check_data(struct dmx_filter *filter,unsigned char *data)
{
	int i;
	int eq_flag = ((filter->flags & DMX_EQ) >> 1);
		//gx_printf("filterid %x eg %d depth\n",filter->id,eq_flag,filter->depth);
	for (i = 0; i <= (filter->depth+3); i++) {
		//gx_printf("0x%x 0x%x \n",(data[i] & (filter->key[i].mask)),(filter->key[i].mask & filter->key[i].value));
		if((0 == eq_flag) && (i==5)){
			if((data[i] & (filter->key[i].mask)) == (filter->key[i].mask & filter->key[i].value)){
				return 0;
			}
			continue;
		}
		if((data[i] & (filter->key[i].mask)) != (filter->key[i].mask & filter->key[i].value)){
			//gx_printf("0x%x 0x%x depth %d\n",(data[i] & (filter->key[i].mask)),(filter->key[i].mask & filter->key[i].value),filter->depth);
			return 0;
		}
	}
	return 1;

}
static unsigned int put_data(struct dmx_slot *slot,unsigned char *data,unsigned int len)
{
	int i=0;
	static int d = 0;
	struct dmx_filter *filter = NULL;
	struct gxfifo *fifo = NULL;
	unsigned int fifo_left=0;
	unsigned char *p=NULL;
	unsigned int sc_len=0,tlen=0;
	unsigned int head =0;
	for(i=0;i<MAX_FILTER_NUM;i++){
		filter = slot->filters[i];
		if(filter != NULL && filter->status == DMX_STATE_GO ){
			p=data;
			tlen=len;
			if(slot->type == DEMUX_SLOT_PSI)
				head = 3;
			else if(slot->type == DEMUX_SLOT_PES)
				head = 6;
			while(tlen>head){
				if(slot->type == DEMUX_SLOT_PSI)
					sc_len = (((p[1]) & 0x0F) << 8) + (p[2]) +3 +1;
				else if(slot->type == DEMUX_SLOT_PES)
					sc_len = (((p[4]) & 0x0F) << 8) + (p[5]) +6;
				//gx_printf("len %d sc_len %d\n",len,sc_len);
				if(tlen < sc_len)
					break;
				if(check_data(filter,p)){
					fifo = &filter->section_fifo;
					fifo_left = gxfifo_freelen(fifo);
					if(sc_len <= fifo_left)
						gxfifo_put(fifo, p, sc_len);
					else {
						d=(d)%MAX_DNUM;
						if(d==0)
							gx_printf("d!!! pid 0x%x slotid %d filterid %d\n",filter->slot->pid,filter->slot->id,filter->id);
						d++;
					}
				}
				tlen = tlen-sc_len;
				p = p+sc_len;
			}
		}
	}
	return tlen;
}

static unsigned char tbuf[64*1024];
static void gx6605_dmx_df_dealwith(struct dmx_filter *filter)
{
	static int d = 0;
	unsigned int fifo_left=0;
	unsigned char *df_addr = NULL;
	unsigned int df_pwrite = 0,df_pwrite_rd = 0,df_pread = 0,df_size = 0,df_len = 0,left_len=0;
	volatile int i=0;
	struct gxfifo *fifo = &filter->section_fifo;
	struct dmx_slot *slot = filter->slot;
	if(slot == NULL)
		return;

	df_pwrite = REG_GET_VAL(&(gx6605_demux_reg->df_waddr[(unsigned int)filter->id]));
	for(i=0;i<3;i++){
		df_pread = REG_GET_VAL(&(gx6605_demux_reg->df_addr[(unsigned int)filter->id].df_read_addr));
		df_pwrite_rd = REG_GET_VAL(&(gx6605_demux_reg->df_waddr[(unsigned int)filter->id]));
		if(df_pread != filter->pread_rd)
			return;

		if(df_pwrite != df_pwrite_rd)

			return;
	}

	if(df_pwrite == 0)
		return;

	df_size = filter->buffer_size;
	df_addr = (unsigned char *)((unsigned int)filter->virtual_dfbuf_addr + df_pread);
	gx_dcache_inv_range(0,0);

	fifo_left = gxfifo_freelen(fifo);

	if(df_pwrite >= df_pread){
		df_len = df_pwrite - df_pread;
		if(slot->filter_num == 1){
			if(df_len <= fifo_left){
				if(filter->status == DMX_STATE_GO)
					gxfifo_put(fifo, df_addr, df_len);
			}else{
				d=(d)%MAX_DNUM;
				if(d==0)
					gx_printf("d!!! pid 0x%x slotid %d filterid %d\n",filter->slot->pid,filter->slot->id,filter->id);
				d++;
				return;
			}
		}else{
			left_len = put_data(slot, df_addr, df_len);
			df_len = df_len-left_len;
			// gx_printf("isr id %d len %d\n",filter->id,df_len);
		}

		df_pread +=df_len;
		REG_SET_VAL(&(gx6605_demux_reg->df_addr[(unsigned int)filter->id].df_read_addr), df_pread);
	}
	else{
		df_len = df_size - df_pread;
		gx_memcpy(tbuf, df_addr, df_len);
		df_len = df_pwrite;
		df_addr = (unsigned char *)((unsigned int)filter->virtual_dfbuf_addr);
		gx_memcpy(tbuf+(df_size - df_pread), df_addr, df_len);

		df_len = df_pwrite + df_size - df_pread;
		//gx_printf("table id 0x%x len %d\n",tbuf[0],df_len);
		if(slot->filter_num == 1){
			if(df_len <= fifo_left){
				if(filter->status == DMX_STATE_GO)
					gxfifo_put(fifo, tbuf, df_len);
			}else{
				d=(d)%MAX_DNUM;
				if(d==0)
					gx_printf("d!!! pid 0x%x slotid %d filterid %d\n",filter->slot->pid,filter->slot->id,filter->id);
				d++;
				return;
			}
		}else{
			left_len = put_data(slot, tbuf, df_len);
			df_len = df_len-left_len;
			//gx_printf("r isr id %d len %d\n",filter->id,df_len);
		}

		df_pread =(df_pread + df_len)%df_size;
		REG_SET_VAL(&(gx6605_demux_reg->df_addr[(unsigned int)filter->id].df_read_addr), df_pread);
	}
	filter->pread_rd=df_pread;
}

static void gx6605_dmx_df_al_isr(long long int_dmx_df)
{
	int i;
	long long temp = 0,temp1 = 0;
	static int df_isr_num = 0;
	struct dmx_filter *filter = NULL;
#if 1
	temp1 = (0x01ULL<<df_isr_num);
	if (int_dmx_df & temp1) {
		if(df_isr_num <32)
			REG_SET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_l), (1<<df_isr_num));
		else
			REG_SET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_h), (1<<(df_isr_num-32)));
		filter = gx6605_dmx.filters[df_isr_num];
        if(filter == NULL)//free的接口已经释放,到中断就不处理已经释放的filter
            return;

		gx6605_dmx_df_dealwith(filter);
		goto set_event;
	}
#endif
	for (i = 0; i < MAX_FILTER_NUM; i++) {
		temp = (0x01ULL << i);
		if (int_dmx_df & temp) {
			if(i <32)
				REG_SET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_l), (1<<i));
			else
				REG_SET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_h), (1<<(i-32)));
			filter = gx6605_dmx.filters[i];
			if(filter == NULL)//free的接口已经释放,到中断就不处理已经释放的filter
				return;
			if(filter->slot == NULL)//delayfree的接口已经释放,到中断就不处理已经释放的filter
				return;

			gx6605_dmx_df_dealwith(filter);
			df_isr_num = i;
			goto set_event;
		}
	}

	if (i == MAX_FILTER_NUM)
	{
		return;
	}
set_event:
	if (DEMUX_SLOT_PSI == filter->slot->type)
		filter->event_cb(((filter->slot->demux->id == 0) ? EVENT_DEMUX0_FILTRATE_PSI_END : EVENT_DEMUX1_FILTRATE_PSI_END),1,filter->event_priv);
	else if (filter->slot->type == DEMUX_SLOT_PES || filter->slot->type == DEMUX_SLOT_PES_AUDIO
		    || filter->slot->type == DEMUX_SLOT_PES_VIDEO)
		filter->event_cb(((filter->slot->demux->id == 0) ? EVENT_DEMUX0_FILTRATE_PES_END : EVENT_DEMUX1_FILTRATE_PES_END),1,filter->event_priv);
}

static void gx6605_dmx_df_isr(long long int_dmx_df)
{
	int i;
	long long temp = 0;//,temp1 = 0;
	//static int df_isr_num = 0;
	struct dmx_filter *filter = NULL;
#if 0
	temp1 = (0x01ULL<<df_isr_num);
	if (int_dmx_df & temp1) {
		if(df_isr_num <32)
			REG_SET_VAL(&(gx6605_demux_reg->int_dmx_df_l), (1<<df_isr_num));
		else
			REG_SET_VAL(&(gx6605_demux_reg->int_dmx_df_h), (1<<(df_isr_num-32)));
		filter = gx6605_dmx.filters[df_isr_num];
        if(filter == NULL)//free的接口已经释放,到中断就不处理已经释放的filter
            return;

		gx6605_dmx_df_dealwith(filter);
		goto set_event;
	}
#endif
	for (i = 0; i < MAX_FILTER_NUM; i++) {
		temp = (0x01ULL << i);
		if (int_dmx_df & temp) {
			if(i <32)
				REG_SET_VAL(&(gx6605_demux_reg->int_dmx_df_l), (1<<i));
			else
				REG_SET_VAL(&(gx6605_demux_reg->int_dmx_df_h), (1<<(i-32)));
			filter = gx6605_dmx.filters[i];
			if(filter == NULL)//free的接口已经释放,到中断就不处理已经释放的filter
				return;
			if(filter->slot == NULL)//delayfree的接口已经释放,到中断就不处理已经释放的filter
				return;

			gx6605_dmx_df_dealwith(filter);
			//df_isr_num = i;
			goto set_event;
		}
	}

	if (i == MAX_FILTER_NUM)
	{
		return;
	}
set_event:
	if (DEMUX_SLOT_PSI == filter->slot->type) {
		filter->event_cb(((filter->slot->demux->id == 0) ? EVENT_DEMUX0_FILTRATE_PSI_END : EVENT_DEMUX1_FILTRATE_PSI_END),1,filter->event_priv);
	} else if (filter->slot->type == DEMUX_SLOT_PES || filter->slot->type == DEMUX_SLOT_PES_AUDIO
		    || filter->slot->type == DEMUX_SLOT_PES_VIDEO) {
		filter->event_cb(((filter->slot->demux->id == 0) ? EVENT_DEMUX0_FILTRATE_PES_END : EVENT_DEMUX1_FILTRATE_PES_END),1,filter->event_priv);
	}
}

void gx6605_dmx_avbuf_gate_isr(int int_avbuf)
{
	unsigned int i;
	unsigned int int_avbuf_en = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_av_buf_en));
	unsigned int av_buf_wptr;
	unsigned int av_buf_rptr;
	unsigned int pts_buf_wptr;
	struct demux_fifo *avfifo=NULL;
	struct dmx_slot *avslot=NULL;

	for(i=0;i<8;i++) {
		if(int_avbuf & int_avbuf_en & (0x01<<i)){
			REG_SET_VAL(&(gx6605_demux_reg->int_dmx_av_buf),1<<i);

			avslot = gx6605_dmx.avslots[i];
			if(avslot != NULL)
			{
				avfifo = &gx6605_dmx.avfifo[i];
				gxav_sdc_rwaddr_get(avfifo->channel_id, &av_buf_rptr,&av_buf_wptr);
				REG_SET_VAL(&(gx6605_demux_reg->av_addr[i].av_read_addr), av_buf_rptr);

				av_buf_wptr = REG_GET_VAL(&(gx6605_demux_reg->av_addr[i].av_write_addr));
				gxav_sdc_Wptr_Set(avfifo->channel_id, av_buf_wptr);


				pts_buf_wptr = REG_GET_VAL(&(gx6605_demux_reg->pts_w_addr[i]));
				gx_dcache_inv_range(0, 0);
				gxav_sdc_Wptr_Set(avfifo->pts_channel_id, pts_buf_wptr);
			}
		}
	}
}

void gx6605_dmx_avbuf_pes_isr(int int_avbuf)
{
	unsigned int i,size;//,len;
	unsigned int int_avbuf_en = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_av_buf_en));
	unsigned int av_buf_wptr;
	unsigned int av_buf_rptr;
	unsigned int pts_buf_wptr;
	struct demux_fifo *avfifo=NULL;

	for(i=0;i<8;i++) {
		if((int_avbuf>>24) & (int_avbuf_en>>24) & (0x01<<i)){
			REG_SET_VAL(&(gx6605_demux_reg->int_dmx_av_buf),1<<(i+24));
			avfifo = &gx6605_dmx.avfifo[i];
			gxav_sdc_rwaddr_get(avfifo->channel_id, &av_buf_rptr,&av_buf_wptr);
			REG_SET_VAL(&(gx6605_demux_reg->av_addr[i].av_read_addr), av_buf_rptr);
			av_buf_wptr = REG_GET_VAL(&(gx6605_demux_reg->av_addr[i].av_write_addr));
			av_buf_rptr = REG_GET_VAL(&(gx6605_demux_reg->av_addr[i].av_read_addr));
			size = REG_GET_VAL(&(gx6605_demux_reg->av_addr[i].av_buf_size));
			gxav_sdc_Wptr_Set(avfifo->channel_id, av_buf_wptr);

			pts_buf_wptr = REG_GET_VAL(&(gx6605_demux_reg->pts_w_addr[i]));
			gx_dcache_inv_range(0, 0);
			gxav_sdc_Wptr_Set(avfifo->pts_channel_id, pts_buf_wptr);
		}
	}

}

static void gx6605_dmx_tsw_dealwith(struct dmx_filter *filter)
{
	static int d = 0;
	unsigned int fifo_left=0;
	unsigned char *df_addr = NULL;
	struct gxfifo *fifo = &filter->section_fifo;
	unsigned int df_pwrite = 0,df_pread = 0,df_size = 0,df_len = 0;
	unsigned int gate = 0;
	unsigned int tsw_id = filter->id;

again:
	df_pwrite = REG_GET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_write_addr));
	df_pread = REG_GET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_read_addr));
	df_size = filter->buffer_size;
	df_addr = (unsigned char *)((unsigned int)filter->virtual_dfbuf_addr + df_pread);
	gx_dcache_inv_range(0,0);

	fifo_left = gxfifo_freelen(fifo);
	DMX_DBG("fifo_left %d\n",fifo_left);

	if(df_pwrite >= df_pread){
		df_len = (df_pwrite - df_pread)/188*188;
		DMX_DBG("df_len %d\n",df_len);
		if(df_len <= fifo_left){
			gxfifo_put(fifo, df_addr, df_len);
		}else{
			d=(d)%MAX_DNUM;
			if(d==0)
				gx_printf("d!!! :%d   :%d\n", df_len, fifo_left);
			d++;
			REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_read_addr), df_pwrite/188*188);
			//gx6605_set_filter_reset(filter);
			return;
		}
		df_pread += df_len;
		REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_read_addr), df_pread);
	}
	else{
		df_len = (df_pwrite + df_size - df_pread)/188*188;
		DMX_DBG("df_len %d\n",df_len);
		if(df_len <= fifo_left){
			unsigned int split = df_size - df_pread;
			gxfifo_put(fifo, df_addr, split);
			split = df_len - split;
			df_addr = (unsigned char *)((unsigned int)filter->virtual_dfbuf_addr);
			gxfifo_put(fifo, df_addr, split);
		}else{
			d=(d)%MAX_DNUM;
			if(d==0)
				gx_printf("d!!! :%d   :%d\n", df_len, fifo_left);
			d++;
			REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_read_addr), df_pwrite/188*188);
			//gx6605_set_filter_reset(filter);
			return;
		}
		df_pread =(df_pread + df_len)%df_size;
		REG_SET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_read_addr), df_pread);
	}
	df_pwrite = REG_GET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_write_addr));
	df_pread = REG_GET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_read_addr));
	gate = REG_GET_VAL(&(gx6605_demux_reg->tsw_buf[tsw_id].tsw_almost_full_gate));
	if(df_pwrite >= df_pread)
		df_len = df_pwrite - df_pread;
	else
		df_len = df_pwrite + df_size - df_pread;

	if(df_len > gate/2){
		//gx_printf("gate %d df_len %d \n",gate,df_len);
		goto again;
	}
}


static void gx6605_dmx_tsw_isr(long long int_tsw_buf)
{
	int i;
	long long temp;
	struct dmx_filter *filter = NULL;
	for (i = 0; i < MAX_FILTER_NUM; i++) {
		temp = (0x01ULL << i);
		if (int_tsw_buf & temp) {
			gx_spin_lock_irqsave(&(gx6605_dmx.demux_spin_lock),flag);
			thread_tsw_l &=~temp;
			gx_spin_unlock_irqrestore(&(gx6605_dmx.demux_spin_lock),flag);
			filter = gx6605_dmx.muxfilters[i];
			if(filter == NULL)//free的接口已经释放,到中断就不处理已经释放的filter
				return;

			gx6605_dmx_tsw_dealwith(filter);
			goto set_event;
		}
	}
set_event:
	if(filter)
		filter->event_cb(((filter->slot->demux->id == 0) ? EVENT_DEMUX0_FILTRATE_TS_END : EVENT_DEMUX1_FILTRATE_TS_END),1,filter->event_priv);
}

static void gx6605_dmx_tsw_full_isr(long long int_tsw_buf_full)
{
	int i;
	long long temp;
	struct dmx_filter *filter = NULL;
	for (i = 0; i < MAX_FILTER_NUM; i++) {
		temp = (0x01ULL << i);
		if (int_tsw_buf_full & temp) {
			gx_spin_lock_irqsave(&(gx6605_dmx.demux_spin_lock),flag);
			thread_tsw_full_l &=~temp;
			gx_spin_unlock_irqrestore(&(gx6605_dmx.demux_spin_lock),flag);
			filter = gx6605_dmx.muxfilters[i];
			if(filter == NULL)//free的接口已经释放,到中断就不处理已经释放的filter
				return;

			gx6605_set_filter_reset(filter);
			goto set_event;
		}
	}

	if (i == MAX_FILTER_NUM)
	{
		return;
	}
set_event:
	if(filter)
		filter->event_cb(((filter->slot->demux->id == 0) ? EVENT_DEMUX0_TS_OVERFLOW: EVENT_DEMUX1_TS_OVERFLOW),1,filter->event_priv);
}


static void gx6605_dmx_tsr_isr(unsigned int int_dmx_tsr)
{
	unsigned int sdc_wptr,sdc_rptr, sdc_len;
	unsigned int tsr_rptr;
	struct demux_fifo *tsr_fifo = NULL;

	if(int_dmx_tsr&0x01) {
		tsr_fifo = &gx6605_dmx.rfifo;
		REG_SET_VAL(&(gx6605_demux_reg->int_dmx_tsr_buf), 1);

		gxav_sdc_rwaddr_get(tsr_fifo->channel_id, &sdc_rptr,&sdc_wptr);
		gxav_sdc_length_get(tsr_fifo->channel_id, &sdc_len);
		if(sdc_len >= 188){
			sdc_wptr = sdc_wptr/188*188;
			REG_SET_VAL(&(gx6605_demux_reg->rts_sdc_wptr), sdc_wptr);
		}

		tsr_rptr = REG_GET_VAL(&(gx6605_demux_reg->rts_sdc_rptr));
		gxav_sdc_Rptr_Set(tsr_fifo->channel_id,tsr_rptr);
	}
}

static volatile long long int_tsw_l,int_tsw_l_en;
static volatile long long int_tsw_h,int_tsw_h_en;
static volatile long long int_tsw_full_l,int_tsw_full_l_en;
static volatile long long int_tsw_full_h,int_tsw_full_h_en;
void gx6605_dmx_interrupt(filter_event_cb event_cb,void *priv)
{
	unsigned int int_dmx_ts,int_dmx_ts_en;
	long long int_dmx_df_l,int_dmx_df_en_l;
	long long int_dmx_df_h,int_dmx_df_en_h;
	long long int_dmx_df_al_l,int_dmx_df_al_en_l;
	long long int_dmx_df_al_h,int_dmx_df_al_en_h;

	unsigned int int_avbuf,int_avbuf_en;
	unsigned int int_dmx_tsr,int_dmx_tsr_en;

	struct dmxdev *demux_device = NULL;
	demux_device = &gx6605_dmx;


	int_dmx_ts = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_ts));
	int_dmx_ts_en = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_ts_en));
	if (int_dmx_ts & int_dmx_ts_en) {
		DMX_DBG("int_dmx_ts = 0x%x\n", int_dmx_ts);
		gx6605_dmx_stc_isr(int_dmx_ts);
	}

	int_dmx_df_l = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_df_l));
	int_dmx_df_en_l = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_df_en_l));
	int_dmx_df_h = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_df_h));
	int_dmx_df_en_h = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_df_en_h));

	if ((int_dmx_df_l & int_dmx_df_en_l)||(int_dmx_df_h & int_dmx_df_en_h)) {
		DMX_DBG("int_dmx_df = 0x%x\n", int_dmx_df_l);
		gx6605_dmx_df_isr((((int_dmx_df_h & int_dmx_df_en_h)<<32)|(int_dmx_df_l & int_dmx_df_en_l)));
	}

	int_dmx_df_al_l = REG_GET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_l));
	int_dmx_df_al_en_l = REG_GET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_en_l));
	int_dmx_df_al_h = REG_GET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_h));
	int_dmx_df_al_en_h = REG_GET_VAL(&(gx6605_demux_reg->int_df_buf_alfull_en_h));

	if ((int_dmx_df_al_l & int_dmx_df_al_en_l)||(int_dmx_df_al_h & int_dmx_df_al_en_h)) {
		DMX_DBG("int_dmx_df = 0x%x\n", int_dmx_df_al_l);
		gx6605_dmx_df_al_isr((((int_dmx_df_al_h & int_dmx_df_al_en_h)<<32)|(int_dmx_df_al_l & int_dmx_df_al_en_l)));
	}

	int_avbuf    = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_av_buf));
	int_avbuf_en = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_av_buf_en));
	if ((int_avbuf&0xff) & (int_avbuf_en&0xff)) {
		DMX_DBG("int_avbuf gate= 0x%x\n", int_avbuf);
		gx6605_dmx_avbuf_gate_isr(int_avbuf);
	}
	int_avbuf    = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_av_buf));
	int_avbuf_en = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_av_buf_en));
	if ((int_avbuf>>24) & (int_avbuf_en>>24)) {
		DMX_DBG("int_avbuf pes= 0x%x\n", int_avbuf);
		gx6605_dmx_avbuf_pes_isr(int_avbuf);
	}

	int_tsw_full_l = REG_GET_VAL(&(gx6605_demux_reg->int_tsw_buf_ful_l));
	int_tsw_full_l_en = REG_GET_VAL(&(gx6605_demux_reg->int_tsw_buf_ful_en_l));
	int_tsw_full_h = REG_GET_VAL(&(gx6605_demux_reg->int_tsw_buf_ful_h));
	int_tsw_full_h_en = REG_GET_VAL(&(gx6605_demux_reg->int_tsw_buf_ful_en_h));
	if((int_tsw_full_l & int_tsw_full_l_en)||(int_tsw_full_h & int_tsw_full_h_en)){
		DMX_PRINTF("int_tsw_full_l = 0x%llx\n", int_tsw_full_l);
		DMX_DBG("int_tsw_full_h = 0x%llx\n", int_tsw_full_h);
		REG_SET_VAL(&(gx6605_demux_reg->int_tsw_buf_ful_l),int_tsw_full_l );
		REG_SET_VAL(&(gx6605_demux_reg->int_tsw_buf_ful_h),int_tsw_full_h );
		gx_spin_lock_irqsave(&(gx6605_dmx.demux_spin_lock),flag);
		thread_tsw_full_l |= int_tsw_full_l;
		gx_spin_unlock_irqrestore(&(gx6605_dmx.demux_spin_lock),flag);
		gx_sem_post(&(demux_device->sem_id));
	}

	int_tsw_l_en = REG_GET_VAL(&(gx6605_demux_reg->int_tsw_buf_alful_en_l));
	int_tsw_h_en = REG_GET_VAL(&(gx6605_demux_reg->int_tsw_buf_alful_en_h));
	while(1){
		int_tsw_l = REG_GET_VAL(&(gx6605_demux_reg->int_tsw_buf_alful_l));
		int_tsw_h = REG_GET_VAL(&(gx6605_demux_reg->int_tsw_buf_alful_h));
		if((int_tsw_h & int_tsw_h_en)||(int_tsw_l & int_tsw_l_en)){
			DMX_DBG("int_tsw_l = 0x%llx\n", int_tsw_l);
			DMX_DBG("int_tsw_h = 0x%llx\n", int_tsw_l);
			REG_SET_VAL(&(gx6605_demux_reg->int_tsw_buf_alful_l),int_tsw_l);
			REG_SET_VAL(&(gx6605_demux_reg->int_tsw_buf_alful_h),int_tsw_h);

			gx_spin_lock_irqsave(&(gx6605_dmx.demux_spin_lock),flag);
			thread_tsw_l |= int_tsw_l;
			gx_spin_unlock_irqrestore(&(gx6605_dmx.demux_spin_lock),flag);
			gx_sem_post(&(demux_device->sem_id));
		}
		else
			break;
	}

	int_dmx_tsr = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_tsr_buf));
	int_dmx_tsr_en = REG_GET_VAL(&(gx6605_demux_reg->int_dmx_tsr_buf_en));
	if(int_dmx_tsr & int_dmx_tsr_en){
		DMX_DBG("int_tsr = 0x%x\n", int_dmx_tsr);
		gx6605_dmx_tsr_isr(int_dmx_tsr);
	}
}

void gx6605_demux_read_thread(void* p)
{
	struct dmxdev *dmxdev = NULL;
	dmxdev = &gx6605_dmx;
	while(1){
		DMX_DBG("===========wait \n");
		gx_sem_wait_timeout(&(dmxdev->sem_id),6000);
		if((thread_tsw_full_l & int_tsw_full_l_en)||(int_tsw_full_h & int_tsw_full_h_en)){
			DMX_PRINTF("int_tsw_full_l = 0x%llx\n", thread_tsw_full_l);
			DMX_DBG("int_tsw_full_h = 0x%llx\n", int_tsw_full_h);
			DMX_PRINTF("pw = %x\n",gx6605_demux_reg->tsw_buf[0].tsw_write_addr);
			DMX_PRINTF("pr = %x\n",gx6605_demux_reg->tsw_buf[0].tsw_read_addr);
			gx6605_dmx_tsw_full_isr((int_tsw_full_h<<32)|(thread_tsw_full_l));
		}
		if((int_tsw_h & int_tsw_h_en)||(thread_tsw_l & int_tsw_l_en)){
			DMX_DBG("int_tsw_l = 0x%llx\n", thread_tsw_l);
			DMX_DBG("int_tsw_h = 0x%llx\n", int_tsw_h);
			gx6605_dmx_tsw_isr((int_tsw_h<<32)|(thread_tsw_l));
		}

	}
}
