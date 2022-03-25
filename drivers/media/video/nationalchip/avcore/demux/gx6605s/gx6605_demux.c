
/******************************************************************************

  Copyright (C), 2008-2018, Nationalchip Tech. Co., Ltd.

 ******************************************************************************
 File Name     : gx6605_demux.c
Version       : Initial Draft
Author        : Nationalchip multimedia software group
Created       : 2008/12/18
Last Modified :
Description   : gx6605 chip demux module outer interface
Function List :
History       :
1.Date        : 2008/12/18
Author      : zz
Modification: Created file

 ******************************************************************************/
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "fifo.h"
#include "stc_hal.h"
#include "gx6605_demux.h"
#include "gx6605_demux_regs.h"
#include "gx6605_hal_common.h"
#include "porting.h"
#include "profile.h"
#include "firewall.h"

struct dmxdev gx6605_dmx;
struct reg_demux *gx6605_demux_reg = NULL;

#define DEMUX_THREAD_STACK (16 * 1024)
static unsigned char *thread_stack;
static gx_mutex_t demux_mutex;

static int gx6605_demux_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int ret = 0;
	if(device == NULL || inode == NULL)
		return -1;

	if (!gx_request_mem_region(GXAV_DEMUX_BASE_SYS, sizeof(struct reg_demux))) {
		DMX_PRINTF("request DEMUX mem region failed!\n");
		return -1;
	}

	gx6605_demux_reg = gx_ioremap(GXAV_DEMUX_BASE_SYS, sizeof(struct reg_demux));
	if (!gx6605_demux_reg) {
		DMX_PRINTF("ioremap DEMUX space failed!\n");
		return -1;
	}

	DMX_DBG("gx6605_demux_reg = 0x%p\n",gx6605_demux_reg);

	gx6605_dmx_initialization(&gx6605_dmx);
	REG_CLR_BIT(&gx6605_demux_reg->pts_mode, 0);
	REG_SET_VAL(&gx6605_demux_reg->df_buf_full_stop_th, 0);
	REG_SET_VAL(&gx6605_demux_reg->av_buf_full_stop_th, 0);

	ret = gx_sem_create(&(gx6605_dmx.sem_id), 0);
	if (ret < 0) {
		DMX_PRINTF("demux semaphore created fail!\n");
		return -1;
	}

	thread_stack = gx_malloc(DEMUX_THREAD_STACK);
	if (thread_stack == NULL) {
		DMX_PRINTF("demux thread stack memory malloc fail!\n");
		thread_stack = NULL;
		return -1;
	}
	ret = gx_thread_create("gx6605_demux_thread", &(gx6605_dmx.thread_id), gx6605_demux_read_thread,
			NULL, thread_stack, DEMUX_THREAD_STACK, 9, &(gx6605_dmx.thread_info));
	if (ret < 0) {
		DMX_PRINTF("demux thread created fail!\n");
		return -1;
	}

	gx_spin_lock_init(&(gx6605_dmx.demux_spin_lock));
	gx_mutex_init(&(demux_mutex));
	return 0;
}

static int gx6605_demux_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int i, j;
	struct dmx_demux *demux = NULL;

	if(device == NULL || inode == NULL)
		return -1;

	for (j = 0; j < MAX_DEMUX_CHANNEL; j++) {
		demux = &gx6605_dmx.demuxs[j];

		for (i = 0; i < MAX_SLOT_NUM; i++) {
			if (NULL != demux->slots[i]) {
				if (NULL != demux->slots[i]->filters[i]) {
					gx_free(demux->slots[i]->filters[i]);
				}
				gx_free(demux->slots[i]);
			}
		}
	}

	if (0 != gx6605_dmx.ca_mask) {
		for (i = 0; i < MAX_CA_NUM; i++) {
			if (NULL != gx6605_dmx.ca[i]) {
				gx6605_dmx_free_ca(gx6605_dmx.ca[i]);
			}
		}
	}

	gx_iounmap(gx6605_demux_reg);
	gx6605_demux_reg = NULL;
	gx_release_mem_region(GXAV_DEMUX_BASE_SYS, sizeof(struct reg_demux));

	gx_sem_delete(&(gx6605_dmx.sem_id));
	gx_memset(&(gx6605_dmx.sem_id),0,sizeof(gx_sem_id));

	gx_thread_delete(gx6605_dmx.thread_id);
	if(thread_stack !=NULL){
		gx_free(thread_stack);
		thread_stack = NULL;
	}
	gx6605_dmx_cleanup(&gx6605_dmx);
	gx_mutex_destroy(&(demux_mutex));

	return 0;
}

static int gx6605_demux_open(struct gxav_module *module)
{
	struct dmx_demux *demux = NULL;

	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count))
		return -1;

	demux = &gx6605_dmx.demuxs[module->sub];
	demux->demux_device = &gx6605_dmx;
	demux->id = module->sub;

	return 0;
}

// clear all int status
static void gx6605_demux_clr_ints(void)
{
	int i;
	for(i=gx6605_demux_reg->int_ts_if; i<= gx6605_demux_reg->int_dmx_cc_en_l; i+=4){
		*(volatile int*)i = 0xffffffff;
	}
}

static int gx6605_demux_close(struct gxav_module *module)
{
	int i;
	struct dmx_demux *demux = NULL;

	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count))
		return -1;

	demux = &gx6605_dmx.demuxs[module->sub];
	if (0 != demux->slot_num) {
		for (i = 0; i < MAX_SLOT_NUM; i++) {
			if (NULL != demux->slots[i]) {
				gx6605_dmx_free_slot(demux->slots[i]);
				if (0 == demux->slot_num) {
					DMX_DBG("slot free over!\n");
					break;
				}
			}
		}
	}
	if (0 != demux->muxslot_num) {
		for (i = 0; i < MAX_SLOT_NUM; i++) {
			if (NULL != demux->muxslots[i]) {
				gx6605_dmx_free_slot(demux->muxslots[i]);
				if (0 == demux->muxslot_num) {
					DMX_DBG("slot free over!\n");
					break;
				}
			}
		}
	}
	for (i = 0; i < MAX_CA_NUM; i++) {
		if (NULL != demux->ca[i]) {
			gx6605_dmx_free_ca(demux->ca[i]);
		}
	}
	gx6605_demux_clr_ints();

	return 0;
}

