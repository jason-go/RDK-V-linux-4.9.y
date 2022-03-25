#include "kernelcalls.h"
#include "avcore.h"
#include "gxav_bitops.h"
#include "gxdemux.h"
#include "sirius_demux.h"
#include "gx3211_regs.h"
#include "sirius_regs.h"
#include "profile.h"

static void kbuf_free(struct dmxdev *dev, struct dmx_gse_slot *slot)
{
	if (slot->vaddr_data)
		gx_page_free(slot->vaddr_data, slot->hw_buffer_size);

	if (slot->vaddr_status)
		gx_page_free(slot->vaddr_status, BUFFER_GSE_STATUS_SIZE);

	gxfifo_free(&slot->fifo);
	slot->vaddr_data   = NULL;
	slot->vaddr_status = NULL;
}

static int kbuf_malloc(struct dmxdev *dev, struct dmx_gse_slot *slot)
{
	slot->vaddr_status = gx_page_malloc(BUFFER_GSE_STATUS_SIZE);
	if (slot->hw_buffer_size <= BUFFER_GSE_DATA_SIZE) {
		slot->vaddr_data = gx_page_malloc(BUFFER_GSE_DATA_SIZE);
		slot->hw_buffer_size = BUFFER_GSE_DATA_SIZE;
	} else
		slot->vaddr_data = gx_page_malloc(slot->hw_buffer_size);

	gxfifo_init(&slot->fifo, NULL, slot->sw_buffer_size);

	if (slot->vaddr_data == NULL ||
		slot->vaddr_status == NULL) {
		gxlog_e(LOG_DEMUX, "alloc gse memory failed in kernel!\n");
		kbuf_free(dev, slot);
		return -1;
	}

	gx_memset(slot->vaddr_data  , 0, slot->hw_buffer_size);
	gx_memset(slot->vaddr_status, 0, BUFFER_GSE_STATUS_SIZE);
	gx_dcache_clean_range(0,0);

	slot->paddr_data   = gx_virt_to_phys((unsigned int)slot->vaddr_data);
	slot->paddr_status = gx_virt_to_phys((unsigned int)slot->vaddr_status);
	return 0;
}

struct dmx_gse_slot* _dmx_gse_slot_alloc(struct dmx_demux *demux, int id)
{
	int i;
	struct dmxdev *dev = demux->dev;
	struct dmx_gse_slot* slot = NULL;

	if ((i = get_one(dev->gse_slot_mask, 1, 0, dev->max_gse_slot))< 0) {
		gxlog_e(LOG_DEMUX, "have no gse slot left!\n");
		return NULL;
	}
	SET_MASK(dev->gse_slot_mask, i);
	slot = (struct dmx_gse_slot *)gx_malloc(sizeof(struct dmx_gse_slot));
	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "gse slot alloc NULL!\n");
		CLR_MASK(dev->gse_slot_mask, i);
		return NULL;
	}
	gx_memset(slot, 0, sizeof(struct dmx_gse_slot));
	slot->demux         = demux;
	slot->id            = i;
	slot->refcount      = 0;
	demux->gse_slots[i] = slot;

	return slot;
}

int _dmx_gse_slot_free(struct dmx_demux *demux, int id)
{
	struct dmxdev *dev = demux->dev;
	struct demux_gse_ops *ops = dev->gse_ops;
	struct dmx_gse_slot* slot = demux->gse_slots[id];
	if (NULL == slot)
		return -1;

	if (ops->disable_slot)
		ops->disable_slot(dev->gse_reg, id, dev->gse_work_mode);
	if (ops->wait_finish)
		ops->wait_finish(dev->gse_reg, id, &dev->thread_slot_finish);
	if (ops->clr_slot_cfg)
		ops->clr_slot_cfg(dev->gse_reg, id);

	kbuf_free(dev, slot);
	gx_free(slot);
	demux->gse_slots[id] = NULL;
	CLR_MASK(dev->gse_slot_mask, id);
	if (dev->gse_slot_mask == 0)
		dev->gse_work_mode = GSE_MODE_IDLE;

	return 0;
}


