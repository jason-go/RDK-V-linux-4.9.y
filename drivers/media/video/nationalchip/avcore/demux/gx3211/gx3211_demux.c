/******************************************************************************

  Copyright (C), 2008-2018, Nationalchip Tech. Co., Ltd.

 ******************************************************************************
 File Name     : gx3211_demux.c
Version       : Initial Draft
Author        : Nationalchip multimedia software group
Created       : 2008/12/18
Last Modified :
Description   : gx3211 chip demux module outer interface
Function List :
History       :
1.Date        : 2008/12/18
Author      : zz
Modification: Created file

 ******************************************************************************/
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "fifo.h"
#include "gx3211_demux.h"
#include "gx3211_demux_regs.h"
#include "gx3211_hal_common.h"
#include "porting.h"
#include "sdc_hal.h"
#include "stc_hal.h"
#include "profile.h"

struct dmxdev gx3211_dmx[MAX_DEMUX_UNIT];
struct reg_demux *gx3211_demux_reg[MAX_DEMUX_UNIT];
static gx_mutex_t demux_mutex;

/* add two function for descramble device . for multi device protect
 * demux is not use the funtion
 * add time 20160317
 * */
extern void gx3211_demux_func_lock(void)
{
	gx_mutex_lock(&demux_mutex);
}

extern void gx3211_demux_func_unlock(void)
{
	gx_mutex_unlock(&demux_mutex);
}

#define DEMUX_THREAD_STACK (16 * 1024)
static int gx3211_demux_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int i=0, ret;
	struct dmxdev *dmxdev;
	unsigned int addr[MAX_DEMUX_UNIT];
	const char* kcache_psi[MAX_DEMUX_UNIT] = {"psi-demux0", "psi-demux1"};
	const char* kcache_pes[MAX_DEMUX_UNIT] = {"pes-demux0", "pes-demux1"};
	const char* kcache_ts[MAX_DEMUX_UNIT] = {"ts-demux0", "ts-demux1"};
	const char* kcache_thread[MAX_DEMUX_UNIT] = {"thread-demux0", "thread-demux1"};

	if(device == NULL || inode == NULL)
		return -1;

	addr[0] = GXAV_DEMUX_BASE_SYS;
	addr[1] = GXAV_DEMUX_BASE_SYS1;
	for(i=0;i<MAX_DEMUX_UNIT;i++){
		dmxdev = &gx3211_dmx[i];
		if (!gx_request_mem_region(addr[i], sizeof(struct reg_demux))) {
			DMX_PRINTF("request DEMUX mem region failed!\n");
			return -1;
		}

		gx3211_demux_reg[i] = gx_ioremap(addr[i], sizeof(struct reg_demux));
		if (!gx3211_demux_reg[i]) {
			DMX_PRINTF("ioremap DEMUX space failed!\n");
			return -1;
		}

		DMX_DBG("gx3211_demux_reg = 0x%p\n",gx3211_demux_reg[i]);
		dmxdev->reg = gx3211_demux_reg[i];
		gx3211_dmx_initialization(dmxdev);
		ret = gx_sem_create(&(dmxdev->sem_id), 0);
		if (ret < 0) {
			DMX_PRINTF("demux semaphore created fail!\n");
			return -1;
		}

		dmxdev->thread_stack = gx_malloc(DEMUX_THREAD_STACK);
		if (dmxdev->thread_stack == NULL) {
			DMX_PRINTF("demux thread stack memory malloc fail!\n");
			dmxdev->thread_stack = NULL;
			return -1;
		}

		ret = gx_thread_create(kcache_thread[i], &(dmxdev->thread_id), gx3211_demux_read_thread,
				dmxdev, dmxdev->thread_stack, DEMUX_THREAD_STACK, 9, &(dmxdev->thread_info));
		if (ret < 0) {
			DMX_PRINTF("demux thread created fail!\n");
			return -1;
		}

		dmxdev->pes_buffer_cache = kcache_create(kcache_pes[i], BUFFER_PES_SIZE, 0);
		dmxdev->psi_buffer_cache = kcache_create(kcache_psi[i], BUFFER_PSI_SIZE, 0);
		dmxdev->ts_buffer_cache  = kcache_create(kcache_ts[i], (BUFFER_TS_SIZE+188*2), 0);

		gx_spin_lock_init(&(dmxdev->demux_spin_lock));
	}
	gx_mutex_init(&(demux_mutex));
	return 0;
}

