#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gxdemux.h"
#include "porting.h"
#include "profile.h"
#include "gx3211_regs.h"

struct dmxdev gxdmxdev[MAX_DEMUX_UNIT];
static gx_mutex_t demux_mutex;
static gx_mutex_t demux_dvr_mutex;
static gx_mutex_t demux_tsw_mutex;

/* add two function for descramble device . for multi device protect
 * demux is not use the funtion
 * add time 20160317
 * */
void gxdemux_lock(void)
{
	gx_mutex_lock(&demux_mutex);
}

void gxdemux_unlock(void)
{
	gx_mutex_unlock(&demux_mutex);
}

void gxdemux_dvr_lock(void)
{
	gx_mutex_lock(&demux_dvr_mutex);
}

void gxdemux_dvr_unlock(void)
{
	gx_mutex_unlock(&demux_dvr_mutex);
}

void gxdemux_tsw_lock(void)
{
	gx_mutex_lock(&demux_tsw_mutex);
}

void gxdemux_tsw_unlock(void)
{
	gx_mutex_unlock(&demux_tsw_mutex);
}

static void dvr_module_full(struct dmxdev *dev, unsigned int sub)
{
	struct gxav_module_inode *inode = dev->inode;
	struct gxav_module_inode *dvr = &inode->dev->modules_inode[GXAV_MOD_DVR];
	unsigned int event = EVENT_MODULE_FULL((0+2*dev->id)) | EVENT_MODULE_FULL((1+2*dev->id));
	gx_wake_event(dvr->module[sub], event);
}

static void demux_irq_thread(void* p)
{
	struct dmxdev *dev = (struct dmxdev *)p;
#ifndef NO_OS
	while(1) {
#endif
		gx_sem_wait_timeout(&(dev->swirq_sem),6000);
#ifndef NO_OS
		if (dev->thread_stop) {
			gx_msleep(100);
			if (gx_thread_should_stop())
				break;
		}
#endif
		if (dev->dvr_ops && dev->dvr_ops->tsw_isr) {
			if (dev->thread_tsw) {
				dvr_module_full(dev, 0);
				dev->dvr_ops->tsw_isr(dev, 0);
			}
		}

		if (dev->gse_ops) {
			if (dev->gse_ops->al_isr)
				dev->gse_ops->al_isr(dev);

			if (dev->gse_ops->pdu_isr)
				dev->gse_ops->pdu_isr(dev);
		}

#ifndef NO_OS
	}
#endif
}

#define DEMUX_THREAD_STACK (8 * 1024)
int demux_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int i = 0, j = 0, ret, max_dmx_unit;
	struct dmxdev *dev = NULL;
	struct dmx_demux *demux = NULL;
	const char* kcache_thread[2] = {"thread-demux0", "thread-demux1"};
	if (device == NULL || inode == NULL) {
		gxlog_e(LOG_DEMUX, "device/inode is NULL!\n");
		return -1;
	}

	max_dmx_unit = gxdmxdev[0].max_dmx_unit;
	for (i=0; i<max_dmx_unit; i++) {
		dev = &gxdmxdev[i];
		dev->id = i;
		if(dev->ops == NULL){
			gxlog_e(LOG_DEMUX, "demux ops is NULL!\n");
			return -1;
		}

		gx_memset(dev->avfifo,      0, sizeof(struct demux_fifo )  * MAX_AVBUF_NUM);
		gx_memset(dev->demuxs,      0, sizeof(struct dmx_demux)    * MAX_DEMUX_UNIT);
		dev->map_filters      = NULL;
		dev->map_filters_size = 0;
		dev->filter_mask      = 0x0ULL;
		dev->map_filter_mask  = 0x0ULL;
		dev->slot_mask        = 0x0ULL;
		dev->avbuf_mask       = 0;
		dev->inode            = inode;

		demux = &(dev->demuxs[0]);
		demux->id = 0;
		demux->dev = dev;
		demux->refcount = 0;
		demux = &(dev->demuxs[1]);
		demux->id = 1;
		demux->dev = dev;
		demux->refcount = 0;
		ret = gx_sem_create(&(dev->swirq_sem), 0);
		if (ret < 0) {
			gxlog_e(LOG_DEMUX, "demux semaphore created fail!\n");
			return -1;
		}

		dev->thread_stack = gx_malloc(DEMUX_THREAD_STACK);
		if (dev->thread_stack == NULL) {
			gxlog_e(LOG_DEMUX, "demux thread stack memory malloc fail!\n");
			dev->thread_stack = NULL;
			return -1;
		}

		ret = gx_thread_create(kcache_thread[i], &(dev->thread_id), demux_irq_thread,
				dev, dev->thread_stack, DEMUX_THREAD_STACK, 9, &(dev->thread_info));
		if (ret < 0) {
			gxlog_e(LOG_DEMUX, "demux thread created fail!\n");
			return -1;
		}

		gx_spin_lock_init(&(dev->spin_lock));
		for (j = 0; j < dev->max_filter; j++)
			gx_spin_lock_init(&(dev->df_spin_lock[j]));
	}
	gx_mutex_init(&(demux_mutex));
	gx_mutex_init(&(demux_dvr_mutex));
	gx_mutex_init(&(demux_tsw_mutex));

