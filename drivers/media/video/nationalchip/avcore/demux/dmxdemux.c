#include "fifo.h"
#include "gxav_bitops.h"
#include "porting.h"
#include "sdc_hal.h"
#include "stc_hal.h"
#include "sdc_module.h"
#include "gxdemux.h"
#include "kernelcalls.h"
#include "profile.h"
#include "gx3211_demux.h"
#include "firewall.h"

int get_one(long long mask, unsigned int count, unsigned int min, unsigned int max)
{
	int i;
	long long temp  = mask;
	unsigned int value = 1;

	if (count >= 2 || count < 32) {
		for (i = 0; i < count; i++)
			value |= 0x1<<i;
	}

	for (i = min; i < max; i++) {
		if (((temp >> i)&value) == 0)
			return i;
	}
	return -1;
}

struct dmx_filter_buffer {
	unsigned char *vaddr;
	unsigned int   paddr;
	unsigned int   size;
	unsigned int   used;
};

static struct dmx_filter_buffer buffer_array[MAX_FILTER_NUM*2];
int _dmx_filter_get_buffer(struct dmxdev *dev, struct dmx_filter *filter)
{
	int id, pos;
	static int init = 0;
	if (init == 0) {
		memset(buffer_array, 0, sizeof(buffer_array));
		init = 1;
	}

	id  = filter->id;
	pos = (dev->id) * (dev->max_filter);
	if (buffer_array[pos+id].used == 0)
		return -1;

	filter->vaddr = buffer_array[pos+id].vaddr;
	filter->paddr = buffer_array[pos+id].paddr;
	filter->size  = buffer_array[pos+id].size;
	return id;
}

int _dmx_filter_set_buffer(struct dmxdev *dev, struct dmx_filter *filter)
{
	int id, pos;

	id  = filter->id;
	pos = (dev->id) * (dev->max_filter);
	if (buffer_array[pos+id].used == 1)
		return -1;

	buffer_array[pos+id].vaddr = filter->vaddr;
	buffer_array[pos+id].paddr = filter->paddr;
	buffer_array[pos+id].size  = filter->size;
	buffer_array[pos+id].used  = 1;
	return id;
}

static int kbuf_malloc(struct dmxdev *dev, struct dmx_filter *filter)
{
	unsigned int min_psi_bufsize = 0, min_pes_bufsize = 0;
	SET_DEFAULT_VALUE(min_psi_bufsize, dev->filter_min_psi_hw_bufsize, BUFFER_PSI_SIZE);
	SET_DEFAULT_VALUE(min_pes_bufsize, dev->filter_min_pes_hw_bufsize, BUFFER_PES_SIZE);

	if (dev->filter_nofree_en) {
		if (_dmx_filter_get_buffer(dev, filter) >= 0)
			return 0;
	}

	switch (filter->type)
	{
		case DEMUX_SLOT_PSI:
			if (filter->size <= min_psi_bufsize) {
				filter->size = min_psi_bufsize;
				if (dev->filter_nofree_en)
					filter->size = min_pes_bufsize;
			}
			filter->vaddr = gx_page_malloc(filter->size);
			break;
		case DEMUX_SLOT_PES:
		case DEMUX_SLOT_PES_AUDIO:
		case DEMUX_SLOT_PES_VIDEO:
			if (filter->size <= min_pes_bufsize)
				filter->size = min_pes_bufsize;
			filter->vaddr = gx_page_malloc(filter->size);
			break;
		default:
			break;
	}

	if (filter->vaddr == NULL) {
		gxlog_e(LOG_DEMUX, "alloclate memory failed in kernel!\n");
		return -1;
	}
	filter->paddr = gx_virt_to_phys((unsigned int)filter->vaddr);
	if (dev->filter_nofree_en)
		_dmx_filter_set_buffer(dev, filter);

	return 0;
}

static void kbuf_free(struct dmxdev *dev, struct dmx_filter *filter)
{
    int id, pos;
	if (filter->vaddr == NULL)
		return;

	id  = filter->id;
	pos = (dev->id) * (dev->max_filter);

	if (dev->filter_nofree_en == 0 ||
		(dev->filter_nofree_en > 0 && buffer_array[pos+id].used == 0)) {
		gx_page_free(filter->vaddr, filter->size);
		buffer_array[pos+id].used = 0;
	}
	filter->vaddr = NULL;
}

void delay_free(struct dmxdev *dev)
{
	struct dmx_demux *demux;
	struct dmx_filter *filter = NULL;
	struct timeval tv;
	struct timezone tz;
	struct timeval *ftv = NULL;
	int i=0, id, dmx_id;
	unsigned long flags;

	for (dmx_id = 0; dmx_id < 2; dmx_id++) {
		demux = &dev->demuxs[dmx_id];
		for (i = 0; i < dev->max_filter; i++) {
			long time=0;
			gx_spin_lock_irqsave(&(dev->df_spin_lock[i]), flags);
			filter = demux->filters[i];
			if (NULL == filter) {
				gx_spin_unlock_irqrestore(&(dev->df_spin_lock[i]), flags);
				continue;
			}

			if (filter->status == DMX_STATE_DELAY_FREE) {
				gx_gettimeofday(&tv,&tz);
				ftv = &(filter->tv);
				time = ((tv.tv_sec*1000000+tv.tv_usec)-(ftv->tv_sec*1000000+ftv->tv_usec));
				gxlog_d(LOG_DEMUX, "i %d time %ld\n",i, time);
			}

			if (time > 4000 || filter->status == DMX_STATE_FREE) {
				id = filter->id;
				if (ftv)
					gxlog_d(LOG_DEMUX, "i %d id %d old sec %ld old usec%ld t_sec %ld\n",i,id,filter->tv.tv_sec,filter->tv.tv_usec,(ftv->tv_sec*1000000+ftv->tv_usec));
				gxlog_d(LOG_DEMUX, "i %d id %d new sec %ld new usec%ld t_sec %ld\n",i,id,tv.tv_sec,tv.tv_usec,(tv.tv_sec*1000000+tv.tv_usec));
				filter->status = DMX_STATE_FREE;
				if (dev->filter_nofree_en == 0)
					dev->ops->set_dfbuf(dev->reg,id,0,0,0);

				kbuf_free(dev, filter);
				demux->filters[id] = NULL;
				if (filter->flags & DMX_MAP)
					CLR_MASK(dev->map_filter_mask, id);
				else
					gx_free(filter);
				CLR_MASK(dev->filter_mask, id);
			}
			gx_spin_unlock_irqrestore(&(dev->df_spin_lock[i]), flags);
		}
	}
}

static void _demux_avchannel_enable(struct dmx_demux *demux, struct dmx_slot *slot)
{
	unsigned long flags;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;

	if (slot->avid < 0)
		return;

	if ((DEMUX_SLOT_AUDIO == slot->type) || (DEMUX_SLOT_VIDEO == slot->type)) {
		gx_spin_lock_irqsave(&(dev->spin_lock),flags);
		if (ops->enable_av_int)
			ops->enable_av_int(dev->reg, slot->avid);
		gx_spin_unlock_irqrestore(&(dev->spin_lock),flags);
	}
}

static void _demux_avchannel_disable(struct dmx_demux *demux, struct dmx_slot *slot)
{
	unsigned long flags;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;

	if (slot->avid < 0)
		return;

	if ((DEMUX_SLOT_AUDIO == slot->type) || (DEMUX_SLOT_VIDEO == slot->type)) {
		gx_spin_lock_irqsave(&(dev->spin_lock),flags);
		if (ops->disable_av_int)
			ops->disable_av_int(dev->reg, slot->avid);
		gx_spin_unlock_irqrestore(&(dev->spin_lock),flags);
	}
}

void demux_source_debug_print(unsigned int dmxid)
{
#if (GX_DEBUG_PRINTF_LEVEL >= 4)
	struct dmx_demux *demux = get_subdemux(dmxid%4);
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;

	if (dmxid >= dev->max_dvr) {
		gxlog_d(LOG_DEMUX, "%s %d: Parameter error.\n", __func__, __LINE__);
		return;
	}

	if (ops->log_d_source)
		ops->log_d_source(dev->reg);
#endif
}

void demux_slot_debug_print(unsigned int dmxid, unsigned int slotid)
{
#if (GX_DEBUG_PRINTF_LEVEL >= 4)
	unsigned short hwpid = 0;
	struct dmx_demux *demux = get_subdemux(dmxid%4);
	struct dmxdev *dev = demux->dev;
	struct dmx_slot *slot = demux->slots[slotid];

	if (dmxid >= dev->max_dvr || slotid >= dev->max_slot) {
		gxlog_d(LOG_DEMUX, "%s %d: Parameter error.\n", __func__, __LINE__);
		return;
	}

	gx3211_get_pid(dev->reg, slotid, &hwpid);

	if (NULL == slot)
		gxlog_d(LOG_DEMUX, "DEMUX(%d) Slot(%d) is NULL\n", dmxid, slotid);
	else {
		gxlog_d(LOG_DEMUX, "DEMUX(%d) Slot(%d) :\n", dmxid, slotid);
		gxlog_d(LOG_DEMUX, "\tPID=0x%x, HWPID=0x%x, type=%d, filternum=%d, mask=0x%llx\n",
				slot->pid, hwpid, slot->type, slot->filter_num, dev->slot_mask);
		gxlog_d(LOG_DEMUX, "\tFlagCRCDisable=%d\n", (slot->flags & DMX_CRC_DISABLE) >> 0);
		gxlog_d(LOG_DEMUX, "\tFlagRepeat=%d\n", (slot->flags & DMX_REPEAT_MODE) >> 1);
		gxlog_d(LOG_DEMUX, "\tFlagTSOut=%d\n", (slot->flags & DMX_TSOUT_EN) >> 4);
		gxlog_d(LOG_DEMUX, "\tFlagTSDesOut=%d\n", (slot->flags & DMX_DES_EN) >> 6);
		gxlog_d(LOG_DEMUX, "\tFlagAVOut=%d\n", (slot->flags & DMX_AVOUT_EN) >> 5);
		gxlog_d(LOG_DEMUX, "\tFlagDiscard=%d\n", ((slot->flags & DMX_ERR_DISCARD_EN)>>7) |
				((slot->flags & DMX_DUP_DISCARD_EN)>>9));

		if (slot->type == DEMUX_SLOT_VIDEO || slot->type == DEMUX_SLOT_AUDIO)
			gxlog_d(LOG_DEMUX, "\tLink AVBufID = %d\n", slot->avid);
		if (slot->flags & DMX_TSOUT_EN)
			gxlog_d(LOG_DEMUX, "\tLink DVRHandle = %d\n", slot->ts_out_pin);
		if (slot->flags & DMX_DES_EN)
			gxlog_d(LOG_DEMUX, "\tLink DescrambleID = %d\n", slot->casid);
	}
#endif
}

void demux_filter_debug_print(unsigned int dmxid, unsigned int filterid)
{
#if (GX_DEBUG_PRINTF_LEVEL >= 4)
	int i = 0;
	struct dmx_demux *demux = get_subdemux(dmxid%4);
	struct dmx_filter *filter = demux->filters[filterid];
	struct dmxdev *dev = demux->dev;

	if (dmxid >= dev->max_dvr || filterid >= dev->max_filter) {
		gxlog_d(LOG_DEMUX, "%s %d: Parameter error.\n", __func__, __LINE__);
		return;
	}

	if (NULL == filter)
		gxlog_d(LOG_DEMUX, "DEMUX(%d) Filter(%d) is NULL\n", dmxid, filterid);
	else {
		if (NULL == filter->slot) {
			gxlog_d(LOG_DEMUX, "DEMUX(%d) Filter(%d)'s Slot is NULL\n", dmxid, filterid);
			return;
		}
		gxlog_d(LOG_DEMUX, "DEMUX(%d) Filter(%d) :\n", dmxid, filterid);
		gxlog_d(LOG_DEMUX, "\tSlotID=0x%d, Depth=%d, nofree=%d, mask=0x%llx\n",
				filter->slot->id, filter->depth, dev->filter_nofree_en, dev->filter_mask);
		gxlog_d(LOG_DEMUX, "\tFlagCRC=%d\n", (filter->flags & DMX_CRC_IRQ) >> 0);
		gxlog_d(LOG_DEMUX, "\tFlagEQ=%d\n", (filter->flags & DMX_EQ) >> 1);
		gxlog_d(LOG_DEMUX, "\tFlagSW=%d\n", (filter->flags & DMX_SW_FILTER) >> 7);

		for (i = 0; i < filter->depth; i++)
			gxlog_d(LOG_DEMUX, "\tKeyValue[%d]=0x%08x, KeyMask[%d]=0x%08x\n",
				i, filter->key[i].value, i, filter->key[i].mask);
	}
#endif
}

