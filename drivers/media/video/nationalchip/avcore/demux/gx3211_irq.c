#include "fifo.h"
#include "sdc_hal.h"
#include "gxdemux.h"
#include "gx3211_demux.h"
#include "gx3211_regs.h"
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "profile.h"
#include "gxav_m2m.h"

#define MAX_DNUM   20

static int check_data(struct dmx_filter *filter,unsigned char *data)
{
	int i;
	int eq_flag = ((filter->flags & DMX_EQ) >> 1);
		//gx_printf("filterid %x eg %d depth\n",filter->id,eq_flag,filter->depth);
	for (i = 0; i <= (filter->depth); i++) {
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

/*
 * 获取section的头部18字节用于过滤
 */
#define GET_SECTION_HEAD(head, hw_buf, buf_size, data_offset) \
	do {                                                      \
		if (data_offset + 18 > buf_size){                     \
			memcpy((void *)head, (void *)((unsigned int)hw_buf+data_offset), buf_size-data_offset); \
			memcpy((void *)((unsigned int)head+buf_size-data_offset), (void *)hw_buf, 18-(buf_size-data_offset)); \
		} else \
			memcpy((void *)head, (void *)((unsigned int)hw_buf+data_offset), 18); \
	}while(0)

/*
 * 获取硬件循环buffer数据存放到FIFO缓存buffer中
 */
#define FIFO_PUT_LOOP_BUF(fifo, hw_buf, buf_size, data_offset, data_len) \
	do { \
		if (data_offset + data_len > buf_size) { \
			gxfifo_put(fifo, hw_buf+data_offset, buf_size-data_offset); \
			gxfifo_put(fifo, hw_buf, data_len-(buf_size-data_offset)); \
		} else \
			gxfifo_put(fifo, hw_buf+data_offset, data_len); \
	}while(0)

#define GET_SECTION_LEN(type, section_head) \
	({ \
		unsigned int section_len = 0; \
		if (type == DEMUX_SLOT_PES) \
			section_len = (section_head[4] << 8) + section_head[5] + 6; \
		else \
			section_len = (((section_head[1]) & 0x0F) << 8) + (section_head[2]) + 3 + 1; \
		section_len; \
	})

/*
 * 实现统一slot下多个filter的软件分发
 */
static unsigned int put_data(struct dmx_slot *slot,unsigned char *data,unsigned int df_pwrite, unsigned int df_pread)
{
	int i=0;
	static int d = 0;
	struct dmx_filter *filter = NULL;
	struct gxfifo *fifo = NULL;
	unsigned int pread = 0;
	unsigned char p_head[18] = {0};
	unsigned int sc_len=0,tlen=0, fifo_left = 0, sc_head_len = 3;

	for(i=0;i<MAX_FILTER_NUM;i++){
		filter = slot->filters[i];

		if (NULL == filter || filter->status != DMX_STATE_GO)
			continue;

		pread = df_pread;
		tlen = (df_pwrite + filter->size - df_pread) % filter->size;
		sc_head_len = (filter->type == DEMUX_SLOT_PES) ? 6: 3;

		while (tlen > sc_head_len) {
			GET_SECTION_HEAD((void *)p_head, data, filter->size, pread);
			sc_len = GET_SECTION_LEN(filter->type, p_head);

			if (tlen < sc_len)
				break;

			if (check_data(filter, p_head)) {
				fifo = &filter->fifo;
				fifo_left = gxfifo_freelen(fifo);

				if (sc_len <= fifo_left) {
					FIFO_PUT_LOOP_BUF(fifo, data, filter->size, pread, sc_len);
					filter->flags &= ~(DMX_MAP_OVERFLOW);

					if ((filter->slot->flags & DMX_REPEAT_MODE) == 0)
						filter->status = DMX_STATE_STOP;

				} else {
					filter->flags |= DMX_MAP_OVERFLOW;
					d = (d) % MAX_DNUM;

					if (d == 0)
						gxlog_e(LOG_DEMUX, "d!!! pid 0x%x slotid %d filterid %d\n", slot->pid, slot->id, i);

					d++;
				}
			}

			tlen = tlen-sc_len;
			pread = (pread + sc_len) % filter->size;
		}

		if (filter->flags & DMX_MAP)
			gx_dcache_clean_range((unsigned int)filter->fifo.data, (unsigned int)filter->fifo.data + filter->fifo.size);
	}
	return tlen;
}

static void gx3211_df_dealwith(struct gx3211_reg_demux *reg,struct dmx_filter *filter)
{
	static int d = 0;
	volatile int i = 0;
	unsigned int id = filter->id, fifo_left = 0;
	unsigned int df_pwrite = 0,df_pwrite_rd = 0,df_pread = 0,df_size = 0,df_len = 0,left_len=0;
	struct gxfifo *fifo = &filter->fifo;
	struct dmx_slot *slot = filter->slot;

	if(slot == NULL)
		return;

	df_pwrite = REG_GET_VAL(&(reg->df_waddr[id]));

	for(i=0;i<3;i++){
		df_pread = REG_GET_VAL(&(reg->df_addr[id].df_read_addr));
		df_pwrite_rd = REG_GET_VAL(&(reg->df_waddr[id]));
		if(df_pread != filter->pread)
			return;

		if(df_pwrite != df_pwrite_rd)
			return;
	}

	if(df_pwrite == 0)
		return;

	df_size = filter->size;
	gx_dcache_inv_range((unsigned int)filter->vaddr, (unsigned int)filter->vaddr + df_size);
	fifo_left = gxfifo_freelen(fifo);
	df_len = (df_pwrite + df_size - df_pread) % df_size;

	if (slot->filter_num == 1) {

		if (df_len <= fifo_left) {
			FIFO_PUT_LOOP_BUF(fifo, filter->vaddr, df_size, df_pread, df_len);
			filter->flags &= ~(DMX_MAP_OVERFLOW);

			if ((filter->slot->flags  & DMX_REPEAT_MODE) == 0)
				filter->status = DMX_STATE_STOP;

		} else {
			filter->flags |= DMX_MAP_OVERFLOW;
			d = (d) % MAX_DNUM;

			if (d == 0) {
				gxlog_e(LOG_DEMUX, "d!!! pid 0x%x slotid %d filterid %d\n",slot->pid,slot->id,id);
			}

			d++;
			return;
		}

	} else {
		left_len = put_data(slot, (unsigned char *)filter->vaddr, df_pwrite, df_pread);
		df_len = df_len-left_len;
		// gx_printf("isr id %d len %d\n",filter->id,df_len);
	}

	df_pread =(df_pread + df_len)%df_size;
	REG_SET_VAL(&(reg->df_addr[id].df_read_addr), df_pread);
	filter->pread = df_pread;

	if (filter->flags & DMX_MAP) {
		gx_dcache_clean_range((unsigned int)filter->fifo.data, (unsigned int)filter->fifo.data + filter->fifo.size);
	}
}

int gx3211_df_al_isr(void *dev)
{
	unsigned long flags;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	int i, dmxid = 0, type = 0;
	long long temp = 0,temp1 = 0;
	static int df_isr_num = 0;
	struct dmx_filter *filter = NULL;
	long long int_dmx_df;
	long long int_dmx_df_al_l,int_dmx_df_al_en_l;
	long long int_dmx_df_al_h,int_dmx_df_al_en_h;

	int_dmx_df_al_l    = REG_GET_VAL(&(reg->int_df_buf_alfull_l));
	int_dmx_df_al_en_l = REG_GET_VAL(&(reg->int_df_buf_alfull_en_l));
	int_dmx_df_al_h    = REG_GET_VAL(&(reg->int_df_buf_alfull_h));
	int_dmx_df_al_en_h = REG_GET_VAL(&(reg->int_df_buf_alfull_en_h));

	if (!((int_dmx_df_al_l & int_dmx_df_al_en_l)||(int_dmx_df_al_h & int_dmx_df_al_en_h)))
		return -1;

	int_dmx_df = ((int_dmx_df_al_h & int_dmx_df_al_en_h)<<32)|(int_dmx_df_al_l & int_dmx_df_al_en_l);
	temp1 = (0x01ULL<<df_isr_num);
	if (int_dmx_df & temp1) {
		if(df_isr_num <32)
			REG_SET_VAL(&(reg->int_df_buf_alfull_l), (1<<df_isr_num));
		else
			REG_SET_VAL(&(reg->int_df_buf_alfull_h), (1<<(df_isr_num-32)));

		gx_spin_lock_irqsave(&(dmxdev->df_spin_lock[df_isr_num]), flags);
		dmxid = 0;
		filter = dmxdev->demuxs[0].filters[df_isr_num];
		if(filter == NULL) {
			dmxid = 1;
			filter = dmxdev->demuxs[1].filters[df_isr_num];
		}
		if((filter == NULL) ||
			(filter->status == DMX_STATE_DELAY_FREE) ||
			(filter->status == DMX_STATE_FREE) ||
			(filter->status == DMX_STATE_ALLOCATED)) {
			gx_spin_unlock_irqrestore(&(dmxdev->df_spin_lock[df_isr_num]), flags);
			goto next;
		}

		gx3211_df_dealwith(reg, filter);
		type = filter->type;
		gx_spin_unlock_irqrestore(&(dmxdev->df_spin_lock[df_isr_num]), flags);
		goto set_event;
	}

next:
	for (i = 0; i < MAX_FILTER_NUM; i++) {
		temp = (0x01ULL << i);
		if (int_dmx_df & temp) {
			if(i <32)
				REG_SET_VAL(&(reg->int_df_buf_alfull_l), (1<<i));
			else
				REG_SET_VAL(&(reg->int_df_buf_alfull_h), (1<<(i-32)));

			gx_spin_lock_irqsave(&(dmxdev->df_spin_lock[i]), flags);
			dmxid = 0;
			filter = dmxdev->demuxs[0].filters[i];
			if (filter == NULL) {
				dmxid = 1;
				filter = dmxdev->demuxs[1].filters[i];
			}
			if ((filter == NULL) || (filter->slot == NULL) ||
				(filter->status == DMX_STATE_DELAY_FREE) ||
				(filter->status == DMX_STATE_FREE) ||
				(filter->status == DMX_STATE_ALLOCATED)) {
				gx_spin_unlock_irqrestore(&(dmxdev->df_spin_lock[i]), flags);
				continue;
			}

			gx3211_df_dealwith(reg, filter);
			df_isr_num = i;
			type = filter->type;
			gx_spin_unlock_irqrestore(&(dmxdev->df_spin_lock[i]), flags);
			goto set_event;
		}
	}
	return 0;

set_event:
	if (DEMUX_SLOT_PSI == type)
		return (dmxid == 0) ? EVENT_DEMUX0_FILTRATE_PSI_END : EVENT_DEMUX1_FILTRATE_PSI_END;
	else if (type == DEMUX_SLOT_PES || type == DEMUX_SLOT_PES_AUDIO || type == DEMUX_SLOT_PES_VIDEO)
		return (dmxid == 0) ? EVENT_DEMUX0_FILTRATE_PES_END : EVENT_DEMUX1_FILTRATE_PES_END;
	else
		return 0;
}

int gx3211_df_isr(void *dev)
{
	unsigned long flags;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	int i, dmxid = 0, type = 0;
	long long temp = 0;
	static unsigned int df = 0;
	struct dmx_filter *filter = NULL;
	long long int_dmx_df;
	long long int_dmx_df_l,int_dmx_df_en_l;
	long long int_dmx_df_h,int_dmx_df_en_h;
	int_dmx_df_l    = REG_GET_VAL(&(reg->int_dmx_df_l));
	int_dmx_df_en_l = REG_GET_VAL(&(reg->int_dmx_df_en_l));
	int_dmx_df_h    = REG_GET_VAL(&(reg->int_dmx_df_h));
	int_dmx_df_en_h = REG_GET_VAL(&(reg->int_dmx_df_en_h));

	if (!((int_dmx_df_l & int_dmx_df_en_l)||(int_dmx_df_h & int_dmx_df_en_h)))
		return -1;

	int_dmx_df = ((int_dmx_df_h & int_dmx_df_en_h)<<32) | ((int_dmx_df_l & int_dmx_df_en_l)&0xffffffff);
	for (i = 0; i < MAX_FILTER_NUM; i++) {
		temp = (0x01ULL << df);
		if (int_dmx_df & temp) {
			if(df < 32)
				REG_SET_VAL(&(reg->int_dmx_df_l), (1<<df));
			else
				REG_SET_VAL(&(reg->int_dmx_df_h), (1<<(df-32)));
			gx_spin_lock_irqsave(&(dmxdev->df_spin_lock[df]), flags);
			dmxid = 0;
			filter = dmxdev->demuxs[0].filters[df];
			if(filter == NULL) {
				dmxid = 1;
				filter = dmxdev->demuxs[1].filters[df];
			}
			if ((filter == NULL) || (filter->slot == NULL) ||
				(filter->status == DMX_STATE_DELAY_FREE) ||
				(filter->status == DMX_STATE_FREE) ||
				(filter->status == DMX_STATE_ALLOCATED)) {
				gx_spin_unlock_irqrestore(&(dmxdev->df_spin_lock[df]), flags);
				continue;
			}

			gx3211_df_dealwith(reg, filter);
			type = filter->type;
			gx_spin_unlock_irqrestore(&(dmxdev->df_spin_lock[df]), flags);
			df = (df+1)%MAX_FILTER_NUM;
			goto set_event;
		}
		df = (df+1)%MAX_FILTER_NUM;
	}
	return 0;
set_event:
	if (DEMUX_SLOT_PSI == type)
		return (dmxid == 0) ? EVENT_DEMUX0_FILTRATE_PSI_END : EVENT_DEMUX1_FILTRATE_PSI_END;
	else if (type == DEMUX_SLOT_PES || type == DEMUX_SLOT_PES_AUDIO || type == DEMUX_SLOT_PES_VIDEO)
		return (dmxid == 0) ? EVENT_DEMUX0_FILTRATE_PES_END : EVENT_DEMUX1_FILTRATE_PES_END;
	else
		return 0;
}

void gx3211_av_gate_isr(void *dev)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	unsigned int i;
	unsigned long flags;
	unsigned int av_wptr;
	unsigned int av_rptr;
	unsigned int pts_wptr;
	struct demux_fifo *avfifo=NULL;
	unsigned int int_avbuf,int_avbuf_en;
	int_avbuf    = REG_GET_VAL(&(reg->int_dmx_av_buf));
	int_avbuf_en = REG_GET_VAL(&(reg->int_dmx_av_buf_en));

	if (!((int_avbuf&0xff) & (int_avbuf_en&0xff)))
		return;

	for(i=0;i<MAX_AVBUF_NUM;i++) {
		if(int_avbuf & int_avbuf_en & (0x01<<i)){
			REG_SET_VAL(&(reg->int_dmx_av_buf),1<<i);

			avfifo = &(dmxdev->avfifo[i]);
			gxav_sdc_rwaddr_get(avfifo->channel_id, &av_rptr,&av_wptr);
			REG_SET_VAL(&(reg->av_addr[i].av_read_addr), av_rptr);

			av_wptr = REG_GET_VAL(&(reg->av_addr[i].av_write_addr));
			pts_wptr = REG_GET_VAL(&(reg->pts_w_addr[i]));
			gx_spin_lock_irqsave(&(dmxdev->spin_lock),flags);
			if (REG_GET_VAL(&(reg->int_dmx_av_buf_en)) & (0x01<<i)) {
				gxav_sdc_Wptr_Set(avfifo->channel_id, av_wptr);
				gxav_sdc_Wptr_Set(avfifo->pts_channel_id, pts_wptr);
			}
			gx_spin_unlock_irqrestore(&(dmxdev->spin_lock),flags);
		}
	}
}

void gx3211_av_section_isr(void *dev)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	unsigned long flags;
	unsigned int i;
	unsigned int av_wptr;
	unsigned int av_rptr;
	unsigned int pts_wptr;
	struct demux_fifo *avfifo=NULL;
	unsigned int int_avbuf,int_avbuf_en;
	int_avbuf    = REG_GET_VAL(&(reg->int_dmx_av_buf));
	int_avbuf_en = REG_GET_VAL(&(reg->int_dmx_av_buf_en));

	if (!((int_avbuf>>24) & (int_avbuf_en>>24)))
		return;

	for(i=0;i<8;i++) {
		if((int_avbuf>>24) & (int_avbuf_en>>24) & (0x01<<i)){
			REG_SET_VAL(&(reg->int_dmx_av_buf),1<<(i+24));

			avfifo = &(dmxdev->avfifo[i]);
			gxav_sdc_rwaddr_get(avfifo->channel_id, &av_rptr,&av_wptr);
			REG_SET_VAL(&(reg->av_addr[i].av_read_addr), av_rptr);

			av_wptr = REG_GET_VAL(&(reg->av_addr[i].av_write_addr));
			pts_wptr = REG_GET_VAL(&(reg->pts_w_addr[i]));
			gx_spin_lock_irqsave(&(dmxdev->spin_lock),flags);
			if (REG_GET_VAL(&(reg->int_dmx_av_buf_en)) & (0x01<<i)) {
				gxav_sdc_Wptr_Set(avfifo->channel_id, av_wptr);
				gxav_sdc_Wptr_Set(avfifo->pts_channel_id, pts_wptr);
			}
			gx_spin_unlock_irqrestore(&(dmxdev->spin_lock),flags);
		}
	}
}