static int gx6605_demux_config(int sub,GxDemuxProperty_ConfigDemux *config_dmx)
{
	struct dmx_demux *demux = NULL;

	if (NULL == config_dmx)
		return -1;

	if ((config_dmx->sync_lock_gate > 0xf) || (config_dmx->sync_loss_gate > 0xf)
			|| (config_dmx->time_gate > 0xf) || (config_dmx->byt_cnt_err_gate > 0xf)) {
		DMX_PRINTF("ConfigDmx: the demux module's configure gate param error!\n");
		return -1;
	}

	if ((config_dmx->stream_mode < DEMUX_PARALLEL) || (config_dmx->stream_mode > DEMUX_SERIAL)) {
		DMX_PRINTF("ConfigDmx: the demux module's configure stream_mode param error!\n");
		return -1;
	}

	if ((config_dmx->ts_select < FRONTEND) || (config_dmx->ts_select > OTHER)) {
		DMX_PRINTF("ConfigDmx: the demux module's configure ts_select param error!\n");
		return -1;
	}

	if ((config_dmx->source < DEMUX_TS1) || (config_dmx->source > DEMUX_SDRAM)) {
		DMX_PRINTF("ConfigDmx: the demux module's configure source param error!\n");
		return -1;
	}

	demux = &gx6605_dmx.demuxs[sub];
	demux->demux_device = &gx6605_dmx;
	demux->id = sub;
	demux->sync_lock_gate = config_dmx->sync_lock_gate;
	demux->sync_loss_gate = config_dmx->sync_loss_gate;
	demux->time_gate = config_dmx->time_gate;
	demux->byt_cnt_err_gate = config_dmx->byt_cnt_err_gate;

	demux->stream_mode = config_dmx->stream_mode;
	demux->ts_select = config_dmx->ts_select;
	demux->source = config_dmx->source;

	gx6605_dmx_config(demux);

	return 0;
}

static int gx6605_demux_lock(int sub,GxDemuxProperty_TSLockQuery *query_tslock)
{
	struct dmx_demux *demux = NULL;

	if (NULL == query_tslock)
		return -1;

	demux = &(gx6605_dmx.demuxs[sub]);
	gx6605_dmx_query_tslock(demux);
	query_tslock->ts_lock = demux->ts_lock;

	return 0;
}

int gx6605_dmx_query_dmx_slot_fifo(unsigned int buf_id, int slot_type, unsigned int *dmx_id, struct dmx_slot *slot, struct demux_fifo *fifo)
{
	unsigned int i = 0;

	for(i=0; i<MAX_AVBUF_NUM; i++) {
		if(gx6605_dmx.avfifo[i].channel_id == buf_id && gx6605_dmx.avfifo[i].channel != NULL) {
			gx_memcpy(fifo, &gx6605_dmx.avfifo[i], sizeof(struct demux_fifo));
			break;
		}
	}

	if(i >= MAX_AVBUF_NUM || fifo->pin_id >= MAX_SLOT_NUM)
		return -1;

	if(gx6605_dmx.slots[fifo->pin_id] && gx6605_dmx.slots[fifo->pin_id]->type == slot_type)
		*dmx_id = gx6605_dmx.slots[fifo->pin_id]->demux->id;
	else
		return -1;

	*slot = *gx6605_dmx.slots[fifo->pin_id];

	return 0;
}

int gx6605_demux_link_fifo(int sub, struct demux_fifo *fifo)
{
	struct dmx_demux *demux = NULL;

	demux = &gx6605_dmx.demuxs[sub];
	GXAV_ASSERT(demux != NULL);
	GXAV_ASSERT(fifo != NULL);

	if ((fifo->pin_id > 64*3) || (fifo->pin_id < 0)) {
		DMX_PRINTF("property error!\n");
		return -1;
	}

	if (fifo->pin_id < PIN_ID_SDRAM_OUTPUT) {
		if(sub != 2){
			if (!(gx6605_dmx.slots[fifo->pin_id])
					|| (gx6605_dmx.slots[fifo->pin_id]->type < DEMUX_SLOT_AUDIO)) {
				DMX_PRINTF("pin_id error!\n");
				return -1;
			}
		}
	}

	return gx6605_dmx_link_fifo(demux, fifo);
}

int gx6605_demux_unlink_fifo(int sub, struct demux_fifo *fifo)
{
	struct dmx_demux *demux = NULL;

	demux = &gx6605_dmx.demuxs[sub];
	GXAV_ASSERT(demux != NULL);
	GXAV_ASSERT(fifo != NULL);

	if ((fifo->pin_id >= 64*3) || (fifo->pin_id < 0)) {
		DMX_PRINTF("property error!\n");
		return -1;
	}

	if (fifo->pin_id < PIN_ID_SDRAM_OUTPUT) {
		if(sub != 2){
			if (!(gx6605_dmx.slots[fifo->pin_id])
					|| (gx6605_dmx.slots[fifo->pin_id]->type < DEMUX_SLOT_AUDIO)) {
				DMX_PRINTF("pin_id error!\n");
				return -1;
			}
		}
	}

	return gx6605_dmx_unlink_fifo(demux, fifo);
}

static int gx6605_demux_pcr(int sub,GxDemuxProperty_Pcr *pcr)
{
	if (NULL == pcr)
		return -1;

	gx6605_dmx_pcr_sync(&gx6605_dmx, sub, pcr->pcr_pid);
	return 0;
}

static int gx6605_demux_stc(int sub,GxDemuxProperty_ReadStc *stc)
{
#ifndef NO_OS
	if (NULL == stc)
		return -1;

	gxav_stc_read_stc(0, &stc->stc_value);
#endif
	return 0;
}

static int gx6605_demux_slot_allocate(int sub,GxDemuxProperty_Slot *alloc_slot)
{
	enum dmx_slot_type slot_type;
	unsigned short pid;

	if (NULL == alloc_slot)
		return -1;

	if ((alloc_slot->type < DEMUX_SLOT_PSI) || (alloc_slot->type > DEMUX_SLOT_VIDEO)) {
		DMX_PRINTF("gx6605 Allocate: the slot's type error!\n");
		return -1;
	}

	slot_type = alloc_slot->type;
	pid = alloc_slot->pid;
	alloc_slot->slot_id = gx6605_dmx_alloc_slot(&gx6605_dmx, sub, slot_type,pid);
	DMX_DBG("alloc_slot->slot_id = %d\n",  alloc_slot->slot_id,pid);
	if (alloc_slot->slot_id < 0) {
		DMX_PRINTF("allocate slot failed!!!\n");
		return -1;
	}

	return 0;
}

static int gx6605_demux_slot_free(int sub,GxDemuxProperty_Slot *free_slot)
{
	struct dmx_slot *slot = NULL;

	if (NULL == free_slot)
		return -1;

	//if ((free_slot->slot_id >= MAX_SLOT_NUM) || (free_slot->slot_id < 0)) {
	//	DMX_PRINTF("ConfigSlot:slot's configure slot slot_id error!\n");
	//	return -1;
	//}

	if(DEMUX_SLOT_MUXTS == free_slot->type){
		slot = gx6605_dmx.muxslots[(free_slot->slot_id - PIN_ID_SDRAM_OUTPUT)];
	}else{
		if(sub == 2)
			slot = gx6605_dmx.pidslots[free_slot->slot_id];
		else
			slot = gx6605_dmx.slots[free_slot->slot_id];
	}

	if (NULL == slot) {
		DMX_PRINTF("the demux slot's free slot param NULL!\n");
		return -1;
	}
	if(slot->refcount != 0){
		slot->refcount--;
		return 0;
	}

	DMX_DBG("slot->id = %d\n", slot->id);
	gx6605_dmx_free_slot(slot);

	return 0;
}

