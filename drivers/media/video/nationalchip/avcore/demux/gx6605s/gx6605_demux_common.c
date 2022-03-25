#include "fifo.h"
#include "gxav_bitops.h"
#include "profile.h"
#include "porting.h"
#include "firewall.h"
#include "gx6605_demux.h"
#include "gx6605_hal_common.h"
#include "sdc_hal.h"
#include "kernelcalls.h"

extern struct reg_demux *gx6605_demux_reg;

static int find_first_zero(long long mask)
{
	long long temp  = mask;
	int i;

	for (i = 0; i < MAX_SLOT_NUM; i++) {
		if (((temp >> i)&0x1) == 0)
			return i;
	}
	return -1;
}

static int __filter_buffer_alloc(struct dmxdev *demux_device, struct dmx_filter *filter)
{
	if (filter->buffer_size <= BUFFER_PSI_SIZE)
		filter->virtual_dfbuf_addr = kcache_alloc(demux_device->psi_buffer_cache, BUFFER_PSI_SIZE);
	else if (filter->buffer_size <= BUFFER_TS_SIZE)
		filter->virtual_dfbuf_addr = kcache_alloc(demux_device->ts_buffer_cache, BUFFER_TS_SIZE);
	else if (filter->buffer_size <= BUFFER_PES_SIZE)
		filter->virtual_dfbuf_addr = kcache_alloc(demux_device->pes_buffer_cache, BUFFER_PES_SIZE);
	else
		filter->virtual_dfbuf_addr = gx_page_malloc(filter->buffer_size);

	if (filter->virtual_dfbuf_addr == NULL) {
		DMX_PRINTF("alloclate memory failed in kernel!\n");
		return -1;
	}

	return 0;
}

static void __filter_buffer_free(struct dmxdev *dev, struct dmx_filter *filter)
{
	if (filter->virtual_dfbuf_addr == NULL)
		return;

	if (filter->buffer_size <= BUFFER_PSI_SIZE)
		kcache_free(dev->psi_buffer_cache, filter->virtual_dfbuf_addr);
	else if (filter->buffer_size <= BUFFER_TS_SIZE)
		kcache_free(dev->ts_buffer_cache, filter->virtual_dfbuf_addr);
	else if (filter->buffer_size <= BUFFER_PES_SIZE)
		kcache_free(dev->pes_buffer_cache, filter->virtual_dfbuf_addr);
	else
		gx_page_free(filter->virtual_dfbuf_addr, filter->buffer_size);

	filter->virtual_dfbuf_addr = NULL;
}

void gx6605_demux_delay_free(struct dmxdev *dmxdev)
{
	struct dmx_filter *filter = NULL;
	struct timeval tv;
	struct timezone tz;
	struct timeval *ftv;
	long time=0;
	int i=0;
	int id=0;;
	for(i=0;i<MAX_FILTER_NUM;i++){
		filter = dmxdev->filters[i];
		if(filter != NULL && filter->status == DMX_STATE_DELAY_FREE){
			gx_gettimeofday(&tv,&tz);
			ftv = &(filter->tv);
			time = ((tv.tv_sec*1000000+tv.tv_usec)-(ftv->tv_sec*1000000+ftv->tv_usec));
			DMX_DBG("i %d time %ld\n",i, time);
			if(time > 4000){
				id = filter->id;
				DMX_DBG("i %d id %d old sec %ld old usec%ld t_sec %ld\n",i,id,filter->tv.tv_sec,filter->tv.tv_usec,(ftv->tv_sec*1000000+ftv->tv_usec));
				DMX_DBG("i %d id %d new sec %ld new usec%ld t_sec %ld\n",i,id,tv.tv_sec,tv.tv_usec,(tv.tv_sec*1000000+tv.tv_usec));
				filter->status = DMX_STATE_FREE;
				gx6605_clear_df_rw(id);
				gx6605_clear_df_buf(id);

				__filter_buffer_free(dmxdev, filter);
				dmxdev->filters[id] = NULL;
				gx_free(filter);
				filter = NULL;
				CLEAR_VAL_BIT(dmxdev->filter_mask, id);
			}
		}
	}
}
int gx6605_dmx_avbuf_callback(unsigned int channel_id,unsigned int length, unsigned int underflow,void *arg)
{
	int av_bufid = *(int *)arg;
	unsigned int av_read_addr = 0;
	unsigned int av_write_addr = 0;
	unsigned int sdc_write_addr = 0;
	unsigned int pts_write_addr = 0;
	struct demux_fifo *avfifo=NULL;

	avfifo = &gx6605_dmx.avfifo[av_bufid];
	gxav_sdc_rwaddr_get(avfifo->channel_id, &av_read_addr,&sdc_write_addr);
	//gx_printf("update=%08x %08x\n",av_write_addr,av_read_addr);
	REG_SET_VAL(&(gx6605_demux_reg->av_addr[av_bufid].av_read_addr), av_read_addr);
	av_write_addr = REG_GET_VAL(&(gx6605_demux_reg->av_addr[av_bufid].av_write_addr));
	if(av_write_addr != sdc_write_addr)
		gxav_sdc_Wptr_Set(avfifo->channel_id, av_write_addr);

	pts_write_addr = REG_GET_VAL(&(gx6605_demux_reg->pts_w_addr[av_bufid]));
	gxav_sdc_Wptr_Set(gx6605_dmx.avfifo[av_bufid].pts_channel_id, pts_write_addr);
	DMX_DBG("demux callback av_wptr 0x%x \n",av_write_addr);
	return 0;
}