int gx3211_dvr2dvr_dealwith(struct dvr_info *info, unsigned int df_pread, unsigned int df_len)
{
	struct dvr_phy_buffer src, dst;
	struct dmx_demux *dmx = get_subdemux(info->dest);
	struct demux_dvr_ops *dvr_ops = dmx->dev->dvr_ops;
	struct dvr_info *tsr_info = &dmx->dvr.tsr_info;
	struct gxfifo *fifo = &info->fifo;
	unsigned int dst_offset, df_size = info->hw_buffer_size;

	gxdemux_dvr_lock();
	src.dvr_handle = info->dvr_handle;
	dst.dvr_handle = tsr_info->dvr_handle;
	dst_offset   = gxfifo_wptr(&dmx->infifo);
	src.vaddr    = info->vaddr;
	src.size     = df_size;
	src.offset   = gxfifo_rptr(fifo);
	src.data_len = df_len;
	dst.vaddr    = tsr_info->vaddr;
	dst.size     = tsr_info->hw_buffer_size;
	dst.offset   = dst_offset;
	dst.data_len = 0;

	if (info->protocol_callback)
		dst.data_len = info->protocol_callback(&src, &dst);
	else
		dst.data_len = df_len;

	if(dst.data_len) {
		gxfifo_wptr_set(&dmx->infifo, (dst_offset+dst.data_len)%dst.size);
		if (tsr_info->status != DVR_STATE_IDLE) {
			if (dvr_ops->tsr_dealwith)
				dvr_ops->tsr_dealwith(dmx->dev, info->dest, info->dest);
		}
	}
	gxdemux_dvr_unlock();

	gxfifo_wptr_set(fifo, df_pread);
	gxfifo_rptr_set(fifo, df_pread);

	return 0;
}