static int gx6605_demux_slot_config(int sub,GxDemuxProperty_Slot *config_slot)
{
	struct dmx_slot *slot = NULL;

	if (NULL == config_slot)
		return -1;

	if ((config_slot->slot_id >= MAX_SLOT_NUM) || (config_slot->slot_id < 0)) {
		gx_printf("ConfigSlot: slot id error!\n");
		return -1;
	}

	if ((config_slot->type < DEMUX_SLOT_PSI) || (config_slot->type > DEMUX_SLOT_VIDEO)) {
		gx_printf("ConfigSlot: slot type error!\n");
		return -1;
	}

	if (config_slot->pid > 0x1FFF) {
		gx_printf("ConfigSlot: slot pid > 0x1fff!!\n");
		return -1;
	}

	if(sub == 2)
		slot = gx6605_dmx.pidslots[config_slot->slot_id];
	else
		slot = gx6605_dmx.slots[config_slot->slot_id];

	if (NULL == slot) {
		gx_printf("ConfigSlot: slot NULL!\n");
		return -1;
	}

	if (slot->type != config_slot->type) {
		gx_printf("ConfigSlot: the config slot type != alloc slot type!!!\n");
		return -1;
	}

	slot->flags = config_slot->flags;
	slot->pid = config_slot->pid;
	slot->tsw = (config_slot->ts_out_pin - PIN_ID_SDRAM_OUTPUT);
	DMX_DBG("slot->type = %d  slot->flags = 0x%x  slot->pid = 0x%x\n",
			slot->type, slot->flags, slot->pid);
	if(config_slot->type == DEMUX_SLOT_TS){
		if(0 /*slot->tsw< PIN_ID_SDRAM_OUTPUT0 || slot->tsw>PIN_ID_SDRAM_OUTPUT15*/){
			DMX_DBG("tsw pin err\n");
			return -1;
		}
	}
	gx6605_dmx_slot_config(slot);

	return 0;
}

static int gx6605_demux_slot_enable(int sub,GxDemuxProperty_Slot *enable_slot)
{
	struct dmx_slot *slot = NULL;

	if (NULL == enable_slot)
		return -1;

	if ((enable_slot->slot_id >= MAX_SLOT_NUM) || (enable_slot->slot_id < 0)) {
		DMX_PRINTF("slot's configure slot slot_id error!\n");
		return -1;
	}

	if(sub == 2)
		slot = gx6605_dmx.pidslots[enable_slot->slot_id];
	else
		slot = gx6605_dmx.slots[enable_slot->slot_id];

	if (NULL == slot) {
		DMX_PRINTF("the demux slot's configure slot NULL!\n");
		return -1;
	}

	gx6605_dmx_enable_slot(slot);
	return 0;
}

static int gx6605_demux_slot_query(int sub,GxDemuxProperty_SlotQueryByPid *query_slot)
{
	int i;

	if (NULL == query_slot)
		return -1;

	for (i = 0; i < MAX_SLOT_NUM; i++)
	{
		if (NULL != gx6605_dmx.demuxs[sub].slots[i])
		{
			//挂载相同demux下的slot,因为不同的demux可以有相同的pid的不同slot。
			if (query_slot->pid == gx6605_dmx.demuxs[sub].slots[i]->pid
					&& sub == gx6605_dmx.demuxs[sub].slots[i]->demux->id)
			{
				query_slot->slot_id = gx6605_dmx.demuxs[sub].slots[i]->id;
				break;
			}
		}
	}

	if (i == MAX_SLOT_NUM) {
		query_slot->slot_id = -1;
		DMX_PRINTF("It haven't find the slot_id!!!\n");
		return -1;
	}

	return 0;
}

static int gx6605_demux_slot_disable(int sub,GxDemuxProperty_Slot *disable_slot)
{
	struct dmx_slot *slot = NULL;

	if (NULL == disable_slot)
		return -1;

	if ((disable_slot->slot_id >= MAX_SLOT_NUM) || (disable_slot->slot_id < 0)) {
		gx_printf("SlotDisable: slot id error!\n");
		return -1;
	}
	if(sub == 2)
		slot = gx6605_dmx.pidslots[disable_slot->slot_id];
	else
		slot = gx6605_dmx.slots[disable_slot->slot_id];

	if (NULL == slot) {
		gx_printf("SlotDisable: slot NULL!\n");
		return -1;
	}

	if(slot->filter_num < 1)
		gx6605_dmx_disable_slot(slot);

	return 0;
}

static int gx6605_demux_slot_attribute(int sub,GxDemuxProperty_Slot *config_slot)
{
	struct dmx_slot *slot = NULL;

	if (NULL == config_slot)
		return -1;

	if ((config_slot->slot_id >= MAX_SLOT_NUM) || (config_slot->slot_id < 0)) {
		gx_printf("SlotConfig: slot id error!\n");
		return -1;
	}

	if(sub == 2)
		slot = gx6605_dmx.pidslots[config_slot->slot_id];
	else
		slot = gx6605_dmx.slots[config_slot->slot_id];

	if (NULL == slot) {
		gx_printf("SlotConfig: slot is NULL!\n");
		return -1;
	}

	config_slot->type = slot->type;
	config_slot->flags = slot->flags;
	config_slot->pid = slot->pid;
	DMX_DBG("slot->type = %d  slot->flags = 0x%x  slot->pid = 0x%x\n",
			slot->type, slot->flags, slot->pid);

	return 0;
}