void gx6605_dmx_initialization(struct dmxdev *demux_device)
{
	gx_memset(demux_device->slots,       0, sizeof(struct dmx_slot *)   * MAX_SLOT_NUM);
	gx_memset(demux_device->pidslots,    0, sizeof(struct dmx_slot *)   * MAX_SLOT_NUM);
	gx_memset(demux_device->muxslots,    0, sizeof(struct dmx_slot *)   * MAX_SLOT_NUM);
	gx_memset(demux_device->filters,     0, sizeof(struct dmx_filter *) * MAX_FILTER_NUM);
	gx_memset(demux_device->muxfilters,  0, sizeof(struct dmx_filter *) * MAX_FILTER_NUM);
	gx_memset(demux_device->demuxs,      0, sizeof(struct dmx_demux)    * MAX_DEMUX_CHANNEL);
	gx_memset(demux_device->ca,          0, sizeof(struct dmx_ca *)     * MAX_CA_NUM);

	demux_device->filter_mask      = 0x0ULL;
	demux_device->mux_filter_mask  = 0x0ULL;
	demux_device->slot_mask        = 0x0ULL;
	demux_device->pid_slot_mask    = 0x0ULL;
	demux_device->mux_slot_mask    = 0x0ULL;
	demux_device->ca_mask          = 0;
	demux_device->avbuf_mask       = 0;
	demux_device->pes_buffer_cache = kcache_create("pes buffer", BUFFER_PES_SIZE, 0);
	demux_device->psi_buffer_cache = kcache_create("psi buffer", BUFFER_PSI_SIZE, 0);
	demux_device->ts_buffer_cache  = kcache_create("ts buffer",  (BUFFER_TS_SIZE+188*2), 0);

	gx6605_set_clock();
	gx6605_clear_sram();
	gx6605_set_start();
}

void gx6605_dmx_cleanup(struct dmxdev *demux_device)
{
	if (demux_device->pes_buffer_cache)
		kcache_destroy(demux_device->pes_buffer_cache);

	if (demux_device->psi_buffer_cache)
		kcache_destroy(demux_device->psi_buffer_cache);

	if (demux_device->ts_buffer_cache)
		kcache_destroy(demux_device->ts_buffer_cache);
}

void gx6605_dmx_config(struct dmx_demux *dmx)
{
	gx6605_set_gate(dmx);
	gx6605_set_ts_source(dmx);
	gx6605_set_ts_mode(dmx);
}

int gx6605_dmx_link_fifo(struct dmx_demux *dmx, struct demux_fifo *fifo)
{
	unsigned int start_addr;
	unsigned int end_addr;
	unsigned char buf_id;

	start_addr = fifo->buffer_start_addr;
	end_addr   = fifo->buffer_end_addr;
	buf_id     = fifo->buffer_id;

	if (GXAV_PIN_INPUT == fifo->direction) {
		if (fifo->pin_id == PIN_ID_SDRAM_INPUT) {
			struct gxav_channel *channel = (struct gxav_channel *)(fifo->channel);
			DMX_DBG("[rts sdc link] start_addr : 0x%x ,end_addr : 0x%x,buffer id=0x%x\n",
					start_addr, end_addr, buf_id);
			dmx->demux_device->rfifo = *fifo;
			gx6605_link_rts_sdc(start_addr, end_addr - start_addr + 1, buf_id);

			channel->indata = &dmx->demux_device->rfifo;
			channel->incallback = gx6605_rts_cb;
		}
	}
	else if (GXAV_PIN_OUTPUT == fifo->direction) {
		if ((fifo->pin_id >= PIN_ID_SDRAM_OUTPUT)) {
			struct gxav_channel *channel = (struct gxav_channel *)(fifo->channel);
			DMX_DBG("[wts sdc link] start_addr : 0x%x ,end_addr : 0x%x,buffer id=0x%x\n",
					start_addr, end_addr,buf_id);
			if((end_addr - start_addr + 1)%188){
				DMX_PRINTF("tsw buffer size need x188\n");
				return -1;
			}
			dmx->demux_device->fifo[fifo->pin_id-PIN_ID_SDRAM_OUTPUT] = *fifo;
			gx6605_link_wts_sdc(dmx,start_addr, end_addr - start_addr + 1, fifo->pin_id);

			channel->outdata = &dmx->demux_device->fifo[fifo->pin_id-PIN_ID_SDRAM_OUTPUT];
			channel->outcallback = gx6605_wts_cb;
		}
		else if (fifo->pin_id < PIN_ID_SDRAM_OUTPUT) {
			struct dmx_slot *slot = NULL;
			struct gxav_channel *channel = (struct gxav_channel *)fifo->channel;
			slot = dmx->slots[fifo->pin_id];
			dmx->demux_device->avfifo[slot->avbuf_id] = *fifo;
			gx6605_link_avbuf(slot, start_addr, end_addr - start_addr + 1, buf_id);

			start_addr = fifo->pts_start_addr;
			end_addr   = fifo->pts_end_addr;
			buf_id     = fifo->pts_buffer_id;
			gx6605_link_ptsbuf(slot, start_addr, end_addr - start_addr + 1, buf_id);
			slot->dir = fifo->direction;
			gxav_sdc_buffer_reset(fifo->channel_id);
			gxav_sdc_buffer_reset(fifo->pts_channel_id);

			channel->outdata     = (void *)&slot->avbuf_id;
			channel->outcallback = gx6605_dmx_avbuf_callback;
		}
	}

	return 0;
}

