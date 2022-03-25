#include "fifo.h"
#include "sdc_hal.h"
#include "gxdemux.h"
#include "gx3211_demux.h"
#include "cygnus_regs.h"
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "profile.h"

#define MAX_DNUM   20
static int cygnus_tsw_to_dsp(void *dev, struct cygnus_reg_demux *reg, struct dvr_info *info, unsigned int id)
{
	unsigned int rptr, wptr, hwwptr, gate;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct reg_demux_tsw_buf *tsw_buf = NULL;
	if (id < 16)
		tsw_buf	= &(reg->tsw01_buf[id]);
	else
		tsw_buf	= &(reg->tsw23_buf[id%16]);

	gate = REG_GET_VAL(&(tsw_buf->tsw_almost_full_gate));
redo:
	if (gxav_sdc_rwaddr_get(info->link_channel_id, &rptr,&wptr) < 0)
		return -1;

	hwwptr = REG_GET_VAL(&(tsw_buf->tsw_write_addr));
	if (wptr % 4) {
		REG_SET_VAL(&(tsw_buf->tsw_read_addr), hwwptr);
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
		REG_SET_VAL(&(tsw_buf->tsw_read_addr), hwwptr);

		wptr = REG_GET_VAL(&(tsw_buf->tsw_write_addr));
		if (GX_ABS(wptr - hwwptr) > gate)
			goto redo;
	}
	return 0;
}

static int cygnus_tsw_dealwith(void *dev, struct cygnus_reg_demux *reg, struct dvr_info *info, unsigned int id)
{
	static int d = 0;
	unsigned int fifo_left=0;
	unsigned char *df_addr = NULL;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct reg_demux_tsw_buf *tsw_buf = NULL;
	struct gxfifo *fifo = &info->fifo;
	unsigned int df_pwrite = 0,df_pread = 0,df_size = 0,df_len = 0;
	unsigned int gate = 0;
	unsigned int align = 188;

	if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_TSW))
		align = 188*4;

	if (id < 16)
		tsw_buf	= &(reg->tsw01_buf[id]);
	else
		tsw_buf	= &(reg->tsw23_buf[id%16]);

again:
	df_pwrite = REG_GET_VAL(&(tsw_buf->tsw_write_addr));

	if (info->flags & DVR_FLAG_PTR_MODE_EN)
		df_pread = gxfifo_wptr(fifo);
	else
		df_pread = REG_GET_VAL(&(tsw_buf->tsw_read_addr));
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
			REG_SET_VAL(&(tsw_buf->tsw_read_addr), df_pread);
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
		REG_SET_VAL(&(tsw_buf->tsw_read_addr), df_pread);
		df_pread = REG_GET_VAL(&(tsw_buf->tsw_read_addr));
	}
	df_pwrite = REG_GET_VAL(&(tsw_buf->tsw_write_addr));
	gate = REG_GET_VAL(&(tsw_buf->tsw_almost_full_gate));

	if(df_pwrite >= df_pread)
		df_len = df_pwrite - df_pread;
	else
		df_len = df_pwrite + df_size - df_pread;

	if(df_len > gate/2)
		goto again;

	return 0;
}

void cygnus_tsw_update_rptr(void *dev, int id, unsigned int pread)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct cygnus_reg_demux *reg = dmxdev->reg;
	if (dmxdev->id == 0)
		REG_SET_VAL(&(reg->tsw01_buf[id].tsw_read_addr), pread);
	else
		REG_SET_VAL(&(reg->tsw23_buf[id].tsw_read_addr), pread);
}

int cygnus_tsw_isr_en(void *dev)
{
	int ret = 0;
	unsigned long flags;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct cygnus_reg_demux *reg = dmxdev->reg;
	unsigned int int_tsw_l_en = REG_GET_VAL(&(reg->int_tsw_buf_alful_en_l));
	unsigned int int_tsw_l = REG_GET_VAL(&(reg->int_tsw_buf_alful_l));

	if(int_tsw_l & int_tsw_l_en){
		REG_SET_VAL(&(reg->int_tsw_buf_alful_l),int_tsw_l);
		gx_spin_lock_irqsave(&(dmxdev->spin_lock),flags);
		dmxdev->thread_tsw |= (int_tsw_l & int_tsw_l_en);
		gx_spin_unlock_irqrestore(&(dmxdev->spin_lock),flags);
		ret = 1;
	}

	return ret;
}

