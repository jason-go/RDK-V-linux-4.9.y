#include "fifo.h"
#include "gxav_bitops.h"
#include "porting.h"
#include "sdc_hal.h"
#include "gx3211_demux.h"
#include "gx3211_hal_common.h"
#include "kernelcalls.h"
#include "profile.h"
#include "firewall.h"

int find_first_zero(long long mask)
{
	long long temp  = mask;
	int i;

	for (i = 0; i < 64; i++) {
		if (((temp >> i)&0x1) == 0)
			return i;
	}
	return -1;
}

static void* tsw_virtual_addr = NULL;
static int __filter_buffer_alloc(struct dmxdev *demux_device, struct dmx_filter *filter)
{
	switch (filter->slot_type)
	{
	case DEMUX_SLOT_PSI:
		if (filter->buffer_size <= BUFFER_PSI_SIZE) {
			filter->virtual_dfbuf_addr = kcache_alloc(demux_device->psi_buffer_cache, BUFFER_PSI_SIZE);
			filter->buffer_size = BUFFER_PSI_SIZE;
		}
		else
			filter->virtual_dfbuf_addr = gx_page_malloc(filter->buffer_size);
		break;
	case DEMUX_SLOT_PES:
	case DEMUX_SLOT_PES_AUDIO:
	case DEMUX_SLOT_PES_VIDEO:
		if (filter->buffer_size <= BUFFER_PES_SIZE) {
			filter->virtual_dfbuf_addr = kcache_alloc(demux_device->pes_buffer_cache, BUFFER_PES_SIZE);
			filter->buffer_size = BUFFER_PES_SIZE;
		}
		else
			filter->virtual_dfbuf_addr = gx_page_malloc(filter->buffer_size);
		break;
	case DEMUX_SLOT_TS:
	case DEMUX_SLOT_MUXTS:
		if (firewall_buffer_protected(GXAV_BUFFER_DEMUX_TSW) && tsw_virtual_addr != NULL)
			filter->virtual_dfbuf_addr = tsw_virtual_addr;
		else {
			if (firewall_access_align()) {
				filter->virtual_dfbuf_addr = gx_page_malloc(filter->buffer_size);
				filter->buffer_size = filter->buffer_size;

			} else {
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
			}
			tsw_virtual_addr = filter->virtual_dfbuf_addr;
		}
		break;
	default:
		break;
	}

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

	switch (filter->slot_type)
	{
	case DEMUX_SLOT_PSI:
		if (filter->buffer_size <= BUFFER_PSI_SIZE)
			kcache_free(dev->psi_buffer_cache, filter->virtual_dfbuf_addr);
		else
			gx_page_free(filter->virtual_dfbuf_addr, filter->buffer_size);
		break;
	case DEMUX_SLOT_PES:
	case DEMUX_SLOT_PES_AUDIO:
	case DEMUX_SLOT_PES_VIDEO:
		if (filter->buffer_size <= BUFFER_PES_SIZE)
			kcache_free(dev->pes_buffer_cache, filter->virtual_dfbuf_addr);
		else
			gx_page_free(filter->virtual_dfbuf_addr, filter->buffer_size);
		break;
	case DEMUX_SLOT_TS:
	case DEMUX_SLOT_MUXTS:
		if (firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES) == 0)
			gx_page_free(filter->virtual_dfbuf_addr, (filter->buffer_size+188*2));
			break;
		}
	default:
		break;
	}

	filter->virtual_dfbuf_addr = NULL;
}