void demux_cas_debug_print(unsigned int dmxid, unsigned int casid)
{
#if (GX_DEBUG_PRINTF_LEVEL >= 4)
	struct dmx_demux *demux = get_subdemux(dmxid%4);
	struct dmxdev *dev = demux->dev;
	struct dmx_cas *cas = dev->cas_configs[casid];

	if (dmxid >= dev->max_dvr || casid >= dev->max_cas) {
		gxlog_d(LOG_DEMUX, "%s %d: Parameter error.\n", __func__, __LINE__);
		return;
	}

	if (NULL == cas)
		gxlog_d(LOG_DEMUX, "DEMUX(%d) CAS(%d) is NULL\n", dmxid, casid);
	else {
		gxlog_d(LOG_DEMUX, "DEMUX(%d) CAS(%d) :\n", dmxid, casid);
		gxlog_d(LOG_DEMUX, "\talg=%d, mask=0x%x\n", cas->ds_mode, dev->cas_mask);
		gxlog_d(LOG_DEMUX, "\tEVEN CW = %08x %08x %08x %08x\n",
				cas->cw_even[0],cas->cw_even[1],cas->cw_even[2],cas->cw_even[3]);
		gxlog_d(LOG_DEMUX, "\tEVEN IV = %08x %08x %08x %08x\n",
				cas->iv_even[0],cas->iv_even[1],cas->iv_even[2],cas->iv_even[3]);
		gxlog_d(LOG_DEMUX, "\tODD CW  = %08x %08x %08x %08x\n",
				cas->cw_odd[0], cas->cw_odd[1], cas->cw_odd[2], cas->cw_odd[3]);
		gxlog_d(LOG_DEMUX, "\tODD IV  = %08x %08x %08x %08x\n",
				cas->iv_odd[0], cas->iv_odd[1], cas->iv_odd[2], cas->iv_odd[3]);
		gxlog_d(LOG_DEMUX, "\tSYSKey  = %08x %08x\n", cas->sys_sel_even, cas->sys_sel_odd);
	}
#endif
}

int demux_set_config(struct dmx_demux *demux,unsigned int module_sub, GxDemuxProperty_ConfigDemux *pconfig)
{
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	demux->sync_lock_gate = pconfig->sync_lock_gate;
	demux->sync_loss_gate = pconfig->sync_loss_gate;
	demux->time_gate = pconfig->time_gate;
	demux->byt_cnt_err_gate = pconfig->byt_cnt_err_gate;
	if (ops->set_sync_gate)
		ops->set_sync_gate(dev->reg,demux->time_gate,demux->byt_cnt_err_gate,
				demux->sync_loss_gate,demux->sync_lock_gate);

	demux->stream_mode = pconfig->stream_mode;
	demux->ts_select = pconfig->ts_select;
	demux->source = pconfig->source;

	if (ops->set_input_source)
		ops->set_input_source(dev->reg,module_sub,demux->source);

	return 0;
}

int demux_attribute(struct dmx_demux *demux,GxDemuxProperty_ConfigDemux *pconfig)
{
	pconfig->source           = demux->source;
	pconfig->ts_select        = demux->ts_select;
	pconfig->stream_mode      = demux->stream_mode;
	pconfig->time_gate        = demux->time_gate;
	pconfig->sync_lock_gate   = demux->sync_lock_gate;
	pconfig->sync_loss_gate   = demux->sync_loss_gate;
	pconfig->byt_cnt_err_gate = demux->byt_cnt_err_gate;
	return 0;
}

int demux_set_pcrpid(struct dmx_demux *demux,GxDemuxProperty_Pcr *pcr)
{
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	demux->pcr_pid = pcr->pcr_pid;
	if (ops->set_pcr_sync)
		ops->set_pcr_sync(dev->reg,demux->id,demux->pcr_pid);
	return 0;
}

int demux_get_lock(struct dmx_demux *demux,GxDemuxProperty_TSLockQuery *pquery)
{
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	if (ops->query_tslock)
		pquery->ts_lock = ops->query_tslock(dev->reg, demux->id, demux->source);

	demux_source_debug_print(dev->id*2 + demux->id);
	return 0;
}

int _dmx_get_avchannel_id(struct dmxdev *dev, unsigned int addr)
{
	int id = dev->max_av;
	int first_valid_avbuf_id = 0;
	unsigned int avbuf_mask = dev->avbuf_mask;
	unsigned int avbuf_used = dev->avbuf_used;

	// Sirius C版芯片 前两路demux只能使用 2,3路av buf
	if (CHIP_IS_SIRIUS && dev->id == 0) {
		first_valid_avbuf_id = 2;
	}

	if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)) {
		for (id = first_valid_avbuf_id; id < dev->max_av; id++) {
			if (addr == dev->secure_av_buf[id])
				break;
		}
	}

	if (id == dev->max_av)
		if ((id = get_one(avbuf_used, 1, first_valid_avbuf_id, dev->max_av)) < 0)
			return -1;

	if ((0x1<<id) & avbuf_mask)
		return -1;

	SET_MASK(dev->avbuf_used, id);
	return id;
}

int demux_link_fifo(struct dmx_demux *demux, struct demux_fifo *fifo)
{
	unsigned int start_addr;
	unsigned int end_addr;
	unsigned char buf_id;
	unsigned int almostfull;
	unsigned int almostempty;
	unsigned int size;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	start_addr = fifo->buffer_start_addr;
	end_addr   = fifo->buffer_end_addr;
	buf_id     = fifo->buffer_id;
	size       = end_addr - start_addr + 1;

	if (GXAV_PIN_OUTPUT == fifo->direction && fifo->pin_id < PIN_ID_SDRAM_OUTPUT) {
		struct dmx_slot *slot = NULL;
		struct gxav_channel *channel = (struct gxav_channel *)fifo->channel;
		int channel_id = (buf_id - 1);
		slot = demux->slots[fifo->pin_id];
		if (slot == NULL) {
			gxlog_e(LOG_DEMUX, "slot is NULL\n");
			return -1;
		}
		if ((slot->type < DEMUX_SLOT_AUDIO) || (slot->type > DEMUX_SLOT_VIDEO)) {
			gxlog_e(LOG_DEMUX, "link av buffer buf slot type is other!\n");
			return -1;
		}
		if ((slot->avid = _dmx_get_avchannel_id(dev, start_addr)) < 0) {
			gxlog_e(LOG_DEMUX, "has no avchannel left\n");
			return -1;
		}
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES))
			dev->secure_av_buf[slot->avid] = start_addr;
		SET_MASK(dev->avbuf_mask, slot->avid);
		dev->avfifo[slot->avid] = *fifo;

		gxav_sdc_algate_get(channel_id,&almostempty,&almostfull);

		if (ops->link_avbuf)
			ops->link_avbuf(dev->id, dev->reg,slot->avid, start_addr, size, almostfull);
		gxav_firewall_register_buffer(GXAV_BUFFER_DEMUX_ES, start_addr, size);


		if (ops->set_avid)
			ops->set_avid(dev->reg,slot->id,slot->avid);
		if (dvr_ops && dvr_ops->set_tsr_avid)
			dvr_ops->set_tsr_avid(dev->id, dev->reg, slot->avid);

		start_addr = fifo->pts_start_addr;
		end_addr   = fifo->pts_end_addr;
		buf_id     = fifo->pts_buffer_id;
		if (ops->link_ptsbuf)
			ops->link_ptsbuf(dev->reg,slot->avid, start_addr, end_addr - start_addr + 1, almostfull);
		gxav_sdc_buffer_reset(fifo->channel_id);
		gxav_sdc_buffer_reset(fifo->pts_channel_id);

		channel->outdata     = (void *)slot;
		if (dev->ops->avbuf_cb)
			channel->outcallback = dev->ops->avbuf_cb;
	}

	return 0;
}

int demux_unlink_fifo(struct dmx_demux *demux, struct demux_fifo *fifo)
{
	unsigned long flags;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;
	if (GXAV_PIN_OUTPUT == fifo->direction && fifo->pin_id < PIN_ID_SDRAM_OUTPUT) {
		struct gxav_channel *channel = (struct gxav_channel *)(fifo->channel);
		struct dmx_slot *slot = demux->slots[fifo->pin_id];

		flags = gx_sdc_spin_lock_irqsave(channel->channel_id);
		channel->outcallback = NULL;
		channel->outdata = NULL;
		gx_sdc_spin_unlock_irqrestore(channel->channel_id, flags);

		if (slot == NULL || slot->avid < 0)
			return -1;
		if ((slot->type < DEMUX_SLOT_AUDIO) || (slot->type > DEMUX_SLOT_VIDEO)) {
			gxlog_e(LOG_DEMUX, "link av buffer buf slot type is other!\n");
			return -1;
		}
		if (ops->unlink_avbuf)
			ops->unlink_avbuf(dev->id, dev->reg,slot->avid);
		if (dvr_ops && dvr_ops->clr_tsr_avid)
			dvr_ops->clr_tsr_avid(dev->id, dev->reg, slot->avid);
		if (ops->unlink_ptsbuf)
			ops->unlink_ptsbuf(dev->reg,slot->avid);
		gx_memset(&(dev->avfifo[slot->avid]), 0 ,sizeof(struct demux_fifo));
		CLR_MASK(dev->avbuf_mask, slot->avid);
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES) == 0)
			CLR_MASK(dev->avbuf_used, slot->avid);
		slot->avid = -1;
	}

	return 0;
}

static int _dmx_get_slot_id(struct dmx_demux *demux, enum dmx_slot_type type)
{
	int ret;
	struct dmxdev *dev = demux->dev;
	long long slot_mask = dev->slot_mask;

	if (type == DEMUX_SLOT_AUDIO || type == DEMUX_SLOT_VIDEO)
		return get_one(slot_mask, 1, 0, MAX_AVSLOT_FOR_MULTI_TRACK);

	if ((ret = get_one(slot_mask, 1, MAX_AVSLOT_FOR_MULTI_TRACK, dev->max_slot)) < 0)
		return get_one(slot_mask, 1, 0, MAX_AVSLOT_FOR_MULTI_TRACK);
	return ret;
}

int demux_slot_allocate(struct dmx_demux *demux,GxDemuxProperty_Slot *pslot)
{
	int i;
	struct dmx_slot* slot = NULL;
	struct dmxdev *dev = demux->dev;
	enum dmx_slot_type type = pslot->type;
	unsigned short pid = pslot->pid;

	for (i = 0; i < dev->max_slot; i++) {
		slot = demux->slots[i];
		if (slot != NULL) {
			if ((slot->pid == pid) && (slot->type != type))
				gxlog_w(LOG_DEMUX, "Find same slot : pid = 0x%x, type = %x, alloc_type = %x!\n",
						pid, slot->type, type);

			if ((slot->pid == pid) && (slot->type == type) &&
				(pid != 0x1fff) && (slot->demux->id == demux->id)) {
				slot->refcount++;
				pslot->slot_id = i;
				return 0;
			}
		}
	}

	i = _dmx_get_slot_id(demux, type);
	if (i < 0) {
		gxlog_e(LOG_DEMUX, "have no slot left!\n");
		goto err;
	}
	SET_MASK(dev->slot_mask, i);

	slot = (struct dmx_slot *)gx_malloc(sizeof(struct dmx_slot));
	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "slot alloc NULL!\n");
		goto err;
	}

	gx_memset(slot, 0, sizeof(struct dmx_slot));
	slot->demux = demux;
	slot->id = i;
	slot->pid = pid;
	slot->type = type;
	slot->filter_num = 0;
	slot->refcount = 0;
	slot->avid = -1;
	demux->slots[i] = slot;
	pslot->slot_id = i;
	return 0;

err:
	if (i < 0)
		return -1;
	if (slot) {
		demux->slots[i] = NULL;
		gx_free(slot);
	}
	CLR_MASK(dev->slot_mask, i);

	pslot->slot_id = -1;
	return -1;
}

int demux_slot_free(struct dmx_demux *demux,GxDemuxProperty_Slot *pslot)
{
	struct dmx_slot *slot = NULL;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	int id = 0;

	slot = demux->slots[pslot->slot_id];
	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "the demux slot's free slot param NULL!\n");
		return -1;
	}

	if (slot->refcount != 0) {
		slot->refcount--;
		return 0;
	}

	if (slot->filter_num > 0) { /*if the filter_num nequal 0,you need free filter */
		gxlog_e(LOG_DEMUX, "Don't be lazy, the original release work is now out of support! Please release filter first!!!! \n");
		return -1;
	}

	id = slot->id;
	_demux_avchannel_disable(demux, slot);
	if (ops->disable_slot)
		ops->disable_slot(dev->reg,demux->id,id);
	if (ops->clr_slot_cfg)
		ops->clr_slot_cfg(dev->reg,id);
	CLR_MASK(dev->slot_mask, id);
	gxlog_d(LOG_DEMUX, "dev->slot_mask = 0x%llx\n", dev->slot_mask);

	if (slot->avid >= 0 && ((DEMUX_SLOT_AUDIO == slot->type) || (DEMUX_SLOT_VIDEO == slot->type))) {
		struct demux_fifo *avfifo = &dev->avfifo[slot->avid];
		if (avfifo->channel) {
			unsigned long flags;
			struct gxav_channel *channel = (struct gxav_channel *)avfifo->channel;
			flags = gx_sdc_spin_lock_irqsave(channel->channel_id);
			channel->outcallback = NULL;
			channel->outdata     = NULL;
			gx_sdc_spin_unlock_irqrestore(channel->channel_id, flags);
			gx_memset(avfifo, 0,sizeof(struct demux_fifo));
		}
	}

	gx_free(slot);
	demux->slots[id] = NULL;

	return 0;
}