int gx6605_dmx_unlink_fifo(struct dmx_demux *dmx, struct demux_fifo *fifo)
{
	if (GXAV_PIN_INPUT == fifo->direction) {
		struct gxav_channel *channel = (struct gxav_channel *)(fifo->channel);
		channel->incallback = NULL;
		channel->indata = NULL;

		if (fifo->pin_id == PIN_ID_SDRAM_INPUT) {
			DMX_DBG("unlink source from sdram.\n");
			gx6605_unlink_rts_sdc();
		}
	}
	else if (GXAV_PIN_OUTPUT == fifo->direction) {
		struct gxav_channel *channel = (struct gxav_channel *)(fifo->channel);
		channel->outcallback = NULL;
		channel->outdata = NULL;

		if (fifo->pin_id == PIN_ID_SDRAM_OUTPUT /* fifo->pin_id >= PIN_ID_SDRAM_OUTPUT0 && fifo->pin_id <= PIN_ID_SDRAM_OUTPUT15 */ ) {
			DMX_DBG("unlink source to sdram.\n");
			gx6605_unlink_wts_sdc(dmx,fifo->pin_id);
		} else if (fifo->pin_id < PIN_ID_SDRAM_OUTPUT) {
			struct dmx_slot *slot = dmx->slots[fifo->pin_id];
			gx6605_unlink_avbuf(slot);
			gx_memset(&(dmx->demux_device->avfifo[slot->avbuf_id]), 0 ,sizeof(struct demux_fifo));
		}
	}

	return 0;
}

inline void gx6605_dmx_query_tslock(struct dmx_demux *dmx)
{
	gx6605_query_tslock(dmx);
}

int gx6605_dmx_alloc_slot(struct dmxdev *demux_device, int demux_id, enum dmx_slot_type type, unsigned short pid)
{
	int i;
	struct dmx_demux *demux = &demux_device->demuxs[demux_id];

	if(DEMUX_SLOT_MUXTS == type){
		i = find_first_zero(demux_device->mux_slot_mask);
		if (i < 0) {
			DMX_PRINTF("have no slot left!\n");
			return -1;
		}
		SET_VAL_BIT(demux_device->mux_slot_mask, i);

		demux_device->muxslots[i] = (struct dmx_slot *)gx_malloc(sizeof(struct dmx_slot));
		if (NULL == demux_device->muxslots[i]) {
			DMX_PRINTF("alloc NULL!\n");
			goto err;
		}

		gx_memset(demux_device->muxslots[i], 0, sizeof(struct dmx_slot));
		demux_device->muxslots[i]->demux = demux;
		demux_device->muxslots[i]->id = i;
		demux_device->muxslots[i]->type = type;
		demux->muxslots[i] = demux_device->muxslots[i];
		demux->muxslot_num++;

		if(pid == 0xffff)
		{
			int temp = REG_GET_VAL(&(gx6605_demux_reg->ts_write_ctrl));
			struct dmx_demux* dmx = &demux_device->demuxs[demux_id];
			if(demux_id == 0){
				temp &= ~(0x07 << 8);
				temp |= (dmx->source << 8);
				temp &= ~(0x3f << 16);
				temp |= (i << 16);
			}
			else{
				temp &= ~(0x07 << 12);
				temp |= (dmx->source << 12);
				temp &= ~(0x3f << 24);
				temp |= (i << 24);
			}
			REG_SET_VAL(&(gx6605_demux_reg->ts_write_ctrl), temp);
		}

		return (demux_device->muxslots[i]->id + PIN_ID_SDRAM_OUTPUT);

	}

	if(demux_id == 2) {
		for (i = 0; i < MAX_SLOT_NUM; i++) {
			if (gx6605_dmx.pidslots[i] != NULL) {
				if ((gx6605_dmx.pidslots[i]->pid == pid) &&
						(gx6605_dmx.pidslots[i]->type == type) &&
						(pid != 0x1fff)) {
					gx6605_dmx.pidslots[i]->refcount++;
					return i;
				}
			}
		}
		i = find_first_zero(demux_device->pid_slot_mask);
		if (i < 0) {
			DMX_PRINTF("have no slot left!\n");
			return -1;
		}
		SET_VAL_BIT(demux_device->pid_slot_mask, i);


		demux_device->pidslots[i] = (struct dmx_slot *)gx_malloc(sizeof(struct dmx_slot));
		if (NULL == demux_device->pidslots[i]) {
			DMX_PRINTF("alloc NULL!\n");
			goto err;
		}

		gx_memset(demux_device->pidslots[i], 0, sizeof(struct dmx_slot));
		demux_device->pidslots[i]->demux = demux;
		demux_device->pidslots[i]->id = i;
		demux_device->pidslots[i]->type = type;
		demux_device->pidslots[i]->refcount = 0;
		demux_device->pidslots[i]->pid = 0x3fff;
		demux->slots[i] = demux_device->pidslots[i];

		return demux_device->pidslots[i]->id;
	}

	for (i = 0; i < MAX_SLOT_NUM; i++) {
		if (gx6605_dmx.slots[i] != NULL) {
			if ((gx6605_dmx.slots[i]->pid == pid) &&
				(gx6605_dmx.slots[i]->type == type) &&
				(gx6605_dmx.slots[i]->demux== demux) &&
				(pid != 0x1fff)) {
				gx6605_dmx.slots[i]->refcount++;
				return i;
			}
		}
	}

	if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)) {
		SET_VAL_BIT(demux_device->slot_mask, 0);
		SET_VAL_BIT(demux_device->slot_mask, 1);
		if(type == DEMUX_SLOT_AUDIO){
			i = 0;
		}else if(type == DEMUX_SLOT_VIDEO){
			i = 1;
		}else{

			i = find_first_zero(demux_device->slot_mask);
			if (i < 0) {
				DMX_PRINTF("have no slot left!\n");
				return -1;
			}
			SET_VAL_BIT(demux_device->slot_mask, i);
		}
	} else {
		i = find_first_zero(demux_device->slot_mask);
		if (i < 0) {
			DMX_PRINTF("have no slot left!\n");
			return -1;
		}
		SET_VAL_BIT(demux_device->slot_mask, i);
	}

	demux_device->slots[i] = (struct dmx_slot *)gx_malloc(sizeof(struct dmx_slot));
	if (NULL == demux_device->slots[i]) {
		DMX_PRINTF("alloc NULL!\n");
		goto err;
	}

	gx_memset(demux_device->slots[i], 0, sizeof(struct dmx_slot));
	demux_device->slots[i]->demux = demux;
	demux_device->slots[i]->id = i;
	demux_device->slots[i]->pid = pid;
	demux_device->slots[i]->type = type;
	demux_device->slots[i]->filter_num = 0;
	demux_device->slots[i]->refcount = 0;
	demux->slots[i] = demux_device->slots[i];
	demux->slot_num++;

	if ((DEMUX_SLOT_AUDIO == demux_device->slots[i]->type)
			|| (DEMUX_SLOT_SPDIF == demux_device->slots[i]->type)
			|| (DEMUX_SLOT_VIDEO == demux_device->slots[i]->type)
			|| (DEMUX_SLOT_AUDIO_SPDIF == demux_device->slots[i]->type))
	{
		int avbuf_id = find_first_zero(demux_device->avbuf_mask);
		if ((avbuf_id >= MAX_AVBUF_NUM) || (avbuf_id < 0)) {
			DMX_PRINTF("allocate avbuf num error!!!!\n");
			goto err;
		}
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)) {
			avbuf_id = i;
		}
		SET_VAL_BIT(demux_device->avbuf_mask, avbuf_id);
		demux_device->slots[i]->avbuf_id = avbuf_id;
		demux_device->avslots[avbuf_id] = demux_device->slots[i];
		DMX_DBG("demux_device->slots[i]->avbuf_id = %d\n", demux_device->slots[i]->avbuf_id);
	}
	else {
		demux_device->slots[i]->avbuf_id = -1;
	}

	return demux_device->slots[i]->id;