int _dmx_gse_slot_enable(struct dmx_demux *demux, int id)
{
	struct dmxdev *dev = demux->dev;
	struct dmx_dvr *dvr = &demux->dvr;
	struct demux_gse_ops *ops = dev->gse_ops;

	CLR_MASK(dev->thread_slot_finish, id);
	if (ops->enable_slot)
		ops->enable_slot(dev->gse_reg, id, dev->gse_work_mode, dvr->mode);

#ifdef FPGA_TEST_STREAM_PRODUCER_EN
	if (dev->gse_work_mode == GSE_MODE_PDU)
		gx_msleep(1000);
	if (dev->fpga_ops->set_stream_addr)
		dev->fpga_ops->set_stream_addr(demux->stream_mode);
	if (dev->fpga_ops->hot_rst)
		dev->fpga_ops->hot_rst(0, dvr->mode);
#endif
	return 0;
}

int _dmx_gse_slot_disable(struct dmx_demux *demux, int id)
{
	struct dmxdev *dev = demux->dev;
	struct demux_gse_ops *ops = dev->gse_ops;

	if (ops->disable_slot)
		ops->disable_slot(dev->gse_reg, id, dev->gse_work_mode);
	if (ops->wait_finish)
		ops->wait_finish(dev->gse_reg, id, &dev->thread_slot_finish);

	return 0;
}

int dvr_gse_open(int sub)
{
	struct dmx_demux *demux = get_subdemux(sub);
	struct dmxdev *dev = demux->dev;

	if (dev->gse_work_mode != GSE_MODE_IDLE)
		return -1;

	_dmx_gse_slot_alloc(demux, 0);
	dev->gse_work_mode = GSE_MODE_RECORD;

	return 0;
}

int dvr_gse_close(int sub)
{
	struct dmx_demux *demux = get_subdemux(sub);
	_dmx_gse_slot_disable(demux, 0);
	_dmx_gse_slot_free(demux, 0);
	return 0;
}

int dvr_gse_set_config(struct dmx_demux *demux, GxDvrProperty_Config *pconfig)
{
	struct dmxdev *dev = demux->dev;
	struct dmx_dvr *dvr = &demux->dvr;
	struct dvr_buffer *buf = &pconfig->dst_buf;
	struct demux_gse_ops *ops = dev->gse_ops;
	struct dmx_gse_slot *slot = NULL;

#ifdef FPGA_TEST_STREAM_PRODUCER_EN
	if (dev->gse_work_mode == GSE_MODE_RECORD)
		gx_msleep(5000);
#endif
	dvr->mode = pconfig->mode;
	slot = demux->gse_slots[0];
	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "gse slot NULL!\n");
		return -1;
	}
	slot->sw_buffer_size   = buf->sw_buffer_size;
	slot->hw_buffer_size   = buf->hw_buffer_size;
	slot->almost_full_gate = buf->almost_full_gate;
	if (kbuf_malloc(dev, slot) < 0)
		return -1;

	if (ops->set_addr)
		ops->set_addr(dev->gse_reg, slot);
	_dmx_gse_slot_enable(demux, 0);

	return 0;
}

int dvr_gse_read(struct gxav_module *module, void *buf, unsigned int size)
{
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(sub);
	struct dmx_gse_slot *slot = demux->gse_slots[0];
	struct gxfifo *fifo = &slot->fifo;
	unsigned int fifo_len;

	if ((NULL == module) || (module->sub < 0) ||
		(module->sub > module->inode->count) || (NULL == buf))
		return -3;

	fifo_len = gxfifo_len(fifo);
	fifo_len = GX_MIN(fifo_len,size);
	gxfifo_user_get(fifo, buf, fifo_len);

	return fifo_len;
}