static int gx6605_demux_filter_allocate(int sub,GxDemuxProperty_Filter *alloc_filter)
{
	struct dmx_slot *slot = NULL;

	if (NULL == alloc_filter)
		return -1;

	//if ((alloc_filter->slot_id >= MAX_SLOT_NUM) || (alloc_filter->slot_id < 0)) {
	//	DMX_PRINTF("FilterAlloc: filter slot_id error!\n");
	//	return -1;
	//}

	if(alloc_filter->slot_id >= PIN_ID_SDRAM_OUTPUT)
		slot = gx6605_dmx.muxslots[(alloc_filter->slot_id - PIN_ID_SDRAM_OUTPUT)];
	else
		slot = gx6605_dmx.slots[alloc_filter->slot_id];

	if ((NULL == slot) || (NULL == slot->demux) || (NULL == slot->demux->demux_device)) {
		DMX_PRINTF("FilterAlloc: the demux allocate filter NULL!\n ");
		return -1;
	}

	if ((slot->type >= DEMUX_SLOT_AUDIO) && (slot->type <= DEMUX_SLOT_VIDEO)) {
		DMX_PRINTF("FilterAlloc: filter can not be related to AV slot!\n");
		return -1;
	}

	if (0 == alloc_filter->hw_buffer_size) {
		if (DEMUX_SLOT_PSI == slot->type) {
			slot->hw_buffer_size = BUFFER_PSI_SIZE;
		} else if (slot->type == DEMUX_SLOT_PES || slot->type == DEMUX_SLOT_PES_AUDIO || slot->type == DEMUX_SLOT_PES_VIDEO) {
			slot->hw_buffer_size = BUFFER_PES_SIZE;
		} else if (DEMUX_SLOT_MUXTS == slot->type || DEMUX_SLOT_TS == slot->type) {
			slot->hw_buffer_size = BUFFER_TS_SIZE;
		}
	} else {
		slot->hw_buffer_size = alloc_filter->hw_buffer_size;
	}

	if (0 == alloc_filter->almost_full_gate) {
		if (slot->type == DEMUX_SLOT_PES || slot->type == DEMUX_SLOT_PES_AUDIO || slot->type == DEMUX_SLOT_PES_VIDEO) {
			slot->almost_full_gate = (BUFFER_PES_SIZE)>>8;
		} else if (DEMUX_SLOT_MUXTS == slot->type || DEMUX_SLOT_TS == slot->type) {
			slot->almost_full_gate = BUFFER_TS_GATE;
		}
	} else {
		slot->almost_full_gate = alloc_filter->almost_full_gate;//TS类型gate不能是size的整除倍数，因为硬件的满空判断有问题
	}

	if (0 == alloc_filter->sw_buffer_size) {
		if (DEMUX_SLOT_PSI == slot->type) {
			slot->sw_buffer_size = FIFO_PSI_SIZE;
		} else if (slot->type == DEMUX_SLOT_PES || slot->type == DEMUX_SLOT_PES_AUDIO || slot->type == DEMUX_SLOT_PES_VIDEO) {
			slot->sw_buffer_size = FIFO_PES_SIZE;
		} else if (DEMUX_SLOT_MUXTS == slot->type || DEMUX_SLOT_TS == slot->type) {
			slot->sw_buffer_size = BUFFER_TS_SIZE;
		}
	} else {
		slot->sw_buffer_size = alloc_filter->sw_buffer_size;
	}

	DMX_DBG("buffersize 0x%x buffergate 0x%x fifosize 0x%x\n",
		slot->hw_buffer_size,slot->almost_full_gate,slot->sw_buffer_size);
	alloc_filter->filter_id = gx6605_dmx_alloc_filter(slot);
	if (alloc_filter->filter_id < 0) {
		DMX_PRINTF("allocate filter failed!!!\n");
		return -1;
	}
	return 0;
}

static int gx6605_demux_filter_free(int sub,GxDemuxProperty_Filter *free_filter)
{
	struct dmx_filter *filter = NULL;

	if (NULL == free_filter)
		return -1;

	//if ((free_filter->filter_id >= MAX_FILTER_NUM) || (free_filter->filter_id < 0)) {
	//	DMX_PRINTF("FilterFree: filter filter_id error!\n");
	//	return -1;
	//}

	if(free_filter->filter_id >= PIN_ID_SDRAM_OUTPUT)
		filter = gx6605_dmx.muxfilters[(free_filter->filter_id - PIN_ID_SDRAM_OUTPUT)];
	else
		filter = gx6605_dmx.filters[free_filter->filter_id];

	if (NULL == filter || filter->slot == NULL) {
		DMX_PRINTF("FilterFree: filter is NULL\n");
		return -1;
	}
	gx6605_dmx_free_filter(filter);

	return 0;
}

static int gx6605_demux_filter_fifo_reset(int sub,GxDemuxProperty_FilterFifoReset *reset_filter_fifo)
{
	struct dmx_filter *filter = NULL;
	unsigned int mask = 0;

	if (NULL == reset_filter_fifo)
		return -1;

	//if ((reset_filter_fifo->filter_id >= MAX_FILTER_NUM) || (reset_filter_fifo->filter_id < 0)) {
	//	DMX_PRINTF("FilterFIFOReset: filter filter_id error!\n");
	//	return -1;
	//}

	if(reset_filter_fifo->filter_id >= PIN_ID_SDRAM_OUTPUT)
		filter = gx6605_dmx.muxfilters[(reset_filter_fifo->filter_id - PIN_ID_SDRAM_OUTPUT)];
	else
		filter = gx6605_dmx.filters[reset_filter_fifo->filter_id];

	if (NULL == filter) {
		DMX_PRINTF("FilterFIFOReset: filter is NULL\n");
		return -1;
	}

	if (DEMUX_SLOT_PSI == filter->slot->type) {
		if (filter->slot->demux->id == 0) {
			mask = EVENT_DEMUX0_FILTRATE_PSI_END;
		} else if (filter->slot->demux->id == 1) {
			mask = EVENT_DEMUX1_FILTRATE_PSI_END;
		}
	} else if (filter->slot->type == DEMUX_SLOT_PES || filter->slot->type == DEMUX_SLOT_PES_AUDIO
		    || filter->slot->type == DEMUX_SLOT_PES_VIDEO) {
		if (filter->slot->demux->id == 0) {
			mask = EVENT_DEMUX0_FILTRATE_PES_END;
		} else if (filter->slot->demux->id == 1) {
			mask = EVENT_DEMUX1_FILTRATE_PES_END;
		}
	} else if (DEMUX_SLOT_MUXTS == filter->slot->type) {
		if (filter->slot->demux->id == 0) {
			mask = EVENT_DEMUX0_FILTRATE_TS_END;
		} else if (filter->slot->demux->id == 1) {
			mask = EVENT_DEMUX1_FILTRATE_TS_END;
		}
	}
	if(filter->event_cb)
		filter->event_cb(mask,0,filter->event_priv);

	gx6605_set_filter_reset(filter);

	return 0;
}

static int gx6605_demux_filter_fifo_query(int sub,GxDemuxProperty_FilterFifoQuery *query_filter_fifo)
{
	if (NULL == query_filter_fifo)
		return -1;

	gx6605_dmx_query_fifo(&(query_filter_fifo->state));

	return 0;
}