err:
	if(demux_device->slots[i]) {
		gx_free(demux_device->slots[i]);
		demux_device->slots[i] = NULL;
		demux->slot_num--;
		demux->slots[i] = NULL;
	}
	CLEAR_VAL_BIT(demux_device->slot_mask, i);
	if(demux_id == 2)
		CLEAR_VAL_BIT(demux_device->pid_slot_mask, i);

	if(DEMUX_SLOT_MUXTS == type)
		CLEAR_VAL_BIT(demux_device->mux_slot_mask, i);

	return -1;
}

void gx6605_dmx_free_slot(struct dmx_slot *slot)
{
	int i;
	struct dmx_demux *demux = slot->demux;
	struct dmxdev *demux_device = demux->demux_device;
	int index = slot->id;

	if(DEMUX_SLOT_MUXTS == slot->type){
		struct dmx_filter *filter = slot->muxfilters;

		if(filter){
			gx6605_unlink_wts_sdc(demux,slot->id);
			CLEAR_VAL_BIT(demux_device->mux_filter_mask, filter->id);
			__filter_buffer_free(demux_device, filter);
			gxfifo_free(&filter->section_fifo);

			if(demux_device->muxfilters[(unsigned int)filter->id]) {
				gx_free(demux_device->muxfilters[(unsigned int)filter->id]);
				demux_device->muxfilters[(unsigned int)filter->id] = NULL;
			}

			slot->muxfilters = NULL;
		}

		CLEAR_VAL_BIT(demux_device->mux_slot_mask, index);
		if (demux_device->muxslots[index]) {
			gx_free(demux_device->muxslots[index]);
			demux_device->muxslots[index] = NULL;
		}
		demux->muxslot_num--;
		return;

	}

	if(demux->id == 2) {
		CLEAR_VAL_BIT(demux_device->pid_slot_mask, index);
		gx6605_clr_pid_slot(slot);
		if (demux_device->pidslots[index]) {
			gx_free(demux_device->pidslots[index]);
			demux_device->pidslots[index] = NULL;
		}
		return;
	}

	DMX_DBG("demux_device->slot_mask = 0x%llx\n", demux_device->slot_mask);

	if(slot->avbuf_id >= 0) {
		 demux_device->avslots[slot->avbuf_id] = NULL;
	}

	gx6605_clr_slot_cfg(slot);
	gx6605_disable_slot(slot);
	CLEAR_VAL_BIT(demux_device->slot_mask, index);

	if (slot->type == DEMUX_SLOT_SPDIF) {
		gx6605_clear_spdif();
	}

	if ((DEMUX_SLOT_AUDIO == slot->type) || (DEMUX_SLOT_SPDIF == slot->type)
			|| (DEMUX_SLOT_AUDIO_SPDIF == slot->type) || (DEMUX_SLOT_VIDEO == slot->type)) {
		CLEAR_VAL_BIT(demux_device->avbuf_mask, slot->avbuf_id);
		demux_device->avslots[slot->avbuf_id] = NULL;

		struct demux_fifo *avfifo = NULL;
		avfifo = &demux_device->avfifo[slot->avbuf_id];
		if(avfifo->channel){
			struct gxav_channel *channel = (struct gxav_channel *)avfifo->channel;
			channel->outcallback = NULL;
			channel->outdata     = NULL;
			gx_memset(avfifo, 0,sizeof(struct demux_fifo));
		}
	}

	demux->slot_num--;
	/*if the filter_num nequal 0,you need free filter */
	if (slot->filter_num > 0) {
		for (i = 0; i < MAX_FILTER_NUM; i++) {
			if (NULL != slot->filters[i]) {
				gx6605_dmx_free_filter(slot->filters[i]);
				if (slot->filter_num == 0) {
					break;
				}
			}
		}
	}

	if(demux_device->slots[index]) {
		gx_free(demux_device->slots[index]);
		demux_device->slots[index] = NULL;
	}
	demux->slots[index] = NULL;
}