void gx3211_demux_delay_free(struct dmxdev *dmxdev)
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
				gx3211_clear_df_rw(dmxdev->reg,id);

				__filter_buffer_free(dmxdev, filter);
				dmxdev->filters[id] = NULL;
				gx_free(filter);
				filter = NULL;
				CLEAR_VAL_BIT(dmxdev->filter_mask, id);
			}
		}
	}
}
int gx3211_dmx_avbuf_callback(unsigned int channel_id,unsigned int length, unsigned int underflow,void *arg)
{
	struct dmx_slot *slot = (struct dmx_slot *)arg;
	struct dmx_demux* demux = slot->demux;
	struct dmxdev* dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;

	int av_bufid = slot->avbuf_id;
	unsigned int av_read_addr = 0;
	unsigned int av_write_addr = 0;
	unsigned int sdc_write_addr = 0;
	unsigned int pts_write_addr = 0;
	struct demux_fifo *avfifo=NULL;

	avfifo = &dmxdev->avfifo[av_bufid];
	gxav_sdc_rwaddr_get(avfifo->channel_id, &av_read_addr,&sdc_write_addr);
	//gx_printf("update=%08x %08x\n",av_write_addr,av_read_addr);
	REG_SET_VAL(&(reg->av_addr[av_bufid].av_read_addr), av_read_addr);
	av_write_addr = REG_GET_VAL(&(reg->av_addr[av_bufid].av_write_addr));
	if(av_write_addr != sdc_write_addr)
		gxav_sdc_Wptr_Set(avfifo->channel_id, av_write_addr);

	pts_write_addr = REG_GET_VAL(&(reg->pts_w_addr[av_bufid]));
	gxav_sdc_Wptr_Set(dmxdev->avfifo[av_bufid].pts_channel_id, pts_write_addr);
	DMX_DBG("demux callback av_wptr 0x%x \n",av_write_addr);
	return 0;
}

void gx3211_dmx_initialization(struct dmxdev *demux_device)
{
	struct reg_demux *reg = demux_device->reg;
	struct dmx_demux *demux = NULL;

	gx_memset(demux_device->slots,       0, sizeof(struct dmx_slot *)   * MAX_SLOT_NUM);
	gx_memset(demux_device->muxslots,    0, sizeof(struct dmx_slot *)   * MAX_SLOT_NUM);
	gx_memset(demux_device->avslots,     0, sizeof(struct dmx_slot *)   * MAX_AVBUF_NUM);
	gx_memset(demux_device->avfifo,      0, sizeof(struct demux_fifo )  * MAX_AVBUF_NUM);
	gx_memset(demux_device->filters,     0, sizeof(struct dmx_filter *) * MAX_FILTER_NUM);
	gx_memset(demux_device->muxfilters,  0, sizeof(struct dmx_filter *) * MAX_FILTER_NUM);
	gx_memset(demux_device->demuxs,      0, sizeof(struct dmx_demux)    * MAX_DEMUX_CHANNEL);
	gx_memset(demux_device->ca,          0, sizeof(struct dmx_ca *)     * MAX_CA_NUM);

	demux_device->filter_mask      = 0x0ULL;
	demux_device->mux_filter_mask  = 0x0ULL;
	demux_device->slot_mask        = 0x0ULL;
	demux_device->mux_slot_mask    = 0x0ULL;
	demux_device->ca_mask          = 0;
	demux_device->avbuf_mask       = 0;

	demux = &(demux_device->demuxs[0]);
	demux->id = 0;
	demux->demux_device = demux_device;
	demux = &(demux_device->demuxs[1]);
	demux->id = 1;
	demux->demux_device = demux_device;
	gx3211_set_clock(reg);
	gx3211_clear_sram(reg);
	gx3211_set_start(reg);
}

void gx3211_dmx_cleanup(struct dmxdev *demux_device)
{
	int i;

	for (i = 0; i < MAX_SLOT_NUM; i++) {
		if (NULL != demux_device->slots[i]) {
			gx_free(demux_device->slots[i]);
		}
		if (NULL != demux_device->muxslots[i]) {
			gx_free(demux_device->muxslots[i]);
		}
	}

	for (i = 0; i < MAX_FILTER_NUM; i++) {
		if (NULL != demux_device->filters[i]) {
			gx_free(demux_device->filters[i]);
		}
		if (NULL != demux_device->muxfilters[i]) {
			gx_free(demux_device->muxfilters[i]);
		}
	}

	for (i = 0; i < MAX_CA_NUM; i++) {
		if (NULL != demux_device->ca[i]) {
			gx_free(demux_device->ca[i]);
		}
	}

	gx_memset(demux_device->slots,       0, sizeof(struct dmx_slot *)   * MAX_SLOT_NUM);
	gx_memset(demux_device->muxslots,    0, sizeof(struct dmx_slot *)   * MAX_SLOT_NUM);
	gx_memset(demux_device->avslots,     0, sizeof(struct dmx_slot *)   * MAX_AVBUF_NUM);
	gx_memset(demux_device->filters,     0, sizeof(struct dmx_filter *) * MAX_FILTER_NUM);
	gx_memset(demux_device->muxfilters,  0, sizeof(struct dmx_filter *) * MAX_FILTER_NUM);
	gx_memset(demux_device->demuxs,      0, sizeof(struct dmx_demux)    * MAX_DEMUX_CHANNEL);
	gx_memset(demux_device->ca,          0, sizeof(struct dmx_ca *)     * MAX_CA_NUM);

	demux_device->filter_mask      = 0x0ULL;
	demux_device->mux_filter_mask  = 0x0ULL;
	demux_device->slot_mask        = 0x0ULL;
	demux_device->mux_slot_mask    = 0x0ULL;
	demux_device->ca_mask          = 0;
	demux_device->avbuf_mask       = 0;
}

