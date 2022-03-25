#include "fifo.h"
#include "sdc_hal.h"
#include "gxdemux.h"
#include "taurus_regs.h"
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "profile.h"

static struct reg_demux_rts_buf * get_tsr_base_reg(struct dmxdev *dev, int modid)
{
	struct taurus_reg_demux *reg = dev->reg;

	if (modid >= dev->max_dvr)
		return &reg->pfilter_rts_buf[modid%2];

	if (modid == 0)
		return &reg->rts_buf;

	return &reg->rts_buf1;
}

void taurus_tsr_dealwith(void *dev, int modid, int tsr_id)
{
	int i;
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct dmx_demux *dmx = &dmxdev->demuxs[modid%2];
	struct dvr_info *info = modid<MAX_DMX_NORMAL ? &dmx->dvr.tsr_info : &dmx->dvr.tsr23_info;
	struct gxfifo *fifo = &dmx->infifo;
	struct reg_demux_rts_buf *reg_rts_buf = get_tsr_base_reg(dmxdev, modid);
	unsigned int sdc_wptr,sdc_rptr, sdc_len;
	int mask = info->tsr_share_mask;

	if (fifo->size == 0)
		return;

	gx_dcache_clean_range(0,0);
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
		reg_rts_buf = get_tsr_base_reg(dmxdev, i);
		REG_SET_VAL(&(reg_rts_buf->rts_sdc_wptr), sdc_wptr);
	}
}

void taurus_tsr_isr(void *dev)
{
	struct dmxdev *dmxdev = (struct dmxdev *)dev;
	struct taurus_reg_demux *reg = dmxdev->reg;
	unsigned int int_tsr01, int_tsr01_en;
	unsigned int int_tsr23, int_tsr23_en;
	int_tsr01    = REG_GET_VAL(&(reg->int_dmx_tsr_buf));
	int_tsr01_en = REG_GET_VAL(&(reg->int_dmx_tsr_buf_en));
	int_tsr23    = REG_GET_VAL(&(reg->int_tsr23_buf));
	int_tsr23_en = REG_GET_VAL(&(reg->int_tsr23_buf_en));
	if (!((int_tsr01 & int_tsr01_en) || (int_tsr23 & int_tsr23_en)))
		return;

	if(int_tsr01&0x1) {
		REG_SET_BIT(&(reg->int_dmx_tsr_buf), 1);
		taurus_tsr_dealwith(dev, 0, 0);
	}

	if(int_tsr01&(0x1<<16)) {
		REG_SET_BIT(&(reg->int_dmx_tsr_buf), 16);
		taurus_tsr_dealwith(dev, 0, 1);
	}

	if(int_tsr23&0x1) {
		REG_SET_BIT(&(reg->int_tsr23_buf), 1);
		taurus_tsr_dealwith(dev, 4, 0);
	}

	if(int_tsr23&(0x1<<16)) {
		REG_SET_BIT(&(reg->int_tsr23_buf), 16);
		taurus_tsr_dealwith(dev, 4, 1);
	}
}