static int _dmx_slot_bind_cas(struct dmx_demux *demux, struct dmx_slot *slot)
{
	int i = 0;
	unsigned int caid = 0;
	unsigned short pid = slot->pid;
	struct dmxdev * dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_cas *cas = NULL;

	for (i = 0; i < MAX_CAS_PID_NUM; i++) {
		if ((dev->pid_mask & (1<<i)) == 0)
			continue;

		if ((dev->pid_array[i] & DMX_CAS_PID_MASK) != pid)
			continue;

		caid = (dev->pid_array[i] >> DMX_CAS_CAID_POS) & DMX_CAS_CAID_MASK;
		if (NULL == dev->cas_configs[caid])
			return -1;

		cas = dev->cas_configs[caid];
		if ((demux->source == DEMUX_SDRAM) && (cas->flags & DMX_CAS_BIND_ALL_SLOT))
			return 0;

		slot->casid = caid;
		if (cas->flags & DMX_CAS_VERSION_2) {
			if (ops->set_cas_descid)
				ops->set_cas_descid(dev->reg, slot->id, caid);
			if (ops->set_cas_key_valid)
				ops->set_cas_key_valid(dev->reg, slot->id);
		} else {
			if (ops->set_descid)
				ops->set_descid(dev->reg, slot->id, caid, cas->flags);
			if (ops->set_even_valid)
				ops->set_even_valid(dev->reg, slot->id);
			if (ops->set_odd_valid)
				ops->set_odd_valid(dev->reg, slot->id);
		}
	}
	return 0;
}

int demux_slot_config(struct dmx_demux *demux,GxDemuxProperty_Slot *pslot)
{
	struct dmx_slot *slot = NULL;
	struct dmxdev *dev = demux->dev;
	struct dmx_dvr *dvr = &demux->dvr;
	struct demux_ops *ops = dev->ops;
	unsigned int crc_disable_flag = ((pslot->flags & DMX_CRC_DISABLE) >> 0);
	unsigned int repeat_flag = ((pslot->flags & DMX_REPEAT_MODE) >> 1);
	unsigned int pts_insert_flag = ((pslot->flags & DMX_PTS_INSERT) >> 2);
	unsigned int pts_to_sdram_flag = ((pslot->flags & DMX_PTS_TO_SDRAM) >> 3);
	unsigned int tsout_flag = ((pslot->flags & DMX_TSOUT_EN) >> 4);
	unsigned int avout_flag = ((pslot->flags & DMX_AVOUT_EN) >> 5);
	unsigned int tsdesout_flag = ((pslot->flags & DMX_DES_EN) >> 6);
	unsigned int discard_flag = ((pslot->flags & DMX_ERR_DISCARD_EN) >> 7);
	unsigned int dup_flag = ((pslot->flags & DMX_DUP_DISCARD_EN) >> 10);
	unsigned int t2miout_flag = ((pslot->flags & DMX_T2MI_OUT_EN) >> 11);
	int pid  = pslot->pid, i = 0;

	slot = demux->slots[pslot->slot_id];
	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "ConfigSlot: slot NULL!\n");
		return -1;
	}

	if (slot->type != pslot->type) {
		gxlog_e(LOG_DEMUX, "ConfigSlot: the config slot type != alloc slot type!!!\n");
		return -1;
	}

	if ((slot->refcount != 0) && (repeat_flag == 0 || slot->pid != pid)) {
		gxlog_e(LOG_DEMUX, "ConfigSlot: The slot used by multiple people!!!\n");
		return -3;
	}

	for (i = 0; i < dev->max_slot; i++) {
		if (i == pslot->slot_id)
			continue;

		slot = demux->slots[i];
		if (slot != NULL) {
			if ((slot->pid == pid) && (pid != 0x1fff)) {
				gxlog_e(LOG_DEMUX, "ConfigSlot: the pid(0x%x) is exists!!!\n", pid);
				return -1;
			}
		}
	}
	slot = demux->slots[pslot->slot_id];
	slot->flags = (slot->flags & ~(0xffff)) | (pslot->flags & 0xffff);
	slot->ts_out_pin = pslot->ts_out_pin;

	if (ops->reset_cc)
		ops->reset_cc(dev->reg,slot->id);

	if ((slot->flags & DMX_SLOT_IS_ENABLE) == 0) {
		slot->pid = pslot->pid;
		if (ops->set_pid)
			ops->set_pid(dev->reg,slot->id,slot->pid);
		_dmx_slot_bind_cas(demux, slot);

		if (slot->flags & DMX_REVERSE_SLOT) {
			if (ops->set_reverse_pid)
				ops->set_reverse_pid(dev->reg, demux->id, slot->pid);
		}

	} else {
		gxlog_w(LOG_DEMUX, "ConfigSlot: Can't modify the PID when slot is enable\n");
	}

	if ((DEMUX_SLOT_PSI == slot->type) || (slot->type == DEMUX_SLOT_PES) ||
		(slot->type == DEMUX_SLOT_PES_AUDIO) || (slot->type == DEMUX_SLOT_PES_VIDEO)) {
		if (ops->set_interrupt_unit)
			ops->set_interrupt_unit(dev->reg,slot->id);

		if (DEMUX_SLOT_PSI == slot->type) {
			if (ops->set_psi_type)
				ops->set_psi_type(dev->reg,slot->id);
		} else {
			if (ops->set_pes_type)
				ops->set_pes_type(dev->reg,slot->id);
		}

		if (repeat_flag) {
			slot->flags |= DMX_REPEAT_MODE;
			if (ops->set_repeat_mode)
				ops->set_repeat_mode(dev->reg,slot->id);
		} else {
			slot->flags &= ~(DMX_REPEAT_MODE);
			if (ops->set_one_mode)
				ops->set_one_mode(dev->reg,slot->id);
		}

		if (crc_disable_flag)
			if (ops->set_crc_disable)
				ops->set_crc_disable(dev->reg,slot->id);

	} else if ((DEMUX_SLOT_AUDIO == slot->type) || (DEMUX_SLOT_VIDEO == slot->type)) {
		if (ops->set_av_type)
			ops->set_av_type(dev->reg,slot->id);

		if ((slot->type == DEMUX_SLOT_AUDIO) && (avout_flag == 1))//音频slot使用对用av_num通道 选择apts恢复配置
			if (ops->set_apts_sync)
				ops->set_apts_sync(dev->reg,slot->avid);

		if (pts_insert_flag) {
			if (ops->set_pts_insert)
				ops->set_pts_insert(dev->reg,slot->id);
		} else {
			if (ops->set_pts_bypass)
				ops->set_pts_bypass(dev->reg,slot->id);
		}
	}

	if (pts_to_sdram_flag) {
		if (ops->set_pts_to_sdram)
			ops->set_pts_to_sdram(dev->reg,slot->id);
	}

	if (ops->set_ts_out)
		ops->set_ts_out(dev->reg,slot->id, tsout_flag);

	if (slot->avid >= 0) {
		if (avout_flag == 0)
			_demux_avchannel_disable(demux, slot);
		else
			_demux_avchannel_enable(demux, slot);
	}
	if (ops->set_av_out)
		ops->set_av_out(dev->reg,slot->id, avout_flag);

	if (ops->set_des_out)
		ops->set_des_out(dev->reg,slot->id, tsdesout_flag);

	if (ops->set_t2mi_out)
		ops->set_t2mi_out(dev->reg, slot->id, t2miout_flag);

	if (ops->set_err_discard)
		ops->set_err_discard(dev->reg,slot->id, discard_flag);

	if (ops->set_dup_discard)
		ops->set_dup_discard(dev->reg,slot->id, dup_flag);

	if (slot->ts_out_pin) {
		unsigned int i;
		struct dvr_info *info;
		struct dmx_dvr *dvr = &demux->dvr;
		struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

		for (i = 0; i < dev->max_tsw; i++) {
			info = &dvr->tsw_info[i];
			if (!info->dvr_handle || slot->ts_out_pin != info->dvr_handle)
				continue;

			if (dvr_ops && dvr_ops->bind_slot_tsw)
				dvr_ops->bind_slot_tsw(dev->reg, slot->id, i);
		}
	}
	demux_slot_debug_print(demux->id, slot->id);

	return 0;
}

int demux_slot_enable(struct dmx_demux *demux,GxDemuxProperty_Slot *pslot)
{
	struct dmx_slot *slot = NULL;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;

	slot = demux->slots[pslot->slot_id];
	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "the demux slot's configure slot NULL!\n");
		return -1;
	}

	if (ops->enable_slot)
		ops->enable_slot(dev->reg,demux->id,slot->id);
	_demux_avchannel_enable(demux, slot);
	slot->flags |= DMX_SLOT_IS_ENABLE;
	slot->enable_counter++;

	return 0;
}

int demux_slot_disable(struct dmx_demux *demux,GxDemuxProperty_Slot *pslot)
{
	int i = 0;
	struct dmx_slot *slot = NULL;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;

	slot = demux->slots[pslot->slot_id];
	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "SlotDisable: slot NULL!\n");
		return -1;
	}

	if (slot->enable_counter <= 1) {
		for(i = 0; i < dev->max_filter; i++) {
			if (slot->filters[i] != NULL && slot->filters[i]->status == DMX_STATE_GO)
				break;
		}
		if (i == dev->max_filter) {
			_demux_avchannel_disable(demux, slot);
			if (ops->disable_slot)
				ops->disable_slot(dev->reg,demux->id,slot->id);
		} else {
			gxlog_w(LOG_DEMUX, "SlotDisable: exists filter not disable\n");
			return -1;
		}
	} else {
		gxlog_w(LOG_DEMUX, "SlotDisable: The slot used by multiple people\n");
		slot->enable_counter--;
		return 0;
	}
	slot->flags &= ~(DMX_SLOT_IS_ENABLE);
	slot->enable_counter = 0;

	return 0;
}

int demux_slot_query(struct dmx_demux *demux,GxDemuxProperty_SlotQueryByPid *pquery)
{
	struct dmx_slot *slot = NULL;
	int i;

	for (i = 0; i < demux->dev->max_slot; i++) {
		slot = demux->slots[i];
		if (NULL != slot) {
			if (pquery->pid == slot->pid) {
				pquery->slot_id = slot->id;
				return 0;
			}
		}
	}

	pquery->slot_id = -1;
	return 0;
}

int demux_slot_attribute(struct dmx_demux *demux,GxDemuxProperty_Slot *pslot)
{
	struct dmx_slot *slot = NULL;

	slot = demux->slots[pslot->slot_id];
	if (NULL == slot) {
		gxlog_e(LOG_DEMUX, "SlotConfig: slot is NULL!\n");
		return -1;
	}

	pslot->type = slot->type;
	pslot->flags = slot->flags;
	pslot->pid = slot->pid;
	demux_slot_debug_print(demux->id, slot->id);

	return 0;
}

int map_filter_alloc(struct dmxdev *dev, int filter_id, struct dmx_filter **filter)
{
	if (filter_id >= dev->max_filter)
		return -2;

	if ((dev->map_filter_mask >> filter_id) & 0x1)
		return 0;

	if (dev->map_filters == NULL) {
		dev->map_filters_size = sizeof(struct dmx_filter) * dev->max_filter;
		if (gxav_firewall_access_align() == 0) {
			dev->map_filters = (struct dmx_filter *)gx_dma_malloc(dev->map_filters_size);
		} else
			dev->map_filters = (struct dmx_filter *)gx_page_malloc(dev->map_filters_size);
		if (NULL == dev->map_filters) {
			gxlog_e(LOG_DEMUX, "alloc map_filters NULL!\n");
			return -1;
		}
	}

	*filter = dev->map_filters + filter_id;
	SET_MASK(dev->map_filter_mask, filter_id);
	return 0;
}

int map_filter_free(struct dmxdev *dev)
{
	if (dev->map_filters != NULL && dev->map_filter_mask == 0) {
		if (gxav_firewall_access_align() == 0)
			gx_dma_free((void *)dev->map_filters, dev->map_filters_size);
		else
			gx_page_free((unsigned char *)dev->map_filters, dev->map_filters_size);
		dev->map_filters = NULL;
		dev->map_filters_size = 0;
	}
	return 0;
}