static int gx6605_demux_filter_config(int sub,GxDemuxProperty_Filter *config_filter)
{
	int depth = 0,depth_tmp=0;
	struct dmx_filter *filter = NULL;

	if (NULL == config_filter)
		return -1;

	if ((config_filter->filter_id >= MAX_FILTER_NUM) || (config_filter->filter_id < 0)) {
		DMX_PRINTF("FilterConfig: filter filter_id error!\n");
		return -1;
	}

	if ((config_filter->depth < 0) || (config_filter->depth > MAX_FILTER_KEY_DEPTH)) {
		DMX_PRINTF("FilterConfig: filter depth  error!\n");
		return -1;
	}

	if ((config_filter->depth == 2) || (config_filter->depth == 3)) {
		DMX_PRINTF("FilterConfig: filter depth =2 or =3!\n");
		return -1;
	}

	//if (config_filter->flags > 0x3) {
	//	DMX_PRINTF("FilterConfig: filter flags > 0x3");
	//	return -1;
	//}

	filter = gx6605_dmx.filters[config_filter->filter_id];
	if (NULL == filter) {
		DMX_PRINTF("FilterConfig: filter is NULL\n");
		return -1;
	}
    gx6605_clear_int_df(config_filter->filter_id);

	filter->depth = config_filter->depth;
	filter->flags = config_filter->flags;
	/*if (filter->depth >= (MAX_FILTER_KEY_DEPTH - 3)) {
	  return -1;
	  } */

	depth_tmp = filter->depth;
	if (filter->depth == 1) {
		filter->depth = 0;
		depth = 3;
	} else {
		depth = filter->depth;
	}

	gx_memcpy(filter->key, config_filter->key, (depth) * sizeof(struct dmx_filter_key));
	gx6605_dmx_filter_config(filter);

	return 0;
}

static int gx6605_demux_filter_read(int sub,GxDemuxProperty_FilterRead *read_filter)
{
	struct dmx_filter *filter = NULL;

	if (NULL == read_filter)
		return -1;

	//if ((read_filter->filter_id >= MAX_FILTER_NUM) || (read_filter->filter_id < 0)) {
	//	DMX_PRINTF("FilterRead: filter filter_id error!\n");
	//	return -1;
	//}

	if (NULL == read_filter->buffer) {
		DMX_PRINTF("FilterRead: filter buffer == NULL!\n");
		return -1;
	}


	if(read_filter->filter_id >= PIN_ID_SDRAM_OUTPUT)
		filter = gx6605_dmx.muxfilters[(read_filter->filter_id - PIN_ID_SDRAM_OUTPUT)];
	else
		filter = gx6605_dmx.filters[read_filter->filter_id];

	if (NULL == filter) {
		DMX_PRINTF("FilterRead: the demux read filter NULL!\n");
		return -1;
	}
	read_filter->read_size = gx6605_dmx_filter_read(filter, (unsigned char *)read_filter->buffer,read_filter->max_size);

	return 0;
}

static int gx6605_demux_filter_enable(int sub,GxDemuxProperty_Filter *enable_filter)
{
	struct dmx_filter *filter = NULL;

	if (NULL == enable_filter)
		return -1;

	if ((enable_filter->filter_id >= MAX_FILTER_NUM) || (enable_filter->filter_id < 0)) {
		DMX_PRINTF("FilterEnable: filter filter_id error!\n");
		return -1;
	}

	filter = gx6605_dmx.filters[enable_filter->filter_id];
	if (NULL == filter) {
		DMX_PRINTF("FilterEnable: filter is NULL\n");
		return -1;
	}

	gx6605_dmx_enable_filter(filter);

	return 0;
}

static int gx6605_demux_filter_disable(int sub,GxDemuxProperty_Filter *disable_filter)
{
	struct dmx_filter *filter = NULL;

	if (NULL == disable_filter)
		return -1;

	if ((disable_filter->filter_id >= MAX_FILTER_NUM) || (disable_filter->filter_id < 0)) {
		DMX_PRINTF("FilterDisable: filter filter_id error!\n");
		return -1;
	}

	filter = gx6605_dmx.filters[disable_filter->filter_id];
	if (NULL == filter) {
		DMX_PRINTF("FilterDisable: filter is NULL\n");
		return -1;
	}

	gx6605_dmx_disable_filter(filter);

	return 0;
}

static int gx6605_demux_filter_attribute(int sub,GxDemuxProperty_Filter *config_filter)
{
	//int depth = 0;
	struct dmx_filter *filter = NULL;

	if (NULL == config_filter)
		return -1;

	if ((config_filter->filter_id >= MAX_FILTER_NUM) || (config_filter->filter_id < 0)) {
		DMX_PRINTF("FilterConfig: filter filter_id error!\n");
		return -1;
	}

	filter = gx6605_dmx.filters[config_filter->filter_id];
	if (NULL == filter) {
		DMX_PRINTF("FilterConfig: config filter is NULL\n");
		return -1;
	}

	config_filter->depth = filter->depth;
	config_filter->flags = filter->flags;
	config_filter->slot_id = filter->slot->id;

	if (filter->depth == 3) {
		config_filter->depth = 1;
	}
	/*else {
	  depth = filter->depth + 3;
	  } */
	gx_memcpy(config_filter->key, filter->key, (filter->depth) * sizeof(struct dmx_filter_key));

	return 0;
}

static int gx6605_demux_filter_eventcb(int sub,unsigned int filter_id,filter_event_cb callback,void *priv)
{
	struct dmx_filter *filter = NULL;

	if(filter_id >= PIN_ID_SDRAM_OUTPUT)
		filter = gx6605_dmx.muxfilters[(filter_id - PIN_ID_SDRAM_OUTPUT)];
	else
		filter = gx6605_dmx.filters[filter_id];

	if (NULL == filter) {
		DMX_PRINTF("Filter: filter is NULL\n");
		return -1;
	}

	filter->event_cb    = callback;
	filter->event_priv  = priv;
	return 0;
}

static int gx6605_demux_ca_allocate(int sub,GxDemuxProperty_CA *alloc_ca)
{
	if (NULL == alloc_ca)
		return -1;

	alloc_ca->ca_id = gx6605_dmx_alloc_ca(&gx6605_dmx,sub);
	if (alloc_ca->ca_id < 0) {
		DMX_PRINTF("allocate ca failed!!!\n");
		return -1;
	}

	return 0;
}

static int gx6605_demux_ca_free(int sub,GxDemuxProperty_CA *config_ca)
{
	struct dmx_ca *ca = NULL;

	if (NULL == config_ca)
		return -1;

	if ((config_ca->ca_id >= MAX_CA_NUM) || (config_ca->ca_id < 0)) {
		DMX_PRINTF("CAConfig: ca ca_id error!\n");
		return -1;
	}

	ca = gx6605_dmx.ca[config_ca->ca_id];
	if (NULL == ca) {
		DMX_PRINTF("the ca config ca is NULL\n");
		return -1;
	}

	gx6605_dmx_free_ca(ca);

	return 0;
}

