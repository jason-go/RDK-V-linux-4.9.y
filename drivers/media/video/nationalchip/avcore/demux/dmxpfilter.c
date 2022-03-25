#include "gxdemux.h"
#include "gx3211_regs.h"
#include "taurus_regs.h"
#include "sirius_regs.h"
#include "profile.h"
#include "gxav_bitops.h"

static void dvr_pfilter_irq_thread(void* p)
{
	int i;
	struct dmxdev *dev = (struct dmxdev *)p;
	struct demux_pfilter_ops *ops = dev->pfilter_ops;
	struct dvr_pfilter *pfilter = NULL;
	int max_pfilter = dev->max_pfilter;
#ifndef NO_OS
	while (1) {
#endif
		gx_sem_wait_timeout(&(dev->pf_swirq_sem), 6000);
#ifndef NO_OS
		if (dev->thread_stop) {
			gx_msleep(100);
			if (gx_thread_should_stop())
				break;
		}
#endif
		for (i = 0; i < max_pfilter; i++) {
			if (ops->get_pfilter)
				pfilter = ops->get_pfilter(i);
			if (pfilter) {
				if (ops->tsw_isr)
					ops->tsw_isr(pfilter);
			}
		}
#ifndef NO_OS
	}
#endif
}

#define DVR_THREAD_STACK (16 * 1024)
int dvr_pfilter_init(void)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct dmxdev *dev = demux->dev;

	if (gx_sem_create(&(dev->pf_swirq_sem), 0) < 0) {
		gxlog_e(LOG_DEMUX, "dvr semaphore created fail!\n");
		return -1;
	}

	dev->pf_thread_stack = gx_malloc(DVR_THREAD_STACK);
	if (NULL == dev->pf_thread_stack) {
		gxlog_e(LOG_DEMUX, "dvr thread stack memory malloc fail!\n");
		return -1;
	}

	if (gx_thread_create("thread-pfilter", &(dev->pf_thread_id), dvr_pfilter_irq_thread,
				dev, dev->pf_thread_stack, DVR_THREAD_STACK, 9, &(dev->pf_thread_info)) < 0) {
		gxlog_e(LOG_DEMUX, "dvr thread created fail!\n");
		return -1;
	}

	return 0;
}

int dvr_pfilter_deinit(void)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct dmxdev *dev = demux->dev;

	gx_sem_post(&(dev->pf_swirq_sem));
	gx_thread_delete(dev->pf_thread_id);
	gx_sem_delete(&(dev->pf_swirq_sem));
	gx_memset(&(dev->pf_swirq_sem),0,sizeof(gx_sem_id));
	if (dev->pf_thread_stack !=NULL){
		gx_free(dev->pf_thread_stack);
		dev->pf_thread_stack = NULL;
	}

	return 0;
}

int dvr_pfilter_open(int id)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct demux_pfilter_ops *ops = demux->dev->pfilter_ops;
	struct dvr_pfilter *pfilter = ops->get_pfilter(id);

	if (id < 0 || id >= demux->dev->max_pfilter)
		return -1;

	if (ops->stop)
		ops->stop(pfilter);

	if (ops->pid_buf_init)
		ops->pid_buf_init(pfilter);

	if (ops->open)
		ops->open(pfilter);

	return 0;
}

int dvr_pfilter_close(int id)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct demux_pfilter_ops *ops = demux->dev->pfilter_ops;
	struct dvr_pfilter *pfilter = ops->get_pfilter(id);

	if (id < 0 || id >= demux->dev->max_pfilter)
		return -1;

	if (ops->close)
		ops->close(pfilter);

	if (pfilter->ivaddr) {
		if (pfilter->dest < DVR_OUTPUT_MEM)
			dvr_pfilter_clr_tsr_buf(pfilter->dest);
		gxfifo_free(&pfilter->fifo);
		gx_page_free(pfilter->ivaddr, pfilter->hw_buffer_size);
		pfilter->ivaddr = NULL;
	}

	return 0;
}

struct gxfifo *dvr_pfilter_get_tsw_fifo(int id)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct demux_pfilter_ops *ops = demux->dev->pfilter_ops;
	struct dvr_pfilter *pfilter = ops->get_pfilter(id);

	/*check phys buffer have left data*/
	if (pfilter->dest == DVR_OUTPUT_MEM && pfilter->time_err_flag) {
		if (ops->tsw_isr)
			ops->tsw_isr(pfilter);
	}

	return &pfilter->fifo;
}

int dvr_pfilter_get_tsr_id(int id)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct demux_pfilter_ops *ops = demux->dev->pfilter_ops;
	struct dvr_pfilter *pfilter = ops->get_pfilter(id);
	return pfilter->dest % DVR_OUTPUT_MEM;
}