int demux_filter_allocate(struct dmx_demux *demux,GxDemuxProperty_Filter *pfilter)
{
	int i, ret;
	unsigned long flags;
	struct dmx_slot *slot = NULL;
	struct dmx_filter *filter = NULL;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	unsigned int hwsize = 0,swsize = 0,gate = 0;
	unsigned int min_psi_sw_bufsize = 0, min_pes_sw_bufsize = 0;
	unsigned int min_psi_hw_bufsize = 0, min_pes_hw_bufsize = 0;
	SET_DEFAULT_VALUE(min_psi_sw_bufsize, dev->filter_min_psi_sw_bufsize, FIFO_PSI_SIZE);
	SET_DEFAULT_VALUE(min_psi_hw_bufsize, dev->filter_min_psi_hw_bufsize, BUFFER_PSI_SIZE);
	SET_DEFAULT_VALUE(min_pes_sw_bufsize, dev->filter_min_pes_sw_bufsize, FIFO_PES_SIZE);
	SET_DEFAULT_VALUE(min_pes_hw_bufsize, dev->filter_min_pes_hw_bufsize, BUFFER_PES_SIZE);

	slot = demux->slots[pfilter->slot_id];
	if ((NULL == slot) || (NULL == slot->demux)) {
		gxlog_e(LOG_DEMUX, "FilterAlloc: the demux allocate filter NULL!\n ");
		return -1;
	}

	if ((slot->type >= DEMUX_SLOT_AUDIO) && (slot->type <= DEMUX_SLOT_VIDEO)) {
		gxlog_e(LOG_DEMUX, "FilterAlloc: filter can not be related to AV slot!\n");
		return -1;
	}

	if (DEMUX_SLOT_PSI == slot->type)
		hwsize = min_psi_hw_bufsize;
	else
		hwsize = min_pes_hw_bufsize;

	if (dev->filter_nofree_en)
		hwsize = min_pes_hw_bufsize;

	if (pfilter->hw_buffer_size > hwsize) {
		if (pfilter->hw_buffer_size % 1024) {
			gxlog_e(LOG_DEMUX, "FilterAlloc: filter buffer size is not 1024bytes align!\n");
			return -1;
		}

		hwsize = pfilter->hw_buffer_size;
	}

	if (DEMUX_SLOT_PSI == slot->type)
		swsize = min_psi_sw_bufsize;
	else
		swsize = min_pes_sw_bufsize;

	if (pfilter->sw_buffer_size > swsize) {
		if (pfilter->sw_buffer_size % 1024) {
			gxlog_e(LOG_DEMUX, "FilterAlloc: filter fifo size is not 1024bytes align!\n");
			return -1;
		}

		swsize = pfilter->sw_buffer_size;
	}

	if (swsize < hwsize)
		gxlog_w(LOG_DEMUX, "FilterAlloc: hw_buffer_size 0x%x > sw_buffer_size 0x%x \n", hwsize, swsize);

	if (DEMUX_SLOT_PSI == slot->type) {
		if (swsize < FIFO_PSI_SIZE)
			gxlog_w(LOG_DEMUX, "FilterAlloc: psi_sw_bufsize 0x%x < 0x%x!\n",
					swsize, FIFO_PSI_SIZE);

		if (hwsize < BUFFER_PSI_SIZE)
			gxlog_w(LOG_DEMUX, "FilterAlloc: psi_hw_bufsize 0x%x < 0x%x!\n",
					hwsize, BUFFER_PSI_SIZE);

	} else {
		if (swsize < FIFO_PES_SIZE)
			gxlog_w(LOG_DEMUX, "FilterAlloc: pes_sw_bufsize 0x%x < 0x%x!\n",
					swsize, FIFO_PES_SIZE);
		if (hwsize < BUFFER_PES_SIZE)
			gxlog_w(LOG_DEMUX, "FilterAlloc: pes_hw_bufsize 0x%x < 0x%x!\n",
					hwsize, BUFFER_PES_SIZE);
	}

	if (0 == pfilter->almost_full_gate)
		gate = hwsize;
	else {
		if (pfilter->almost_full_gate > hwsize) {
			gxlog_e(LOG_DEMUX, "FilterAlloc: gate > hw_buffer_size !\n");
			return -1;
		}
		gate = pfilter->almost_full_gate;
	}

	delay_free(dev);

	i = get_one(dev->filter_mask, 1, 0, dev->max_filter);
	if (i < 0) {
		gxlog_e(LOG_DEMUX, "have no filter left!\n");
		return -1;
	}
	SET_MASK(dev->filter_mask, i);

	filter = (struct dmx_filter *)gx_malloc(sizeof(struct dmx_filter));
	if (NULL == filter) {
		gxlog_e(LOG_DEMUX, "alloc NULL!\n");
		return -1;
	}
	gx_memset(&(filter->tv),0,sizeof(struct timeval));
	gx_memset(filter, 0, sizeof(struct dmx_filter));

	filter->slot = slot;
	filter->type = slot->type;
	filter->id = i;
	filter->size = hwsize;
	filter->gate = gate;
	filter->status = DMX_STATE_ALLOCATED;
	filter->pread = 0;

	if ((DEMUX_SLOT_PSI == slot->type) || (slot->type == DEMUX_SLOT_PES) ||
		(slot->type == DEMUX_SLOT_PES_AUDIO) || (slot->type == DEMUX_SLOT_PES_VIDEO)) {
		ret = gxfifo_init(&filter->fifo, NULL, swsize);
		if (ret < 0) {
			gxlog_e(LOG_DEMUX, "filter allocate fifo failed!\n");
			goto err;
		}
	}

	if (kbuf_malloc(dev, filter) != 0) {
		gxlog_e(LOG_DEMUX, "Kbuf allocate failed!\n");
		goto err;
	}

	gx_memset(filter->vaddr, 0, filter->size);
	gx_dcache_clean_range((unsigned int)filter->vaddr, (unsigned int)filter->vaddr + filter->size);

	gx_spin_lock_irqsave(&(dev->df_spin_lock[i]), flags);

	slot->filters[i] = filter;
	slot->filter_num++;
	demux->filters[i] = filter;
	if (ops->set_dfbuf)
		ops->set_dfbuf(dev->reg,filter->id,filter->paddr,filter->size,(filter->gate>>8));
	if (ops->set_filterid)
		ops->set_filterid(dev->reg, slot->id,filter->id);
	if (ops->set_select)
		ops->set_select(dev->reg,demux->id,filter->id);
	if (ops->set_sec_irqen)
		ops->set_sec_irqen(dev->reg,filter->id);
	if (slot->type == DEMUX_SLOT_PES || slot->type == DEMUX_SLOT_PES_AUDIO
			|| slot->type == DEMUX_SLOT_PES_VIDEO)
		if (ops->set_gate_irqen)
			ops->set_gate_irqen(dev->reg,filter->id);
	gx_spin_unlock_irqrestore(&(dev->df_spin_lock[i]), flags);

	pfilter->filter_id = filter->id;
	return 0;

err:
	if (filter)
		gx_free(filter);
	CLR_MASK(dev->filter_mask, i);

	pfilter->filter_id = -1;
	return -1;
}

int demux_filter_free(struct dmx_demux *demux,GxDemuxProperty_Filter *pfilter)
{
	unsigned long flags;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_filter *filter = NULL;
	struct dmx_slot *slot = NULL;
	struct timezone tz;
	int id = pfilter->filter_id;

	gx_spin_lock_irqsave(&(dev->df_spin_lock[id]), flags);
	filter = demux->filters[id];
	if (NULL == filter || filter->slot == NULL) {
		gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);
		if (filter != NULL)
			goto _free;
		return -1;
	}

	slot = filter->slot;
	if (ops->disable_filter)
		ops->disable_filter(dev->reg,filter->id);
	if (ops->clr_int_df)
		ops->clr_int_df(dev->reg,filter->id);
	if (ops->clr_filterid)
		ops->clr_filterid(dev->reg,slot->id,filter->id);
	if (filter->status != DMX_STATE_FREE)
		filter->status = DMX_STATE_DELAY_FREE;

	filter->pread = 0;
	if (ops->clr_int_df_en)
		ops->clr_int_df_en(dev->reg,filter->id);

	if (filter->flags & DMX_MAP)
		gx_page_free(filter->fifo.data, filter->fifo.size);

	if ((filter->slot->type == DEMUX_SLOT_PSI) || (filter->slot->type == DEMUX_SLOT_PES) ||
		(filter->slot->type == DEMUX_SLOT_PES_AUDIO) || (filter->slot->type == DEMUX_SLOT_PES_VIDEO))
		gxfifo_free(&filter->fifo);
	filter->slot = NULL;
	slot->filter_num--;
	slot->filters[filter->id] = NULL;
	gx_gettimeofday(&(filter->tv),&tz);
	gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);

_free:
	delay_free(dev);
	map_filter_free(dev);
	return 0;
}

int demux_filter_config(struct dmx_demux *demux,GxDemuxProperty_Filter *pfilter)
{
	unsigned int eq_flag  = ((pfilter->flags & DMX_EQ) >> 1);
	unsigned int sw_flag  = ((pfilter->flags & DMX_SW_FILTER) >> 7);
	unsigned int err_flag = ((pfilter->flags & DMX_ERR_MASK_EN) >> 8);
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_filter *filter = NULL, *tempfilter = NULL;
	unsigned int hw_depth = 0;
	int id = pfilter->filter_id, ret = 0;
	unsigned long flags;
	void *map_buf = NULL;
	int map_buf_size = 0;

	if ((pfilter->key[1].mask != 0) || (pfilter->key[2].mask != 0)) {
		gxlog_e(LOG_DEMUX, "filter mask 1 2 bytes err\n");
		return -1;
	}

	if (pfilter->flags & DMX_MAP) {
		ret = map_filter_alloc(dev, id, &tempfilter);
		if (NULL == tempfilter || ret < 0) {
			gxlog_e(LOG_DEMUX, "map filter alloc NULL!\n");
			return -1;
		}
	}

	gx_spin_lock_irqsave(&(dev->df_spin_lock[id]), flags);
	filter = demux->filters[id];
	if (NULL == filter ||
			filter->status == DMX_STATE_FREE || filter->status == DMX_STATE_DELAY_FREE) {
		gxlog_e(LOG_DEMUX, "filter is NULL!\n");
		CLR_MASK(dev->map_filter_mask, id);
		gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);
		return -1;
	}

	if ((filter->flags & DMX_MAP) == 0 && (pfilter->flags & DMX_MAP)) {
		map_buf_size = filter->fifo.size;
		map_buf = gx_page_malloc(map_buf_size);
		if (map_buf == NULL) {
			gxlog_e(LOG_DEMUX, "map buf alloc NULL!\n");
			CLR_MASK(dev->map_filter_mask, id);
			gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);
			return -1;
		}
		memcpy(tempfilter, filter, sizeof(struct dmx_filter));
		gx_free(filter);
		filter = tempfilter;
		filter->slot->filters[filter->id] = filter;
		demux->filters[filter->id] = filter;
		gxfifo_free(&filter->fifo);
		gxfifo_init(&filter->fifo, map_buf, map_buf_size);
	} else if ((filter->flags & DMX_MAP) && (pfilter->flags & DMX_MAP) == 0) {
		gxlog_e(LOG_DEMUX, "The type of filter is map, not a non-map type!\n");
		gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);
		return -1;
	}

	gx_memcpy(filter->key, pfilter->key, (pfilter->depth) * sizeof(struct dmx_filter_key));
	filter->depth = pfilter->depth;
	if (filter->depth == 1 || sw_flag ==1)
		hw_depth = 0;
	else if (filter->depth > 3)
		hw_depth = filter->depth - 3;

	filter->flags = pfilter->flags;
	filter->pread = 0;

	gxfifo_reset(&filter->fifo);

	if (ops->clr_int_df)
		ops->clr_int_df(dev->reg,filter->id);
	if (ops->clr_dfrw)
		ops->clr_dfrw(dev->reg,filter->id);
	if (ops->set_match)
		ops->set_match(dev->reg,filter->id,hw_depth,filter->key,eq_flag);
	if (err_flag && ops->set_err_mask)
		ops->set_err_mask(dev->reg,demux->id,filter->id);

	demux_filter_debug_print(demux->id, filter->id);
	gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);

	return 0;
}

int demux_filter_fifo_reset(struct dmx_demux *demux,GxDemuxProperty_FilterFifoReset *preset)
{
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_filter *filter = NULL;
	int id = preset->filter_id;
	unsigned long flags;

	if (id < 0 || id > dev->max_filter) {
		gxlog_e(LOG_DEMUX, "FilterFIFOReset: filter filter_id error!\n");
		return -1;
	}

	gx_spin_lock_irqsave(&(dev->df_spin_lock[id]), flags);
	filter = demux->filters[id];
	if (NULL == filter ||
			filter->status == DMX_STATE_FREE || filter->status == DMX_STATE_DELAY_FREE) {
		gxlog_e(LOG_DEMUX, "FilterFIFOReset: filter is NULL\n");
		gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);
		return -1;
	}

	gx_memset(filter->vaddr, 0, filter->size);
	filter->pread = 0;
	gxfifo_reset(&filter->fifo);
	if (ops->clr_int_df)
		ops->clr_int_df(dev->reg,filter->id);
	if (ops->clr_dfrw)
		ops->clr_dfrw(dev->reg,filter->id);
	filter->flags &= ~(DMX_MAP_OVERFLOW);
	gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);

	gx_dcache_clean_range((unsigned int)filter->vaddr, (unsigned int)filter->vaddr + filter->size);
	return 0;
}

