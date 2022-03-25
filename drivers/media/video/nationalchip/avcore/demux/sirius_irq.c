#include "fifo.h"
#include "sdc_hal.h"
#include "gxdemux.h"
#include "sirius_regs.h"
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "profile.h"

void sirius_tsr_dealwith(void *dev, int modid, int tsr_id)
{
	int i;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct dmx_demux *dmx = &dmxdev->demuxs[modid%2];
	struct dvr_info *info = &dmx->dvr.tsr_info;
	struct gxfifo *fifo = &dmx->infifo;
	struct sirius_reg_demux *reg = dmxdev->reg;
	struct reg_demux_rts_buf *reg_rts_buf = (modid%2) ? &reg->rts_buf1 : &reg->rts_buf;
	unsigned int sdc_wptr,sdc_rptr, sdc_len;
	int mask = info->tsr_share_mask;
	unsigned int vaddr = 0;

	if (fifo->size == 0)
		return;

	vaddr = gx_phys_to_virt(reg_rts_buf->rts_sdc_addr);
	gx_dcache_clean_range(vaddr, vaddr+reg_rts_buf->rts_sdc_size);
	sdc_wptr = gxfifo_wptr(fifo);
	sdc_len = gxfifo_len(fifo);

	if (sdc_len >= 188) {
		sdc_wptr = sdc_wptr/188*188;
		REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), sdc_wptr);
	}
	sdc_rptr = REG_GET_VAL(&(reg_rts_buf->rts_sdc_rptr));
	gxfifo_rptr_set(fifo, sdc_rptr);
	if (sdc_len < 188)
		return;

	for (i = 0; i < MAX_DVR_UNIT; i++) {
		if ((mask & (1<<i)) == 0)
			continue;
		reg_rts_buf = (i % 2) ? &reg->rts_buf1 : &reg->rts_buf;
		REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), sdc_wptr);
	}
}

void sirius_tsr_isr(void *dev)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct sirius_reg_demux *reg = dmxdev->reg;
	unsigned int int_dmx_tsr;
	int_dmx_tsr = REG_GET_VAL(&(reg->int_dmx_tsr_buf));

	if(int_dmx_tsr&0x1) {
		REG_SET_BIT(&(reg->int_dmx_tsr_buf), 1);
		sirius_tsr_dealwith(dev, 0, 0);
	}

	if(int_dmx_tsr&(0x1<<16)) {
		REG_SET_BIT(&(reg->int_dmx_tsr_buf), 16);
		sirius_tsr_dealwith(dev, 0, 1);
	}
}

void sirius_gse_full_isr(void *dev)
{
	int i;
	unsigned int df_lread;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct sirius_reg_gse *reg = dmxdev->gse_reg;
	unsigned int int_buf_full = REG_GET_VAL(&(reg->int_buf_full));
	unsigned int int_buf_full_en = REG_GET_VAL(&(reg->int_buf_full_en));

	if (!(int_buf_full & int_buf_full_en))
		return;

	REG_SET_VAL(&(reg->int_buf_full), int_buf_full);
	for (i = 0; i < dmxdev->max_gse_slot; i++) {
		if (int_buf_full & (0x01 << i)) {
			df_lread = REG_GET_VAL(&(reg->filters.data_buf[i].logic_read_ptr));
			REG_SET_VAL(&(reg->filters.data_buf[i].logic_read_ptr), df_lread ? (df_lread-8):df_lread);
			return;
		}
	}
}

static void _set_slot_finish_bit(struct dmxdev *dev, int bit)
{
	unsigned long flags;
	gx_spin_lock_irqsave(&(dev->gse_spin_lock),flags);
	SET_MASK(dev->thread_slot_finish, bit);
	gx_spin_unlock_irqrestore(&(dev->gse_spin_lock),flags);
	gxlog_d(LOG_DEMUX, "GSE slot finish %d\n", bit);
}

void sirius_gse_slot_finish_isr(void *dev)
{
	int i;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct sirius_reg_gse *reg = dmxdev->gse_reg;
	unsigned int int_slot_close  = REG_GET_VAL(&(reg->int_slot_close_finish));
	unsigned int int_slot_close_en = REG_GET_VAL(&(reg->int_slot_close_finish_en));

	if (!(int_slot_close & int_slot_close_en))
		return;

	REG_SET_VAL(&(reg->int_slot_close_finish), int_slot_close);
	for (i = 0; i < dmxdev->max_gse_slot; i++) {
		if (dmxdev->thread_slot_finish & 0x1<<i)
			continue;
		if (int_slot_close & (0x01 << i))
			_set_slot_finish_bit(dmxdev, i);
	}
}

