#include "kernelcalls.h"
#include "avcore.h"
#include "gxav_bitops.h"
#include "gxdemux.h"
#include "sirius_regs.h"

int dmx_gp_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = &gxdmxdev[0];
	struct dmxgp  *gp  = &dev->gp;
	struct demux_gp_ops *ops = dev->gp_ops;

	memset(gp, 0, sizeof(struct dmxgp));
	gp->cmd_fifo = (unsigned int)gx_page_malloc(0x100);
	gp->cmd_fifo_len = 0x100;

	memset((void *)gp->cmd_fifo, 0, 0x100);
	gx_dcache_clean_range(0, 0);
	if (ops->set_cmd_fifo)
		ops->set_cmd_fifo(dev->gp_reg, (void *)gx_virt_to_phys(gp->cmd_fifo), gp->cmd_fifo_len);

	return 0;
}

int dmx_gp_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = &gxdmxdev[0];
	struct dmxgp  *gp  = &dev->gp;
	struct demux_gp_ops *ops = dev->gp_ops;

	if (ops->clr_cmd_fifo)
		ops->clr_cmd_fifo(dev->gp_reg);
	gx_page_free((void *)gp->cmd_fifo, gp->cmd_fifo_len);
	gp->cmd_fifo = 0;

	return 0;
}

int dmx_gp_open(struct gxav_module *module)
{
	int id = module->sub;
	struct dmxdev     *dev  = &gxdmxdev[0];
	struct dmxgp_chan *chan = NULL;
	struct demux_gp_ops *ops = dev->gp_ops;

	if ((NULL == module) || (id < 0) || (id > module->inode->count)) {
		gxlog_e(LOG_GP, "Module is NULL.\n");
		return -1;
	}

	chan = &dev->gp.chan[id];
	chan->id     = id;
	chan->v_addr = (unsigned int)gx_page_malloc(0x1000);
	chan->p_addr = gx_virt_to_phys(chan->v_addr);

	chan->rptr   = 0x0;
	chan->length = 0x1000;
	chan->afull_gate = 0x700;
	memset((void *)chan->v_addr, 0, 0x1000);
	gx_dcache_clean_range(0, 0);
	gx_spin_lock_init(&(chan->spin_lock));

	if (ops->set_chan_addr)
		ops->set_chan_addr(dev->gp_reg, chan);
	gxav_firewall_register_buffer(GXAV_BUFFER_GP_ES, chan->p_addr, chan->length);

	return 0;
}

int dmx_gp_close(struct gxav_module *module)
{
	int id = module->sub;
	struct dmxdev     *dev  = &gxdmxdev[0];
	struct dmxgp_chan *chan = NULL;
	struct demux_gp_ops *ops = dev->gp_ops;

	if ((NULL == module) || (id < 0) || (id > module->inode->count)) {
		gxlog_e(LOG_GP, "Module is NULL.\n");
		return -1;
	}

	chan = &dev->gp.chan[id];
	if (ops->clr_chan_addr)
		ops->clr_chan_addr(dev->gp_reg, chan);

	gx_page_free((void *)chan->v_addr, chan->length);
	memset(chan, 0, sizeof(struct dmxgp_chan));

	return 0;
}

int dmx_gp_alloc(int id, GxGPProperty_Cw *pcw)
{
	int i;
	struct dmxgp_cw *gpcw;
	struct dmxdev   *dev  = &gxdmxdev[0];
	struct dmxgp    *gp   = &dev->gp;

	i = get_one(dev->gp_cw_mask, 1, 0, dev->max_gp_cw);
	if ((i >= dev->max_gp_cw) || (i < 0)) {
		gxlog_e(LOG_GP, "gp_cw_mask is full!\n");
		return -1;
	}

	gpcw = (struct dmxgp_cw *)gx_malloc(sizeof(struct dmxgp_cw));
	if (NULL == gpcw) {
		gxlog_e(LOG_GP, "alloc NULL!\n");
		return -1;
	}

	SET_MASK(dev->gp_cw_mask, i);
	gpcw->id = i;
	gp->configs[i] = gpcw;
	pcw->id = i;

	return 0;
}

int dmx_gp_free(int id, GxGPProperty_Cw *pcw)
{
	struct dmxdev       *dev = &gxdmxdev[0];
	struct dmxgp        *gp  = &dev->gp;
	struct dmxgp_cw     *gpcw= gp->configs[pcw->id];

	if (NULL == gpcw) {
		gxlog_e(LOG_GP, "the gpcw config is NULL\n");
		return -1;
	}

	CLR_MASK(dev->gp_cw_mask, pcw->id);
	gp->configs[pcw->id] = NULL;
	gx_free(gpcw);

	return 0;
}