int dvr_pfilter_add_pid(int id, int pid)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct demux_pfilter_ops *ops = demux->dev->pfilter_ops;
	struct dvr_pfilter *pfilter = ops->get_pfilter(id);
	int pid_handle = get_one(pfilter->pid_mask, 1, 0, pfilter->max_pid_num);
	if (pid_handle < 0) {
		pid_handle = get_one(pfilter->pid_buf_mask, 1, 0, pfilter->pid_buf_max_unit);
		if (pid_handle < 0)
			return -1;
		if (ops->pid_buf_add_pid)
			ops->pid_buf_add_pid(pfilter, pid_handle, pid);
		SET_MASK(pfilter->pid_buf_mask, pid_handle);
		return pid_handle + pfilter->max_pid_num;
	}

	if (ops->add_pid)
		ops->add_pid(pfilter, pid_handle, pid);
	SET_MASK(pfilter->pid_mask, pid_handle);
	return pid_handle;
}

int dvr_pfilter_del_pid(int id, int pid_handle)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct demux_pfilter_ops *ops = demux->dev->pfilter_ops;
	struct dvr_pfilter *pfilter = ops->get_pfilter(id);
	if (pid_handle >= pfilter->max_pid_num) {
		pid_handle -= pfilter->max_pid_num;
		if (ops->pid_buf_del_pid)
			ops->pid_buf_del_pid(pfilter, pid_handle);
		CLR_MASK(pfilter->pid_buf_mask, pid_handle);
		return 0;
	}

	if (ops->del_pid)
		ops->del_pid(pfilter, pid_handle);
	CLR_MASK(pfilter->pid_mask, pid_handle);
	return 0;
}

int dvr_pfilter_set_tsr_buf(int dmxid, struct dvr_pfilter *pfilter)
{
	struct dmx_demux *demux = get_subdemux(dmxid);
	struct demux_dvr_ops *dvr_ops = demux->dev->dvr_ops;

	if (demux->dvr.tsr_info.status)
		return -1;

	if (dvr_ops->set_tsr_buf)
		dvr_ops->set_tsr_buf(dmxid, demux->dev->reg, pfilter->ipaddr,
				pfilter->hw_buffer_size, pfilter->almost_full_gate);
	demux->dvr.tsr_info.status = DVR_STATE_BUSY;
	return 0;
}

int dvr_pfilter_clr_tsr_buf(int dmxid)
{
	struct dmx_demux *demux = get_subdemux(dmxid);
	struct demux_dvr_ops *dvr_ops = demux->dev->dvr_ops;

	if (!demux->dvr.tsr_info.status)
		return -1;

	if (dvr_ops->clr_tsr_buf)
		dvr_ops->clr_tsr_buf(dmxid, demux->dev->reg);

	demux->dvr.tsr_info.status = DVR_STATE_IDLE;
	return 0;
}

int dmx_pfilter_set_config(int id, GxDemuxProperty_ConfigDemux *config)
{
	struct dmx_demux *demux = get_subdemux(id);
	struct dmxdev *dev = demux->dev;
	struct demux_pfilter_ops *ops = dev->pfilter_ops;
	struct dvr_pfilter *pfilter = ops->get_pfilter(id);

	pfilter->novalid_en = config->flags&DMX_NOVALID_EN;
	pfilter->nosync_en  = config->flags&DMX_NOSYNC_EN;
	pfilter->source = config->source;
	pfilter->input_mode = config->stream_mode;

	return 0;
}
#define BUFFER_DEST_SIZE        188*1024
#define BUFFER_CACHE_SIZE       188*1024 * 5
#define BUFFER_AFULL_SIZE       188*100

int dvr_pfilter_set_config(int id, GxDvrProperty_Config *config)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct dmxdev *dev = demux->dev;
	struct demux_pfilter_ops *ops = dev->pfilter_ops;
	struct dvr_pfilter *pfilter = ops->get_pfilter(id);
	struct dvr_buffer *outbuf = &config->dst_buf;
	dvr_pfilter_node *node = NULL;

	SET_DEFAULT_VALUE(pfilter->sw_buffer_size, outbuf->sw_buffer_size, BUFFER_CACHE_SIZE);
	SET_DEFAULT_VALUE(pfilter->hw_buffer_size, outbuf->hw_buffer_size, BUFFER_DEST_SIZE);
	SET_DEFAULT_VALUE(pfilter->almost_full_gate, outbuf->almost_full_gate, BUFFER_AFULL_SIZE);

	if (pfilter->cfg_done) {
		if (ops->stop)
			ops->stop(pfilter);

		if (config->dst < DVR_OUTPUT_MEM)
			dvr_pfilter_clr_tsr_buf(config->dst);
		gxfifo_free(&pfilter->fifo);
		gx_page_free(pfilter->ivaddr, pfilter->hw_buffer_size);
		pfilter->ivaddr = NULL;
	}
	pfilter->mode = config->mode;
	pfilter->dest = config->dst;
	if (ops->set_ctrl)
		ops->set_ctrl(pfilter);

	pfilter->ivaddr = gx_page_malloc(pfilter->hw_buffer_size);
	pfilter->ipaddr = gx_virt_to_phys((unsigned int)pfilter->ivaddr);
	gx_memset(pfilter->ivaddr, 0, pfilter->hw_buffer_size);
	gx_dcache_clean_range(0,0);

	if (config->dst < DVR_OUTPUT_MEM) {
		struct dmx_demux *demux = get_subdemux(config->dst);

		if (dvr_pfilter_set_tsr_buf(config->dst, pfilter) < 0) {
			gx_page_free(pfilter->ivaddr, pfilter->hw_buffer_size);
			pfilter->ivaddr = NULL;
			return -1;
		}

		gxfifo_init(&pfilter->fifo, pfilter->ivaddr, pfilter->hw_buffer_size);
		gx_memcpy(&demux->infifo, &pfilter->fifo, sizeof(struct gxfifo));

	} else {
		gxfifo_init(&pfilter->fifo, NULL, pfilter->sw_buffer_size);
	}

	node = (dvr_pfilter_node *)pfilter->list_start_addr;
	node->start_addr = pfilter->ipaddr;
	node->length = pfilter->hw_buffer_size;
	node->next = gx_virt_to_phys((unsigned int)node);
	gx_memcpy(&pfilter->list, node, sizeof(dvr_pfilter_node));
	gx_dcache_clean_range(0,0);

	if (ops->set_gate)
		ops->set_gate(pfilter);
	if (ops->start)
		ops->start(pfilter);