static int gx3211_demux_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int i;
	struct dmxdev *dmxdev;
	unsigned int addr[MAX_DEMUX_UNIT];

	if(device == NULL || inode == NULL)
		return -1;

	addr[0] = GXAV_DEMUX_BASE_SYS;
	addr[1] = GXAV_DEMUX_BASE_SYS1;
	for(i = 0; i < MAX_DEMUX_UNIT; i++){
		dmxdev = &gx3211_dmx[i];
		gx3211_dmx_cleanup(dmxdev);
		gx_iounmap(dmxdev->reg);
		gx_release_mem_region(addr[i], sizeof(struct reg_demux));
		gx3211_demux_reg[i] = NULL;
		gx_memset(dmxdev,0,sizeof(struct dmxdev));
		gx_sem_delete(&(dmxdev->sem_id));
		gx_memset(&(dmxdev->sem_id),0,sizeof(gx_sem_id));

		gx_thread_delete(dmxdev->thread_id);
		if(dmxdev->thread_stack !=NULL){
			gx_free(dmxdev->thread_stack);
			dmxdev->thread_stack = NULL;
		}
		if (dmxdev->pes_buffer_cache)
			kcache_destroy(dmxdev->pes_buffer_cache);

		if (dmxdev->psi_buffer_cache)
			kcache_destroy(dmxdev->psi_buffer_cache);

		if (dmxdev->ts_buffer_cache)
			kcache_destroy(dmxdev->ts_buffer_cache);
	}

	gx_mutex_destroy(&(demux_mutex));
	return 0;
}

static struct dmx_demux *get_subdemux(int id)
{
	if(id == 0)
		return &gx3211_dmx[0].demuxs[0];
	else if(id == 1)
		return &gx3211_dmx[0].demuxs[1];
	else if(id == 2)
		return &gx3211_dmx[1].demuxs[0];
	else if(id == 3)
		return &gx3211_dmx[1].demuxs[1];
	else
		return NULL;
}

static struct dmxdev *get_dmxdev(int id)
{
	if(id == 0 || id == 1)
		return &gx3211_dmx[0];
	else if(id == 2 || id == 3)
		return &gx3211_dmx[1];
	else
		return NULL;
}

static int gx3211_demux_open(struct gxav_module *module)
{
	struct dmx_demux *demux = NULL;

	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count))
		return -1;

	demux = get_subdemux(module->sub);
	demux->demux_device = get_dmxdev(module->sub);
	gx_memset(demux->slots,       0, sizeof(struct dmx_slot *)   * MAX_SLOT_NUM);
	gx_memset(demux->muxslots,    0, sizeof(struct dmx_slot *)   * MAX_SLOT_NUM);

	return 0;
}

extern void gx3211_demux_delay_free(struct dmxdev *dmxdev);
static int gx3211_demux_close(struct gxav_module *module)
{
	int i;
	struct dmx_demux *demux = NULL;

	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count))
		return -1;

	demux = get_subdemux(module->sub);
	for (i = 0; i < MAX_SLOT_NUM; i++) {
		if (NULL != demux->slots[i]) {
			gx3211_dmx_free_slot(demux->slots[i]);
		}
	}
	for (i = 0; i < MAX_SLOT_NUM; i++) {
		if (NULL != demux->muxslots[i]) {
			gx3211_dmx_free_slot(demux->muxslots[i]);
		}
	}
	for (i = 0; i < MAX_CA_NUM; i++) {
		if(demux->ca[i] != NULL) {
			gx3211_dmx_free_ca(demux->ca[i]);
		}
	}

	gx_mdelay(10);
	gx3211_demux_delay_free(demux->demux_device);
	//gx3211_clr_ints(dmxdev->reg);

	return 0;
}

static int gx3211_demux_config(int sub,GxDemuxProperty_ConfigDemux *config_dmx)
{
	struct dmx_demux *demux = NULL;

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

	demux = get_subdemux(sub);
	demux->sync_lock_gate = config_dmx->sync_lock_gate;
	demux->sync_loss_gate = config_dmx->sync_loss_gate;
	demux->time_gate = config_dmx->time_gate;
	demux->byt_cnt_err_gate = config_dmx->byt_cnt_err_gate;

	demux->stream_mode = config_dmx->stream_mode;
	demux->ts_select = config_dmx->ts_select;
	demux->source = config_dmx->source;

	gx3211_dmx_config(demux);

	return 0;
}