int gx6605_dmx_alloc_filter(struct dmx_slot *slot)
{
	int i,ret;
	struct dmx_filter *filter = NULL;
	struct dmxdev *demux_device = NULL;
	struct dmx_demux *demux = slot->demux;

	if(slot == NULL || slot->demux == NULL || slot->demux->demux_device == NULL)
		return -1;

	demux_device = slot->demux->demux_device;
	gx6605_demux_delay_free(demux_device);

	DMX_DBG("slot->id = %d\n", slot->id);

	if(DEMUX_SLOT_MUXTS == slot->type){
		i = find_first_zero(demux_device->mux_filter_mask);
		if (i < 0) {
			DMX_PRINTF("have no filter left!\n");
			return -1;
		}
		SET_VAL_BIT(demux_device->mux_filter_mask, i);

		demux_device->muxfilters[i] = (struct dmx_filter *)gx_malloc(sizeof(struct dmx_filter));
		if (NULL == demux_device->muxfilters[i]) {
			DMX_PRINTF("alloc NULL!\n");
			return -1;
		}

		gx_memset(demux_device->muxfilters[i], 0, sizeof(struct dmx_filter));
		demux_device->muxfilters[i]->slot = slot;
		demux_device->muxfilters[i]->id = i;
		slot->muxfilters = demux_device->muxfilters[i];

		filter = demux_device->muxfilters[i];
		filter->buffer_size = slot->hw_buffer_size;
		// 48128(4*47*256) is the least common multiple of 188, 1024
		if(filter->buffer_size < 48128) {
			filter->buffer_size = 48128;
			DMX_PRINTF("tsw buf is error and user buf =%d  \n",filter->buffer_size);
		} else {
			filter->buffer_size = (filter->buffer_size)/48128*48128;
			if(filter->buffer_size%4096 == 0)
				filter->buffer_size += 48128;

			DMX_PRINTF("tsw buf 188 && 1024 align not 4k align modify %d\n",filter->buffer_size);
		}
		filter->virtual_dfbuf_addr = gx_page_malloc((filter->buffer_size+188*2));

		if (NULL == filter->virtual_dfbuf_addr) {
			DMX_PRINTF("alloclate memory failed in kernel!\n");
			goto err;
		}

		ret = gxfifo_init(&filter->section_fifo, NULL, slot->sw_buffer_size);
		if (ret < 0) {
			DMX_PRINTF("filter allocate fifo failed!\n");
			goto err;
		}

		gx_memset(filter->virtual_dfbuf_addr, 0, filter->buffer_size);
		gx_dcache_clean_range(0,0);
		filter->phy_dfbuf_addr = gx_virt_to_phys((unsigned int)filter->virtual_dfbuf_addr);
		gx6605_link_wts_sdc(demux,filter->phy_dfbuf_addr, filter->buffer_size, slot->id);

		return (demux_device->muxfilters[i]->id + PIN_ID_SDRAM_OUTPUT);

	}

	i = find_first_zero(demux_device->filter_mask);
	if (i < 0) {
		DMX_PRINTF("have no filter left!\n");
		return -1;
	}
	SET_VAL_BIT(demux_device->filter_mask, i);

	demux_device->filters[i] = (struct dmx_filter *)gx_malloc(sizeof(struct dmx_filter));
	if (NULL == demux_device->filters[i]) {
		DMX_PRINTF("alloc NULL!\n");
		goto err;
	}

	gx_memset(demux_device->filters[i], 0, sizeof(struct dmx_filter));
	demux_device->filters[i]->slot = slot;
	demux_device->filters[i]->id = i;
	demux_device->filters[i]->status = DMX_STATE_ALLOCATED;
	demux_device->filters[i]->refcount = 0;
	slot->filters[i] = demux_device->filters[i];
	slot->filter_num++;

	filter = demux_device->filters[i];
	gx_memset(&(filter->tv),0,sizeof(struct timeval));
	filter->buffer_size = slot->hw_buffer_size;

	if (__filter_buffer_alloc(demux_device, filter) != 0)
		goto err;


	if ((DEMUX_SLOT_PSI == filter->slot->type) || (filter->slot->type == DEMUX_SLOT_PES || filter->slot->type == DEMUX_SLOT_PES_AUDIO
		    || filter->slot->type == DEMUX_SLOT_PES_VIDEO) || (DEMUX_SLOT_TS == filter->slot->type)) {
		int ret = gxfifo_init(&filter->section_fifo, NULL, slot->sw_buffer_size);
		if (ret < 0) {
			DMX_PRINTF("filter allocate fifo failed!\n");
			goto err;
		}
	}

	gx_memset(filter->virtual_dfbuf_addr, 0, filter->buffer_size);
	gx_dcache_clean_range(0,0);
	filter->phy_dfbuf_addr = gx_virt_to_phys((unsigned int)filter->virtual_dfbuf_addr);

	return demux_device->filters[i]->id;

err:
	__filter_buffer_free(demux_device, filter);

	if(demux_device->muxfilters[i]) {
		gx_free(demux_device->muxfilters[i]);
		demux_device->muxfilters[i] = NULL;
		slot->muxfilters = NULL;
	}
	CLEAR_VAL_BIT(demux_device->mux_filter_mask, i);

	if(demux_device->filters[i]) {
		gx_free(demux_device->filters[i]);
		demux_device->filters[i] = NULL;
		slot->filter_num--;
		slot->filters[i] = NULL;
	}
	CLEAR_VAL_BIT(demux_device->filter_mask, i);

	return -1;
}