int demux_filter_enable(struct dmx_demux *demux,GxDemuxProperty_Filter *pfilter)
{
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_filter *filter = NULL;
	int id = pfilter->filter_id;
	unsigned long flags;

	gx_spin_lock_irqsave(&(dev->df_spin_lock[id]), flags);
	filter = demux->filters[id];
	if (NULL == filter ||
			filter->status == DMX_STATE_FREE || filter->status == DMX_STATE_DELAY_FREE) {
		gxlog_e(LOG_DEMUX, "FilterEnable: filter is NULL\n");
		gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);
		return -1;
	}

	filter->status = DMX_STATE_GO;
	filter->pread = 0;
	if (ops->clr_dfrw)
		ops->clr_dfrw(dev->reg,filter->id);
	if (ops->enable_filter)
		ops->enable_filter(dev->reg,filter->id);
	gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);

	return 0;
}

int demux_filter_disable(struct dmx_demux *demux,GxDemuxProperty_Filter *pfilter)
{
	unsigned long flags;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_filter *filter = NULL;
	int id = pfilter->filter_id;

	gx_spin_lock_irqsave(&(dev->df_spin_lock[id]), flags);
	filter = demux->filters[id];
	if (NULL == filter ||
			filter->status == DMX_STATE_FREE || filter->status == DMX_STATE_DELAY_FREE) {
		gxlog_e(LOG_DEMUX, "FilterDisable: filter is NULL\n");
		gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);
		return -1;
	}

	filter->status = DMX_STATE_STOP;
	if (ops->disable_filter)
		ops->disable_filter(dev->reg,filter->id);
	gx_spin_unlock_irqrestore(&(dev->df_spin_lock[id]), flags);

	return 0;
}

int demux_filter_attribute(struct dmx_demux *demux,GxDemuxProperty_Filter *pfilter)
{
	struct dmx_filter *filter = NULL;

	filter = demux->filters[pfilter->filter_id];
	if (NULL == filter ||
			filter->status == DMX_STATE_FREE || filter->status == DMX_STATE_DELAY_FREE) {
		gxlog_e(LOG_DEMUX, "FilterAttribute: filter is NULL\n");
		return -1;
	}

	pfilter->depth = filter->depth;
	pfilter->flags = filter->flags;
	pfilter->slot_id = filter->slot->id;
	if (filter->depth == 3)
		pfilter->depth = 1;

	gx_memcpy(pfilter->key, filter->key, (filter->depth) * sizeof(struct dmx_filter_key));
	demux_filter_debug_print(demux->id, filter->id);

	return 0;
}

int demux_filter_get_hwbuf(struct dmx_demux *demux,GxDemuxProperty_FilterMap *pfilter)
{
	struct dmx_filter *filter = NULL;

	filter = demux->filters[pfilter->filter_id];
	if (NULL == filter ||
			filter->status == DMX_STATE_FREE || filter->status == DMX_STATE_DELAY_FREE) {
		gxlog_e(LOG_DEMUX, "FilterMap: filter is NULL\n");
		return -1;
	}

	if ((filter->flags & DMX_MAP) == 0) {
		gxlog_e(LOG_DEMUX, "FilterMap: filter type is not map \n");
		return -1;
	}

	pfilter->buffer_addr = (unsigned int)filter->fifo.data;
	pfilter->buffer_size = filter->fifo.size;
	pfilter->control_addr = (unsigned int)demux->dev->map_filters;
	pfilter->control_size = demux->dev->map_filters_size;

	return 0;
}

static int demux_filter_free_size(struct dmx_demux *demux,
		GxDemuxProperty_FilterFifoFreeSize *fifo_free_size)
{
	struct dmx_filter *filter = NULL;
	struct gxfifo *fifo = NULL;
	int id = fifo_free_size->filter_id;

	if ((id >= MAX_FILTER_NUM) || (id < 0)) {
		gxlog_e(LOG_DEMUX, "FilterFIFOFreeSize: filter id error!\n");
		return -1;
	}

	filter = demux->filters[id];
	if (NULL == filter ||
			filter->status == DMX_STATE_FREE || filter->status == DMX_STATE_DELAY_FREE) {
		gxlog_e(LOG_DEMUX, "FilterFIFOFreeSize: filter is NULL\n");
		return -1;
	}
	fifo = &filter->fifo;
	if (NULL == fifo) {
		gxlog_e(LOG_DEMUX, "FilterFIFOFreeSize: fifo is NULL\n");
		return -1;
	}

	fifo_free_size->free_size = gxfifo_freelen(fifo);
	return 0;
}

unsigned char tmp[4099];
static int filter_read(struct dmx_filter *filter, unsigned char *data_buf, unsigned int size, unsigned int mode)
{
	unsigned char crc = 0;
	unsigned char data[16];
	unsigned char data_next[16];
	unsigned int section_size = 0;
	struct gxfifo *fifo = &filter->fifo;
	unsigned int fifo_len = 0, real_len = 0;
	int crc_en_flag = ((filter->flags & DMX_CRC_IRQ) >> 0);
	int eq_flag = ((filter->flags & DMX_EQ) >> 1);
	int sw_flag = ((filter->flags & DMX_SW_FILTER) >> 7);
	unsigned int i =0,depth;

	if (data_buf == NULL) {
		gxlog_e(LOG_DEMUX, "user data buffer is null!");
		return -1;
	}

	if (filter->slot == NULL) {
		gxlog_e(LOG_DEMUX, "filter read error!");
		return -1;
	}

	if (DEMUX_SLOT_PSI == filter->slot->type) {
		while (real_len < size) {
discard:
			fifo_len = gxfifo_len(fifo);
			if (fifo_len >= 3) {
				gxfifo_peek(fifo, data, 3,0);
				section_size = (((data[1]) & 0x0F) << 8) + (data[2]) +3;
				if (section_size > 4096 || section_size == 3) {
					gxlog_e(LOG_DEMUX, "filter id %d section len %d fifo_len %d real_len %d\n", \
							filter->id, section_size,fifo_len,real_len);
					/* Reset filter in demux_filter_read */
					return -3;
				}
				if (section_size+1 > fifo_len)
					return real_len;

			}else {
				return real_len;
			}
			if (sw_flag == 1) {
				if (eq_flag == 1) {
					depth = filter->depth;

					gxfifo_peek(fifo, data, depth,0);
					for(i=0;i<depth;i++) {
						if ((data[i] & filter->key[i].mask) != (filter->key[i].value & filter->key[i].mask)) {
							gxlog_d(LOG_DEMUX, "data[%d] = %x key[%d] = %x\n",i,data[i],i,filter->key[i].value);
							gxfifo_get(fifo, tmp, section_size+1);
							goto discard;
						}

					}
				}
			}
			real_len += section_size ;
			if (real_len > size) {
				gxlog_d(LOG_DEMUX, "real len %d > size %d\n",real_len,size);
				return real_len - section_size ;
			}

			real_len -= section_size ;
			gxfifo_user_get(fifo, data_buf+real_len, section_size);
			gxfifo_get(fifo, &crc, 1);
			fifo_len = gxfifo_len(fifo);
			if (fifo_len >= 3) {
				gxfifo_peek(fifo, data, 1,0);
			}
			if (crc_en_flag && (crc != 0 && crc != 1)) {
				gxlog_e(LOG_DEMUX, "filter id %d CRC error\n", filter->id);
				return -3;
			}
			if (crc == 0 && (data[0] & filter->key[0].mask) == (filter->key[0].value & filter->key[0].mask) && crc_en_flag == 1)
				real_len += section_size;
			else if (crc == 0 && (data[0] & filter->key[0].mask) != (filter->key[0].value & filter->key[0].mask) && crc_en_flag == 1)
				gxlog_d(LOG_DEMUX, "!!!!!!tid=%d len = %d key=%d mask=%x!!!!!crc err!\n",data[0],section_size,filter->key[0].value,filter->key[0].mask);
			else if (crc_en_flag == 0)
				real_len += section_size;
			else
				gxlog_d(LOG_DEMUX, "section len %d real_len%d crc%d tid = %d\n",section_size,real_len,crc,data[0]);
			if (mode == FILTER_READ_MODE_ONE_SECTION)
				return real_len;
		}
	}
	else if (DEMUX_SLOT_PES == filter->slot->type) {
		while (real_len < size) {
			fifo_len = gxfifo_len(fifo);
			if (fifo_len >= 6) {
				gxfifo_peek(fifo, data, 6,0);
				section_size = ((data[4]) << 8) + (data[5]) +6;
				fifo_len = gxfifo_len(fifo);
				if (fifo_len >= 6+section_size) {
					gxfifo_peek(fifo, data_next, 6,section_size);
					gxlog_d(LOG_DEMUX, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
							data[0],data[1],data[2],data[3],data_next[0],data_next[1],data_next[2],data_next[3]);
					if (data[0] != 0x00 || data[1] != 0x00 || data[2] != 0x01
							||data_next[0] != 0x00 || data_next[1] != 0x00 || data_next[2] != 0x01) {

						while(section_size--) {
							gxfifo_get(fifo, data, 1);
							if (section_size >= 6) {
								gxfifo_peek(fifo, data, 6,0);
								if (filter->key[0].mask != 0) {
									if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01
											&& data[3] == (filter->key[0].value & filter->key[0].mask)) {
										gxlog_e(LOG_DEMUX, "soft err correction!\n");
										return real_len;
									}
								}else{
									if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
										gxlog_e(LOG_DEMUX, "soft err correction!\n");
										return real_len;
									}
								}
							}else
								return real_len;
						}
					}
				}
				section_size = ((data[4]) << 8) + (data[5]) +6;
				gxlog_d(LOG_DEMUX, "section_len = %d fifo_len =%d real_len = %d!\n",section_size,fifo_len,real_len);
				if (section_size > fifo_len) {
					return real_len;
				}
			}else {
				return real_len;
			}

			if (real_len + section_size > size)
			{
				gxlog_d(LOG_DEMUX, "------------[%s]%d real_len = %d, section_size = %d -----------\n", __func__, __LINE__, real_len, section_size);
				return real_len;
			}

			gxfifo_user_get(fifo, data_buf+real_len, section_size);
			real_len += section_size ;
		}
	}
	else if (DEMUX_SLOT_PES_AUDIO == filter->slot->type || DEMUX_SLOT_PES_VIDEO == filter->slot->type) {
		fifo_len = gxfifo_len(fifo);
		if (fifo_len <= size)
			gxfifo_user_get(fifo, data_buf, fifo_len);
		else
			gxfifo_user_get(fifo, data_buf, size);

		return GX_MIN(fifo_len,size);
	}

	return real_len;
}

int demux_filter_read(struct dmx_demux *demux,GxDemuxProperty_FilterRead *pread)
{
	int ret;
	struct dmx_filter *filter = NULL;

	if (pread->filter_id < 0) {
		gxlog_e(LOG_DEMUX, "FilterRead: filter filter_id error!\n");
		return -1;
	}

	if (NULL == pread->buffer) {
		gxlog_e(LOG_DEMUX, "FilterRead: filter buffer == NULL!\n");
		return -1;
	}

	filter = demux->filters[pread->filter_id];
	if (NULL == filter) {
		gxlog_e(LOG_DEMUX, "FilterRead: the demux read filter NULL!\n");
		return -1;
	}

	pread->bit_error = 0;
	if ((ret = filter_read(filter, (unsigned char *)pread->buffer,pread->max_size,pread->read_size)) >= 0)
		pread->read_size = ret;
	else
		pread->read_size = 0;

	if (ret == -3 && (filter->flags & DMX_AUTO_RESET_EN)) {
		GxDemuxProperty_Filter pfilter = {0};
		GxDemuxProperty_FilterFifoReset preset = {0};

		pfilter.filter_id = filter->id;
		preset.filter_id = filter->id;
		demux_filter_disable(demux, &pfilter);
		demux_filter_fifo_reset(demux, &preset);
		demux_filter_enable(demux, &pfilter);
		ret = 0;
	}

	if (ret == -3)
		pread->bit_error = 1;

	return ret < 0 ? ret : 0;
}

int demux_filter_fifo_query(struct dmx_demux *demux,GxDemuxProperty_FilterFifoQuery *pquery)
{
	unsigned int len,i;
	long long id = 0,st=0;
	struct dmx_filter *filter = NULL;
	struct gxfifo *fifo = NULL;

	st = 0;
	for (i = 0; i < demux->dev->max_filter; i++) {
		id = (0x1ULL << i);
		filter = demux->filters[i];
		if (filter != NULL) {
			fifo = &filter->fifo;
			if (fifo != NULL) {
				len = gxfifo_len(fifo);
				if (len > 0)
					st |= id;
				else
					st &= ~id;
			}
		}
	}
	pquery->state =st;

	return 0;
}