void sirius_gse_record_finish_isr(void *dev)
{
	int record_sel;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct sirius_reg_gse *reg = dmxdev->gse_reg;
	unsigned int int_over_time = REG_GET_VAL(&(reg->int_over_time));
	unsigned int int_record    = int_over_time & GSE_RECORD_FINISH;
	unsigned int int_record_en = int_over_time & GSE_RECORD_FINISH_EN;

	if (!(int_record && int_record_en))
		return;

	REG_SET_BIT(&(reg->int_over_time), BIT_GSE_RECORD_FINISH);

	record_sel = REG_GET_FIELD(&(reg->gse_ctrl), GSE_RECORD_BUF_SEL, BIT_GSE_RECORD_BUF_SEL);
	if (dmxdev->thread_slot_finish & 0x1<<record_sel)
		return;
	_set_slot_finish_bit(dmxdev, record_sel);
}

int sirius_gse_al_isr_en(void *dev)
{
	unsigned long flags;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct sirius_reg_gse *reg = dmxdev->gse_reg;
	unsigned int int_almost_full = REG_GET_VAL(&(reg->int_almost_full));
	unsigned int int_almost_full_en = REG_GET_VAL(&(reg->int_almost_full_en));

	if (int_almost_full & int_almost_full_en) {
		REG_SET_VAL(&(reg->int_almost_full), int_almost_full);
		gx_spin_lock_irqsave(&(dmxdev->gse_spin_lock),flags);
		dmxdev->thread_almost_full |= int_almost_full;
		gx_spin_unlock_irqrestore(&(dmxdev->gse_spin_lock),flags);
		return 1;
	}
	return 0;
}

int sirius_gse_pdu_isr_en(void *dev)
{
	unsigned long flags;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct sirius_reg_gse *reg = dmxdev->gse_reg;
	unsigned int int_pdu_finish = REG_GET_VAL(&(reg->int_pdu_finish));
	unsigned int int_pdu_finish_en = REG_GET_VAL(&(reg->int_pdu_finish_en));

	if (int_pdu_finish & int_pdu_finish_en) {
		REG_SET_VAL(&(reg->int_pdu_finish), int_pdu_finish);
		gx_spin_lock_irqsave(&(dmxdev->gse_spin_lock),flags);
		dmxdev->thread_pdu_finish |= int_pdu_finish;
		gx_spin_unlock_irqrestore(&(dmxdev->gse_spin_lock),flags);
		return 1;
	}
	return 0;
}

static void sirius_gse_data_dealwith(struct dmxdev *dev, struct dmx_gse_slot *slot)
{
	static int d = 0;
	struct gxfifo *fifo    = &slot->fifo;
	unsigned char *df_addr = NULL;
	unsigned int fifo_left = 0, id = slot->id, df_len = 0;
	unsigned int df_pwrite = 0,df_pread = 0,df_size = 0, df_lwrite = 0,df_lread = 0;
	unsigned int df_start_addr = (unsigned int)slot->vaddr_data;
	struct sirius_reg_gse *reg = dev->gse_reg;

	gx_dcache_inv_range(df_start_addr, df_start_addr + slot->hw_buffer_size);
	df_pwrite = REG_GET_VAL(&(reg->filters.data_buf[id].phy_write_ptr));
	df_lwrite = REG_GET_VAL(&(reg->filters.data_buf[id].logic_write_ptr));
	df_lread  = REG_GET_VAL(&(reg->filters.data_buf[id].logic_read_ptr));
	df_size   = slot->hw_buffer_size;
	df_pread  = df_lread % df_size;
	fifo_left = gxfifo_freelen(fifo);

	if (df_pwrite < df_pread)
		df_len = df_pwrite + df_size - df_pread;
	else
		df_len = df_pwrite - df_pread;
	df_addr = (unsigned char *)((unsigned int)slot->vaddr_data + df_pread);

	if (df_len <= fifo_left) {
		if (df_pwrite >= df_pread) {
			gxfifo_put(fifo, df_addr, df_len);
		} else {
			int split = df_size - df_pread;
			gxfifo_put(fifo, df_addr, split);
			df_addr = (unsigned char *)((unsigned int)slot->vaddr_data);
			gxfifo_put(fifo, df_addr, df_pwrite);
		}
	} else {
		if (d%20 == 0)
			gxlog_e(LOG_DEMUX, "d!!! :%d   :%d\n", df_len, fifo_left);
		d++;
	}
	REG_SET_VAL(&(reg->filters.data_buf[id].logic_read_ptr), df_lread + df_len);
}