static int gx3211_tsw_to_dsp(void *dev, struct gx3211_reg_demux *reg, struct dvr_info *info, unsigned int id)
{
	unsigned int rptr, wptr, hwwptr, gate;

	gate = REG_GET_VAL(&(reg->tsw_buf[id].tsw_almost_full_gate));
redo:
	if (gxav_sdc_rwaddr_get(info->link_channel_id, &rptr,&wptr) < 0)
		return -1;

	hwwptr = REG_GET_VAL(&(reg->tsw_buf[id].tsw_write_addr));
	if (wptr % 4) {
		REG_SET_VAL(&(reg->tsw_buf[id].tsw_read_addr), hwwptr);
		return 0;
	}

	if (hwwptr != wptr) {
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
			unsigned int align = 188*4;
			unsigned int total_len = 0, split = 0;

			if (hwwptr < wptr) {
				total_len = info->hw_buffer_size + hwwptr - wptr;
				split = info->hw_buffer_size - wptr;
			} else {
				total_len = hwwptr - wptr;
				split = 0;
			}

			if ((total_len = total_len/align*align) < align)
				return 0;

			split = total_len + wptr > info->hw_buffer_size ? split : 0;
			if (split) {
				gxm2m_encrypt(info->trans_temp_buf, info->vaddr+wptr, split);
				gxm2m_decrypt((void *)info->link_vaddr+wptr, info->trans_temp_buf, split);
				hwwptr = wptr + total_len - info->hw_buffer_size;
				wptr = 0;
			} else
				hwwptr = wptr + total_len;
			gxm2m_encrypt(info->trans_temp_buf, info->vaddr+wptr, total_len-split);
			gxm2m_decrypt((void *)info->link_vaddr+wptr, info->trans_temp_buf, total_len-split);
		}

		gxav_sdc_Wptr_Set(info->link_channel_id, hwwptr);
		REG_SET_VAL(&(reg->tsw_buf[id].tsw_read_addr), hwwptr);

		wptr = REG_GET_VAL(&(reg->tsw_buf[id].tsw_write_addr));
		if (GX_ABS(wptr - hwwptr) > gate)
			goto redo;
	}
	return 0;
}