static int _dmx_cas_bind_slot(struct dmx_demux *demux, uint32_t caid)
{
	int i = 0;
	struct dmxdev * dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_cas *cas = NULL;
	struct dmx_slot *slot = NULL;
	GxDemuxProperty_SlotQueryByPid pquery = {0};

	cas = dev->cas_configs[caid];
	if ((demux->source == DEMUX_SDRAM) && (cas->flags & DMX_CAS_BIND_ALL_SLOT))
		return 0;

	for (i = 0; i < MAX_CAS_PID_NUM; i++) {
		if ((dev->pid_mask & (1<<i)) == 0)
			continue;
		if (((dev->pid_array[i] >> DMX_CAS_CAID_POS) & DMX_CAS_CAID_MASK) != caid)
			continue;

		pquery.pid = dev->pid_array[i] & DMX_CAS_PID_MASK;
		demux_slot_query(demux, &pquery);
		if (pquery.slot_id >= 0) {
			slot = demux->slots[pquery.slot_id];
			slot->casid = caid;
			if (cas->flags & DMX_CAS_VERSION_2) {
				if (ops->set_cas_descid)
					ops->set_cas_descid(dev->reg, pquery.slot_id, caid);
				if (ops->set_cas_key_valid)
					ops->set_cas_key_valid(dev->reg, pquery.slot_id);
			} else {
				if (ops->set_descid)
					ops->set_descid(dev->reg, pquery.slot_id, caid, cas->flags);
				if (ops->set_even_valid)
					ops->set_even_valid(dev->reg, pquery.slot_id);
				if (ops->set_odd_valid)
					ops->set_odd_valid(dev->reg, pquery.slot_id);
			}
		}
	}
	return 0;
}

static int _get_cas_pid_pos(struct dmxdev *dev, uint32_t caid, uint16_t pid)
{
	int i = 0;

	for (i = 0; i < MAX_CAS_PID_NUM; i++) {
		if ((dev->pid_mask & (1<<i)) == 0)
			continue;

		if ((dev->pid_array[i] & DMX_CAS_PID_MASK) == pid)
			break;
	}

	return i == MAX_CAS_PID_NUM ? -1 : i;
}

static int demux_cas_add_pid(struct dmx_demux *demux, uint32_t caid, uint16_t pid)
{
	int pos = 0;
	struct dmxdev *dev = demux->dev;

	if (NULL == dev->cas_configs[caid])
		return -1;

	if ((pos = _get_cas_pid_pos(dev, caid, pid)) >= 0)
		return ((dev->pid_array[pos] >> DMX_CAS_CAID_POS) == caid) ? 0 : -1;

	if ((pos = get_one(dev->pid_mask, 1, 0, MAX_CAS_PID_NUM)) < 0)
		return -1;

	SET_MASK(dev->pid_mask, pos);
	dev->pid_array[pos] = (pid & DMX_CAS_PID_MASK) |
						((caid & DMX_CAS_CAID_MASK) << DMX_CAS_CAID_POS);

	return 0;
}

static int demux_cas_del_pid(struct dmx_demux *demux, uint32_t caid, uint16_t pid)
{
	int pos = 0;
	struct dmxdev *dev = demux->dev;

	if (NULL == dev->cas_configs[caid])
		return -1;

	if ((pos = _get_cas_pid_pos(dev, caid, pid)) < 0)
		return 0;

	if ((dev->pid_array[pos] >> DMX_CAS_CAID_POS) != caid)
		return -1;

	CLR_MASK(dev->pid_mask, pos);

	return 0;
}

static int demux_cas_del_all_pid(struct dmx_demux *demux, uint32_t caid)
{
	int i = 0;
	struct dmxdev *dev = demux->dev;

	if (NULL == dev->cas_configs[caid])
		return -1;

	for (i = 0; i < MAX_CAS_PID_NUM; i++) {
		if ((dev->pid_array[i] >> DMX_CAS_CAID_POS) != caid)
			continue;

		CLR_MASK(dev->pid_mask, i);
	}

	return 0;
}

int demux_ca_config(struct dmx_demux *demux,GxDemuxProperty_CA *pca)
{
	unsigned int even_flag = ((pca->flags & DMX_CA_KEY_EVEN) >> 0);
	unsigned int odd_flag = ((pca->flags & DMX_CA_KEY_ODD) >> 1);
	struct dmx_slot *slot = NULL;
	struct dmx_cas *ca = NULL;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;

	ca = dev->cas_configs[pca->ca_id];
	if (NULL == ca || pca->slot_id > MAX_SLOT_NUM) {
		gxlog_e(LOG_DEMUX, "the ca config is NULL\n");
		return -1;
	}

	if (pca->slot_id >= 0)
		slot = demux->slots[pca->slot_id];

	ca->flags = pca->flags;
	ca->ds_mode = pca->ds_mode;
	if (dev->max_cas == MAX_CAS_NUM)
		ca->flags |= DMX_CAS_VERSION_2;
	if (ca->ds_mode == DEMUX_DES_DES_ECB)
		ca->flags |= DMX_CA_DES_MODE;

	if (even_flag) {
		ca->cw_even[1] = pca->even_key_high;
		ca->cw_even[0] = pca->even_key_low;
		if (ops->set_evenkey)
			ops->set_evenkey(dev->reg, ca->id, ca->cw_even[1], ca->cw_even[0], ca->flags);
	}
	if (odd_flag) {
		ca->cw_odd[1] = pca->odd_key_high;
		ca->cw_odd[0] = pca->odd_key_low;
		if (ops->set_oddkey)
			ops->set_oddkey(dev->reg, ca->id, ca->cw_odd[1], ca->cw_odd[0], ca->flags);
	}

	if (slot) {
		if (ops->set_cas_dsc_type)
			ops->set_cas_dsc_type(dev->reg, slot->id, (ca->flags & DMX_CA_PES_LEVEL) ? 1 : 0);
		if (ops->set_ca_mode)
			ops->set_ca_mode(dev->reg,demux->id,slot->id,ca->ds_mode);
		demux_cas_add_pid(demux, ca->id, slot->pid);
	}

	_dmx_cas_bind_slot(demux, ca->id);
	if (ca->flags & DMX_CAS_BIND_ALL_SLOT) {
		demux = &dev->demuxs[GX_ABS(1-demux->id) % 2];
		_dmx_cas_bind_slot(demux, ca->id);
	}

	demux_cas_debug_print(demux->id, ca->id);
	return 0;
}

int demux_ca_attribute(struct dmx_demux *demux,GxDemuxProperty_CA *pca)
{
	struct dmx_cas *ca = NULL;

	ca = demux->dev->cas_configs[pca->ca_id];
	if (NULL == ca) {
		gxlog_e(LOG_DEMUX, "the ca's config ca is NULL\n");
		return -1;
	}

	pca->flags         = ca->flags;
	pca->even_key_high = ca->cw_even[1];
	pca->even_key_low  = ca->cw_even[0];
	pca->odd_key_high  = ca->cw_odd[1];
	pca->odd_key_low   = ca->cw_odd[0];

	return 0;
}

int demux_cas_allocate(struct dmx_demux *demux)
{
	int i;
	struct dmx_cas *cas;
	struct dmxdev *dev = demux->dev;

	i = get_one(dev->cas_mask, 1, 0, dev->max_cas);
	if ((i >= dev->max_cas) || (i < 0)) {
		gxlog_e(LOG_DEMUX, "ca_mask is full!\n");
		return -1;
	}

	cas = (struct dmx_cas *)gx_malloc(sizeof(struct dmx_cas));
	if (NULL == cas) {
		gxlog_e(LOG_DEMUX, "alloc NULL!\n");
		return -1;
	}

	memset(cas, 0, sizeof(struct dmx_cas));
	SET_MASK(dev->cas_mask, i);
	cas->id = i;
	cas->demux= demux;
	dev->cas_configs[i] = cas;

	return i;
}

int demux_cas_setcw(struct dmx_demux *demux, GxDemuxProperty_CAS *pcas)
{
	int i;
	struct dmx_cas *cas = NULL;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;

	cas = dev->cas_configs[pcas->cas_id];
	if (NULL == cas) {
		gxlog_e(LOG_DEMUX, "the cas config is NULL\n");
		return -1;
	}

	if (pcas->ds_mode == DEMUX_DES_MULTI2 && pcas->cas_id >= 16) {
		gxlog_e(LOG_DEMUX, "cas set cw error \n");
		return -1;
	}

	cas->sys_sel_even     = pcas->sys_sel_even;
	cas->sys_sel_odd      = pcas->sys_sel_odd;
	cas->ds_mode          = pcas->ds_mode;
	if (pcas->cw_from_acpu)
		cas->flags |= DMX_CAS_CW_FROM_CPU;
	else
		cas->flags &= ~DMX_CAS_CW_FROM_CPU;

	if (pcas->set_cw_addr_even)
		cas->flags |= DMX_CAS_CW_EVEN;
	else
		cas->flags &= ~DMX_CAS_CW_EVEN;
	for (i = 0; i < 4; i++) {
		cas->cw_even[i] = pcas->cw_even[i];
		cas->cw_odd[i]  = pcas->cw_odd[i];
		cas->iv_even[i] = pcas->iv_even[i];
		cas->iv_odd[i]  = pcas->iv_odd[i];
	}
	if (pcas->ds_mode == DEMUX_DES_MULTI2) {
		cas->_M = (pcas->_M % 4) ? 16 : pcas->_M;
		cas->iv_even[0] = cas->iv_odd[0] = 0;
		cas->iv_even[1] = cas->iv_odd[1] = cas->_M;
	}

	if (pcas->set_cw_addr_even) {
		if (ops->set_cas_evenkey)
			ops->set_cas_evenkey(dev->cas_reg, cas);
	} else {
		if (ops->set_cas_oddkey)
			ops->set_cas_oddkey(dev->cas_reg, cas);
	}
	return 0;
}

int demux_cas_set_syskey(struct dmx_demux *demux, GxDemuxProperty_SysKey *psys)
{
	int i;
	struct dmx_sys_key sys;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;

	if (psys->sys_sel > 3) {
		gxlog_e(LOG_DEMUX, "the cas sys param error\n");
		return -1;
	}

	sys.sys_sel = psys->sys_sel;
	for (i = 0; i < 4; i++) {
		sys.sys_key_h[i] = psys->sys_key_h[i];
		sys.sys_key_l[i] = psys->sys_key_l[i];
	}
	if (ops->set_cas_syskey)
		ops->set_cas_syskey(dev->cas_reg, &sys);

	return 0;
}

int demux_cas_config(struct dmx_demux *demux,GxDemuxProperty_CAS *pcas)
{
	unsigned int pes_level = 0;
	struct dmx_slot *slot = NULL;
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_cas *cas = dev->cas_configs[pcas->cas_id];

	if (NULL == cas) {
		gxlog_e(LOG_DEMUX, "the cas config is NULL\n");
		return -1;
	}

	if (pcas->slot_id > MAX_SLOT_NUM ||
		(pcas->ds_mode == DEMUX_DES_MULTI2 && pcas->cas_id >= 16)) {
		gxlog_e(LOG_DEMUX, "cas config error \n");
		return -1;
	}

	if (pcas->slot_id >= 0)
		slot = demux->slots[pcas->slot_id];

	cas->ds_mode = pcas->ds_mode;
	if (ops->set_cas_mode)
		ops->set_cas_mode(dev->cas_reg, cas);

	cas->flags |= DMX_CAS_VERSION_2;
	if (slot) {
		if (pcas->pes_level_dsc) {
			cas->flags |= DMX_CAS_PES_LEVEL;
			pes_level = 1;
		}
		if (ops->set_cas_dsc_type)
			ops->set_cas_dsc_type(dev->reg, slot->id, pes_level);
		demux_cas_add_pid(demux, cas->id, slot->pid);
	}

	_dmx_cas_bind_slot(demux, cas->id);
	if (cas->flags & DMX_CAS_BIND_ALL_SLOT) {
		demux = &dev->demuxs[GX_ABS(1-demux->id) % 2];
		_dmx_cas_bind_slot(demux, cas->id);
	}
	demux_cas_debug_print(demux->id, cas->id);

	return 0;
}

static int demux_cas_set_bind_mode(struct dmx_demux *demux, uint32_t caid, uint32_t flags)
{
	struct dmxdev *dev = demux->dev;
	struct dmx_cas *cas = dev->cas_configs[caid];

	if (flags & DMX_CAS_BIND_MODE_SAVE) {
		if (flags & DMX_CAS_BIND_ALL_SLOT)
			cas->flags |= DMX_CAS_BIND_ALL_SLOT;
		else
			cas->flags &= ~DMX_CAS_BIND_ALL_SLOT;
	}

	_dmx_cas_bind_slot(demux, caid);
	if (flags & DMX_CAS_BIND_ALL_SLOT) {
		demux = &dev->demuxs[GX_ABS(1-demux->id) % 2];
		_dmx_cas_bind_slot(demux, caid);
	}

	return 0;
}