static int gx3211_demux_lock(int sub,GxDemuxProperty_TSLockQuery *query_tslock)
{
	struct dmx_demux *demux = NULL;

	demux = get_subdemux(sub);
	gx3211_dmx_query_tslock(demux);
	query_tslock->ts_lock = demux->ts_lock;

	return 0;
}

int gx3211_dmx_query_dmx_slot_fifo(unsigned int buf_id, int slot_type, unsigned int *dmx_id, struct dmx_slot *slot, struct demux_fifo *fifo)
{
	unsigned int i = 0;
	struct dmxdev *dmxdev;

	//if(sub == 0 || sub == 1)
	dmxdev = &gx3211_dmx[0];
	//if(sub == 2 || sub == 3)
	//    dmxdev = &gx3211_dmx[1];

	for(i=0; i<MAX_AVBUF_NUM; i++) {
		if(dmxdev->avfifo[i].channel_id == buf_id && dmxdev->avfifo[i].channel != NULL) {
			gx_memcpy(fifo, &dmxdev->avfifo[i], sizeof(struct demux_fifo));
			break;
		}
	}

	if(i >= MAX_AVBUF_NUM || fifo->pin_id >= MAX_SLOT_NUM)
		return -1;

	if(dmxdev->slots[fifo->pin_id] && dmxdev->slots[fifo->pin_id]->type == slot_type)
		*dmx_id = dmxdev->slots[fifo->pin_id]->demux->id;
	else
		return -1;

	*slot = *dmxdev->slots[fifo->pin_id];

	return 0;
}

int gx3211_demux_link_fifo(int sub, struct demux_fifo *fifo)
{
	struct dmx_demux *demux = NULL;

	demux = get_subdemux(sub);

	if ((fifo->pin_id > 64*3) || (fifo->pin_id < 0)) {
		DMX_PRINTF("property error!\n");
		return -1;
	}

	if (fifo->pin_id < 32) {
		if (!(demux->slots[fifo->pin_id])
				|| (demux->slots[fifo->pin_id]->type < DEMUX_SLOT_AUDIO)) {
			DMX_PRINTF("pin_id error!\n");
			return -1;
		}
	}

	return gx3211_dmx_link_fifo(demux, fifo);
}

int gx3211_demux_unlink_fifo(int sub, struct demux_fifo *fifo)
{
	struct dmx_demux *demux = NULL;

	demux = get_subdemux(sub);

	if ((fifo->pin_id >= 64*3) || (fifo->pin_id < 0)) {
		DMX_PRINTF("property error!\n");
		return -1;
	}

	if (fifo->pin_id < 64) {
		if (!(demux->slots[fifo->pin_id])
				|| (demux->slots[fifo->pin_id]->type < DEMUX_SLOT_AUDIO)) {
			DMX_PRINTF("pin_id error!\n");
			return -1;
		}
	}

	return gx3211_dmx_unlink_fifo(demux, fifo);
}

static int gx3211_demux_pcr(int sub,GxDemuxProperty_Pcr *pcr)
{
	struct dmx_demux *demux = NULL;

	demux = get_subdemux(sub);
	gx3211_dmx_pcr_sync(demux, pcr->pcr_pid);
	return 0;
}

static int gx3211_demux_slot_allocate(int sub,GxDemuxProperty_Slot *alloc_slot)
{
	struct dmx_demux *demux = NULL;
	enum dmx_slot_type slot_type;
	unsigned short pid;

	if ((alloc_slot->type < DEMUX_SLOT_PSI) || (alloc_slot->type > DEMUX_SLOT_VIDEO)) {
		DMX_PRINTF("gx3211 Allocate: the slot's type error!\n");
		return -1;
	}

	demux = get_subdemux(sub);
	slot_type = alloc_slot->type;
	pid = alloc_slot->pid;
	alloc_slot->slot_id = gx3211_dmx_alloc_slot(demux,slot_type,pid);
	DMX_DBG("alloc_slot->slot_id = %d\n",  alloc_slot->slot_id,pid);
	if (alloc_slot->slot_id < 0) {
		DMX_PRINTF("allocate slot failed!!!\n");
		return -1;
	}

	return 0;
}