static int gx6605_demux_ca_config(int sub,GxDemuxProperty_CA *config_ca)
{
	struct dmx_slot *slot = NULL;
	struct dmx_ca *ca = NULL;

	if (NULL == config_ca)
		return -1;

	if ((config_ca->ca_id >= MAX_CA_NUM) || (config_ca->ca_id < 0)) {
		DMX_PRINTF("CAConfig: ca ca_id error!\n");
		return -1;
	}

	if ((config_ca->slot_id >= MAX_SLOT_NUM) || (config_ca->slot_id < 0)) {
		DMX_PRINTF("CAConfig: ca slot_id error!\n");
		return -1;
	}

	if (config_ca->flags > (0x3|(1<<7))) {
		DMX_PRINTF("CAConfig: ca flags > 0x3!!\n");
		return -1;
	}

	slot = gx6605_dmx.slots[config_ca->slot_id];
	ca = gx6605_dmx.ca[config_ca->ca_id];
	if ((NULL == ca) || (NULL == slot)) {
		DMX_PRINTF("the ca's config ca is NULL\n");
		return -1;
	}

    slot->ca = ca;
	ca->flags = config_ca->flags;
	ca->even_key_high = config_ca->even_key_high;
	ca->even_key_low = config_ca->even_key_low;
	ca->odd_key_high = config_ca->odd_key_high;
	ca->odd_key_low = config_ca->odd_key_low;
	gx6605_dmx_ca_config(slot, ca);

	return 0;
}

static int gx6605_demux_ca_attribute(int sub,GxDemuxProperty_CA *config_ca)
{
	struct dmx_ca *ca = NULL;

	if (NULL == config_ca)
		return -1;

	if ((config_ca->ca_id >= MAX_CA_NUM) || (config_ca->ca_id < 0)) {
		DMX_PRINTF("CAConfig: ca ca_id error!\n");
		return -1;
	}

	if ((config_ca->slot_id >= MAX_SLOT_NUM) || (config_ca->slot_id < 0)) {
		DMX_PRINTF("CAConfig: ca slot_id error!\n");
		return -1;
	}

	if (config_ca->flags > 0x3) {
		DMX_PRINTF("CAConfig: ca flags > 0x3!!\n");
		return -1;
	}

	ca = gx6605_dmx.ca[config_ca->ca_id];
	if (NULL == ca) {
		DMX_PRINTF("the ca's config ca is NULL\n");
		return -1;
	}

	config_ca->flags         =ca->flags;
	config_ca->even_key_high =ca->even_key_high  ;
	config_ca->even_key_low  =ca->even_key_low;
	config_ca->odd_key_high  =ca->odd_key_high;
	config_ca->odd_key_low   =ca->odd_key_low;

	return 0;
}

static int gx6605_demux_camtc_config(int sub,GxDemuxProperty_MTCCA *config_mtc)
{
	struct dmx_slot *slot = NULL;
	struct dmx_ca *ca = NULL;

	unsigned char ck_key[16] = {0};
	gx_mtc_info  mtc_info_cfg;

	if (NULL == config_mtc) {
		DMX_PRINTF("you have a joke?! the MTCCA property cannot be NULL!\n");
		return -1;
	}

	if ((config_mtc->ca_id >= MAX_CA_NUM) || (config_mtc->ca_id < 0)) {
		DMX_PRINTF("MTCCAConfig: ca id error!\n");
		return -1;
	}

	if (NULL == config_mtc->eck) {
		DMX_PRINTF("MTCCAConfig: eck cannot be NULL!\n");
		return -1;
	}

	if (0 == config_mtc->eck_len) {
		DMX_PRINTF("MTCCAConfig: eck_len cannot be zero!\n");
		return -1;
	}

	slot = gx6605_dmx.slots[config_mtc->slot_id];
	ca = gx6605_dmx.ca[config_mtc->ca_id];
	if (NULL == ca) {
		DMX_PRINTF("MTCCAConfig : ca is NULL or slot is NULL\n");
		return -1;
	}

	/* Get ck_key for OTP */
	//#define CK_KEY_START_OFFSET     32
	//ret = gx_otp_read(ck_key, CK_KEY_START_OFFSET, GX_ARRAY_SIZE(ck_key));
	//if (ret) {
	//	DMX_PRINTF("MTCCAConfig : read otp failed!\n");
	//	return -1;
	//}

	mtc_info_cfg.arithmetic = config_mtc->arith_type;
	mtc_info_cfg.ciphermode = config_mtc->arith_mode;
	mtc_info_cfg.decryptsel = GXMTCCA_DECRYPT;
	mtc_info_cfg.output = NULL;

	/* Get CK */
	mtc_info_cfg.key = ck_key;
	mtc_info_cfg.key_len = GX_ARRAY_SIZE(ck_key);
	mtc_info_cfg.input = config_mtc->eck;
	mtc_info_cfg.input_len = config_mtc->eck_len;

	if (MTC_KEY_EVEN == (config_mtc->flags & MTC_KEY_EVEN)) {
		/* Get Even CW */
		if (NULL == config_mtc->ecw_even) {
			DMX_PRINTF("MTCCAConfig: ecw_even cannot be NULL!\n");
			return -1;
		}

		if (0 == config_mtc->ecw_even_len) {
			DMX_PRINTF("MTCCAConfig: ecw_even_len cannot be zero!\n");
			return -1;
		}

		mtc_info_cfg.cw_even = config_mtc->ecw_even;
		mtc_info_cfg.cw_even_len = config_mtc->ecw_even_len;

	}

	if (MTC_KEY_ODD == (config_mtc->flags & MTC_KEY_ODD)) {
		/* Get Odd CW */
		if (NULL == config_mtc->ecw_odd) {
			DMX_PRINTF("MTCCAConfig: ecw_odd cannot be NULL!\n");
			return -1;
		}

		if (0 == config_mtc->ecw_odd_len) {
			DMX_PRINTF("MTCCAConfig: ecw_odd_len cannot be zero!\n");
			return -1;
		}

		mtc_info_cfg.cw_odd = config_mtc->ecw_odd;
		mtc_info_cfg.cw_odd_len = config_mtc->ecw_odd_len;
	}


	ca->flags = config_mtc->flags;
	gx6605_dmx_mtc_decrypt(&mtc_info_cfg,ca);
	gx6605_set_mtc_descramble(ca,slot);

	return 0;
}

static int gx6605_demux_event_cb(unsigned int event,int ops,void *priv)
{
	struct gxav_module_inode *inode = (struct gxav_module_inode *)priv;

	return ops ? gxav_module_inode_set_event(inode, event) : gxav_module_inode_clear_event(inode, event);
}

static struct gxav_module_inode* gx6605_demux_interrupt(struct gxav_module_inode *inode, int irq)
{
	if (irq != DMX_INTERRUPT_NUMBER || inode == NULL)
	{
		gx6605_demux_clr_ints();
		DMX_PRINTF("gx6605_demux_interrupt inode = NULL\n");
		return NULL;
	}

#ifdef CONFIG_AV_MODULE_ICAM
	if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)) {
		extern int  gxav_iemm_interrupt(void) ;
		gxav_iemm_interrupt();
	}
#endif

	gx6605_dmx_interrupt(gx6605_demux_event_cb,inode);
	return inode;
}