int demux_cas_free(struct dmx_demux *demux, unsigned int caid)
{
	struct dmx_cas *cas = NULL;
	struct dmxdev *dev = demux->dev;

	cas = dev->cas_configs[caid];
	if (NULL == cas) {
		gxlog_e(LOG_DEMUX, "the cas config is NULL\n");
		return -1;
	}

	demux_cas_del_all_pid(demux, caid);
	CLR_MASK(dev->cas_mask, caid);
	gx_free(cas);
	dev->cas_configs[caid] = NULL;

	return 0;
}

static int demux_tsw_slot_config(struct dmx_demux *demux,GxDemuxProperty_Slot *pslot)
{
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_dvr *dvr = &demux->dvr;
	unsigned int t2miout_flag = ((pslot->flags & DMX_T2MI_OUT_EN) >> 11);
	unsigned int reverse_flag = ((pslot->flags & DMX_REVERSE_SLOT) >> 12);
	unsigned int id = pslot->slot_id;
	unsigned int pid = pslot->pid;
	unsigned int pin = pslot->ts_out_pin;

	if (id > dev->max_tsw || dvr->tsw_slot[id].refcount == 0)
		return -1;

	dvr->tsw_slot[id].pid = pid;
	if (reverse_flag) {
		if (ops->set_reverse_pid)
			ops->set_reverse_pid(dev->reg, demux->id%2+2, pid);
	}
	if (ops->pf_set_pid)
		ops->pf_set_pid(dev->reg, id, pid);
	if (ops->pf_set_ts_out)
		ops->pf_set_ts_out(dev->reg, id);
	if (ops->pf_set_t2mi_out)
		ops->pf_set_t2mi_out(dev->reg, id, t2miout_flag);

	if (pin) {
		unsigned int i;
		struct dvr_info *info;
		struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

		for (i = 0; i < dev->max_tsw; i++) {
			info = &dvr->tsw_info[i];
			if (!info->dvr_handle || pin != info->dvr_handle)
				continue;

			if (dvr_ops && dvr_ops->pf_bind_slot_tsw)
				dvr_ops->pf_bind_slot_tsw(dev->reg, id, i);
		}
	}

	return 0;
}

static int demux_tsw_slot_alloc(int sub, int pid, int handle, unsigned int flags)
{
	struct dmx_demux *demux = get_subdemux(sub);
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_dvr *dvr = &demux->dvr;
	int id = get_one(dev->tsw_slot_mask, 1, 0, dev->max_tsw_slot);
	if (id < 0) {
		gxlog_e(LOG_DEMUX, "There is no tsw_slot left!\n");
		return -1;
	}

	SET_MASK(dev->tsw_slot_mask, id);
	dvr->tsw_slot[id].pid = pid;
	dvr->tsw_slot[id].handle = handle;
	dvr->tsw_slot[id].refcount++;
	return id;
}

int demux_tsw_slot_free(int sub, int id, int handle)
{
	struct dmx_demux *demux = get_subdemux(sub);
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	struct dmx_dvr *dvr = &demux->dvr;

	if (id < 0 || id >= dev->max_tsw_slot || !dvr->tsw_slot[id].refcount)
		return 0;

	dvr->tsw_slot[id].refcount--;
	if (dvr->tsw_slot[id].refcount >= 1)
		return 0;

	CLR_MASK(dev->tsw_slot_mask, id);
	if (ops->pf_disable_slot)
		ops->pf_disable_slot(dev->reg, sub, id);
	if (ops->pf_clr_slot_cfg)
		ops->pf_clr_slot_cfg(dev->reg, id);

	return id;
}

static int demux_tsw_slot_enable(int sub, int id)
{
	struct dmx_demux *demux = get_subdemux(sub);
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	if (id < 0) {
		gxlog_e(LOG_DEMUX, "slot id is error\n");
		return -1;
	}

	if (ops->pf_enable_slot)
		ops->pf_enable_slot(dev->reg, demux->id, id);

	return 0;
}

static int demux_tsw_slot_disable(int sub, int id)
{
	struct dmx_demux *demux = get_subdemux(sub);
	struct dmxdev *dev = demux->dev;
	struct demux_ops *ops = dev->ops;
	if (id < 0) {
		gxlog_e(LOG_DEMUX, "slot id is error\n");
		return -1;
	}

	if (ops->pf_disable_slot)
		ops->pf_disable_slot(dev->reg, sub, id);

	return 0;
}