#ifdef  FPGA_TEST_STREAM_PRODUCER_EN
	dmx_fpga_init();
#endif
	return 0;
}

int demux_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	int i;
	struct dmxdev *dev;

	if (device == NULL || inode == NULL) {
		gxlog_e(LOG_DEMUX, "device/inode is NULL!\n");
		return -1;
	}

	for(i = 0; i < gxdmxdev[0].max_dmx_unit; i++){
		dev = &gxdmxdev[i];
		gx_memset(dev->avfifo,      0, sizeof(struct demux_fifo )  * MAX_AVBUF_NUM);

		dev->filter_mask      = 0x0ULL;
		dev->slot_mask        = 0x0ULL;
		dev->avbuf_mask       = 0;
		dev->thread_stop      = 1;
		gx_sem_post(&(dev->swirq_sem));
		gx_thread_delete(dev->thread_id);
		gx_sem_delete(&(dev->swirq_sem));
		gx_memset(&(dev->swirq_sem),0,sizeof(gx_sem_id));

		if(dev->thread_stack !=NULL){
			gx_free(dev->thread_stack);
			dev->thread_stack = NULL;
		}
	}

	gx_mutex_destroy(&(demux_mutex));
	return 0;
}

struct dmx_demux *get_subdemux(int id)
{
	id %= 4;
	return &gxdmxdev[id/2].demuxs[id%2];
}

int demux_open(struct gxav_module *module)
{
	int sub = module->sub%4;
	int module_id = module->module_id;
	struct dmx_demux *demux = NULL;

	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count)) {
		gxlog_e(LOG_DEMUX, "module is NULL!\n");
		return -1;
	}

	demux = get_subdemux(sub);
	if (demux->refcount >= GXAV_MAX_HANDLE) {
		gxlog_e(LOG_DEMUX, "refcount is overflow.\n");
		return -1;
	}

	gxdemux_lock();
	if (!(demux->dev->dmx_mask & 0x1<<sub)) {
		int i = 0;
		for (i = 0; i < MAX_FILTER_NUM; i++) {
			if (demux->filters[i]) {
				if ((demux->filters[i]->status != DMX_STATE_DELAY_FREE) &&
					(demux->filters[i]->status != DMX_STATE_FREE)) {
					memset(demux->filters[i], 0, sizeof(struct dmx_filter));
				}
			}
		}
		gx_memset(demux->handles,     0, sizeof(unsigned int)        * GXAV_MAX_HANDLE);
		gx_memset(demux->slots,       0, sizeof(struct dmx_slot *)   * MAX_SLOT_NUM);
		gx_memset(demux->gse_slots,   0, sizeof(struct dmx_gse *)    * MAX_GSE_SLOT);
		SET_MASK(demux->dev->dmx_mask, sub);
	}
	demux->handles[module_id] = module_id;
	demux->refcount++;
	gxdemux_unlock();

	return 0;
}