static int gx3211_tsw_dealwith(void *dev, struct gx3211_reg_demux *reg, struct dvr_info *info, unsigned int id)
{
	static int d = 0;
	unsigned int fifo_left=0;
	unsigned char *df_addr = NULL;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gxfifo *fifo = &info->fifo;
	unsigned int df_pwrite = 0,df_pread = 0,df_size = 0,df_len = 0;
	unsigned int gate = 0;
	unsigned int align = 188;

	if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_TSW))
		align = 188*4;

again:
	df_pwrite = REG_GET_VAL(&(reg->tsw_buf[id].tsw_write_addr));

	if (info->flags & DVR_FLAG_PTR_MODE_EN)
		df_pread = gxfifo_wptr(fifo);
	else
		df_pread = REG_GET_VAL(&(reg->tsw_buf[id].tsw_read_addr));
	df_addr = (unsigned char *)((unsigned int)info->vaddr + df_pread);
	gx_dcache_inv_range((unsigned int)info->vaddr, (unsigned int)info->vaddr + info->hw_buffer_size);

	df_size = info->hw_buffer_size;
	fifo_left = gxfifo_freelen(fifo);
	df_len = (df_pwrite >= df_pread) ? (df_pwrite - df_pread) : (df_pwrite + df_size - df_pread);
	df_len = df_len/align*align;

	if (df_len == 0)
		return (df_pwrite > df_pread);
	if (df_len <= fifo_left) {
		if (info->dest >= DVR_OUTPUT_MEM) {
			if (df_pwrite >= df_pread) {
				TSW_FIFO_PUT(fifo, df_addr, df_len, info->flags);

			} else {
				unsigned int split = df_size - df_pread;
				TSW_FIFO_PUT(fifo, df_addr, split, info->flags);
				split = df_len - split;
				if (split) {
					df_addr = (unsigned char *)((unsigned int)info->vaddr);
					TSW_FIFO_PUT(fifo, df_addr, split, info->flags);
				}
			}
		}

	} else {
		if (d==MAX_DNUM)
			gxav_module_inode_set_event(dmxdev->inode, (dmxdev->id) ?
					EVENT_DEMUX1_TS_OVERFLOW : EVENT_DEMUX0_TS_OVERFLOW);
		d=(d)%MAX_DNUM;
		if (d==0)
			gxlog_e(LOG_DEMUX, "d!!! :%d   :%d\n", df_len, fifo_left);

		d++;
		df_pread = df_pwrite/align*align;
		if ((info->flags & DVR_FLAG_PTR_MODE_EN) == 0)
			REG_SET_VAL(&(reg->tsw_buf[id].tsw_read_addr), df_pread);
		if (info->dest < DVR_OUTPUT_MEM) {
			gxfifo_wptr_set(fifo, df_pread);
			gxfifo_rptr_set(fifo, df_pread);
		}

		return 0;
	}

	df_pread = (df_pread + df_len)%df_size;
	if (info->dest < DVR_OUTPUT_MEM)
		gx3211_dvr2dvr_dealwith(info, df_pread, df_len);

	d = 0;
	if ((info->flags & DVR_FLAG_PTR_MODE_EN) == 0) {
		REG_SET_VAL(&(reg->tsw_buf[id].tsw_read_addr), df_pread);
		df_pread = REG_GET_VAL(&(reg->tsw_buf[id].tsw_read_addr));
	}
	df_pwrite = REG_GET_VAL(&(reg->tsw_buf[id].tsw_write_addr));
	gate = REG_GET_VAL(&(reg->tsw_buf[id].tsw_almost_full_gate));

	if(df_pwrite >= df_pread)
		df_len = df_pwrite - df_pread;
	else
		df_len = df_pwrite + df_size - df_pread;

	if(df_len > gate/2)
		goto again;

	return 0;
}