int demux_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0;
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(sub);

	gxdemux_lock();
	switch (property_id) {
	case GxAVGenericPropertyID_ModuleLinkChannel:
	case GxAVGenericPropertyID_ModuleUnLinkChannel:
		{
			struct fifo_info *module_set_fifo = (struct fifo_info *)property;
			struct demux_fifo demux_fifo;

			gxav_channel_get_phys(module_set_fifo->channel,
					&(demux_fifo.buffer_start_addr),&(demux_fifo.buffer_end_addr), &(demux_fifo.buffer_id));
			demux_fifo.channel_id    = gxav_channel_id_get(module_set_fifo->channel);

			if ((module_set_fifo->pin_id > 64) || (module_set_fifo->pin_id < 0)) {
				gxlog_e(LOG_DEMUX, "property error!\n");
				goto err;
			}

			if (module_set_fifo->pin_id < 64) {
				if (!(demux->slots[module_set_fifo->pin_id]) || (demux->slots[module_set_fifo->pin_id]->type < DEMUX_SLOT_AUDIO)) {
					gxlog_e(LOG_DEMUX, "pin_id error!\n");
					goto err;
				}
				gxav_channel_get_ptsbuffer(module_set_fifo->channel,
						&(demux_fifo.pts_start_addr),
						&(demux_fifo.pts_end_addr),
						&(demux_fifo.pts_buffer_id));
				demux_fifo.pts_channel_id = gxav_channel_pts_id_get(module_set_fifo->channel);
			}
			demux_fifo.pin_id       = module_set_fifo->pin_id;
			demux_fifo.direction    = module_set_fifo->dir;
			demux_fifo.channel      = module_set_fifo->channel;

			if (GxAVGenericPropertyID_ModuleLinkChannel == property_id) {
				ret = demux_link_fifo(demux, &demux_fifo);
			} else
				ret = demux_unlink_fifo(demux, &demux_fifo);
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

			if ((pconfig->ts_select < FRONTEND) || (pconfig->ts_select > OTHER)) {
				gxlog_e(LOG_DEMUX, "ConfigDmx: the demux module's configure ts_select param error!\n");
				goto err;
			}

			if ((pconfig->source < DEMUX_TS1) || (pconfig->source > DEMUX_SDRAM)) {
				gxlog_e(LOG_DEMUX, "ConfigDmx: the demux module's configure source param error!\n");
				goto err;
			}

			ret = demux_set_config(demux, module->sub, pconfig);
		}
		break;

	case GxDemuxPropertyID_SlotConfig:
	case GxDemuxPropertyID_SlotEnable:
	case GxDemuxPropertyID_SlotDisable:
	case GxDemuxPropertyID_SlotFree:
		{
			GxDemuxProperty_Slot *pslot = (GxDemuxProperty_Slot *) property;

			if (size != sizeof(GxDemuxProperty_Slot)) {
				gxlog_e(LOG_DEMUX, "slot's param error!\n");
				goto err;
			}

			if ((pslot->slot_id >= MAX_SLOT_NUM) || (pslot->slot_id < 0)) {
				gxlog_e(LOG_DEMUX, "slot id error!\n");
				goto err;
			}

			if (pslot->type < DEMUX_SLOT_PSI) {
				gxlog_e(LOG_DEMUX, "slot type error!\n");
				goto err;
			}

			if ((pslot->type == DEMUX_SLOT_TS) ||
				(pslot->type == DEMUX_SLOT_SPDIF) || (pslot->type == DEMUX_SLOT_AUDIO_SPDIF)) {
				gxlog_e(LOG_DEMUX, "slot type not support now!\n");
				goto err;
			}

			if (pslot->pid > 0x1FFF) {
				gxlog_e(LOG_DEMUX, "slot pid > 0x1fff!!\n");
				goto err;
			}

			if (module->sub < MAX_DMX_NORMAL) {
				if (GxDemuxPropertyID_SlotConfig == property_id) {
					if (demux_slot_config(demux, pslot) < 0) {
						gxlog_e(LOG_DEMUX, "SlotConfig: error!\n");
						goto err;
					}
				} else if (GxDemuxPropertyID_SlotEnable == property_id) {
					if (demux_slot_enable(demux, pslot) < 0) {
						gxlog_e(LOG_DEMUX, "SlotEnable: error!\n");
						goto err;
					}
				} else if (GxDemuxPropertyID_SlotDisable == property_id) {
					if ((ret = demux_slot_disable(demux, pslot)) == -1) {
						gxlog_e(LOG_DEMUX, "SlotDisable: error!\n");
						goto err;
					}
				} else if (GxDemuxPropertyID_SlotFree == property_id) {
					if (demux_slot_free(demux, pslot) < 0) {
						gxlog_e(LOG_DEMUX, "SlotFree: error!\n");
						goto err;
					}

				}
			} else if (module->sub < MAX_DMX_TSW_SLOT) {
				if (GxDemuxPropertyID_SlotConfig == property_id)
					ret = demux_tsw_slot_config(demux, pslot);
				else if (property_id == GxDemuxPropertyID_SlotEnable)
					ret = demux_tsw_slot_enable(sub, pslot->slot_id);
				else if (property_id == GxDemuxPropertyID_SlotDisable)
					ret = demux_tsw_slot_disable(sub, pslot->slot_id);
				else if (property_id == GxDemuxPropertyID_SlotFree)
					ret = demux_tsw_slot_free(sub, pslot->slot_id, module->module_id);
			}
		}
		break;

	case GxDemuxPropertyID_FilterMinBufSizeConfig:
		{
			GxDemuxProperty_FilterMinBufSize *config = (GxDemuxProperty_FilterMinBufSize *)property;

			if (size != sizeof(GxDemuxProperty_FilterMinBufSize)) {
				gxlog_e(LOG_DEMUX, "filter: the param error!\n");
				goto err;
			}
			demux->dev->filter_min_psi_sw_bufsize = config->psi_sw_bufsize;
			demux->dev->filter_min_psi_hw_bufsize = config->psi_hw_bufsize;
			demux->dev->filter_min_pes_sw_bufsize = config->pes_sw_bufsize;
			demux->dev->filter_min_pes_hw_bufsize = config->pes_hw_bufsize;
		}
		break;

	case GxDemuxPropertyID_FilterNofreeConfig:
		{
			GxDemuxProperty_FilterNofree *config = (GxDemuxProperty_FilterNofree *)property;

			if (size != sizeof(GxDemuxProperty_FilterNofree)) {
				gxlog_e(LOG_DEMUX, "filter: the param error!\n");
				goto err;
			}
			demux->dev->filter_nofree_en = config->enable;
		}
		break;

	case GxDemuxPropertyID_FilterConfig:
	case GxDemuxPropertyID_FilterEnable:
	case GxDemuxPropertyID_FilterDisable:
	case GxDemuxPropertyID_FilterFree:
		{
			GxDemuxProperty_Filter *pfilter = (GxDemuxProperty_Filter *) property;

			if (size != sizeof(GxDemuxProperty_Filter)) {
				gxlog_e(LOG_DEMUX, "filter the param error! id = %d\n", property_id);
				goto err;
			}

			if ((pfilter->filter_id >= MAX_FILTER_NUM) || (pfilter->filter_id < 0)) {
				gxlog_e(LOG_DEMUX, "filter filter_id error! id = %d\n", property_id);
				goto err;
			}

			if (GxDemuxPropertyID_FilterConfig == property_id) {
				if ((pfilter->depth < 0) || (pfilter->depth > MAX_FILTER_KEY_DEPTH)) {
					gxlog_e(LOG_DEMUX, "filter depth  error! id = %d\n", property_id);
					goto err;
				}

				if ((pfilter->depth == 2) || (pfilter->depth == 3)) {
					gxlog_e(LOG_DEMUX, "filter depth =2 or =3! id = %d\n", property_id);
					goto err;
				}

				if (demux_filter_config(demux, pfilter) < 0) {
					gxlog_e(LOG_DEMUX, "FilterConfig: error!\n");
					goto err;
				}

			} else if (GxDemuxPropertyID_FilterEnable == property_id) {
				if (demux_filter_enable(demux, pfilter) < 0) {
					gxlog_e(LOG_DEMUX, "FilterEnable: error!\n");
					goto err;
				}

			} else if (GxDemuxPropertyID_FilterDisable == property_id) {
				if (demux_filter_disable(demux, pfilter) < 0) {
					gxlog_e(LOG_DEMUX, "FilterDisable: error!\n");
					goto err;
				}

			} else if (GxDemuxPropertyID_FilterFree == property_id) {
				if (demux_filter_free(demux, pfilter) < 0) {
					gxlog_e(LOG_DEMUX, "FilterFree: error!\n");
					goto err;
				}
			}
		}
		break;

	case GxDemuxPropertyID_FilterFIFOReset:
		{
			GxDemuxProperty_FilterFifoReset *preset = (GxDemuxProperty_FilterFifoReset *) property;
			if (size != sizeof(GxDemuxProperty_FilterFifoReset)) {
				gxlog_e(LOG_DEMUX, "FilterFIFOReset: the param error!\n");
				goto err;
			}

			ret = demux_filter_fifo_reset(demux,preset);
		}
		break;

	case GxDemuxPropertyID_CAConfig:
	case GxDemuxPropertyID_CAFree:
		{
			GxDemuxProperty_CA *pca = (GxDemuxProperty_CA *) property;
			if (size != sizeof(GxDemuxProperty_CA)) {
				gxlog_e(LOG_DEMUX, "ca's param error!\n");
				goto err;
			}

			if ((pca->ca_id >= MAX_CA_NUM) || (pca->ca_id < 0)) {
				gxlog_e(LOG_DEMUX, "ca ca_id error!\n");
				goto err;
			}

			if (GxDemuxPropertyID_CAConfig == property_id) {
				if ((pca->slot_id >= MAX_SLOT_NUM) || (pca->slot_id < 0)) {
					gxlog_e(LOG_DEMUX, "ca slot_id error!\n");
					goto err;
				}

				if (demux_ca_config(demux, pca) < 0) {
					gxlog_e(LOG_DEMUX, "CAConfig: error!\n");
					goto err;
				}
			} else if (GxDemuxPropertyID_CAFree == property_id) {
				if (demux_cas_free(demux, pca->ca_id) < 0) {
					gxlog_e(LOG_DEMUX, "CAFree: error!\n");
					goto err;
				}
			}
		}
		break;

	case GxDemuxPropertyID_CASConfig:
	case GxDemuxPropertyID_CASFree:
	case GxDemuxPropertyID_CASSetCW:
		{
			GxDemuxProperty_CAS *pcas = (GxDemuxProperty_CAS *) property;
			if (size != sizeof(GxDemuxProperty_CAS) ||
				pcas->cas_id < 0 || pcas->cas_id > MAX_CAS_NUM) {
				gxlog_e(LOG_DEMUX, "the demux cas's param error!\n");
				goto err;
			}

			if (GxDemuxPropertyID_CASConfig == property_id)
				ret = demux_cas_config(demux, pcas);
			else if (GxDemuxPropertyID_CASFree == property_id)
				ret = demux_cas_free(demux, pcas->cas_id);
			else if (GxDemuxPropertyID_CASSetCW == property_id)
				ret = demux_cas_setcw(demux, pcas);
		}
		break;

	case GxDemuxPropertyID_CASAddPID:
	case GxDemuxPropertyID_CASDelPID:
	case GxDemuxPropertyID_CASSetBindMode:
		{
			GxDemuxProperty_CAS_EX *pcas = (GxDemuxProperty_CAS_EX *) property;
			if (size != sizeof(GxDemuxProperty_CAS_EX) ||
				pcas->cas_id < 0 || pcas->cas_id > MAX_CAS_NUM) {
				gxlog_e(LOG_DEMUX, "the demux cas's param error!\n");
				goto err;
			}

			else if (GxDemuxPropertyID_CASAddPID == property_id)
				ret = demux_cas_add_pid(demux, pcas->cas_id, pcas->pid);
			else if (GxDemuxPropertyID_CASDelPID == property_id)
				ret = demux_cas_del_pid(demux, pcas->cas_id, pcas->pid);
			else if (GxDemuxPropertyID_CASSetBindMode == property_id)
				ret = demux_cas_set_bind_mode(demux, pcas->cas_id, pcas->flags);
		}
		break;

	case GxDemuxPropertyID_CASSetSysKey:
		{
			GxDemuxProperty_SysKey *psys = (GxDemuxProperty_SysKey *) property;
			if (size != sizeof(GxDemuxProperty_SysKey)) {
				gxlog_e(LOG_DEMUX, "the demux cas's param error!\n");
				goto err;
			}

			ret = demux_cas_set_syskey(demux, psys);
		}
		break;

	case GxDemuxPropertyID_Pcr:
		{
			GxDemuxProperty_Pcr *pcr = (GxDemuxProperty_Pcr *) property;
			if (size != sizeof(GxDemuxProperty_Pcr)) {
				gxlog_e(LOG_DEMUX, "Sync : the demux's recover param error!\n");
				goto err;
			}

			ret = demux_set_pcrpid(demux, pcr);
		}
		break;

	case GxDemuxPropertyID_MTCCAConfig:
		{
			gxlog_e(LOG_DEMUX, "this propterty now is not support!!!!\n");
			goto err;
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

int demux_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0, sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(sub);

	gxdemux_lock();
	switch (property_id) {
	case GxDemuxPropertyID_Config:
		{
			GxDemuxProperty_ConfigDemux *pconfig = (GxDemuxProperty_ConfigDemux *) property;

			if (size != sizeof(GxDemuxProperty_ConfigDemux)) {
				gxlog_e(LOG_DEMUX, "ConfigDmx: the param error!\n");
				goto err;
			}

			if (module->sub < MAX_DMX_NORMAL)
				ret = demux_attribute(demux,pconfig);
		}
		break;

	case GxDemuxPropertyID_TSLockQuery:
		{
			GxDemuxProperty_TSLockQuery *pquery = (GxDemuxProperty_TSLockQuery *) property;
			if (size != sizeof(GxDemuxProperty_TSLockQuery)) {
				gxlog_e(LOG_DEMUX, "QueryTsLock: the param error!\n");
				goto err;
			}

			ret = demux_get_lock(demux, pquery);
		}
		break;

	case GxDemuxPropertyID_SlotAlloc:
		{
			GxDemuxProperty_Slot *pslot = (GxDemuxProperty_Slot *) property;

			if (size != sizeof(GxDemuxProperty_Slot)) {
				gxlog_e(LOG_DEMUX, "AllocSlot: the param error!\n");
				goto err;
			}

			if (pslot->type == DEMUX_SLOT_TS) {
				gxlog_e(LOG_DEMUX, "AllocSlot: Now is not support,Please use dvr, thank you!\n");
				goto err;
			}

			if (module->sub < MAX_DMX_NORMAL)
				ret = demux_slot_allocate(demux, pslot);

			else if (module->sub < MAX_DMX_TSW_SLOT) {
				ret = demux_tsw_slot_alloc(sub, pslot->pid, module->module_id, pslot->flags);
				pslot->slot_id = ret;
				ret = (ret >= 0) ? 0 : -1;
			}
		}
		break;

	case GxDemuxPropertyID_SlotConfig:
		{
			GxDemuxProperty_Slot *pslot = (GxDemuxProperty_Slot *) property;

			if (size != sizeof(GxDemuxProperty_Slot)) {
				gxlog_e(LOG_DEMUX, "SlotConfig: the param error!\n");
				goto err;
			}

			ret = demux_slot_attribute(demux, pslot);
		}
		break;

	case GxDemuxPropertyID_FilterAlloc:
		{
			GxDemuxProperty_Filter *pfilter = (GxDemuxProperty_Filter *) property;

			if (size != sizeof(GxDemuxProperty_Filter)) {
				gxlog_e(LOG_DEMUX, "FilterAlloc: the param error!\n");
				goto err;
			}

			ret = demux_filter_allocate(demux, pfilter);

		}
		break;

	case GxDemuxPropertyID_FilterConfig:
		{
			GxDemuxProperty_Filter *pfilter = (GxDemuxProperty_Filter *) property;

			if (size != sizeof(GxDemuxProperty_Filter)) {
				gxlog_e(LOG_DEMUX, "FilterConfig: the param error!\n");
				goto err;
			}

			ret = demux_filter_attribute(demux, pfilter);
		}
		break;

	case GxDemuxPropertyID_FilterMap:
		{
			GxDemuxProperty_FilterMap *pfilter = (GxDemuxProperty_FilterMap *) property;

			if (size != sizeof(GxDemuxProperty_FilterMap)) {
				gxlog_e(LOG_DEMUX, "FilterConfig: the param error!\n");
				goto err;
			}

			ret = demux_filter_get_hwbuf(demux, pfilter);
		}
		break;

	case GxDemuxPropertyID_CAAlloc:
		{
			GxDemuxProperty_CA *pca = (GxDemuxProperty_CA *) property;

			if (size != sizeof(GxDemuxProperty_CA)) {
				gxlog_e(LOG_DEMUX, "CAAlloc: the param error!\n");
				goto err;
			}

			if ((ret = demux_cas_allocate(demux)) < 0)
				goto err;

			pca->ca_id = ret;
			ret = 0;
		}
		break;
	case GxDemuxPropertyID_CAConfig:
		{
			GxDemuxProperty_CA *pca = (GxDemuxProperty_CA *) property;

			if (size != sizeof(GxDemuxProperty_CA)) {
				gxlog_e(LOG_DEMUX, "CAConfig: the param error!\n");
				goto err;
			}

			if ((pca->ca_id >= MAX_CA_NUM) || (pca->ca_id < 0)) {
				gxlog_e(LOG_DEMUX, "ca id err!\n");
				goto err;
			}
			demux_ca_attribute(demux, pca);
		}
		break;

	case GxDemuxPropertyID_CASAlloc:
		{
			GxDemuxProperty_CAS *pcas = (GxDemuxProperty_CAS *) property;

			if (size != sizeof(GxDemuxProperty_CAS)) {
				gxlog_e(LOG_DEMUX, "the demux cas's allocate param error!\n");
				goto err;
			}

			if ((ret = demux_cas_allocate(demux)) < 0)
				goto err;

			pcas->cas_id = ret;
			ret = 0;
		}
		break;

	case GxDemuxPropertyID_FilterRead:
		{
			GxDemuxProperty_FilterRead *pread = (GxDemuxProperty_FilterRead *) property;

			if (size != sizeof(GxDemuxProperty_FilterRead)) {
				gxlog_e(LOG_DEMUX, "FilterRead: the param error!\n");
				goto err;
			}

			ret = demux_filter_read(demux, pread);
		}
		break;

	case GxDemuxPropertyID_FilterFIFOQuery:
		{
			GxDemuxProperty_FilterFifoQuery *pquery = (GxDemuxProperty_FilterFifoQuery *) property;

			if (size != sizeof(GxDemuxProperty_FilterFifoQuery)) {
				gxlog_e(LOG_DEMUX, "QueryFilterFifo: the param error!\n");
				goto err;
			}

			ret = demux_filter_fifo_query(demux, pquery);
		}
		break;

	case GxDemuxPropertyID_SlotQueryByPid:
		{
			GxDemuxProperty_SlotQueryByPid *pquery = (GxDemuxProperty_SlotQueryByPid *) property;

			if (size != sizeof(GxDemuxProperty_SlotQueryByPid)) {
				gxlog_e(LOG_DEMUX, "SlotQueryByPid: the param error!\n");
				goto err;
			}

			ret = demux_slot_query(demux, pquery);
		}
		break;

	case GxDemuxPropertyID_ReadStc:
		{
#ifdef CONFIG_AV_MODULE_STC
			GxDemuxProperty_ReadStc *stc = (GxDemuxProperty_ReadStc *) property;

			if (size != sizeof(GxDemuxProperty_ReadStc)) {
				gxlog_e(LOG_DEMUX, "ReadStc: the param error!\n");
				goto err;
			}

			ret = gxav_stc_read_stc(sub/2, &stc->stc_value);
#endif
		}
		break;

	case GxDemuxPropertyID_ReadPcr:
		{
#ifdef CONFIG_AV_MODULE_STC
			GxDemuxProperty_ReadPcr *pcr = (GxDemuxProperty_ReadPcr *) property;

			if (size != sizeof(GxDemuxProperty_ReadPcr)) {
				gxlog_e(LOG_DEMUX, "ReadPcr: the param error!\n");
				goto err;
			}

			ret = gxav_stc_read_pcr(sub/2, &pcr->pcr_value);
#endif
		}
		break;

	case GxDemuxPropertyID_FilterFifoFreeSize:
		{
			GxDemuxProperty_FilterFifoFreeSize *fifo_free_size
				= (GxDemuxProperty_FilterFifoFreeSize *) property;

			if (size != sizeof(GxDemuxProperty_FilterFifoFreeSize)) {
				gxlog_e(LOG_DEMUX, "FilterFIFOFreeSize: GxDemuxProperty_FilterFifoFreeSize error!\n");
				goto err;
			}

			ret = demux_filter_free_size(demux, fifo_free_size);
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