int dmx_gse_set_config(struct dmx_demux *demux,GxDemuxProperty_ConfigDemux *pconfig)
{
	struct dmxdev *dev = demux->dev;
	struct demux_gse_ops *ops = dev->gse_ops;

	demux->source      = pconfig->source;
	demux->stream_mode = pconfig->stream_mode;

	if (ops->reg_init)
		ops->reg_init(dev->gse_reg);
	if (ops->set_sync_gate)
		ops->set_sync_gate(dev->gse_reg);
	if (ops->set_input_source)
		ops->set_input_source(dev->gse_reg, demux->source);
	if (ops->set_stream_mode)
		ops->set_stream_mode(dev->gse_reg, demux->stream_mode);

	return 0;
}

int dmx_gse_get_lock(struct dmx_demux *demux,GxDemuxProperty_TSLockQuery *pquery)
{
	struct dmxdev *dev = demux->dev;
	struct demux_gse_ops *ops = dev->gse_ops;
	if (ops->query_tslock)
		pquery->ts_lock = ops->query_tslock(dev->gse_reg);

	return 0;
}

int dmx_gse_slot_allocate(struct dmx_demux *demux, GxDemuxProperty_GseSlot *pslot)
{
	int i;
	struct dmxdev *dev = demux->dev;
	struct dmx_gse_slot* slot = NULL;

	if (dev->gse_work_mode == GSE_MODE_RECORD)
		return -1;

	for (i = 0; i < dev->max_gse_slot; i++) {
		slot = demux->gse_slots[i];
		if (slot != NULL) {
			if ((slot->label_type == pslot->label_type) &&
				(slot->protocol   == pslot->protocol)) {
				slot->refcount++;
				pslot->slot_id = i;
				return 0;
			}
		}
	}

	if ((i = get_one(dev->gse_slot_mask, 1, 0, dev->max_gse_slot)) < 0)
		return -1;

	slot = _dmx_gse_slot_alloc(demux, i);
	slot->label_type    = pslot->label_type;
	slot->protocol      = pslot->protocol;
	slot->label.h       = pslot->label.h;
	slot->label.l       = pslot->label.l;
	pslot->slot_id      = i;
	dev->gse_work_mode  = GSE_MODE_PDU;

	return 0;
}

int dmx_gse_slot_free(struct dmx_demux *demux, GxDemuxProperty_GseSlot *pslot)
{
	int id = pslot->slot_id;
	struct dmx_gse_slot *slot = demux->gse_slots[pslot->slot_id];

	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "gse slot NULL!\n");
		return -1;
	}

	if (slot->refcount != 0) {
		slot->refcount--;
		return 0;
	}
	_dmx_gse_slot_free(demux, id);
	return 0;
}

int dmx_gse_slot_config(struct dmx_demux *demux, GxDemuxProperty_GseSlot *pslot)
{
	struct dmx_gse_slot *slot = NULL;
	struct dmxdev *dev = demux->dev;
	struct demux_gse_ops *ops = dev->gse_ops;
	unsigned int pass_flag = ((pslot->flags & DMX_GSE_ALL_PASS_EN) >> 0);

	slot = demux->gse_slots[pslot->slot_id];
	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "gse slot NULL!\n");
		return -1;
	}
	slot->sw_buffer_size   = pslot->sw_buffer_size;
	slot->hw_buffer_size   = pslot->hw_buffer_size;
	slot->almost_full_gate = pslot->almost_full_gate;
	if (kbuf_malloc(dev, slot) < 0)
		return -1;

	if (ops->set_addr)
		ops->set_addr(dev->gse_reg, slot);

	if (pass_flag && ops->all_pass_en)
		ops->all_pass_en(dev->gse_reg, slot->id);
	else {
		if (ops->set_label)
			ops->set_label(dev->gse_reg, slot);
		if (ops->set_protocol)
			ops->set_protocol(dev->gse_reg, slot);
	}

	slot->flags = pslot->flags;
	return 0;
}

int dmx_gse_slot_enable(struct dmx_demux *demux, GxDemuxProperty_GseSlot *pslot)
{
	return _dmx_gse_slot_enable(demux, pslot->slot_id);
}