int gx3211_tsw_isr_en(void *dev)
{
	int ret = 0;
	unsigned long flags;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	unsigned int int_tsw_l_en = REG_GET_VAL(&(reg->int_tsw_buf_alful_en_l));
	unsigned int int_tsw_h_en = REG_GET_VAL(&(reg->int_tsw_buf_alful_en_h));
	unsigned int int_tsw_l = REG_GET_VAL(&(reg->int_tsw_buf_alful_l));
	unsigned int int_tsw_h = REG_GET_VAL(&(reg->int_tsw_buf_alful_h));

	if(int_tsw_l & int_tsw_l_en){
		REG_SET_VAL(&(reg->int_tsw_buf_alful_l),int_tsw_l);
		gx_spin_lock_irqsave(&(dmxdev->spin_lock),flags);
		dmxdev->thread_tsw |= (int_tsw_l & int_tsw_l_en);
		gx_spin_unlock_irqrestore(&(dmxdev->spin_lock),flags);
		ret = 1;
	}

	if(int_tsw_h & int_tsw_h_en){
		REG_SET_VAL(&(reg->int_tsw_buf_alful_h),int_tsw_h);
		gx_spin_lock_irqsave(&(dmxdev->spin_lock),flags);
		dmxdev->thread_tsw |= ((long long)(int_tsw_h & int_tsw_h_en))<<32;
		gx_spin_unlock_irqrestore(&(dmxdev->spin_lock),flags);
		ret = 1;
	}

	return ret;
}