int dmx_gp_setcw(int id, GxGPProperty_Cw *pcw)
{
	struct dmxdev       *dev = &gxdmxdev[0];
	struct dmxgp        *gp  = &dev->gp;
	struct demux_gp_ops *ops = dev->gp_ops;
	struct dmxgp_cw     *gpcw= gp->configs[pcw->id];

	if (NULL == gpcw) {
		gxlog_e(LOG_GP, "the gpcw config is NULL\n");
		return -1;
	}
	memcpy(gpcw, pcw, sizeof(GxGPProperty_Cw));
	if (ops->set_cw)
		ops->set_cw(dev->gp_reg, gpcw);

	return 0;
}

static void _gp_create_cmd_list(int id, GxGPProperty_Des *pdes)
{
	GPCmd *node, *priv_node = NULL;
	int i = 0, j, unit_clr_len, wcount = 0, discontinue = 0;

	struct dmxdev       *dev  = &gxdmxdev[0];
	struct dmxgp        *gp   = &dev->gp;
	struct demux_gp_ops *ops  = dev->gp_ops;
	struct dmxgp_cw     *gpcw= gp->configs[pdes->cw_id];

	unsigned int wptr = gp->cmd_fifo_wptr;
	int loop_time = pdes->total_len/pdes->unit_len;
	int data_left = pdes->total_len%pdes->unit_len;

	while (1) {
		node = (GPCmd *)(gp->cmd_fifo + wptr);
		node->info.bits.mode = GP_MODE_NORMAL;
		node->info.bits.size_info_bit = 0;

	/* set head */
		if (i == 0) {
			node->data_ptr = gx_virt_to_phys(pdes->data_ptr);
			node->data_len = pdes->unit_len;
			if (loop_time == 1) {
				node->info.bits.end_of_data = 1;
				discontinue = 1;
			}
			i++;

	/* set loop node */
		} else if (i < loop_time - 1) {
			node->info.bits.mode = GP_MODE_LOOP;
			node->data_ptr = pdes->unit_len;

			if (data_left) {
				node->data_len = loop_time - 1;
				i = loop_time;
			} else {
				node->data_len = loop_time - 2;
				i = loop_time - 1;
			}

		} else {
	/* set data end*/
			if (data_left)
				node->data_len = data_left;

	/* set loop end */
			else
				node->data_len = pdes->unit_len;

			node->data_ptr = gx_virt_to_phys((pdes->data_ptr + i*pdes->unit_len));
			node->info.bits.end_of_data = 1;
			discontinue = 1;
			i++;
		}

	/* set scramble and clr length */
		if (pdes->unit_scramble_len &&
			node->info.bits.mode == GP_MODE_NORMAL &&
			node->data_len == pdes->unit_len) {
			unit_clr_len = pdes->unit_len - pdes->unit_scramble_len;
			for (j = 31; j > 0; j--) {
				if (unit_clr_len & (0x1<<j)) {
					node->info.bits.size_info_bit = j+1;
					break;
				}
			}
			node->data_len = unit_clr_len | ((pdes->unit_scramble_len)<<(j+1));
		}

		wcount++;
		wptr += sizeof(GPCmd);
		if (wptr == gp->cmd_fifo_len) {
			wptr = 0;
			node->info.bits.end_of_data = 1;
		}

		priv_node = node;
		node->info.bits.index       = pdes->cw_id;
		node->info.bits.av_channel  = id;
		node->info.bits.ctr_iv_size = (gpcw->mode==GP_DES_AES_CTR) ? gpcw->ctr_iv_size : 0;
		node->info.bits.iv_reset    = (gpcw->mode==GP_DES_AES_ECB) ? 1 : ((pdes->iv_reset) ? 1 : 0);
		node->next = (GPCmd *)gx_virt_to_phys((gp->cmd_fifo + wptr));

		if (discontinue)
			break;
	}
	gx_dcache_clean_range(0, 0);
	gp->cmd_fifo_wptr = wptr;
	if (ops->update_cmd)
		ops->update_cmd(dev->gp_reg, wcount);
}

int dmx_gp_des(int id, GxGPProperty_Des *pdes)
{
	struct dmxdev       *dev  = &gxdmxdev[0];
	struct dmxgp        *gp   = &dev->gp;
	struct dmxgp_cw     *gpcw = gp->configs[pdes->cw_id];
	struct dmxgp_chan   *chan = &gp->chan[id];

	if (NULL == gpcw) {
		gxlog_e(LOG_GP, "the gpcw config is NULL\n");
		return -1;
	}

	if (pdes->cw_id > 63 ||
		pdes->data_ptr == 0 ||
		pdes->total_len == 0 ||
		pdes->total_len < pdes->unit_len ||
		pdes->unit_len < pdes->unit_scramble_len) {
		gxlog_e(LOG_GP, "GP des param error\n");
		return -1;
	}

	if (pdes->unit_len == 0)
		pdes->unit_len = pdes->total_len;

	chan->start = 1;

	_gp_create_cmd_list(id, pdes);
	return 0;
}