static int gx6605_demux_fifo_free_size(GxDemuxProperty_FilterFifoFreeSize *fifo_free_size)
{
	struct dmx_filter *filter = NULL;
	struct gxfifo *fifo = NULL;


	if ((fifo_free_size->filter_id >= MAX_FILTER_NUM) || (fifo_free_size->filter_id < 0)) {
		DMX_PRINTF("FilterFIFOFreeSize: filter id error!\n");
		return -1;
	}

	filter = gx6605_dmx.filters[fifo_free_size->filter_id];
	if (NULL == filter) {
		DMX_PRINTF("FilterFIFOFreeSize: filter is NULL\n");
		return -1;
	}
	fifo = &filter->section_fifo;
	if (NULL == fifo) {
		DMX_PRINTF("FilterFIFOFreeSize: fifo is NULL\n");
		return -1;
	}

	fifo_free_size->free_size =  gxfifo_freelen(fifo);

	return 0;
}

extern int gx6605_demux_framebuffer_alloc(void);
extern int gx6605_demux_framebuffer_free(int i);
extern int gx6605_demux_query_fifo(void);

static int gx6605_demux_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0;
	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count))
		return -3;

	gx_mutex_lock(&demux_mutex);
	switch (property_id) {
		case GxAVGenericPropertyID_ModuleLinkChannel:
		case GxAVGenericPropertyID_ModuleUnLinkChannel:
			{
				struct fifo_info *module_set_fifo = (struct fifo_info *)property;
				struct demux_fifo demux_fifo;

				if (NULL == property)
					goto err;

				gxav_channel_get_phys(module_set_fifo->channel,
						&(demux_fifo.buffer_start_addr),&(demux_fifo.buffer_end_addr), &(demux_fifo.buffer_id));
				demux_fifo.channel_id    = gxav_channel_id_get(module_set_fifo->channel);

				if(module_set_fifo->pin_id < 64)
				{
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
					ret = gx6605_demux_link_fifo(module->sub, &demux_fifo);
				} else
					ret = gx6605_demux_unlink_fifo(module->sub, &demux_fifo);
			}
			break;

		case GxDemuxPropertyID_Config:
			{
				GxDemuxProperty_ConfigDemux *config_dmx = (GxDemuxProperty_ConfigDemux *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_ConfigDemux)) {
					DMX_PRINTF("gx6605  ConfigDmx: the demux module's configure param error!\n");
					goto err;
				}

				ret = gx6605_demux_config(module->sub,config_dmx);
			}
			break;

		case GxDemuxPropertyID_SlotConfig:
			{
				GxDemuxProperty_Slot *config_slot = (GxDemuxProperty_Slot *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Slot)) {
					DMX_PRINTF("ConfigSlot:slot's configure slot property error!\n");
					goto err;
				}

				ret = gx6605_demux_slot_config(module->sub,config_slot);
			}
			break;

		case GxDemuxPropertyID_SlotEnable:
			{
				GxDemuxProperty_Slot *enable_slot = (GxDemuxProperty_Slot *) property;


				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Slot)) {
					DMX_PRINTF("SlotEnable: the demux slot's configure slot param error!\n");
					goto err;
				}

				ret = gx6605_demux_slot_enable(module->sub,enable_slot);
			}
			break;

		case GxDemuxPropertyID_SlotDisable:
			{
				GxDemuxProperty_Slot *disable_slot = (GxDemuxProperty_Slot *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Slot)) {
					DMX_PRINTF("SlotDisable: the demux slot's configure slot param error!\n");
					goto err;
				}

				ret = gx6605_demux_slot_disable(module->sub,disable_slot);
			}
			break;

		case GxDemuxPropertyID_SlotFree:
			{
				GxDemuxProperty_Slot *free_slot = (GxDemuxProperty_Slot *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Slot)) {
					DMX_PRINTF("the demux's free slot param error!\n");
					goto err;
				}

				ret = gx6605_demux_slot_free(module->sub,free_slot);
			}
			break;

		case GxDemuxPropertyID_FilterConfig:
			{
				GxDemuxProperty_Filter *config_filter = (GxDemuxProperty_Filter *) property;

				if (NULL == property) {
					DMX_PRINTF("FilterConfig: filter property is NULL!\n");
					goto err;
				}

				if (size != sizeof(GxDemuxProperty_Filter)) {
					DMX_PRINTF("FilterConfig: filter property error!\n");
					goto err;
				}

				ret = gx6605_demux_filter_config(module->sub,config_filter);
			}
			break;

		case GxDemuxPropertyID_FilterEnable:
			{
				GxDemuxProperty_Filter *enable_filter = (GxDemuxProperty_Filter *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Filter)) {
					DMX_PRINTF("FilterEnable: filter property error!\n");
					goto err;
				}

				ret = gx6605_demux_filter_enable(module->sub,enable_filter);
			}
			break;

		case GxDemuxPropertyID_FilterDisable:
			{
				GxDemuxProperty_Filter *disable_filter = (GxDemuxProperty_Filter *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Filter)) {
					DMX_PRINTF("FilterDisable: filter property error!\n");
					goto err;
				}

				ret = gx6605_demux_filter_disable(module->sub,disable_filter);
			}
			break;

		case GxDemuxPropertyID_FilterFIFOReset:
			{
				GxDemuxProperty_FilterFifoReset *reset_filter_fifo
					= (GxDemuxProperty_FilterFifoReset *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_FilterFifoReset)) {
					DMX_PRINTF("FilterFIFOReset: filter property error!\n");
					goto err;
				}

				ret = gx6605_demux_filter_fifo_reset(module->sub,reset_filter_fifo);
			}
			break;

		case GxDemuxPropertyID_FilterFree:
			{
				GxDemuxProperty_Filter *free_filter = (GxDemuxProperty_Filter *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Filter)) {
					DMX_PRINTF("FilterFree: filter property error!\n");
					goto err;
				}

				ret = gx6605_demux_filter_free(module->sub,free_filter);
			}
			break;

		case GxDemuxPropertyID_CAConfig:
			{
				GxDemuxProperty_CA *config_ca = (GxDemuxProperty_CA *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_CA)) {
					DMX_PRINTF("the demux's ca config param error!\n");
					goto err;
				}

				ret = gx6605_demux_ca_config(module->sub, config_ca);
			}
			break;

		case GxDemuxPropertyID_CAFree:
			{
				GxDemuxProperty_CA *config_ca = (GxDemuxProperty_CA *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_CA)) {
					DMX_PRINTF("the ca config param error!\n");
					goto err;
				}

				ret = gx6605_demux_ca_free(module->sub, config_ca);
			}
			break;

		case GxDemuxPropertyID_Pcr:
			{
				GxDemuxProperty_Pcr *pcr = (GxDemuxProperty_Pcr *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Pcr)) {
					DMX_PRINTF("Sync : the demux's recover param error!\n");
					goto err;
				}

				ret = gx6605_demux_pcr(module->sub, pcr);
			}
			break;

		case GxDemuxPropertyID_MTCCAConfig:
			{
				GxDemuxProperty_MTCCA *config_mtc = (GxDemuxProperty_MTCCA *) property;

				if (NULL == property) {
					DMX_PRINTF("you have a joke?! the MTCCA property cannot be NULL!\n");
					goto err;
				}

				if (size != sizeof(GxDemuxProperty_MTCCA)) {
					DMX_PRINTF("MTCCAConfig : GxDemuxProperty_MTCCA error!\n");
					goto err;
				}

				ret = gx6605_demux_camtc_config(module->sub, config_mtc);
			}
			break;

		default:
			DMX_PRINTF("the parameter which demux's property_id is wrong, please set the right parameter\n");
			goto err;

	}

	gx_mutex_unlock(&demux_mutex);
	return ret;