void gx3211_tsw_update_rptr(void *dev, int id, unsigned int pread)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	REG_SET_VAL(&(reg->tsw_buf[id].tsw_read_addr), pread);
}

void gx3211_tsw_isr(void *dev, int manual_mode)
{
	int i, ret = 0;
	unsigned long flags;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	struct dmx_dvr *dvr0 = &dmxdev->demuxs[0].dvr;
	struct dmx_dvr *dvr1 = &dmxdev->demuxs[1].dvr;
	long long int_tsw_buf = dmxdev->thread_tsw;
	struct dvr_info *info = NULL;
	unsigned int tsw_en = 0;

	for (i = 0; i < dmxdev->max_tsw; i++) {
		if (manual_mode) {
			unsigned int df_pwrite, df_pread, df_len, df_plen;

			if (dvr0->tsw_info[i].status == DVR_STATE_BUSY) {
				if (i < 32)
					tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_0_l), i);
				else
					tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_0_h), i-32);
			} else {
				if (i < 32)
					tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_1_l), i);
				else
					tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_1_h), i-32);
			}

			if (tsw_en == 0)
				continue;

			df_pwrite = REG_GET_VAL(&(reg->tsw_buf[i].tsw_write_addr));
			df_pread = REG_GET_VAL(&(reg->tsw_buf[i].tsw_read_addr));
			df_plen = REG_GET_VAL(&(reg->tsw_buf[i].tsw_buf_len));

			if (df_pwrite == df_pread)
				continue;
			if (df_pwrite > df_pread)
				df_len = df_pwrite - df_pread;
			else
				df_len = df_pwrite + df_plen - df_pread;
			if (df_len < 188)
				continue;

		} else {
			if (!(int_tsw_buf & (0x1<<i)))
				continue;
		}

		gx_spin_lock_irqsave(&(dmxdev->spin_lock),flags);
		dmxdev->thread_tsw &=~(0x1<<i);
		gx_spin_unlock_irqrestore(&(dmxdev->spin_lock),flags);

		gxdemux_tsw_lock();
		ret = 0;
		tsw_en = 0;
		info = NULL;
		if (dvr0->tsw_info[i].status == DVR_STATE_BUSY) {
			info = &dvr0->tsw_info[i];
			if (i < 32)
				tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_0_l), i);
			else
				tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_0_h), i-32);

		} else if (dvr1->tsw_info[i].status == DVR_STATE_BUSY) {
			info = &dvr1->tsw_info[i];
			if (i < 32)
				tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_1_l), i);
			else
				tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_1_h), i-32);
		}

		if (info && tsw_en) {
			if (info->flags & DVR_FLAG_DVR_TO_DSP)
				ret = gx3211_tsw_to_dsp(dev, reg, info, i);
			else
				ret = gx3211_tsw_dealwith(dev, reg, info, i);
		}
		gxdemux_tsw_unlock();

		if (ret > 0) {
			gx_spin_lock_irqsave(&(dmxdev->spin_lock),flags);
			dmxdev->thread_tsw |= 0x1<<i;
			gx_spin_unlock_irqrestore(&(dmxdev->spin_lock),flags);
		}
	}
}