void gx6605_dmx_free_filter(struct dmx_filter *filter)
{
	int filter_index = filter->id;
	struct dmx_slot *slot = filter->slot;
	struct dmxdev *demux_device = slot->demux->demux_device;
	struct dmx_demux *demux = slot->demux;
	struct timezone tz;

	if(filter->slot->type == DEMUX_SLOT_MUXTS){
		gx6605_unlink_wts_sdc(demux,slot->id);
		CLEAR_VAL_BIT(demux_device->mux_filter_mask, filter->id);
		gx_page_free(filter->virtual_dfbuf_addr, (filter->buffer_size+188*2));
		gxfifo_free(&filter->section_fifo);

		slot->muxfilters = NULL;
		demux_device->muxfilters[filter_index] = NULL;
		gx_free(filter);
		return;
	}

	gx6605_clear_int_df(filter_index);
	gx6605_disable_filter(filter_index);
	gx6605_disable_pid_sel(slot->id, filter_index);
	filter->status = DMX_STATE_DELAY_FREE;
	if ((DEMUX_SLOT_PSI == filter->slot->type) || (filter->slot->type == DEMUX_SLOT_PES || filter->slot->type == DEMUX_SLOT_PES_AUDIO
		    || filter->slot->type == DEMUX_SLOT_PES_VIDEO))
		gxfifo_free(&filter->section_fifo);

	slot->filter_num--;
	slot->filters[filter_index] = NULL;
	filter->slot = NULL;
	gx_gettimeofday(&(filter->tv),&tz);
	gx6605_demux_delay_free(demux_device);
}

void gx6605_dmx_slot_config(struct dmx_slot *slot)
{
	struct dmx_demux *demux = slot->demux;

	if(demux->id == 2){
		gx6605_set_pid_slot(slot);
		return;
	}

	if ((DEMUX_SLOT_PSI == slot->type) || (slot->type == DEMUX_SLOT_PES) || (slot->type == DEMUX_SLOT_PES_AUDIO)
			|| (slot->type == DEMUX_SLOT_PES_VIDEO) || (DEMUX_SLOT_TS == slot->type)) {
		gx6605_set_stop_unit(slot);
		gx6605_set_interrupt_unit(slot);
		gx6605_set_filter_type(slot);
		gx6605_set_crc_disable(slot);
	}else if ((DEMUX_SLOT_AUDIO == slot->type) || (DEMUX_SLOT_SPDIF == slot->type)
			|| (DEMUX_SLOT_VIDEO == slot->type) || (DEMUX_SLOT_AUDIO_SPDIF == slot->type)){
		if (DEMUX_SLOT_SPDIF == slot->type) {
			gx6605_set_audio_ac3();
			//gx6605_audioout_pcmtocompress();
		}
		gx6605_set_av_type(slot);
		gx6605_set_pts_bypass(slot);
		gx6605_set_avnum(slot);

	}
	gx6605_reset_cc(slot);
	gx6605_set_pts_to_sdram(slot);
	gx6605_set_type_sel(slot);
	gx6605_set_pid(slot);
	gx6605_set_av_out(slot);
	gx6605_set_ts_out(slot);
	gx6605_set_err_discard(slot);
	gx6605_set_dup_discard(slot);
}

void gx6605_dmx_filter_config(struct dmx_filter *filter)
{
	struct timeval tv;
	struct timezone tz;
	struct timeval *ftv;
	long time=0;
	if(filter ->status != DMX_STATE_ALLOCATED){
		gx_gettimeofday(&tv,&tz);
		ftv = &(filter->tv);
		time = ((tv.tv_sec*1000000+tv.tv_usec)-(ftv->tv_sec*1000000+ftv->tv_usec));
		DMX_DBG("i %d time %ld\n",filter->id, time);
		if(time < 4000)
			gx_mdelay(3);
	}
	filter ->status =  DMX_STATE_SET;
	gx6605_set_dfbuf(filter);
	gx6605_set_select(filter->slot, filter);
	gx6605_set_filtrate(filter);
	gx6605_set_filter_reset(filter);
}