static int gx3211_demux_slot_free(int sub,GxDemuxProperty_Slot *free_slot)
{
	struct dmx_demux *demux = NULL;
	struct dmx_slot *slot = NULL;

	if (free_slot->slot_id < 0) {
		DMX_PRINTF("ConfigSlot:slot's configure slot slot_id error!\n");
		return -1;
	}

	demux = get_subdemux(sub);

	if(DEMUX_SLOT_MUXTS == free_slot->type){
		slot = demux->muxslots[(free_slot->slot_id - PIN_ID_SDRAM_OUTPUT)];
	}else{
		slot = demux->slots[free_slot->slot_id];
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
	gx3211_dmx_free_slot(slot);

	return 0;
}

static int gx3211_demux_slot_config(int sub,GxDemuxProperty_Slot *config_slot)
{
	struct dmx_demux *demux = NULL;
	struct dmx_slot *slot = NULL;

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

	demux = get_subdemux(sub);
	slot = demux->slots[config_slot->slot_id];

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

	gx3211_dmx_slot_config(slot);

	return 0;
}

static int gx3211_demux_slot_enable(int sub,GxDemuxProperty_Slot *enable_slot)
{
	struct dmx_demux *demux = NULL;
	struct dmx_slot *slot = NULL;

	if ((enable_slot->slot_id >= MAX_SLOT_NUM) || (enable_slot->slot_id < 0)) {
		DMX_PRINTF("slot's configure slot slot_id error!\n");
		return -1;
	}

	demux = get_subdemux(sub);
	slot = demux->slots[enable_slot->slot_id];

	if (NULL == slot) {
		DMX_PRINTF("the demux slot's configure slot NULL!\n");
		return -1;
	}

	gx3211_dmx_enable_slot(slot);
	return 0;
}

static int gx3211_demux_slot_query(int sub,GxDemuxProperty_SlotQueryByPid *query_slot)
{
	struct dmx_demux *demux = NULL;
	struct dmx_slot *slot = NULL;
	int i;

	demux = get_subdemux(sub);

	for (i = 0; i < MAX_SLOT_NUM; i++) {
		slot = demux->slots[i];
		if (NULL != slot) {
			if (query_slot->pid == slot->pid) {
				query_slot->slot_id = slot->id;
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

static int gx3211_demux_slot_disable(int sub,GxDemuxProperty_Slot *disable_slot)
{
	struct dmx_demux *demux = NULL;
	struct dmx_slot *slot = NULL;

	if ((disable_slot->slot_id >= MAX_SLOT_NUM) || (disable_slot->slot_id < 0)) {
		gx_printf("SlotDisable: slot id error!\n");
		return -1;
	}
	demux = get_subdemux(sub);
	slot = demux->slots[disable_slot->slot_id];

	if (NULL == slot) {
		gx_printf("SlotDisable: slot NULL!\n");
		return -1;
	}

	if(slot->filter_num < 1)
		gx3211_dmx_disable_slot(slot);

	return 0;
}

static int gx3211_demux_slot_attribute(int sub,GxDemuxProperty_Slot *config_slot)
{
	struct dmx_demux *demux = NULL;
	struct dmx_slot *slot = NULL;

	if ((config_slot->slot_id >= MAX_SLOT_NUM) || (config_slot->slot_id < 0)) {
		gx_printf("SlotConfig: slot id error!\n");
		return -1;
	}

	demux = get_subdemux(sub);
	slot = demux->slots[config_slot->slot_id];

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

static int gx3211_demux_filter_allocate(int sub,GxDemuxProperty_Filter *alloc_filter)
{
	struct dmx_demux *demux = NULL;
	struct dmx_slot *slot = NULL;

	if (alloc_filter->slot_id < 0) {
		DMX_PRINTF("FilterAlloc: filter slot_id error!\n");
		return -1;
	}

	demux = get_subdemux(sub);
	if(alloc_filter->slot_id >= PIN_ID_SDRAM_OUTPUT)
		slot = demux->muxslots[(alloc_filter->slot_id - PIN_ID_SDRAM_OUTPUT)];
	else
		slot = demux->slots[alloc_filter->slot_id];

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
		} else if (DEMUX_SLOT_MUXTS == slot->type || DEMUX_SLOT_TS == slot->type ) {
			slot->hw_buffer_size = BUFFER_TS_SIZE;
		}
	} else {
		slot->hw_buffer_size = alloc_filter->hw_buffer_size;
	}

	if (0 == alloc_filter->almost_full_gate) {
		if (DEMUX_SLOT_PES == slot->type) {
			slot->almost_full_gate = (BUFFER_TS_GATE)>>8;
		} else if (DEMUX_SLOT_MUXTS == slot->type || DEMUX_SLOT_TS == slot->type ) {
			slot->almost_full_gate = BUFFER_TS_GATE;
		}
	} else {
		slot->almost_full_gate = alloc_filter->almost_full_gate;
	}

	//	if (!is_power_of_2(slot->hw_buffer_size)) {
	//		gx_printf("FilterAlloc: filter buffer size is not power of 2!\n");
	//		return -1;
	//	}

	if (0 == alloc_filter->sw_buffer_size) {
		if (DEMUX_SLOT_PSI == slot->type) {
			slot->sw_buffer_size = FIFO_PSI_SIZE;
		} else if (slot->type == DEMUX_SLOT_PES || slot->type == DEMUX_SLOT_PES_AUDIO || slot->type == DEMUX_SLOT_PES_VIDEO) {
			slot->sw_buffer_size = FIFO_PES_SIZE;
		} else if (DEMUX_SLOT_MUXTS == slot->type || DEMUX_SLOT_TS == slot->type ) {
			slot->sw_buffer_size = BUFFER_TS_SIZE;
		}
	} else {
		slot->sw_buffer_size = alloc_filter->sw_buffer_size;
	}

	//if (!is_power_of_2(slot->sw_buffer_size)) {
	//	gx_printf("FilterAlloc: filter fifo size is not power of 2!\n");
	//	return -1;
	//}

	alloc_filter->filter_id = gx3211_dmx_alloc_filter(slot);
	if (alloc_filter->filter_id < 0) {
		DMX_PRINTF("allocate filter failed!!!\n");
		return -1;
	}
	return 0;
}

static int gx3211_demux_filter_free(int sub,GxDemuxProperty_Filter *free_filter)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_filter *filter = NULL;

	if (free_filter->filter_id < 0) {
		DMX_PRINTF("FilterFree: filter filter_id error!\n");
		return -1;
	}

	dmxdev = get_dmxdev(sub);
	if(free_filter->filter_id >= PIN_ID_SDRAM_OUTPUT)
		filter = dmxdev->muxfilters[(free_filter->filter_id - PIN_ID_SDRAM_OUTPUT)];
	else
		filter = dmxdev->filters[free_filter->filter_id];

	if (NULL == filter || filter->slot == NULL) {
		DMX_PRINTF("FilterFree: filter is NULL\n");
		return -1;
	}

	gx3211_dmx_free_filter(filter);

	return 0;
}

static int gx3211_demux_filter_fifo_reset(int sub,GxDemuxProperty_FilterFifoReset *reset_filter_fifo)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_filter *filter = NULL;
	unsigned int mask = 0;

	if (NULL == reset_filter_fifo)
		return -1;

	if (reset_filter_fifo->filter_id < 0) {
		DMX_PRINTF("FilterFIFOReset: filter filter_id error!\n");
		return -1;
	}

	dmxdev = get_dmxdev(sub);
	if(reset_filter_fifo->filter_id >= PIN_ID_SDRAM_OUTPUT)
		filter = dmxdev->muxfilters[(reset_filter_fifo->filter_id - PIN_ID_SDRAM_OUTPUT)];
	else
		filter = dmxdev->filters[reset_filter_fifo->filter_id];

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

	gx3211_set_filter_reset(filter);

	return 0;
}

static int gx3211_demux_filter_fifo_query(int sub,GxDemuxProperty_FilterFifoQuery *query_filter_fifo)
{
	struct dmx_demux *demux = NULL;

	demux = get_subdemux(sub);
	gx3211_dmx_query_fifo(demux,&(query_filter_fifo->state));

	return 0;
}

static int gx3211_demux_filter_config(int sub,GxDemuxProperty_Filter *config_filter)
{
	int depth = 0,depth_tmp=0;
	struct dmxdev *dmxdev = NULL;
	struct dmx_filter *filter = NULL;

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

	dmxdev = get_dmxdev(sub);
	filter = dmxdev->filters[config_filter->filter_id];
	if (NULL == filter) {
		DMX_PRINTF("FilterConfig: filter is NULL\n");
		return -1;
	}

	gx3211_clear_int_df(filter);
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
	gx3211_dmx_filter_config(filter);

	return 0;
}

static int gx3211_demux_filter_read(int sub,GxDemuxProperty_FilterRead *read_filter)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_filter *filter = NULL;

	if (read_filter->filter_id < 0) {
		DMX_PRINTF("FilterRead: filter filter_id error!\n");
		return -1;
	}

	if (NULL == read_filter->buffer) {
		DMX_PRINTF("FilterRead: filter buffer == NULL!\n");
		return -1;
	}

	dmxdev = get_dmxdev(sub);
	if(read_filter->filter_id >= PIN_ID_SDRAM_OUTPUT)
		filter = dmxdev->muxfilters[(read_filter->filter_id - PIN_ID_SDRAM_OUTPUT)];
	else
		filter = dmxdev->filters[read_filter->filter_id];

	if (NULL == filter) {
		DMX_PRINTF("FilterRead: the demux read filter NULL!\n");
		return -1;
	}
	read_filter->read_size = gx3211_dmx_filter_read(filter, (unsigned char *)read_filter->buffer,read_filter->max_size);

	return 0;
}

static int gx3211_demux_filter_enable(int sub,GxDemuxProperty_Filter *enable_filter)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_filter *filter = NULL;

	if ((enable_filter->filter_id >= MAX_FILTER_NUM) || (enable_filter->filter_id < 0)) {
		DMX_PRINTF("FilterEnable: filter filter_id error!\n");
		return -1;
	}

	dmxdev = get_dmxdev(sub);
	filter = dmxdev->filters[enable_filter->filter_id];
	if (NULL == filter) {
		DMX_PRINTF("FilterEnable: filter is NULL\n");
		return -1;
	}

	filter->status = DMX_STATE_GO;
	gx3211_dmx_enable_filter(filter);

	return 0;
}