void gx3211_tsw_full_isr(void *dev)
{
	struct dmx_demux *dmx = NULL;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	unsigned int int_tsw_full, int_tsw_full_en, i;
	int_tsw_full = REG_GET_VAL(&(reg->int_tsw_buf_ful_l));
	int_tsw_full_en = REG_GET_VAL(&(reg->int_tsw_buf_ful_en_l));
	int_tsw_full = (int_tsw_full & int_tsw_full_en);

	if (!int_tsw_full)
		return;

	REG_SET_VAL(&(reg->int_tsw_buf_ful_l),int_tsw_full);
	for (i = 0; i < dmxdev->max_tsw; i++) {
		unsigned int dmxid = 0, align = 188 * 4, value;
		struct dvr_info *info = NULL;

		if ((int_tsw_full & (0x1<<i)) == 0)
			continue;

		dmx = &dmxdev->demuxs[0];
		if (dmx->dvr.tsw_info[i].status == DVR_STATE_BUSY)
			dmxid = 0;
		else
			dmxid = 1;

		dmx = &dmxdev->demuxs[dmxid];
		info = &dmx->dvr.tsw_info[i];
		info->hw_full_count %= 100;
		if (info->hw_full_count == 0)
			gx_printf("%s %x!!!\n", __func__, int_tsw_full);
		info->hw_full_count++;

		REG_CLR_BIT(&(reg->ts_write_ctrl), dmxid);
		value = REG_GET_VAL(&(reg->tsw_buf[i].tsw_write_addr)) / align * align;
		REG_SET_VAL(&(reg->tsw_buf[i].tsw_read_addr), value);
		REG_SET_BIT(&(reg->ts_write_ctrl), dmxid);
	}
}