void gx3211_dmx_config(struct dmx_demux *dmx)
{
	gx3211_set_gate(dmx);
	gx3211_set_ts_source(dmx);
	gx3211_set_ts_mode(dmx);
}

int gx3211_dmx_link_fifo(struct dmx_demux *dmx, struct demux_fifo *fifo)
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
			gx3211_link_rts_sdc(dmx,start_addr, end_addr - start_addr + 1, buf_id);

			channel->indata = dmx;
			channel->incallback = gx3211_rts_cb;
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
			gx3211_link_wts_sdc(dmx,start_addr, end_addr - start_addr + 1, fifo->pin_id);

			channel->outdata = dmx;
			channel->outcallback = gx3211_wts_cb;
		}
		else if (fifo->pin_id < PIN_ID_SDRAM_OUTPUT) {
			struct dmx_slot *slot = NULL;
			struct gxav_channel *channel = (struct gxav_channel *)fifo->channel;
			slot = dmx->slots[fifo->pin_id];
			dmx->demux_device->avfifo[slot->avbuf_id] = *fifo;
			gx3211_link_avbuf(slot, start_addr, end_addr - start_addr + 1, buf_id);

			start_addr = fifo->pts_start_addr;
			end_addr   = fifo->pts_end_addr;
			buf_id     = fifo->pts_buffer_id;
			gx3211_link_ptsbuf(slot, start_addr, end_addr - start_addr + 1, buf_id);
			slot->dir = fifo->direction;
			gxav_sdc_buffer_reset(fifo->channel_id);
			gxav_sdc_buffer_reset(fifo->pts_channel_id);

			channel->outdata     = (void *)slot;
			channel->outcallback = gx3211_dmx_avbuf_callback;
		}
	}

	return 0;
}

int gx3211_dmx_unlink_fifo(struct dmx_demux *dmx, struct demux_fifo *fifo)
{
	if (GXAV_PIN_INPUT == fifo->direction) {
		struct gxav_channel *channel = (struct gxav_channel *)(fifo->channel);
		channel->incallback = NULL;
		channel->indata = NULL;

		if (fifo->pin_id == PIN_ID_SDRAM_INPUT) {
			DMX_DBG("unlink source from sdram.\n");
			gx3211_unlink_rts_sdc(dmx);
		}
	}
	else if (GXAV_PIN_OUTPUT == fifo->direction) {
		struct gxav_channel *channel = (struct gxav_channel *)(fifo->channel);
		channel->outcallback = NULL;
		channel->outdata = NULL;

		if (fifo->pin_id == PIN_ID_SDRAM_OUTPUT /* fifo->pin_id >= PIN_ID_SDRAM_OUTPUT0 && fifo->pin_id <= PIN_ID_SDRAM_OUTPUT15 */ ) {
			DMX_DBG("unlink source to sdram.\n");
			gx3211_unlink_wts_sdc(dmx,fifo->pin_id);
		} else if (fifo->pin_id < 64) {
			struct dmx_slot *slot = dmx->slots[fifo->pin_id];
			gx3211_unlink_avbuf(slot);
			gx_memset(&(dmx->demux_device->avfifo[slot->avbuf_id]), 0 ,sizeof(struct demux_fifo));
		}
	}

	return 0;
}

void gx3211_dmx_query_tslock(struct dmx_demux *dmx)
{
	gx3211_query_tslock(dmx);
}