#ifdef FPGA_TEST_STREAM_PRODUCER_EN
	gx_msleep(100);
	if (dev->fpga_ops->set_stream_addr)
		dev->fpga_ops->set_stream_addr(pfilter->input_mode);
	if (dev->fpga_ops->hot_rst)
		dev->fpga_ops->hot_rst(0, 0);
#endif
	return 0;
}

int dvr_pfilter_irq_entry(int max_pfilter_num)
{
	int i;
	struct dmx_demux *demux = get_subdemux(0);
	struct demux_pfilter_ops *ops = demux->dev->pfilter_ops;
	struct dvr_pfilter *pfilter = ops->get_pfilter(0);

	for (i = 0; i < max_pfilter_num; i++) {
		if (ops->irq_entry)
			ops->irq_entry(pfilter);
#ifdef NO_OS
		dvr_pfilter_irq_thread(demux->dev);
#endif
		pfilter++;
	}
	return 0;
}

int dvr_pfilter_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0, sub = module->sub%4;

	gxdemux_lock();
	switch (property_id) {
	case GxDemuxPropertyID_Config:
		{
			GxDemuxProperty_ConfigDemux *pconfig = (GxDemuxProperty_ConfigDemux *) property;

			if (size != sizeof(GxDemuxProperty_ConfigDemux)) {
				gxlog_e(LOG_DEMUX, "ConfigDmx: the param error!\n");
				goto err;
			}

			if ((pconfig->source < DEMUX_TS1) || (pconfig->source > DEMUX_TS3)) {
				gxlog_e(LOG_DEMUX, "ConfigDmx: the demux module's configure source param error!\n");
				goto err;
			}

			ret = dmx_pfilter_set_config(sub, pconfig);
		}
		break;

	case GxDvrPropertyID_Config:
		{
			GxDvrProperty_Config *config = (GxDvrProperty_Config *) property;

			if (size != sizeof(GxDvrProperty_Config)) {
				gxlog_e(LOG_DEMUX, "the dvr module's configure param error!\n");
				goto err;
			}

			if (dvr_pfilter_set_config(sub, config) < 0) {
				gxlog_e(LOG_DEMUX, "dvr set config error\n");
				goto err;
			}
		}
		break;

	case GxDemuxPropertyID_SlotFree:
		{
			GxDemuxProperty_Slot *pslot = (GxDemuxProperty_Slot *) property;
			if (size != sizeof(GxDemuxProperty_Slot)) {
				gxlog_e(LOG_DEMUX, "slot's param error!\n");
				goto err;
			}

			ret = dvr_pfilter_del_pid(sub, pslot->slot_id);
		}
		break;

	default:
		gxlog_e(LOG_DEMUX, "demux_id(%d)'s property_id(%d) set wrong \n", module->sub, property_id);
		goto err;
	}

	gxdemux_unlock();
	return ret;
err:
	gxdemux_unlock();
	return -1;
}

int dvr_pfilter_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0, sub = module->sub%4;

	gxdemux_lock();
	switch (property_id) {
	case GxDemuxPropertyID_SlotAlloc:
		{
			GxDemuxProperty_Slot *pslot = (GxDemuxProperty_Slot *) property;
			if (size != sizeof(GxDemuxProperty_Slot)) {
				gxlog_e(LOG_DEMUX, "AllocSlot: the param error!\n");
				goto err;
			}

			ret = dvr_pfilter_add_pid(sub, pslot->pid);
			pslot->slot_id = ret;
			ret = (ret >= 0) ? 0 : -1;
		}
		break;

	default:
		gxlog_e(LOG_DEMUX, "demux_id(%d)'s property_id(%d) get wrong \n", module->sub, property_id);
		goto err;
	}

	gxdemux_unlock();
	return ret;
err:
	gxdemux_unlock();
	return -1;
}