static int gx3211_demux_filter_disable(int sub,GxDemuxProperty_Filter *disable_filter)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_filter *filter = NULL;

	if ((disable_filter->filter_id >= MAX_FILTER_NUM) || (disable_filter->filter_id < 0)) {
		DMX_PRINTF("FilterDisable: filter filter_id error!\n");
		return -1;
	}

	dmxdev = get_dmxdev(sub);
	filter = dmxdev->filters[disable_filter->filter_id];
	if (NULL == filter) {
		DMX_PRINTF("FilterDisable: filter is NULL\n");
		return -1;
	}

	filter->status = DMX_STATE_STOP;
	gx3211_dmx_disable_filter(filter);

	return 0;
}

static int gx3211_demux_filter_attribute(int sub,GxDemuxProperty_Filter *config_filter)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_filter *filter = NULL;

	if ((config_filter->filter_id >= MAX_FILTER_NUM) || (config_filter->filter_id < 0)) {
		DMX_PRINTF("FilterConfig: filter filter_id error!\n");
		return -1;
	}

	dmxdev = get_dmxdev(sub);
	filter = dmxdev->filters[config_filter->filter_id];
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

static int gx3211_demux_filter_eventcb(int sub,unsigned int filter_id,filter_event_cb callback,void *priv)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_filter *filter = NULL;

	dmxdev = get_dmxdev(sub);
	if(filter_id >= PIN_ID_SDRAM_OUTPUT)
		filter = dmxdev->muxfilters[(filter_id - PIN_ID_SDRAM_OUTPUT)];
	else
		filter = dmxdev->filters[filter_id];

	if (NULL == filter) {
		DMX_PRINTF("Filter: filter is NULL\n");
		return -1;
	}

	filter->event_cb    = callback;
	filter->event_priv  = priv;
	return 0;
}