int gx3211_dmx_alloc_slot(struct dmx_demux *demux, enum dmx_slot_type type, unsigned short pid)
{
	int i;
	struct dmxdev *dmxdev = demux->demux_device;
	struct dmx_slot* slot = NULL;
	struct dmx_slot* muxslot = NULL;

	if(DEMUX_SLOT_MUXTS == type){
		i = find_first_zero(dmxdev->mux_slot_mask);
		if (i < 0) {
			DMX_PRINTF("have no slot left!\n");
			return -1;
		}
		SET_VAL_BIT(dmxdev->mux_slot_mask, i);

		dmxdev->muxslots[i]= (struct dmx_slot *)gx_malloc(sizeof(struct dmx_slot));
		muxslot = dmxdev->muxslots[i];
		if (NULL == muxslot) {
			DMX_PRINTF("alloc NULL!\n");
			goto muxerr;
		}

		gx_memset(muxslot, 0, sizeof(struct dmx_slot));
		muxslot->demux = demux;
		muxslot->id = i;
		muxslot->type = type;
		muxslot->filter_num = 0;
		demux->muxslots[i] = muxslot;

		if(pid == 0xffff)
			gx3211_set_tsw(demux,i);

		return (muxslot->id + PIN_ID_SDRAM_OUTPUT);

muxerr:
		if(dmxdev->muxslots[i]) {
			gx_free(dmxdev->muxslots[i]);
			dmxdev->muxslots[i] = NULL;
			demux->muxslots[i] = NULL;
		}
		if(DEMUX_SLOT_MUXTS == type)
			CLEAR_VAL_BIT(dmxdev->mux_slot_mask, i);

		return -1;
	}

	for (i = 0; i < MAX_SLOT_NUM; i++) {
        slot = demux->slots[i];
		if (slot != NULL) {
			if ((slot->pid == pid) &&
				(slot->type == type) &&
				(pid != 0x1fff)) {
				slot->refcount++;
				return i;
			}
		}
	}

	if (firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES))

		SET_VAL_BIT(dmxdev->slot_mask, 60);
		SET_VAL_BIT(dmxdev->slot_mask, 61);
		SET_VAL_BIT(dmxdev->slot_mask, 62);
		SET_VAL_BIT(dmxdev->slot_mask, 63);
		if(type == DEMUX_SLOT_AUDIO){
			if(demux->slots[60] == NULL)
				i = 60;
			else if(demux->slots[61] == NULL)
				i = 61;
			else
				goto err;
		}else if(type == DEMUX_SLOT_VIDEO){
			if(demux->slots[62] == NULL)
				i = 62;
			else if(demux->slots[63] == NULL)
				i = 63;
			else
				goto err;
		}else{
			i = find_first_zero(dmxdev->slot_mask);
			if (i < 0) {
				DMX_PRINTF("have no slot left!\n");
				return -1;
			}
			SET_VAL_BIT(dmxdev->slot_mask, i);
		}
	}
	else {
		i = find_first_zero(dmxdev->slot_mask);
		if (i < 0) {
			DMX_PRINTF("have no slot left!\n");
			return -1;
		}
		SET_VAL_BIT(dmxdev->slot_mask, i);
	}

	dmxdev->slots[i] = (struct dmx_slot *)gx_malloc(sizeof(struct dmx_slot));
	slot = dmxdev->slots[i];
	if (NULL == slot) {
		DMX_PRINTF("alloc NULL!\n");
		goto err;
	}

	gx_memset(slot, 0, sizeof(struct dmx_slot));
	slot->demux = demux;
	slot->id = i;
	slot->pid = pid;
	slot->type = type;
	slot->filter_num = 0;
	slot->refcount = 0;
	demux->slots[i] = slot;

	if ((DEMUX_SLOT_AUDIO == slot->type)
			|| (DEMUX_SLOT_SPDIF == slot->type)
			|| (DEMUX_SLOT_VIDEO == slot->type)
			|| (DEMUX_SLOT_AUDIO_SPDIF == slot->type))
	{
		int avbuf_id = 0;
		avbuf_id = find_first_zero(dmxdev->avbuf_mask);
		if ((avbuf_id >= MAX_AVBUF_NUM) || (avbuf_id < 0)) {
			DMX_PRINTF("allocate avbuf num error!!!!\n");
			goto err;
		}
		if (firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES))
			avbuf_id = i-60;
		}
		SET_VAL_BIT(dmxdev->avbuf_mask, avbuf_id);
		slot->avbuf_id = avbuf_id;
		dmxdev->avslots[avbuf_id] = slot;
		DMX_DBG("dmxdev->slots[i]->avbuf_id = %d\n", slot->avbuf_id);
	}

	return slot->id;