void sirius_gse_almost_full_isr(void *dev)
{
	int i;
	unsigned long flags;
	struct dmx_gse_slot *slot = NULL;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	unsigned int int_almost_full = dmxdev->thread_almost_full;

	for (i = 0; i < dmxdev->max_gse_slot; i++) {
		if (int_almost_full & (0x01 << i)) {
			gx_spin_lock_irqsave(&(dmxdev->gse_spin_lock),flags);
			dmxdev->thread_almost_full &= ~(0x01 << i);
			gx_spin_unlock_irqrestore(&(dmxdev->gse_spin_lock),flags);
			slot = dmxdev->demuxs[0].gse_slots[i];
			if (slot == NULL)
				continue;

			sirius_gse_data_dealwith(dmxdev, slot);
			return;
		}
	}
}

void sirius_gse_pdu_isr(void *dev)
{
	int i;
	unsigned long flags;
	struct dmx_gse_slot *slot = NULL;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	unsigned int int_pdu_finish = dmxdev->thread_pdu_finish;

	for (i = 0; i < dmxdev->max_gse_slot; i++) {
		if (int_pdu_finish & (0x01 << i)) {
			gx_spin_lock_irqsave(&(dmxdev->gse_spin_lock),flags);
			dmxdev->thread_pdu_finish &= ~(0x01 << i);
			gx_spin_unlock_irqrestore(&(dmxdev->gse_spin_lock),flags);
			slot = dmxdev->demuxs[0].gse_slots[i];
			if (slot == NULL)
				continue;

			sirius_gse_data_dealwith(dmxdev, slot);
		}
	}
}

static void _gp_status_isr(void *dev, int status, enum gp_status type)
{
	int id;
	unsigned long flags;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct dmxgp  *gp     = &dmxdev->gp;
	struct sirius_reg_gp *reg = dmxdev->gp_reg;
	struct dmxgp_chan *chan = NULL;

	for (id = 0; id < dmxdev->max_gp; id++) {
		chan = &gp->chan[id];
		if (type == GP_STATUS_ALMOST_FULL)
			reg->gp_int.value |= 0x1<<(id+4);

		if (!chan->start)
			continue;

		if (status & (0x1<<id)) {
			gx_dcache_inv_range(chan->v_addr, chan->v_addr + chan->length);
			gx_spin_lock_irqsave(&(chan->spin_lock), flags);
			chan->read_en = 1;
			if (type == GP_STATUS_IDLE)
				chan->start = 0;
			gx_spin_unlock_irqrestore(&(chan->spin_lock), flags);
		}
	}
}

void sirius_gp_almost_full_isr(void *dev)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct sirius_reg_gp *reg = dmxdev->gp_reg;

	if (reg->config.bits.av_buf_almost_full_int_en & reg->gp_int.bits.av_buf_almost_full_int)
		_gp_status_isr(dev, reg->gp_int.bits.av_buf_almost_full_int, GP_STATUS_ALMOST_FULL);
}

void sirius_gp_idle_isr(void *dev)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct sirius_reg_gp *reg = dmxdev->gp_reg;

	if (reg->config.bits.gp_idle_int_en & reg->gp_int.bits.gp_idle_int) {
		reg->gp_int.bits.gp_idle_int = 1;
		_gp_status_isr(dev, 0xf, GP_STATUS_IDLE);
	}
}

static void _pfilter_tsr_dealwith(struct dvr_pfilter *pfilter)
{
	struct dmx_demux *demux = get_subdemux(pfilter->dest);
	struct demux_dvr_ops *dvr_ops = demux->dev->dvr_ops;
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;
	struct gxfifo *fifo = &demux->infifo;
	int update_len = reg->buf_wptr_logic - reg->buf_rptr;
	int fifo_free_len = gxfifo_freelen(fifo);
	int total_len = reg->buf_full_gate;
	int priv_rptr, rptr;

	if (fifo_free_len > update_len)
		gxfifo_wptr_set(fifo, reg->buf_wptr_logic % reg->buf_full_gate);
	else {
		gxlog_e(LOG_DEMUX, "pf ddd!\n");
		reg->buf_rptr = reg->buf_wptr_logic;
		return;
	}

	if (dvr_ops->tsr_dealwith)
		dvr_ops->tsr_dealwith(demux->dev, pfilter->id, pfilter->dest);

	rptr =  gxfifo_rptr(fifo);
	priv_rptr = reg->buf_rptr % total_len;
	if (priv_rptr > rptr)
		reg->buf_rptr += rptr + total_len - priv_rptr;
	else
		reg->buf_rptr += rptr - priv_rptr;
}