static int gx3211_demux_ca_allocate(int sub,GxDemuxProperty_CA *alloc_ca)
{
	struct dmxdev *dmxdev = NULL;
	dmxdev = get_dmxdev(sub);

	alloc_ca->ca_id = gx3211_dmx_alloc_ca(dmxdev,sub);
	if (alloc_ca->ca_id < 0) {
		DMX_PRINTF("allocate ca failed!!!\n");
		return -1;
	}

	return 0;
}

static int gx3211_demux_ca_free(int sub,GxDemuxProperty_CA *config_ca)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_ca *ca = NULL;
	dmxdev = get_dmxdev(sub);

	if ((config_ca->ca_id >= MAX_CA_NUM) || (config_ca->ca_id < 0)) {
		DMX_PRINTF("CAConfig: ca ca_id error!\n");
		return -1;
	}

	ca = dmxdev->ca[config_ca->ca_id];
	if (NULL == ca) {
		DMX_PRINTF("the ca config ca is NULL\n");
		return -1;
	}

	gx3211_dmx_free_ca(ca);

	return 0;
}

static int gx3211_demux_ca_config(int sub,GxDemuxProperty_CA *config_ca)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_slot *slot = NULL;
	struct dmx_ca *ca = NULL;
	dmxdev = get_dmxdev(sub);

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

	slot = dmxdev->slots[config_ca->slot_id];
	ca = dmxdev->ca[config_ca->ca_id];
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
	gx3211_dmx_ca_config(slot, ca);

	return 0;
}