int dmx_gp_read(int id, GxGPProperty_Read *pread)
{
	unsigned long flags;
	int len = 0, read_len, temp, parta, partb;

	struct dmxdev     *dev  = &gxdmxdev[0];
	struct dmxgp      *gp   = &dev->gp;
	struct dmxgp_chan *chan = &gp->chan[id];
	struct demux_gp_ops *ops = dev->gp_ops;
	int total_len = chan->length;
	int size = pread->max_size;

	if (chan->read_en == 0) {
		gxlog_e(LOG_GP, "GP channel (%d) is not ready.\n", id);
		return -1;
	}

	if (ops->query_data_length)
		len = ops->query_data_length(dev->gp_reg, id);

	read_len = GX_MIN(size, len);
	if (read_len == len) {
		gx_spin_lock_irqsave(&(chan->spin_lock), flags);
		chan->read_en = 0;
		gx_spin_unlock_irqrestore(&(chan->spin_lock), flags);
	}

	temp = chan->rptr + read_len;
	if (temp > total_len) {
		parta = total_len - chan->rptr;
		partb = temp - total_len;

		gx_copy_to_user(pread->buffer, (void *)chan->v_addr+chan->rptr, parta);
		gx_copy_to_user(pread->buffer+parta, (void *)chan->v_addr, partb);
		chan->rptr = partb;

	} else {
		gx_copy_to_user(pread->buffer, (void *)chan->v_addr+chan->rptr, read_len);
		chan->rptr = temp;
	}
	if (ops->update_rptr)
		ops->update_rptr(dev->gp_reg, id, read_len);

	pread->read_size = read_len;
	return 0;
}

int dmx_gp_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret, id = module->sub;
	struct dmxdev     *dev  = &gxdmxdev[0];
	struct dmxgp_chan *chan = NULL;
	if ((NULL == module) || (id < 0) || (id > module->inode->count) || (NULL == property)) {
		gxlog_e(LOG_GP, "Module is NULL.\n");
		return -3;
	}

	chan = &dev->gp.chan[id];
	switch (property_id) {
	case GxGPPropertyID_Free:
	case GxGPPropertyID_SetCw:
		{
			GxGPProperty_Cw *info = (GxGPProperty_Cw *)property;
			if (size != sizeof(GxGPProperty_Cw)) {
				gxlog_e(LOG_GP, "the param error!\n");
				return -1;
			}
			if (property_id == GxGPPropertyID_Free)
				ret = dmx_gp_free(id, info);

			if (property_id == GxGPPropertyID_SetCw)
				ret = dmx_gp_setcw(id, info);
		}
		break;

	case GxGPPropertyID_Des:
		{
			GxGPProperty_Des *info = (GxGPProperty_Des *)property;
			if (size != sizeof(GxGPProperty_Des)) {
				gxlog_e(LOG_GP, "the param error!\n");
				return -1;
			}
			ret = dmx_gp_des(id, info);
		}
		break;

	default:
		gxlog_e(LOG_GP, "the parameter which gp's property_id is wrong, please set the right parameter\n");
		return -1;

	}
	return ret;
}

int dmx_gp_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret, id = module->sub;
	struct dmxdev     *dev  = &gxdmxdev[0];
	struct dmxgp_chan *chan = NULL;
	if ((NULL == module) || (id < 0) || (id > module->inode->count) || (NULL == property)) {
		gxlog_e(LOG_GP, "Module is NULL.\n");
		return -3;
	}

	chan = &dev->gp.chan[id];
	switch (property_id) {
	case GxGPPropertyID_Alloc:
		{
			GxGPProperty_Cw *info = (GxGPProperty_Cw *)property;
			if (size != sizeof(GxGPProperty_Cw)) {
				gxlog_e(LOG_GP, "the param error!\n");
				return -1;
			}
			ret = dmx_gp_alloc(id, info);
		}
		break;

	case GxGPPropertyID_Read:
		{
			GxGPProperty_Read *pread = (GxGPProperty_Read *)property;
			if (size != sizeof(GxGPProperty_Read) || NULL == pread->buffer) {
				gxlog_e(LOG_GP, "the param error!\n");
				return -1;
			}
			ret = dmx_gp_read(id, pread);
		}
		break;

	default:
		gxlog_e(LOG_GP, "the parameter which gp's property_id is wrong, please set the right parameter\n");
		return -1;
	}
	return ret;
}

struct gxav_module_inode* dmx_gp_irq(struct gxav_module_inode *inode, int irq)
{
	struct dmxdev       *dev    = &gxdmxdev[0];
	struct demux_gp_ops *gp_ops = dev->gp_ops;

	if (gp_ops->almost_full_isr)
		gp_ops->almost_full_isr(dev);

	if (gp_ops->idle_isr)
		gp_ops->idle_isr(dev);

	return inode;
}