int dmx_gse_slot_disable(struct dmx_demux *demux, GxDemuxProperty_GseSlot *pslot)
{
	return _dmx_gse_slot_disable(demux, pslot->slot_id);
}

int dmx_gse_slot_attribute(struct dmx_demux *demux,GxDemuxProperty_GseSlot *pslot)
{
	struct dmx_gse_slot *slot = demux->gse_slots[pslot->slot_id];
	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "gse slot is NULL!\n");
		return -1;
	}

	pslot->label.h          = slot->label.h;
	pslot->label.l          = slot->label.l;
	pslot->label_type       = slot->label_type;
	pslot->protocol         = slot->protocol;
	pslot->hw_buffer_size   = slot->hw_buffer_size;
	pslot->sw_buffer_size   = slot->sw_buffer_size;
	pslot->almost_full_gate = slot->almost_full_gate;
	return 0;
}

int dmx_gse_slot_read(struct dmx_demux *demux, GxDemuxProperty_FilterRead *pread)
{
	struct dmxdev *dev = demux->dev;
	struct demux_gse_ops *ops = dev->gse_ops;

	unsigned char *st_addr;
	unsigned char *unuse_buf;
	unsigned char *data_buf = pread->buffer;
	struct dmx_gse_slot *slot = demux->gse_slots[pread->filter_id];
	struct gxfifo *fifo = &slot->fifo;
	unsigned int size = pread->max_size;
	unsigned int unuse_read = 0, buf_pos = 0, id = slot->id;
	unsigned int df_size, st_size, st_pread, st_pwrite;
	unsigned int pdu_len, pdu_addr, pdu_lread, pdu_pread;
	unsigned int fifo_len, read_len = 0, total_len = 0, temp;

	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "gse slot is NULL!\n");
		return -1;
	}

	if (data_buf == NULL) {
		gxlog_e(LOG_DEMUX, "user data buffer is null!");
		return -1;
	}

	fifo_len  = gxfifo_len(fifo);
	size      = GX_MIN(size, fifo_len);
	st_size   = BUFFER_GSE_STATUS_SIZE;
	df_size   = slot->hw_buffer_size;
	st_pread  = ops->get_status_rptr(dev->gse_reg, id);
	st_pwrite = ops->get_status_wptr(dev->gse_reg, id);

	unuse_buf = gx_malloc(0x2000);
	/* discard unuse_data left size */
	if (slot->pdu_unuse_left) {
		unuse_read = gxfifo_user_get(fifo, unuse_buf, slot->pdu_unuse_left);
		slot->pdu_pos += slot->pdu_unuse_left;
		slot->pdu_unuse_left -= unuse_read;
		if (slot->pdu_unuse_left) {
			gx_free(unuse_buf);
			return 0;
		}
	}

	/* analyze all status */
	while (st_pread != st_pwrite) {
		st_addr   = (unsigned char *)((unsigned int)slot->vaddr_status + st_pread);
		pdu_len   = st_addr[7] | (st_addr[6] << 8);
		pdu_addr  = gx_virt_to_phys((st_addr[0]<<24 | st_addr[1]<<16 |
				st_addr[2]<<8 | st_addr[3]));

		pdu_lread =  pdu_addr - (unsigned int)slot->vaddr_data;
		pdu_pread = pdu_lread % slot->hw_buffer_size;

		/* when we get a pdu jump, discard unuse_data */
		if (slot->pdu_pos != pdu_pread) {
			int unuse_len, unuse_read;

			if (read_len > buf_pos) {
				gxfifo_user_get(fifo, data_buf + buf_pos, read_len - buf_pos);
				buf_pos = read_len;
			}

			if (pdu_pread > slot->pdu_pos)
				unuse_len = pdu_pread - slot->pdu_pos;
			else
				unuse_len = pdu_pread + df_size- slot->pdu_pos;

			unuse_read = gxfifo_user_get(fifo, unuse_buf, unuse_len);
			if (unuse_read < unuse_len) {
				slot->pdu_unuse_left = unuse_len - unuse_read;
				slot->pdu_pos = pdu_pread - slot->pdu_unuse_left;
				break;
			}
			total_len += unuse_read;
			slot->pdu_pos = pdu_pread;
		}

		/* when cur total pdu_len is bigger than read_size get readable pdu_data */
		if (total_len + pdu_len > size) {
			if (read_len > buf_pos) {
				gxfifo_user_get(fifo, data_buf + buf_pos, read_len - buf_pos);
				buf_pos = read_len;
			}
			if (pdu_len > size)
				gxlog_e(LOG_DEMUX, "this pdu packet is too large : %d, %d, %d\n", total_len, pdu_len, size);

			break;
		}

		/* count readable pdu_len */
		read_len += pdu_len;
		total_len += pdu_len;

		temp = pdu_pread + pdu_len;
		slot->pdu_pos = (temp > df_size) ? temp - df_size : temp;
		st_pread = (st_pread == st_size - 8) ? 0 : st_pread + 8;

		/* when we analyze all readable status get readable pdu_data */
		if (st_pread == st_pwrite) {
			if (read_len > buf_pos) {
				gxfifo_user_get(fifo, data_buf + buf_pos, read_len - buf_pos);
				buf_pos = read_len;
			}
		}
	}
	ops->set_status_rptr(dev->gse_reg, id, st_pread);

	gx_free(unuse_buf);
	return read_len;
}