static int gx3211_demux_ca_attribute(int sub,GxDemuxProperty_CA *config_ca)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_ca *ca = NULL;
	dmxdev = get_dmxdev(sub);

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

	ca = dmxdev->ca[config_ca->ca_id];
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

static int gx3211_demux_event_cb(unsigned int event,int ops,void *priv)
{
	struct gxav_module_inode *inode = (struct gxav_module_inode *)priv;

	return ops ? gxav_module_inode_set_event(inode, event) : gxav_module_inode_clear_event(inode, event);
}

static struct gxav_module_inode* gx3211_unit0_irq(struct gxav_module_inode *inode, int irq)
{
	struct dmxdev *dmxdev = NULL;

	if (irq != DMX_INTERRUPT_NUMBER || inode == NULL) {
		//gx3211_clr_ints(dmxdev->reg);
		DMX_PRINTF("gx3211_demux_interrupt inode = NULL\n");
		return NULL;
	}

#ifdef CONFIG_AV_MODULE_ICAM
	if (firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)) {
		extern int  gxav_iemm_interrupt(void) ;
		gxav_iemm_interrupt();
	}
#endif

	dmxdev = get_dmxdev(0);
	gx3211_dmx_interrupt(gx3211_demux_event_cb,dmxdev);
	return inode;
}

static struct gxav_module_inode* gx3211_unit1_irq(struct gxav_module_inode *inode, int irq)
{
	struct dmxdev *dmxdev = NULL;

	if (irq != 41 || inode == NULL) {
		//gx3211_clr_ints(dmxdev->reg);
		DMX_PRINTF("gx3211_demux_interrupt inode = NULL\n");
		return NULL;
	}