err:
	if(dmxdev->slots[i]) {
		gx_free(dmxdev->slots[i]);
		dmxdev->slots[i] = NULL;
		demux->slots[i] = NULL;
	}
	CLEAR_VAL_BIT(dmxdev->slot_mask, i);

	return -1;
}

void gx3211_dmx_free_slot(struct dmx_slot *slot)
{
	int i;
	struct dmx_demux *demux = slot->demux;
	struct dmxdev *demux_device = demux->demux_device;
	int index = slot->id;

	if(DEMUX_SLOT_MUXTS == slot->type){
		struct dmx_filter *filter = slot->muxfilters;

		if(filter){
			gx3211_unlink_wts_sdc(demux,slot->id);
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
            demux->muxslots[index] = NULL;
		}
		return;

	}

	DMX_DBG("demux_device->slot_mask = 0x%llx\n", demux_device->slot_mask);

	gx3211_clr_slot_cfg(slot);
	gx3211_disable_slot(slot);
	CLEAR_VAL_BIT(demux_device->slot_mask, index);

	if (slot->type == DEMUX_SLOT_SPDIF) {
		gx3211_clear_spdif(slot);
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

	/*if the filter_num nequal 0,you need free filter */
	if (slot->filter_num > 0) {
		for (i = 0; i < MAX_FILTER_NUM; i++) {
			if (NULL != slot->filters[i]) {
				gx3211_dmx_free_filter(slot->filters[i]);
				if (slot->filter_num == 0) {
					break;
				}
			}
		}
	}

	if(demux_device->slots[index]) {
		gx_free(demux_device->slots[index]);
		demux_device->slots[index] = NULL;
        demux->slots[index] = NULL;
	}
}

int gx3211_dmx_alloc_filter(struct dmx_slot *slot)
{
	int i,ret;
	struct dmx_filter *filter = NULL;
	struct dmx_demux *demux = slot->demux;
	struct dmxdev *demux_device = demux->demux_device;

	DMX_DBG("slot->id = %d\n", slot->id);
	gx3211_demux_delay_free(demux_device);

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
		filter->slot_type = slot->type;
		filter->status = DMX_STATE_ALLOCATED;
		filter->buffer_size = slot->hw_buffer_size;
		if (__filter_buffer_alloc(demux_device, filter) != 0)
			goto err;

		ret = gxfifo_init(&filter->section_fifo, NULL, slot->sw_buffer_size);
		if (ret < 0) {
			DMX_PRINTF("filter allocate fifo failed!\n");
			goto err;
		}

		gx_memset(filter->virtual_dfbuf_addr, 0, filter->buffer_size);
		gx_dcache_clean_range(0,0);
		filter->phy_dfbuf_addr = gx_virt_to_phys((unsigned int)filter->virtual_dfbuf_addr);
		gx3211_link_wts_sdc(demux,filter->phy_dfbuf_addr, filter->buffer_size, slot->id);

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
	filter = demux_device->filters[i];
	gx_memset(&(filter->tv),0,sizeof(struct timeval));

	gx_memset(filter, 0, sizeof(struct dmx_filter));
	filter->slot = slot;
	filter->id = i;
	filter->status = DMX_STATE_ALLOCATED;
	filter->slot_type = slot->type;
	filter->refcount = 0;
	slot->filters[i] = filter;
	slot->filter_num++;

	filter->buffer_size = slot->hw_buffer_size;

	if (__filter_buffer_alloc(demux_device, filter) != 0)
		goto err;


	if ((DEMUX_SLOT_PSI == filter->slot->type) || (filter->slot->type == DEMUX_SLOT_PES)
			|| (filter->slot->type == DEMUX_SLOT_PES_AUDIO) || (filter->slot->type == DEMUX_SLOT_PES_VIDEO)) {
		int ret = gxfifo_init(&filter->section_fifo, NULL, slot->sw_buffer_size);
		if (ret < 0) {
			DMX_PRINTF("filter allocate fifo failed!\n");
			goto err;
		}
	}

	gx_memset(filter->virtual_dfbuf_addr, 0, filter->buffer_size);
	gx_dcache_clean_range(0,0);
	filter->phy_dfbuf_addr = gx_virt_to_phys((unsigned int)filter->virtual_dfbuf_addr);

	return filter->id;

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

void gx3211_dmx_free_filter(struct dmx_filter *filter)
{
	int filter_index = filter->id;
	struct dmx_slot *slot = filter->slot;
	struct dmxdev *demux_device = slot->demux->demux_device;
	struct dmx_demux *demux = slot->demux;
	struct timezone tz;

	if(filter->slot->type == DEMUX_SLOT_MUXTS){
        demux_device->muxfilters[(unsigned int)filter->id] = NULL;
		gx3211_unlink_wts_sdc(demux,slot->id);
		CLEAR_VAL_BIT(demux_device->mux_filter_mask, filter->id);
		__filter_buffer_free(demux_device, filter);
		gxfifo_free(&filter->section_fifo);

		if(demux_device->muxfilters[(unsigned int)filter->id]) {
			gx_free(demux_device->muxfilters[(unsigned int)filter->id]);
			//demux_device->muxfilters[(unsigned int)filter->id] = NULL;
		}

		slot->muxfilters = NULL;

		return;
	}

	gx3211_clear_int_df(filter);
	gx3211_disable_filter(filter);
	gx3211_disable_pid_sel(filter,slot);

	filter->status = DMX_STATE_DELAY_FREE;
	if ((DEMUX_SLOT_PSI == filter->slot->type) || (filter->slot->type == DEMUX_SLOT_PES)
			|| (filter->slot->type == DEMUX_SLOT_PES_AUDIO) || (filter->slot->type == DEMUX_SLOT_PES_VIDEO))
		gxfifo_free(&filter->section_fifo);


	slot->filter_num--;
	slot->filters[filter_index] = NULL;
	filter->slot = NULL;
	gx_gettimeofday(&(filter->tv),&tz);
	gx3211_demux_delay_free(demux_device);
	DMX_DBG("demux_device->filter_mask = 0x%llx\n", demux_device->filter_mask);
}

void gx3211_dmx_slot_config(struct dmx_slot *slot)
{
	if ((DEMUX_SLOT_PSI == slot->type) || (slot->type == DEMUX_SLOT_PES) || (slot->type == DEMUX_SLOT_PES_AUDIO)
			|| (slot->type == DEMUX_SLOT_PES_VIDEO) || (DEMUX_SLOT_TS == slot->type)) {
		gx3211_set_stop_unit(slot);
		gx3211_set_interrupt_unit(slot);
		gx3211_set_filter_type(slot);
		gx3211_set_crc_disable(slot);
	}else if ((DEMUX_SLOT_AUDIO == slot->type) || (DEMUX_SLOT_SPDIF == slot->type)
			|| (DEMUX_SLOT_VIDEO == slot->type) || (DEMUX_SLOT_AUDIO_SPDIF == slot->type)){
		if (DEMUX_SLOT_SPDIF == slot->type) {
			gx3211_set_audio_ac3(slot);
		}
		gx3211_set_av_type(slot);
		gx3211_set_pts_bypass(slot);
		gx3211_set_avnum(slot);

	}
	gx3211_reset_cc(slot);
	gx3211_set_pts_to_sdram(slot);
	gx3211_set_type_sel(slot);
	gx3211_set_pid(slot);
	gx3211_set_av_out(slot);
	gx3211_set_ts_out(slot);
	gx3211_set_err_discard(slot);
	gx3211_set_dup_discard(slot);
}

void gx3211_dmx_filter_config(struct dmx_filter *filter)
{
	gx3211_set_dfbuf(filter);
	gx3211_set_select(filter, filter->slot);
	gx3211_set_filtrate(filter);
	gx3211_set_filter_reset(filter);
}

void gx3211_dmx_enable_slot(struct dmx_slot *slot)
{
	gx3211_enable_slot(slot);
}

void gx3211_dmx_disable_slot(struct dmx_slot *slot)
{
	gx3211_disable_slot(slot);
}

void gx3211_dmx_enable_filter(struct dmx_filter *filter)
{
	struct dmx_slot*  slot = filter->slot;
	struct dmx_demux* demux = slot->demux;
	struct dmxdev*  dmxdev = demux->demux_device;
	struct reg_demux *reg = dmxdev->reg;
	filter->pread_rd = 0;
	gx3211_clear_df_rw(reg,filter->id);
	gx3211_enable_filter(filter);
}

void gx3211_dmx_disable_filter(struct dmx_filter *filter)
{
	gx3211_disable_filter(filter);
}

int gx3211_dmx_alloc_ca(struct dmxdev *demux_device,int demux_id)
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
	demux_device->ca[i]->demux= demux;
	demux_device->ca[i]->id = i;
	demux->ca[i] = demux_device->ca[i];
	return demux_device->ca[i]->id;
}

void gx3211_dmx_ca_config(struct dmx_slot *slot, struct dmx_ca *ca)
{
	gx3211_set_descramble(ca, slot);
}

void gx3211_dmx_free_ca(struct dmx_ca *ca)
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

void gx3211_dmx_pcr_sync(struct dmx_demux *demux, unsigned short pcr_pid)
{
	demux->pcr_pid = pcr_pid;
	gx3211_set_pcr_sync(demux);
}

unsigned char tmp[4099];
unsigned int gx3211_dmx_filter_read(struct dmx_filter *filter, unsigned char *data_buf, unsigned int size)
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
					if(section_size >4096 ){
						gx3211_disable_filter(filter);
						gx3211_set_filter_reset(filter);
						gx3211_enable_filter(filter);
						DMX_PRINTF("filter id %d section len %d fifo_len %d real_len %d\n", \
								filter->id, section_size,fifo_len,real_len);
						return real_len;
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
							//gx_printf("data[%d] = %x key[%d] = %x\n",i,data[i],i,filter->key[i].value);
							gxfifo_get(fifo, tmp, section_size+1);
							goto discard;
						}

					}
				}
			}
			real_len += section_size ;
			if(real_len > size){
				DMX_DBG("real len %d> size %d\n",real_len,size);
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
				;//DMX_PRINTF("!!!!!!tid=%d len = %d key=%d mask=%x!!!!!crc err!\n",data[0],section_size,filter->key[0].value,filter->key[0].mask);
			else if(crc_en_flag == 0)
				real_len += section_size ;
			else
				;//DMX_PRINTF("section len %d real_len%d crc%d tid = %d\n",section_size,real_len,crc,data[0]);
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
					DMX_DBG("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
							data[0],data[1],data[2],data[3],data_next[0],data_next[1],data_next[2],data_next[3]);
					if(data[0] != 0x00 || data[1] != 0x00 || data[2] != 0x01
							||data_next[0] != 0x00 || data_next[1] != 0x00 || data_next[2] != 0x01) {

						while(section_size--){
							gxfifo_get(fifo, data, 1);
							if(section_size >= 6){
								gxfifo_peek(fifo, data, 6,0);
								if(filter->key[0].mask != 0){
									if(data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01
											&& data[3] == (filter->key[0].value & filter->key[0].mask)) {
										gx_printf("soft err correction!\n");
										return real_len;
									}
								}else{
									if(data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
										gx_printf("soft err correction!\n");
										return real_len;
									}
								}
							}else
								return real_len;
						}
					}
				}
				section_size = ((data[4]) << 8) + (data[5]) +6;
				//gx_printf("section_len = %d fifo_len =%d real_len = %d!\n",section_size,fifo_len,real_len);
				if(section_size > fifo_len){
					return real_len;
				}
			}else {
				return real_len;
			}

			if(real_len + section_size > size)
			{
				//gx_printf("------------[%s]%d real_len = %d, section_size = %d -----------\n", __func__, __LINE__, real_len, section_size);
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
	else if (DEMUX_SLOT_MUXTS == filter->slot->type) {
		unsigned int align = 188;
		fifo_len = gxfifo_len(fifo);
		fifo_len = fifo_len/align*align;
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

unsigned int gx3211_dmx_query_fifo(struct dmx_demux *dmx,long long *state)
{
    struct dmxdev *dmxdev = dmx->demux_device;
	unsigned int len,i;
	long long id = 0,st=0;
	struct dmx_filter *filter = NULL;
	struct gxfifo *fifo = NULL;

	st = 0;
	for (i = 0; i < MAX_FILTER_NUM; i++) {
		id = (0x1ULL << i);
		filter = dmxdev->filters[i];
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