int demux_close(struct gxav_module *module)
{
	int i;
	int sub = module->sub%4;
	int module_id = module->module_id;
	struct dmx_demux *demux = NULL;
	GxDemuxProperty_Slot pslot;
	GxDemuxProperty_CA pca;
	GxDemuxProperty_CAS pcas;
	GxDemuxProperty_Filter pfilter;
#ifdef CONFIG_AV_MODULE_GSE
	GxDemuxProperty_GseSlot pgse;
#endif

	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count)) {
		gxlog_e(LOG_DEMUX, "module is NULL!\n");
		return -1;
	}

	gxdemux_lock();
	demux = get_subdemux(sub);
	if (module->sub < MAX_DMX_NORMAL) {
		for (i = 0; i < MAX_FILTER_NUM; i++) {
			if (demux->filters[i]) {
				pfilter.filter_id = demux->filters[i]->id;
				demux->filters[i]->status = DMX_STATE_FREE;
				demux_filter_free(demux, &pfilter);
			}
		}
		for (i = 0; i < MAX_SLOT_NUM; i++) {
			if (demux->slots[i]) {
				pslot.slot_id = demux->slots[i]->id;
				demux_slot_free(demux, &pslot);
			}
		}
		for (i = 0; i < MAX_CAS_NUM; i++) {
			struct dmx_cas* cas = demux->dev->cas_configs[i];
			if ((cas != NULL) && (cas->demux == demux)) {
				pcas.cas_id = demux->dev->cas_configs[i]->id;
				demux_cas_free(demux, pcas.cas_id);
			}
		}
	} else if (module->sub < MAX_DMX_TSW_SLOT) {
		for (i = 0; i < MAX_TSW_SLOT; i++) {
			if (demux->dvr.tsw_slot[i].handle) {
				demux_tsw_slot_free(module->sub, i, module->module_id);
			}
 		}
	}
#ifdef CONFIG_AV_MODULE_GSE
	for (i = 0; i < MAX_GSE_SLOT; i++) {
		if (demux->gse_slots[i]) {
			pgse.slot_id = demux->slots[i]->id;
			dmx_gse_slot_free(demux, &pgse);
		}
	}
#endif
	demux->handles[module_id] = module_id;
	demux->refcount--;
	if (!demux->refcount)
		CLR_MASK(demux->dev->dmx_mask, sub);
	gxdemux_unlock();

	return 0;
}

static int demux_event_cb(unsigned int event,int ops,void *priv)
{
	struct gxav_module_inode *inode = (struct gxav_module_inode *)priv;
	return ops ? gxav_module_inode_set_event(inode, event) : gxav_module_inode_clear_event(inode, event);
}

int demux_irq_entry(void *inode,void *dev)
{
	int ret = 0;
	struct dmxdev *dmxdev =(struct dmxdev *)dev;

	if (dmxdev->ops->ts_clk_err_isr)
		dmxdev->ops->ts_clk_err_isr(dmxdev);

	if (dmxdev->ops->df_isr) {
		ret = dmxdev->ops->df_isr(dmxdev);
		if (ret > 0)
			demux_event_cb(ret,1,inode);
	}

	if (dmxdev->ops->df_al_isr) {
		ret = dmxdev->ops->df_al_isr(dmxdev);
		if (ret > 0)
			demux_event_cb(ret,1,inode);
	}

	if (dmxdev->ops->av_gate_isr)
		dmxdev->ops->av_gate_isr(dmxdev);

	if (dmxdev->ops->av_section_isr)
		dmxdev->ops->av_section_isr(dmxdev);

	if (dmxdev->dvr_ops && dmxdev->dvr_ops->tsw_full_isr)
		dmxdev->dvr_ops->tsw_full_isr(dmxdev);

	if (dmxdev->dvr_ops && dmxdev->dvr_ops->tsw_isr_en)
		return dmxdev->dvr_ops->tsw_isr_en(dmxdev);

	return ret;
}