static void _pfilter_tsw_dealwith(struct dvr_pfilter *pfilter)
{
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;
	unsigned int phys_rptr = reg->buf_rptr % reg->buf_full_gate;
	unsigned int phys_left = reg->buf_full_gate - phys_rptr;
	unsigned char *df_addr = (unsigned char *)((unsigned int)pfilter->ivaddr + phys_rptr);
	unsigned char *df_start_addr = (unsigned char *)((unsigned int)pfilter->ivaddr);
	struct gxfifo *fifo = &pfilter->fifo;
	int update_len = reg->buf_wptr_logic - reg->buf_rptr;
	int fifo_free_len = gxfifo_freelen(fifo);
	int total_len = pfilter->sw_buffer_size;
	int priv_rptr, rptr;

	gx_dcache_inv_range((unsigned int)df_start_addr, (unsigned int)df_start_addr + pfilter->hw_buffer_size);
	if (fifo_free_len > update_len) {
		if (update_len > phys_left) {
			gxfifo_put(fifo, df_addr, phys_left);
			gxfifo_put(fifo, df_start_addr, update_len - phys_left);
		} else
			gxfifo_put(fifo, df_addr, update_len);
	} else {
		gxlog_e(LOG_DEMUX, "pf dddd!\n");
		reg->buf_rptr = reg->buf_wptr_logic;
		return;
	}

	rptr = gxfifo_wptr(fifo) % total_len;
	priv_rptr = reg->buf_rptr % total_len;
	if (priv_rptr > rptr)
		reg->buf_rptr += rptr + total_len - priv_rptr;
	else
		reg->buf_rptr += rptr - priv_rptr;
}

void sirius_pfilter_dealwith(struct dvr_pfilter *pfilter)
{
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;
again:
	if (NULL == pfilter->ivaddr)
		return;

	if (pfilter->dest < DVR_OUTPUT_MEM)
		_pfilter_tsr_dealwith(pfilter);
	else
		_pfilter_tsw_dealwith(pfilter);

	if (reg->buf_wptr_logic - reg->buf_rptr > reg->buf_almost_full_gate/2)
		goto again;
}

int sirius_pfilter_irq_entry(struct dvr_pfilter *pfilter)
{
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;

	if (REG_GET_BIT(&reg->ctrl1, BIT_PFILTER_ISR_TS_LOCK) &
		REG_GET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_TS_LOCK)) {
		REG_SET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_TS_LOCK);
		gxlog_d(LOG_DEMUX, "[DVR] pid filter %d lock\n", pfilter->id);
		pfilter->time_err_flag = 0;
	}

	if (REG_GET_BIT(&reg->ctrl1, BIT_PFILTER_ISR_TS_LOSS) &
		REG_GET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_TS_LOSS)) {
		REG_SET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_TS_LOSS);
		gxlog_d(LOG_DEMUX, "[DVR] pid filter %d loss\n", pfilter->id);
	}

	if (REG_GET_BIT(&reg->ctrl1, BIT_PFILTER_ISR_BUF_FULL) &
		REG_GET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_BUF_FULL)) {
		REG_SET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_BUF_FULL);
		gxlog_i(LOG_DEMUX, "[DVR] pid filter %d buf full\n", pfilter->id);
	}

	if (REG_GET_BIT(&reg->ctrl1, BIT_PFILTER_ISR_TIME_ERR) &
		REG_GET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_TIME_ERR)) {
		REG_SET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_TIME_ERR);
		gxlog_i(LOG_DEMUX, "[DVR] pid filter %d time err\n", pfilter->id);

		/*ensure pidfilter module have exported all ts data*/
		if (pfilter->cfg_done && !pfilter->time_err_flag) {
			REG_SET_BIT(&reg->ctrl1, BIT_PFILTER_CTRL1_CFG_REQ);
			while(!REG_GET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_ALLOW_CFG));
			REG_SET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_ALLOW_CFG);
			REG_CLR_BIT(&reg->ctrl1, BIT_PFILTER_CTRL1_CFG_REQ);
			pfilter->time_err_flag = 1;
		}
	}

	if (REG_GET_BIT(&reg->ctrl1, BIT_PFILTER_ISR_BUF_AFULL) &
		REG_GET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_BUF_AFULL)) {
		struct dmx_demux *demux = get_subdemux(0);
		REG_SET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_BUF_AFULL);
		gx_sem_post(&(demux->dev->pf_swirq_sem));
	}
	return 0;
}