void cygnus_tsw_isr(void *dev, int manual_mode)
{
	int i, ret = 0;
	unsigned long flags;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct cygnus_reg_demux *reg = dmxdev->reg;
	struct dmx_dvr *dvr0 = &dmxdev->demuxs[0].dvr;
	struct dmx_dvr *dvr1 = &dmxdev->demuxs[1].dvr;
	long long int_tsw_buf = dmxdev->thread_tsw;
	struct dvr_info *info = NULL;
	unsigned int tsw_en = 0;
	unsigned int* tsw_buf_en = 0;

	for (i = 0; i < dmxdev->max_tsw; i++) {
		if (!(int_tsw_buf & (0x1<<i)))
			continue;

		gx_spin_lock_irqsave(&(dmxdev->spin_lock),flags);
		dmxdev->thread_tsw &=~(0x1<<i);
		gx_spin_unlock_irqrestore(&(dmxdev->spin_lock),flags);

		gxdemux_tsw_lock();
		ret = 0;
		tsw_en = 0;
		info = NULL;
		if (i < 16) {
			if (dvr0->tsw_info[i].status == DVR_STATE_BUSY) {
				info = &dvr0->tsw_info[i];
				tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_0_l), i);

			} else if (dvr1->tsw_info[i].status == DVR_STATE_BUSY) {
				info = &dvr1->tsw_info[i];
				tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_1_l), i);
			}
		} else {
			if (dvr0->tsw_info[i % 16].status == DVR_STATE_BUSY) {
				info = &dvr0->tsw_info[i % 16];
				tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_2_l), i % 16);

			} else if (dvr1->tsw_info[i % 16].status == DVR_STATE_BUSY) {
				info = &dvr1->tsw_info[i % 16];
				tsw_en  = REG_GET_BIT(&(reg->tsw_buf_en_3_l), i % 16);
			}
		}

		if (info && tsw_en) {
			if (info->flags & DVR_FLAG_DVR_TO_DSP)
				ret = cygnus_tsw_to_dsp(dev, reg, info, i);
			else
				ret = cygnus_tsw_dealwith(dev, reg, info, i);
		}
		gxdemux_tsw_unlock();

		if (ret > 0) {
			gx_spin_lock_irqsave(&(dmxdev->spin_lock),flags);
			dmxdev->thread_tsw |= 0x1<<i;
			gx_spin_unlock_irqrestore(&(dmxdev->spin_lock),flags);
		}
	}
}

void cygnus_tsw_full_isr(void *dev)
{
	struct dmx_demux *dmx = NULL;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct cygnus_reg_demux *reg = dmxdev->reg;
	unsigned int int_tsw_full, int_tsw_full_en, i;
	int_tsw_full = REG_GET_VAL(&(reg->int_tsw_buf_ful_l));
	int_tsw_full_en = REG_GET_VAL(&(reg->int_tsw_buf_ful_en_l));
	int_tsw_full = (int_tsw_full & int_tsw_full_en);

	if (!int_tsw_full)
		return;

	REG_SET_VAL(&(reg->int_tsw_buf_ful_l),int_tsw_full);
	for (i = 0; i < dmxdev->max_tsw; i++) {
		unsigned int dmxid = 0;
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

		if (i < 16) {
			REG_CLR_BIT(&(reg->ts_write_ctrl), dmxid);
			REG_SET_VAL(&(reg->tsw01_buf[i].tsw_write_addr),0);
			REG_SET_VAL(&(reg->tsw01_buf[i].tsw_read_addr),0);
			REG_SET_BIT(&(reg->ts_write_ctrl), dmxid);
		} else {
			REG_CLR_BIT(&(reg->ts_write_ctrl_23), dmxid);
			REG_SET_VAL(&(reg->tsw23_buf[i - 16].tsw_write_addr),0);
			REG_SET_VAL(&(reg->tsw23_buf[i - 16].tsw_read_addr),0);
			REG_SET_BIT(&(reg->ts_write_ctrl_23), dmxid);
		}
	}
}