int demux_gse_irq_entry(void *inode,void *dev)
{
	int ret = 0;
	struct dmxdev *dmxdev =(struct dmxdev *)dev;

	if (dmxdev->gse_ops->full_isr)
		dmxdev->gse_ops->full_isr(dmxdev);

	if (dmxdev->gse_ops->slot_close_isr)
		dmxdev->gse_ops->slot_close_isr(dmxdev);

	if (dmxdev->gse_ops->record_finish_isr)
		dmxdev->gse_ops->record_finish_isr(dmxdev);

	if (dmxdev->gse_ops->al_isr_en)
		ret = dmxdev->gse_ops->al_isr_en(dmxdev);

	if (dmxdev->gse_ops->pdu_isr_en)
		ret = dmxdev->gse_ops->pdu_isr_en(dmxdev);

	return ret;
}

struct gxav_module_inode* demux_irq(struct gxav_module_inode *inode, int irq)
{
	int ret = 0;
	struct dmxdev *dev = &gxdmxdev[0];

	if ((irq != dev->irqs[0] && irq != dev->irqs[1] && irq != dev->irqs[2]) || inode == NULL) {
		gxlog_e(LOG_DEMUX, "demux irq %d inode = NULL\n",irq);
		return NULL;
	}

#ifdef CONFIG_AV_MODULE_ICAM
	if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)) {
		extern int  gxav_iemm_interrupt(void) ;
		gxav_iemm_interrupt();
	}
#endif

	if (irq == dev->irqs[0]) {
		dev = &gxdmxdev[0];
		ret = demux_irq_entry(inode, dev);
	} else if (irq == dev->irqs[1]) {
		dev = &gxdmxdev[1];
		ret = demux_irq_entry(inode, dev);
	} else if (irq == dev->irqs[2]) {
		ret = demux_gse_irq_entry(inode, &gxdmxdev[0]);
	}

	if (ret == 1) {
		gx_sem_post(&(dev->swirq_sem));
#ifdef NO_OS
		demux_irq_thread(dev);
#endif
	}
	return inode;
}

int demux_set_entry(struct gxav_module *module, int property_id, void *property, int size)
{
	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count)) {
		gxlog_e(LOG_DEMUX, "module is NULL!\n");
		return -3;
	}

	if (NULL == property) {
		gxlog_e(LOG_DEMUX, "the property param is NULL!\n");
		return -1;
	}

	if (module->sub < MAX_DMX_TSW_SLOT)
		return demux_set_property(module, property_id, property, size);
#ifdef CONFIG_AV_MODULE_PIDFILTER
	else if (module->sub < MAX_DMX_PID_FILTER)
		return dvr_pfilter_set_property(module, property_id, property, size);
#endif
#ifdef CONFIG_AV_MODULE_GSE
	else if (module->sub < MAX_DMX_GSE)
		return dmx_gse_set_property(module, property_id, property, size);
#endif

	return -1;
}

int demux_get_entry(struct gxav_module *module, int property_id, void *property, int size)
{
	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count)) {
		gxlog_e(LOG_DEMUX, "module is NULL!\n");
		return -3;
	}

	if (NULL == property) {
		gxlog_e(LOG_DEMUX, "the property param is NULL!\n");
		return -1;
	}

	if (module->sub < MAX_DMX_TSW_SLOT)
		return demux_get_property(module, property_id, property, size);
#ifdef CONFIG_AV_MODULE_PIDFILTER
	else if (module->sub < MAX_DMX_PID_FILTER)
		return dvr_pfilter_get_property(module, property_id, property, size);
#endif
#ifdef CONFIG_AV_MODULE_GSE
	else if (module->sub < MAX_DMX_GSE)
		return dmx_gse_get_property(module, property_id, property, size);
#endif

	return -1;
}