	dmxdev = get_dmxdev(2);
	gx3211_dmx_interrupt(gx3211_demux_event_cb,dmxdev);
	return inode;
}
static int gx3211_demux_fifo_free_size(int sub,GxDemuxProperty_FilterFifoFreeSize *fifo_free_size)
{
	struct dmxdev *dmxdev = NULL;
	struct dmx_filter *filter = NULL;
	struct gxfifo *fifo = NULL;

	dmxdev = get_dmxdev(sub);
	if ((fifo_free_size->filter_id >= MAX_FILTER_NUM) || (fifo_free_size->filter_id < 0)) {
		DMX_PRINTF("FilterFIFOFreeSize: filter id error!\n");
		return -1;
	}

	filter = dmxdev->filters[fifo_free_size->filter_id];
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

static int gx3211_demux_set_property(struct gxav_module *module, int property_id, void *property, int size)
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
				ret = gx3211_demux_link_fifo(module->sub, &demux_fifo);
			} else
				ret = gx3211_demux_unlink_fifo(module->sub, &demux_fifo);
		}
		break;

	case GxDemuxPropertyID_Config:
		{
			GxDemuxProperty_ConfigDemux *config_dmx = (GxDemuxProperty_ConfigDemux *) property;

			if (NULL == property)
				goto err;

			if (size != sizeof(GxDemuxProperty_ConfigDemux)) {
				DMX_PRINTF("gx3211  ConfigDmx: the demux module's configure param error!\n");
				goto err;
			}

			ret = gx3211_demux_config(module->sub,config_dmx);
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

			ret = gx3211_demux_slot_config(module->sub,config_slot);
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

			ret = gx3211_demux_slot_enable(module->sub,enable_slot);
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

			ret = gx3211_demux_slot_disable(module->sub,disable_slot);
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

			ret = gx3211_demux_slot_free(module->sub,free_slot);
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

			ret = gx3211_demux_filter_config(module->sub,config_filter);
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

			ret = gx3211_demux_filter_enable(module->sub,enable_filter);
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

			ret = gx3211_demux_filter_disable(module->sub,disable_filter);
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

			ret = gx3211_demux_filter_fifo_reset(module->sub,reset_filter_fifo);
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

			ret = gx3211_demux_filter_free(module->sub,free_filter);
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

			ret = gx3211_demux_ca_config(module->sub, config_ca);
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

			ret = gx3211_demux_ca_free(module->sub, config_ca);
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

			ret = gx3211_demux_pcr(module->sub, pcr);
		}
		break;

	case GxDemuxPropertyID_MTCCAConfig:
		{
			DMX_PRINTF("this propterty now is not support!!!!\n");
			return -1;
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

static int gx3211_demux_get_property(struct gxav_module *module, int property_id, void *property, int size)
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
				DMX_PRINTF("gx3211 QueryTsLock: the property error!\n");
				goto err;
			}

			ret = gx3211_demux_lock(module->sub, query_tslock);
		}
		break;

	case GxDemuxPropertyID_SlotAlloc:
		{
			GxDemuxProperty_Slot *alloc_slot = (GxDemuxProperty_Slot *) property;

			if (NULL == property)
				goto err;

			if (size != sizeof(GxDemuxProperty_Slot)) {
				DMX_PRINTF("gx3211 AllocSlot: the demux slot's allocate param error!\n");
				goto err;
			}

			ret = gx3211_demux_slot_allocate(module->sub, alloc_slot);
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

			ret = gx3211_demux_slot_attribute(module->sub, config_slot);
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

			ret = gx3211_demux_filter_allocate(module->sub, alloc_filter);
			if(ret != 0)
				goto err;

			ret = gx3211_demux_filter_eventcb(module->sub, alloc_filter->filter_id,
					gx3211_demux_event_cb, module->inode);
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

			ret = gx3211_demux_filter_attribute(module->sub, config_filter);
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

			ret = gx3211_demux_ca_allocate(module->sub, alloc_ca);
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

			ret = gx3211_demux_ca_attribute(module->sub, config_ca);
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

			ret = gx3211_demux_filter_read(module->sub, read_filter);
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

			ret = gx3211_demux_filter_fifo_query(module->sub, query_filter_fifo);
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

			ret = gx3211_demux_slot_query(module->sub, query_slot);
		}
		break;

	case GxDemuxPropertyID_ReadStc:
		{
#ifndef NO_OS
			GxDemuxProperty_ReadStc *stc = (GxDemuxProperty_ReadStc *) property;

			if (NULL == property)
				goto err;

			if (size != sizeof(GxDemuxProperty_ReadStc)) {
				DMX_PRINTF("ReadStc: the demux  param error!\n");
				goto err;
			}

			ret = gxav_stc_read_stc(module->sub/2, &stc->stc_value);
#endif
		}
		break;

	case GxDemuxPropertyID_ReadPcr:
		{
#ifndef NO_OS
			GxDemuxProperty_ReadPcr *pcr = (GxDemuxProperty_ReadPcr *) property;

			if (NULL == property)
				goto err;

			if (size != sizeof(GxDemuxProperty_ReadPcr)) {
				DMX_PRINTF("ReadPcr: the demux  param error!\n");
				goto err;
			}

			ret = gxav_stc_read_pcr(module->sub/2, &pcr->pcr_value);
#endif
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

			ret = gx3211_demux_fifo_free_size(module->sub,fifo_free_size);
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


struct gxav_module_ops gx3211_demux_module = {
	.module_type  = GXAV_MOD_DEMUX,
	.count        = 4,
	.irqs         = {DMX_INTERRUPT_NUMBER,41, -1},
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = gx3211_demux_open,
	.close        = gx3211_demux_close,
	.init         = gx3211_demux_init,
	.cleanup      = gx3211_demux_cleanup,
	.set_property = gx3211_demux_set_property,
	.get_property = gx3211_demux_get_property,
	.interrupts[DMX_INTERRUPT_NUMBER]    = gx3211_unit0_irq,
	.interrupts[41]    = gx3211_unit1_irq
};