void gx6605_dmx_enable_slot(struct dmx_slot *slot)
{
	gx6605_enable_slot(slot);
}

void gx6605_dmx_disable_slot(struct dmx_slot *slot)
{
	gx6605_disable_slot(slot);
}

void gx6605_dmx_enable_filter(struct dmx_filter *filter)
{
	struct timeval tv;
	struct timezone tz;
	struct timeval *ftv;
	long time=0;
	if(filter ->status != DMX_STATE_SET){
		gx_gettimeofday(&tv,&tz);
		ftv = &(filter->tv);
		time = ((tv.tv_sec*1000000+tv.tv_usec)-(ftv->tv_sec*1000000+ftv->tv_usec));
		DMX_DBG("i %d time %ld\n",filter->id, time);
		if(time < 4000)
			gx_mdelay(3);

		gx_memset(&(filter->tv),0,sizeof(struct timeval));
	}

	filter->status = DMX_STATE_GO;
	filter->pread_rd = 0;
	gx6605_clear_df_rw(filter->id);
	gx6605_enable_filter(filter);
}

void gx6605_dmx_disable_filter(struct dmx_filter *filter)
{
	struct timezone tz;
	gx_gettimeofday(&(filter->tv),&tz);
	filter->status = DMX_STATE_STOP;
	gx6605_disable_filter(filter->id);
}

int gx6605_dmx_alloc_ca(struct dmxdev *demux_device , int demux_id)
{
	int i;
	struct dmx_demux *demux = &demux_device->demuxs[demux_id];

	i = find_first_zero(demux_device->ca_mask);
	if ((i >= MAX_CA_NUM) || (i < 0)) {
		DMX_PRINTF("ca_mask is full!\n");
		return -1;
	}

	demux_device->ca[i] = (struct dmx_ca *)gx_malloc(sizeof(struct dmx_ca));
	if (NULL == demux_device->ca[i]) {
		DMX_PRINTF("alloc NULL!\n");
		return -1;
	}

	SET_VAL_BIT(demux_device->ca_mask, i);

	demux_device->ca[i]->demux_device = demux_device;
	demux_device->ca[i]->demux = demux;
	demux_device->ca[i]->id = i;
	demux->ca[i] = demux_device->ca[i];
	return demux_device->ca[i]->id;
}

void gx6605_dmx_ca_config(struct dmx_slot *slot, struct dmx_ca *ca)
{
	gx6605_set_descramble(ca, slot);
}

void gx6605_dmx_free_ca(struct dmx_ca *ca)
{
	int index = ca->id;
	struct dmxdev *demux_device = ca->demux_device;
	struct dmx_demux *demux = ca->demux;

	CLEAR_VAL_BIT(demux_device->ca_mask, index);
	if(demux_device->ca[index]) {
		demux->ca[index] = NULL;
		gx_free(demux_device->ca[index]);
		demux_device->ca[index] = NULL;
	}
}

void gx6605_dmx_mtc_decrypt(gx_mtc_info *mtc_info_cfg,struct dmx_ca *ca)
{
	gx6605_mtc_decrypt(mtc_info_cfg,ca);
}

void gx6605_dmx_pcr_sync(struct dmxdev *demux_device, int demux_id, unsigned short pcr_pid)
{
	struct dmx_demux *demux = &demux_device->demuxs[demux_id];
	demux->pcr_pid = pcr_pid;

	gx6605_set_pcr_sync(demux);
}