err:
	gx_mutex_unlock(&demux_mutex);
	return -1;
}

static int gx6605_demux_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0;
	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count))
		return -3;

	gx_mutex_lock(&demux_mutex);
	switch (property_id) {
		case GxDemuxPropertyID_TSLockQuery:
			{
				GxDemuxProperty_TSLockQuery *query_tslock = (GxDemuxProperty_TSLockQuery *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_TSLockQuery)) {
					DMX_PRINTF("gx6605 QueryTsLock: the property error!\n");
					goto err;
				}

				ret = gx6605_demux_lock(module->sub, query_tslock);
			}
			break;

		case GxDemuxPropertyID_SlotAlloc:
			{
				GxDemuxProperty_Slot *alloc_slot = (GxDemuxProperty_Slot *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Slot)) {
					DMX_PRINTF("gx6605 AllocSlot: the demux slot's allocate param error!\n");
					goto err;
				}

				ret = gx6605_demux_slot_allocate(module->sub, alloc_slot);
			}
			break;

		case GxDemuxPropertyID_SlotConfig:
			{
				GxDemuxProperty_Slot *config_slot = (GxDemuxProperty_Slot *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Slot)) {
					DMX_PRINTF("SlotConfig: slot property error!\n");
					goto err;
				}

				ret = gx6605_demux_slot_attribute(module->sub, config_slot);
			}
			break;

		case GxDemuxPropertyID_FilterAlloc:
			{
				GxDemuxProperty_Filter *alloc_filter = (GxDemuxProperty_Filter *) property;
				int ret = 0;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Filter)) {
					DMX_PRINTF("FilterAlloc: the demux filter's allocate param error!\n");
					goto err;
				}

				ret = gx6605_demux_filter_allocate(module->sub, alloc_filter);
				if(ret != 0)
					goto err;

				ret = gx6605_demux_filter_eventcb(module->sub, alloc_filter->filter_id,
						gx6605_demux_event_cb, module->inode);
			}
			break;

		case GxDemuxPropertyID_FilterConfig:
			{
				GxDemuxProperty_Filter *config_filter = (GxDemuxProperty_Filter *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_Filter)) {
					DMX_PRINTF("FilterConfig: filter config property error!\n");
					goto err;
				}

				ret = gx6605_demux_filter_attribute(module->sub, config_filter);
			}
			break;

		case GxDemuxPropertyID_CAAlloc:
			{
				GxDemuxProperty_CA *alloc_ca = (GxDemuxProperty_CA *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_CA)) {
					DMX_PRINTF("the demux ca's allocate param error!\n");
					goto err;
				}

				ret = gx6605_demux_ca_allocate(module->sub, alloc_ca);
			}
			break;
		case GxDemuxPropertyID_CAConfig:
			{
				GxDemuxProperty_CA *config_ca = (GxDemuxProperty_CA *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_CA)) {
					DMX_PRINTF("the demux's ca config param error!\n");
					goto err;
				}

				ret = gx6605_demux_ca_attribute(module->sub, config_ca);
			}
			break;

		case GxDemuxPropertyID_FilterRead:
			{
				GxDemuxProperty_FilterRead *read_filter = (GxDemuxProperty_FilterRead *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_FilterRead)) {
					DMX_PRINTF("FilterRead: the demux read filter param error!\n");
					goto err;
				}

				ret = gx6605_demux_filter_read(module->sub, read_filter);
			}
			break;

		case GxDemuxPropertyID_FilterFIFOQuery:
			{
				GxDemuxProperty_FilterFifoQuery *query_filter_fifo
					= (GxDemuxProperty_FilterFifoQuery *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_FilterFifoQuery)) {
					DMX_PRINTF("QueryFilterFifo: the demux  param error!\n");
					goto err;
				}

				ret = gx6605_demux_filter_fifo_query(module->sub, query_filter_fifo);
			}
			break;

		case GxDemuxPropertyID_SlotQueryByPid:
			{
				GxDemuxProperty_SlotQueryByPid *query_slot = (GxDemuxProperty_SlotQueryByPid *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_SlotQueryByPid)) {
					DMX_PRINTF("SlotQueryByPid: the demux  param error!\n");
					goto err;
				}

				ret = gx6605_demux_slot_query(module->sub, query_slot);
			}
			break;

		case GxDemuxPropertyID_ReadStc:
			{
				GxDemuxProperty_ReadStc *stc = (GxDemuxProperty_ReadStc *) property;

				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_ReadStc)) {
					DMX_PRINTF("ReadStc: the demux  param error!\n");
					goto err;
				}

				ret = gx6605_demux_stc(module->sub, stc);
			}
			break;

		case GxDemuxPropertyID_FilterFifoFreeSize:
			{
				GxDemuxProperty_FilterFifoFreeSize *fifo_free_size
					= (GxDemuxProperty_FilterFifoFreeSize *) property;
				if (NULL == property)
					goto err;

				if (size != sizeof(GxDemuxProperty_FilterFifoFreeSize)) {
					DMX_PRINTF("FilterFIFOFreeSize: GxDemuxProperty_FilterFifoFreeSize error!\n");
					goto err;
				}

				ret = gx6605_demux_fifo_free_size(fifo_free_size);
			}
			break;

		default:
			DMX_PRINTF("the parameter which demux's property_id is wrong, please set the right parameter\n");
			goto err;
	}

	gx_mutex_unlock(&demux_mutex);
	return ret;
err:
	gx_mutex_unlock(&demux_mutex);
	return -1;
}


struct gxav_module_ops gx6605_demux_module = {
	.module_type  = GXAV_MOD_DEMUX,
	.count        = 3,
	.irqs         = {DMX_INTERRUPT_NUMBER, -1},
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = gx6605_demux_open,
	.close        = gx6605_demux_close,
	.init         = gx6605_demux_init,
	.cleanup      = gx6605_demux_cleanup,
	.set_property = gx6605_demux_set_property,
	.get_property = gx6605_demux_get_property,
	.interrupts[DMX_INTERRUPT_NUMBER]    = gx6605_demux_interrupt
};