int dmx_gse_slot_fifo_query(struct dmx_demux *demux, GxDemuxProperty_FilterFifoQuery *pquery)
{
	struct dmxdev *dev = demux->dev;
	struct dmx_gse_slot *slot;

	unsigned int len, i;
	unsigned int id = 0, st=0;
	struct gxfifo *fifo = NULL;

	for (i = 0; i < dev->max_gse_slot; i++) {
		id = (0x1 << i);
		slot = demux->gse_slots[i];
		if (slot != NULL) {
			fifo = &slot->fifo;
			len = gxfifo_len(fifo);
			if (len) {
				st |= id;
			} else {
				st &= ~id;
			}
		}
	}
	pquery->state = st;
	return (pquery->state);
}

int dmx_gse_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0;
	struct dmx_demux *demux = get_subdemux(module->sub%4);

	gxdemux_lock();
	switch (property_id) {
	case GxDvrPropertyID_Config:
		{
			GxDvrProperty_Config *config = (GxDvrProperty_Config *) property;
			if (size != sizeof(GxDvrProperty_Config)) {
				gxlog_e(LOG_DEMUX, "the dvr module's configure param error!\n");
				goto err;
			}

			ret = dvr_gse_set_config(demux, config);
		}
		break;

	case GxDemuxPropertyID_Config:
		{
			GxDemuxProperty_ConfigDemux *pconfig = (GxDemuxProperty_ConfigDemux *) property;
			if (size != sizeof(GxDemuxProperty_ConfigDemux)) {
				gxlog_e(LOG_DEMUX, "ConfigDmx: the param error!\n");
				goto err;
			}

			if ((pconfig->sync_lock_gate > 0xf) || (pconfig->sync_loss_gate > 0xf) ||
				(pconfig->time_gate > 0xf) || (pconfig->byt_cnt_err_gate > 0xf)) {
				gxlog_e(LOG_DEMUX, "ConfigDmx: the demux module's configure gate param error!\n");
				goto err;
			}

			if ((pconfig->source < DEMUX_TS1) || (pconfig->source > DEMUX_SDRAM)) {
				gxlog_e(LOG_DEMUX, "ConfigDmx: the demux module's configure source param error!\n");
				goto err;
			}

			ret = dmx_gse_set_config(demux,pconfig);
		}
		break;

	case GxDemuxPropertyID_SlotConfig:
	case GxDemuxPropertyID_SlotEnable:
	case GxDemuxPropertyID_SlotDisable:
	case GxDemuxPropertyID_SlotFree:
		{
			GxDemuxProperty_GseSlot *pgse = (GxDemuxProperty_GseSlot *) property;

			if (size != sizeof(GxDemuxProperty_GseSlot)) {
				gxlog_e(LOG_DEMUX, "ConfigGseSlot:filter's configure filter property error!\n");
				goto err;
			}

			if ((pgse->slot_id >= MAX_SLOT_NUM) || (pgse->slot_id < 0)) {
				gxlog_e(LOG_DEMUX, "slot id error!\n");
				goto err;
			}

			if (GxDemuxPropertyID_SlotConfig == property_id) {
				if (dmx_gse_slot_config(demux, pgse) < 0) {
					gxlog_e(LOG_DEMUX, "SlotConfig: error!\n");
					goto err;
				}
			} else if (GxDemuxPropertyID_SlotEnable == property_id) {
				if (dmx_gse_slot_enable(demux, pgse) < 0) {
					gxlog_e(LOG_DEMUX, "SlotEnable: error!\n");
					goto err;
				}
			} else if (GxDemuxPropertyID_SlotDisable == property_id) {
				if (dmx_gse_slot_disable(demux, pgse) < 0) {
					gxlog_e(LOG_DEMUX, "SlotDisable: error!\n");
					goto err;
				}
			} else if (GxDemuxPropertyID_SlotFree == property_id) {
				if (dmx_gse_slot_free(demux, pgse) < 0) {
					gxlog_e(LOG_DEMUX, "SlotFree: error!\n");
					goto err;
				}
			}
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

int dmx_gse_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0;
	struct dmx_demux *demux = get_subdemux(module->sub%4);

	gxdemux_lock();
	switch (property_id) {
	case GxDemuxPropertyID_TSLockQuery:
		{
			GxDemuxProperty_TSLockQuery *pquery = (GxDemuxProperty_TSLockQuery *) property;
			if (size != sizeof(GxDemuxProperty_TSLockQuery)) {
				gxlog_e(LOG_DEMUX, "QueryTsLock: the param error!\n");
				goto err;
			}

			ret = dmx_gse_get_lock(demux, pquery);
		}
		break;

	case GxDemuxPropertyID_SlotAlloc:
		{
			GxDemuxProperty_GseSlot *pgse = (GxDemuxProperty_GseSlot *) property;
			if (size != sizeof(GxDemuxProperty_GseSlot)) {
				gxlog_e(LOG_DEMUX, "Alloc gse slot: the param error!\n");
				goto err;
			}

			ret = dmx_gse_slot_allocate(demux, pgse);
		}
		break;

	case GxDemuxPropertyID_SlotConfig:
		{
			GxDemuxProperty_GseSlot *pgse = (GxDemuxProperty_GseSlot *) property;
			if (size != sizeof(GxDemuxProperty_Slot)) {
				gxlog_e(LOG_DEMUX, "SlotConfig: the param error!\n");
				goto err;
			}

			ret = dmx_gse_slot_attribute(demux, pgse);
		}
		break;

	case GxDemuxPropertyID_FilterRead:
		{
			GxDemuxProperty_FilterRead *pread = (GxDemuxProperty_FilterRead *) property;
			if (size != sizeof(GxDemuxProperty_FilterRead)) {
				gxlog_e(LOG_DEMUX, "FilterRead: the param error!\n");
				goto err;
			}

			ret = dmx_gse_slot_read(demux, pread);
		}
		break;

	case GxDemuxPropertyID_FilterFIFOQuery:
		{
			GxDemuxProperty_FilterFifoQuery *pquery = (GxDemuxProperty_FilterFifoQuery *) property;
			if (size != sizeof(GxDemuxProperty_FilterFifoQuery)) {
				gxlog_e(LOG_DEMUX, "QueryFilterFifo: the param error!\n");
				goto err;
			}

			ret = dmx_gse_slot_fifo_query(demux, pquery);
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

