#include "kernelcalls.h"
#include "gx3211_regs.h"
#include "gx3211_demux.h"
#include "sirius_regs.h"
#include "sirius_demux.h"
#include "gxav_bitops.h"
#include "sdc_hal.h"
#include "profile.h"
#include "gxdemux.h"
#include "sirius_pfilter.h"

void sirius_gp_set_irqen(void *vreg)
{
	struct sirius_reg_gp *reg = (struct sirius_reg_gp *)vreg;
	reg->channel[0].almost_full_gate = 0x1000;
	reg->channel[1].almost_full_gate = 0x1000;
	reg->channel[2].almost_full_gate = 0x1000;
	reg->channel[3].almost_full_gate = 0x1000;
	reg->config.value = 0x00f00115;
}

void sirius_gp_clr_irqen(void *vreg)
{
	struct sirius_reg_gp *reg = (struct sirius_reg_gp *)vreg;
	reg->config.value = 0;
}

void sirius_gp_set_chan_addr(void *vreg, struct dmxgp_chan *chan)
{
	int id = chan->id;
	struct sirius_reg_gp *reg = (struct sirius_reg_gp *)vreg;

	reg->channel[id].start_addr       = chan->p_addr;
	reg->channel[id].size             = chan->length;
	reg->channel[id].almost_full_gate = chan->afull_gate;
}

void sirius_gp_clr_chan_addr(void *vreg, struct dmxgp_chan *chan)
{
	int id = chan->id;
	struct sirius_reg_gp *reg = (struct sirius_reg_gp *)vreg;

	reg->channel[id].start_addr       = 0;
	reg->channel[id].size             = 0;
	reg->channel[id].almost_full_gate = 0;
}

void sirius_gp_set_cmd_fifo(void *vreg, unsigned char *buf, unsigned int size)
{
	struct sirius_reg_gp *reg = (struct sirius_reg_gp *)vreg;

	reg->cmd_fifo_start_addr = (unsigned int)buf;
	reg->logic_cmd_fifo_len  = size;
}

void sirius_gp_clr_cmd_fifo(void *vreg)
{
	struct sirius_reg_gp *reg = (struct sirius_reg_gp *)vreg;

	reg->cmd_fifo_start_addr = 0;
	reg->logic_cmd_fifo_len  = 0;
}

void sirius_gp_set_cw(void *vreg, struct dmxgp_cw *gpcw)
{
	int i, id = gpcw->id;
	struct sirius_reg_gp *reg = (struct sirius_reg_gp *)vreg;

	reg->ds_mode[id/8] &= ~(0x3 << ((id%8)*4));
	reg->ds_mode[id/8] |= gpcw->mode << ((id%8)*4);

	if (gpcw->mode == GP_DES_AES_CBC ||
		gpcw->mode == GP_DES_AES_CTR) {
		for (i = 0; i < 4; i++)
			reg->iv[id][i] = gpcw->iv[i];
	}

	if (gpcw->cw_from_acpu) {
		for (i = 0; i < 4; i++)
			reg->cw_buffer[i] = gpcw->cw[i];
		gx_msleep(10);
		REG_SET_VAL(&(reg->wctrl), (((id)<<BIT_DEMUX_CW_WADDR) | 0));
		REG_SET_VAL(&(reg->wctrl), (((id)<<BIT_DEMUX_CW_WADDR) | 1));
		REG_SET_VAL(&(reg->wctrl), (((id)<<BIT_DEMUX_CW_WADDR) | 0));

	} else {
		REG_SET_VAL(&(reg->wctrl), (((id)<<BIT_DEMUX_CW_WADDR) | 0));
		REG_SET_VAL(&(reg->wctrl), (((id)<<BIT_DEMUX_CW_WADDR) | 1));
	}
}

void sirius_gp_update_cmd(void *vreg, unsigned int cmd_num)
{
	struct sirius_reg_gp *reg = (struct sirius_reg_gp *)vreg;
	reg->logic_cmd_fifo_wcount = cmd_num;
}

void sirius_gp_update_rptr(void *vreg, unsigned int id, unsigned int rptr)
{
	struct sirius_reg_gp *reg = (struct sirius_reg_gp *)vreg;
	reg->channel[id].rptr = rptr;
}

unsigned int sirius_gp_query_data_length(void *vreg, unsigned int id)
{
	struct sirius_reg_gp *reg = (struct sirius_reg_gp *)vreg;
	return reg->channel[id].wptr;
}

struct demux_gp_ops sirius_gp_ops = {
	.set_irq_en        = sirius_gp_set_irqen,
	.clr_irq_en        = sirius_gp_clr_irqen,
	.set_chan_addr     = sirius_gp_set_chan_addr,
	.clr_chan_addr     = sirius_gp_clr_chan_addr,
	.set_cw            = sirius_gp_set_cw,
	.set_cmd_fifo      = sirius_gp_set_cmd_fifo,
	.update_cmd        = sirius_gp_update_cmd,
	.update_rptr       = sirius_gp_update_rptr,
	.query_data_length = sirius_gp_query_data_length,

	.almost_full_isr   = sirius_gp_almost_full_isr,
	.idle_isr          = sirius_gp_idle_isr,
};

int sirius_gp_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = &gxdmxdev[0];
	dev->gp_ops = &sirius_gp_ops;
	dev->max_gp = MAX_GP_UNIT;
	dev->max_gp_cw = MAX_GP_CW;
	dev->gp_cw_mask = 0;

	if (!gx_request_mem_region(SIRIUS_DEMUX_BASE_GP, sizeof(struct sirius_reg_gp))) {
		gxlog_e(LOG_DEMUX, "request gp mem region failed\n");
		return -1;
	}

	dev->gp_reg = (struct sirius_reg_gp *)gx_ioremap(SIRIUS_DEMUX_BASE_GP, sizeof(struct sirius_reg_gp));
	if (!dev->gp_reg) {
		gxlog_e(LOG_DEMUX, "gp ioremap failed.\n");
		return -1;
	}
	if (dev->gp_ops->set_irq_en)
		dev->gp_ops->set_irq_en(dev->gp_reg);

	return dmx_gp_init(device, inode);
}

int sirius_gp_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = &gxdmxdev[0];
	dmx_gp_cleanup(device, inode);
	if (dev->gp_ops->clr_irq_en)
		dev->gp_ops->clr_irq_en(dev->gp_reg);

	dev->gp_ops     = NULL;
	dev->max_gp     = 0;
	dev->max_gp_cw  = 0;
	dev->gp_cw_mask = 0;
	gx_iounmap(dev->gp_reg);
	gx_release_mem_region(SIRIUS_DEMUX_BASE_GP, sizeof(struct sirius_reg_gp));

	return 0;
}

struct gxav_module_ops sirius_gp_module = {
	.module_type  = GXAV_MOD_GP,
	.count        = MAX_GP_UNIT,
	.irqs         = {SIRIUS_GP_IRQ, -1},
	.irq_names    = {"gp"},
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = dmx_gp_open,
	.close        = dmx_gp_close,
	.init         = sirius_gp_init,
	.cleanup      = sirius_gp_cleanup,
	.set_property = dmx_gp_set_property,
	.get_property = dmx_gp_get_property,
	.interrupts[SIRIUS_GP_IRQ] = dmx_gp_irq,
};