#include "pegasus_regs.h"
void gx3211_tsr_dealwith(void *dev, int modid, int tsr_id)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	struct gxfifo *fifo = &dmxdev->demuxs[tsr_id%2].infifo;
	unsigned int sdc_wptr,sdc_rptr, sdc_len;
	struct reg_demux_rts_buf *reg_rts_buf = &reg->rts_buf;

	if (fifo->size == 0)
		return;
	gx_dcache_clean_range(0,0);
	sdc_wptr = gxfifo_wptr(fifo);
	sdc_len = gxfifo_len(fifo);
	if(sdc_len >= 188){
		sdc_wptr = sdc_wptr/188*188;
		REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), sdc_wptr);
	}

	sdc_rptr = REG_GET_VAL(&(reg_rts_buf->rts_sdc_rptr));
	gxfifo_rptr_set(fifo, sdc_rptr);
}

void gx3211_tsr_isr(void *dev)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	unsigned int int_dmx_tsr,int_dmx_tsr_en;
	int_dmx_tsr = REG_GET_VAL(&(reg->int_dmx_tsr_buf));
	int_dmx_tsr_en = REG_GET_VAL(&(reg->int_dmx_tsr_buf_en));

	if(!(int_dmx_tsr & int_dmx_tsr_en))
		return;

	if(int_dmx_tsr&0x01) {
		REG_SET_VAL(&(reg->int_dmx_tsr_buf), 1);
		gx3211_tsr_dealwith(dev, 0, 0);
	}
}

void gx3211_check_ts_clk_err_isr(void *dev)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct gx3211_reg_demux *reg = dmxdev->reg;
	unsigned int int_ts_if;
	int_ts_if = REG_GET_VAL(&(reg->int_ts_if));

#if 0
	if (int_ts_if & DEMUX_TS_CLK_ERR)
		gx_printf("Demux ts clk error %#x !!!\n", (int_ts_if & DEMUX_TS_CLK_ERR) >> BIT_DEMUX_TS_CLK_ERR);
#endif
}