static unsigned char tmp[4099];
unsigned int gx6605_dmx_filter_read(struct dmx_filter *filter, unsigned char *data_buf, unsigned int size)
{
	unsigned char crc = 0;
	unsigned char data[16];
	unsigned char data_next[16];
	unsigned int section_size = 0;
	struct gxfifo *fifo = &filter->section_fifo;
	unsigned int fifo_len = 0, real_len = 0;
	int crc_en_flag = ((filter->flags & DMX_CRC_IRQ) >> 0);
	int eq_flag = ((filter->flags & DMX_EQ) >> 1);
	int sw_flag = ((filter->flags & DMX_SW_FILTER) >> 7);
	unsigned int i =0,depth;

	if (data_buf == NULL){
		DMX_PRINTF("user data buffer is null!");
		return -1;
	}
	if (DEMUX_SLOT_PSI == filter->slot->type) {
		while (real_len < size) {
discard:
			fifo_len = gxfifo_len(fifo);
			if(fifo_len >= 3){
				gxfifo_peek(fifo, data, 3,0);
				section_size = (((data[1]) & 0x0F) << 8) + (data[2]) +3;
				if(section_size+1 > fifo_len){
					if(section_size >size ){
						gxfifo_get(fifo, tmp, section_size+1);
						DMX_PRINTF("section len %d> size %d&& fifo_len%d > size%d\n",section_size,size,fifo_len,size);
						return 0;
					}else
						return real_len;
				}

			}else {
				return real_len;
			}
			if(sw_flag == 1){
				if(eq_flag == 1){
					if(filter->depth == 0)
						depth = 1;
					else
						depth = filter->depth;

					gxfifo_peek(fifo, data, depth,0);
					for(i=0;i<depth;i++) {
						if((data[i] & filter->key[i].mask) != (filter->key[i].value & filter->key[i].mask)){
							DMX_DBG("data[%d] = %x key[%d] = %x\n",i,data[i],i,filter->key[i].value);
							gxfifo_get(fifo, tmp, section_size+1);
							goto discard;
						}

					}
				}
			}
			real_len += section_size ;
			if(real_len > size){
				DMX_PRINTF("real len %d> size %d\n",real_len,size);
				return real_len - section_size ;
			}

			real_len -= section_size ;
			gxfifo_user_get(fifo, data_buf+real_len, section_size);
			gxfifo_get(fifo, &crc, 1);
			fifo_len = gxfifo_len(fifo);
			if(fifo_len >= 3){
				gxfifo_peek(fifo, data, 1,0);
			}
			if(crc == 0 && (data[0] & filter->key[0].mask) == (filter->key[0].value & filter->key[0].mask) && crc_en_flag == 1)
				real_len += section_size ;
			else if(crc == 0 && (data[0] & filter->key[0].mask) != (filter->key[0].value & filter->key[0].mask) && crc_en_flag == 1)
				DMX_DBG("!!!!!!tid=%d len = %d key=%d mask=%x!!!!!crc err!\n",data[0],section_size,filter->key[0].value,filter->key[0].mask);
			else if(crc_en_flag == 0)
				real_len += section_size ;
			else
				DMX_DBG("section len %d real_len%d crc%d tid = %d\n",section_size,real_len,crc,data[0]);

		}
	}
	else if (DEMUX_SLOT_PES == filter->slot->type) {
		while (real_len < size) {
			fifo_len = gxfifo_len(fifo);
			if (fifo_len >= 6) {
				gxfifo_peek(fifo, data, 6,0);
				DMX_DBG("first: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
						data[0], data[1], data[2], data[3], data[4], data[5]);
				section_size = ((data[4]) << 8) + (data[5]) +6;
				fifo_len = gxfifo_len(fifo);
				if (fifo_len >= 6+section_size) {
					gxfifo_peek(fifo, data_next, 6,section_size);
					DMX_DBG("next: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
							data_next[0], data_next[1], data_next[2], data_next[3], data_next[4], data_next[5]);

					if(data[0] != 0x00 || data[1] != 0x00 || data[2] != 0x01 ||
							data_next[0] != 0x00 || data_next[1] != 0x00 || data_next[2] != 0x01) {
						while (section_size--) {
							gxfifo_get(fifo, data, 1);
							if (section_size >= 6) {
								gxfifo_peek(fifo, data, 6, 0);
								if(filter->key[0].mask != 0){
									if(data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01 
											&& data[3] == (filter->key[0].value & filter->key[0].mask)) {
										gx_printf("soft err correction!\n");
										return real_len;
									}
								} else {
									if(data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
										gx_printf("soft err correction!\n");
										return real_len;
									}
								}
							} else
								return real_len;
						}
					}
				}
				section_size = ((data[4]) << 8) + (data[5]) +6;
				DMX_DBG("section_len = %d fifo_len =%d real_len = %d!\n",section_size,fifo_len,real_len);
				if(section_size > fifo_len){
					return real_len;
				}
			}else {
				return real_len;
			}

			if(real_len + section_size > size)
				return real_len;

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
	else if (DEMUX_SLOT_MUXTS == filter->slot->type || DEMUX_SLOT_TS == filter->slot->type) {
		fifo_len = gxfifo_len(fifo);
		fifo_len = fifo_len/188*188;
		if (fifo_len < size)
			gxfifo_user_get(fifo, data_buf, fifo_len);
		else
			gxfifo_user_get(fifo, data_buf, size);

		return GX_MIN(fifo_len,size);
	}
	if (fifo_len <= 0){
		if (DEMUX_SLOT_PSI == filter->slot->type) {
			filter->event_cb(((filter->slot->demux->id == 0) ? EVENT_DEMUX0_FILTRATE_PSI_END : EVENT_DEMUX1_FILTRATE_PSI_END),0,filter->event_priv);
		} else if (filter->slot->type == DEMUX_SLOT_PES || filter->slot->type == DEMUX_SLOT_PES_AUDIO
		    || filter->slot->type == DEMUX_SLOT_PES_VIDEO) {
			filter->event_cb(((filter->slot->demux->id == 0) ? EVENT_DEMUX0_FILTRATE_PES_END : EVENT_DEMUX1_FILTRATE_PES_END),0,filter->event_priv);
		} else if (DEMUX_SLOT_MUXTS == filter->slot->type) {
			filter->event_cb(((filter->slot->demux->id == 0) ? EVENT_DEMUX0_FILTRATE_TS_END : EVENT_DEMUX1_FILTRATE_TS_END),0,filter->event_priv);
		}
	}

	return real_len;
}

unsigned int gx6605_dmx_query_fifo(long long *state)
{
	unsigned int len,i;
	long long id = 0,st=0;
	struct dmx_filter *filter = NULL;
	struct gxfifo *fifo = NULL;

	st = 0;
	for (i = 0; i < MAX_FILTER_NUM; i++) {
		id = (0x1ULL << i);
		filter = gx6605_dmx.filters[i];
		if(filter != NULL){
			fifo = &filter->section_fifo;
			if(fifo != NULL){
				len = gxfifo_len(fifo);
				if (len > 0)
					st |= id;
				else
					st &= ~id;
			}else{
				st &= ~id;
			}
		}else{
			st &= ~id;
		}

	}
	*state =st;

	return (*state);
}

