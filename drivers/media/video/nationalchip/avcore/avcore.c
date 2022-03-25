#include "profile.h"
#include "sdc_module.h"
#include "include/avcore.h"
#include "include/gxav_firmware.h"
#include "include/gxav_hdcp_key.h"
#include "include/gxav_common.h"
#include "include/kernelcalls.h"
#include "include/gxav_event_type.h"
#include "include/gxav_module_property.h"
#include "include/gxav_module_propertytypes.h"

#define MODULE_ID_VAILD(dev, module_id) ((module_id) >=0 && (module_id < GXAV_MAX_HANDLE) && dev->modules_handle[module_id] != NULL)
#define DEVICE_INACITVE(dev)            (dev == NULL || dev->active != 0)
#define DEVICE_RUN_FUNC(dev,func)       (dev && dev->interface && dev->interface->func)?dev->interface->func(dev):-1

#define DEVICE_ACITVE(dev)                     \
	do {                                   \
		if (av_device_init(dev) == 0)     \
		av_device_register(dev);  \
	}while (0);

static struct gxav_device *device_list = NULL;

static struct gxav_device *devices[32] = {NULL, };

int chip_register(struct gxav_device *dev)
{
	int i = 0;

	GXAV_DBGEV("%s: (dev=%p, dev->device_id=0x%x)\n", __FUNCTION__, dev, dev->id);

	while (devices[i] && i < 31) i++;

	if (i < 31) {
		devices[i] = dev;
		return 0;
	}

	return -1;
}

static int av_device_register(struct gxav_device *dev)
{
	GXAV_DBGEV("%s: (dev=%p, dev->device_id=0x%x)\n", __FUNCTION__, dev, dev->id);

	if (dev) {
		if (device_list == NULL) {
			dev->next = dev->prev = dev;
			device_list = dev;
		} else {
			dev->prev = device_list->prev;
			dev->next = device_list;
			device_list->prev->next = dev;
			device_list->prev = dev;
		}

		return 0;
	}

	return -1;
}

static int av_device_unregister(struct gxav_device *dev)
{
	GXAV_DBGEV("%s: (dev=%p)\n", __FUNCTION__, dev);

	if (dev && dev->prev && dev->next) {
		if (device_list->next == device_list)
			device_list = NULL;
		else if (dev == device_list)
			device_list = dev->next;
		dev->prev->next = dev->next;
		dev->next->prev = dev->prev;
		dev->next = NULL;
		dev->prev = NULL;

		return 0;
	}

	return -1;
}

static int av_device_init(struct gxav_device *dev)
{
	GXAV_DBGEV("%s: (dev=%p)\n", __FUNCTION__, dev);
	if (dev) {
		gx_memset(dev->modules_inode,  0, sizeof(struct gxav_module_inode) * GXAV_MAX_MODULE);
		gx_memset(dev->modules_handle, 0, sizeof(struct gxav_module_handle) * GXAV_MAX_HANDLE);
		gx_memset(dev->irq_list,       0, sizeof(struct gxav_irq) * GXAV_MAX_IRQ);
#ifdef CONFIG_AV_MODULE_FIRMWARE
		gxav_firmware_init();
#endif

#ifdef CONFIG_AV_MODULE_HDMI
		gxav_hdcp_key_init();
#endif
		dev->active = DEVICE_RUN_FUNC(dev, init);
		gx_spin_lock_init(&dev->spin_lock);

		return dev->active;
	}

	return -1;
}

static int av_device_uninit(struct gxav_device *dev)
{
	int ret = 0;
	GXAV_DBGEV("%s: (dev=%p)\n", __FUNCTION__, dev);

	GXAV_ASSERT(dev != NULL);

#ifdef CONFIG_AV_MODULE_HDMI
	gxav_hdcp_key_uninit();
#endif

#ifdef CONFIG_AV_MODULE_FIRMWARE
	gxav_firmware_uninit();
#endif

	if (dev->refcount > 1) {
		dev->refcount = 1;
		gxav_device_release_modules(dev);
		gxav_device_close(dev);
	}

	dev->active = -1;

	if(dev->profile) {
		gx_free(dev->profile);
		dev->profile = NULL;
	}

	ret = DEVICE_RUN_FUNC(dev, cleanup);
	return ret;
}

static void module_irq_register(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	int i = 0, pos, irq;

	GXAV_ASSERT(dev != NULL);
	GXAV_ASSERT(inode != NULL);

	while (inode->interface->irqs[i] != -1 && i < GXAV_MAX_IRQ) {
		irq = inode->interface->irqs[i];
		for (pos = 0; pos < dev->irq_list[irq].irq_count; pos++) {
			if (dev->irq_list[irq].irq_entry[pos].inode == inode)
				return;
		}

		for (pos = 0; pos < dev->irq_list[irq].irq_count; pos++) {
			if (dev->irq_list[irq].irq_entry[pos].inode == NULL)
				break;
		}
		if (pos >= dev->irq_list[irq].irq_count) {
			dev->irq_list[irq].irq_count++;
			dev->irq_list[irq].irq_entry = (struct irq_handler *)gx_realloc(dev->irq_list[irq].irq_entry,
					(dev->irq_list[irq].irq_count-1) * sizeof(struct irq_handler),
					dev->irq_list[irq].irq_count * sizeof(struct irq_handler));
		}
		dev->irq_list[irq].name = inode->interface->irq_names[i];
		dev->irq_list[irq].irq_entry[pos].inode = inode;
		dev->irq_list[irq].irq_entry[pos].irq_func = inode->interface->interrupts[irq];
		if(inode->interface->irq_flags[i] == GXAV_IRQ_FAST)
			dev->irq_list[irq].irq_entry[pos].irq_mode = GXAV_IRQ_FAST;
		else
			dev->irq_list[irq].irq_entry[pos].irq_mode = GXAV_IRQ_NORMAL;

		i++;
	}
}

static void module_irq_unregister(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	int i = 0, pos, irq;

	GXAV_ASSERT(dev != NULL);
	GXAV_ASSERT(inode != NULL);

	while (inode->interface->irqs[i] != -1 && i < GXAV_MAX_IRQ) {
		irq = inode->interface->irqs[i];

		for (pos=0; pos < dev->irq_list[irq].irq_count; pos++) {
			if (dev->irq_list[irq].irq_entry[pos].inode == inode) {
				dev->irq_list[irq].irq_entry[pos].inode = NULL;
				dev->irq_list[irq].irq_entry[pos].irq_func = NULL;
				dev->irq_list[irq].irq_entry[pos].irq_mode = GXAV_IRQ_NORMAL;
				break;
			}
		}

		i++;
	}
}

struct gxav_device *gxav_devices_setup(char* profile)
{
	int chip_id, i =0;

	GXAV_DBGEV("%s\n", __FUNCTION__);

	chip_id = gxcore_chip_probe();

	GXAV_DBGEV("%s,chip_id : 0x%x\n",__FUNCTION__,chip_id);
	while (devices[i]) {
		if (devices[i]->id == chip_id) {
			if(profile) {
				devices[i]->profile = gx_mallocz(gx_strlen(profile)+1);
				gx_strcpy(devices[i]->profile, profile);
			}
			DEVICE_ACITVE(devices[i]);
			break;
		}
		i++;
	}

	return device_list;
}

void gxav_devices_cleanup(void)
{
	struct gxav_device *pos = device_list;

	GXAV_ASSERT(pos != NULL);

	do {
		av_device_uninit(pos);
		av_device_unregister(pos);

		pos = device_list;
	} while (device_list);
}

int gxav_device_open(struct gxav_device *dev)
{
	int i, ret = -1;
	GXAV_DBGEV("%s: (dev=%p)\n", __FUNCTION__, dev);
	GXAV_ASSERT(dev != NULL);

	if (dev->refcount == 0) {
		ret = DEVICE_RUN_FUNC(dev, open);
		if (ret < 0)
			return -1;

		for (i = 0; i < GXAV_MAX_MODULE; i++) {
			struct gxav_module_inode *inode = dev->modules_inode + i;
			if (inode->count > 0) {
				if (inode->interface && inode->interface->init) {
					if (inode->interface->init(dev, inode) < 0){
						gxav_device_close(dev);
						return -1;
					}
				}
				module_irq_register(dev, inode);
			}
		}
		dev->sdc_module_id = gxav_module_open(dev, GXAV_MOD_SDC, 0);
		gx_printf("%s: ok! \n", __FUNCTION__);
	}

#ifdef CONFIG_AV_MODULE_STC
	GxAvSyncParams sync;
	memset(&sync, 0, sizeof(sync));
	sync.pcr_err_gate = 90000;
	sync.pcr_err_time = 135000;
	sync.apts_err_gate = 90000;
	sync.apts_err_time = 135000;
	sync.audio_high_tolerance = 90000;
	sync.audio_low_tolerance = 4500;
	sync.stc_offset = -9000;

	gxav_set_sync_params(&sync);
#endif

	dev->refcount++;

	return 0;
}

int gxav_device_release_modules(struct gxav_device *dev)
{
	int i,ret = -1;
	struct gxav_module* module;
	GXAV_DBGEV("%s: (dev=%p)\n", __FUNCTION__, dev);
	GXAV_ASSERT(dev != NULL);

	dev->refcount--;

	if(dev->refcount == 0){
		for (i = 0; i < GXAV_MAX_HANDLE; i++) {
			if (dev->modules_handle[i] != NULL && i != dev->sdc_module_id) {
				module = dev->modules_handle[i]->module;
				if(module)
					module->refcount = 1;
				ret = gxav_module_close(dev, i);
				if(ret != 0) {
					return ret;
				}
			}
		}

		if (MODULE_ID_VAILD(dev, dev->sdc_module_id)){
			module = dev->modules_handle[dev->sdc_module_id]->module;
			if(module)
				module->refcount = 1;

			ret = gxav_module_close(dev, dev->sdc_module_id);
			if(ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

int gxav_device_close(struct gxav_device *dev)
{
	int i, ret = -1;
	GXAV_DBGEV("%s: (dev=%p)\n", __FUNCTION__, dev);
	GXAV_ASSERT(dev != NULL);

	if (dev->refcount == 0) {
		for (i = 0; i < GXAV_MAX_MODULE; i++) {
			struct gxav_module_inode *inode = dev->modules_inode + i;
			if (inode->count > 0) {
				if (inode->interface && inode->interface->cleanup) {
					inode->interface->cleanup(dev, inode);
				}
				module_irq_unregister(dev, inode);
			}
		}

		ret = DEVICE_RUN_FUNC(dev, close);
		gx_printf("%s: ok! \n", __FUNCTION__);
	}

	return ret;
}

struct gxav_module_inode *gxav_device_interrupt_entry(struct gxav_device *dev, int irq, int isr_dsr)
{
	int i;
	struct irq_handler *handler;
	struct gxav_module_inode *inode = NULL;

	GXAV_DBGEV("%s: (dev=%p, irq=%d)\n", __FUNCTION__, dev, irq);

	GXAV_ASSERT(dev != NULL);
	GXAV_ASSERT(dev->interface != NULL);

	for (i=0; i < dev->irq_list[irq].irq_count; i++) {
		handler = &dev->irq_list[irq].irq_entry[i];
		if (handler->irq_func != NULL && handler->inode != NULL) {
#ifdef ECOS_OS
			inode = handler->inode;
			if( (handler->irq_mode == GXAV_IRQ_FAST   && isr_dsr == GXAV_ISR_ENTRY) || \
				(handler->irq_mode == GXAV_IRQ_NORMAL && isr_dsr == GXAV_DSR_ENTRY) ) {
				gx_interrupt_disable();
				inode = handler->irq_func(handler->inode, irq);
				gx_interrupt_enable();
				if(inode != NULL)
					break;
			}
#else
			inode = handler->irq_func(handler->inode, irq);
#endif
		}
	}

	return inode;
}

int gxav_device_setup(struct gxav_device *dev, struct gxav_device_resource* res)
{
	unsigned int i;

	GXAV_ASSERT(dev != NULL);
	GXAV_ASSERT(res != NULL);

	for (i = 0; i < GXAV_MOD_MAX; i++) {
		struct gxav_module_inode *inode = dev->modules_inode + i;
		struct gxav_module_ops *ops = inode->interface;
		if (ops && ops->setup) {
			ops->setup(dev, &res->module[i]);
		}
	}

	return 0;
}

int gxav_module_register(struct gxav_device *dev, struct gxav_module_ops *ops)
{
	unsigned int i;
	struct gxav_module_inode *inode = dev->modules_inode + ops->module_type;

	GXAV_DBGEV("%s: (dev=%p, module_ops=%p)\n", __FUNCTION__, dev, ops);

	GXAV_ASSERT(dev != NULL);
	GXAV_ASSERT(ops != NULL);

	inode->dev        = dev;
	inode->count      = ops->count;
	inode->interface  = ops;
	inode->event_mask = ops->event_mask;
	inode->refcount   = 0;
	inode->module = (struct gxav_module **)gx_malloc(sizeof(void *) * ops->count);
	if (inode->module == NULL) {
		gx_printf("%s: (module=%p)\n", __FUNCTION__, inode->module);
		return -1;
	}

	for (i = 0; i < ops->count; i++) {
		struct gxav_module *module = (struct gxav_module *)gx_malloc(sizeof(struct gxav_module));
		if (module == NULL) {
			gx_printf("%s: (av module = %p)\n", __FUNCTION__, module);
			return -1;
		}
		gx_memset(module, 0, sizeof(struct gxav_module));

		module->sub = i;
		module->inode = inode;
		module->module_type = ops->module_type;
		module->event_mask  = ops->event_mask;

		inode->module[i] = module;
	}

	return -1;
}

int gxav_module_unregister(struct gxav_device *dev, struct gxav_module_ops *ops)
{
	unsigned int i;

	GXAV_DBGEV("%s: (dev=%p, module_ops=%p, type=%x)\n", __FUNCTION__, dev, ops, ops->module_type);

	GXAV_ASSERT(dev != NULL);
	GXAV_ASSERT(ops != NULL);

	for (i = 0; i < ops->count; i++) {
		gx_free(dev->modules_inode[ops->module_type].module[i]);
		dev->modules_inode[ops->module_type].module[i] = NULL;
	}

	gx_free(dev->modules_inode[ops->module_type].module);
	dev->modules_inode[ops->module_type].module = NULL;

	return 0;
}

int gxav_module_open(struct gxav_device *dev, GxAvModuleType module_type, int sub)
{
	int i;
	struct gxav_module_inode  *inode;

	GXAV_DBGEV("%s: (dev=%p, module_type=%d, sub=%d)\n", __FUNCTION__, dev, module_type, sub);
	GXAV_ASSERT(dev != NULL);

	if (DEVICE_INACITVE(dev) || sub < 0 || module_type < 0 || module_type >= GXAV_MAX_MODULE)
		return -1;

	inode = dev->modules_inode + module_type;

	for (i = 0; i < GXAV_MAX_HANDLE; i++) {
		if (dev->modules_handle[i] == NULL) {
			struct gxav_module_ops *ops = NULL;
			struct gxav_module_handle *handle = NULL;

			if (inode->count > 0 && sub < inode->count) {
				struct gxav_module *module;
				handle = (struct gxav_module_handle *)gx_malloc(sizeof(struct gxav_module_handle));
				if (handle == NULL) {
					gx_printf("%s: (av module handle= %p)\n", __FUNCTION__, handle);
					return -1;
				}

				module = inode->module[sub];
				module->module_id = i;
				module->inode = inode;

				handle->module = module;
				handle->handle = i;

				ops = gxav_module_getops(module);
				if( inode->refcount < 0) {
					gx_printf("inode->refcount < 0\n");
				}
				if (ops && ops->open && module->refcount == 0) {
					if (ops->open(handle->module)) {
						gx_free(handle);
						handle = NULL;
						gx_printf("%s: (av module ops->open fail!)\n", __FUNCTION__);
						return -1;
					}
					gx_mutex_init(&module->lock);
				}

				dev->modules_handle[i] = handle;
				gxav_module_inc(module);

				GXAV_DBGEV("%s: (dev=%p,module_type=%d, sub=%d,module_id=%d ,refcount =%d)\n", __FUNCTION__, dev,
						module_type, sub, i, inode->refcount-1);

				return i;
			} else {
				//gx_printf("%s: (av module(%d) inode->count error, sub(%d) < inode->count(%d))\n", __FUNCTION__, module_type, sub, inode->count);
				return -1;
			}
		}
	}

	return -1;
}

int gxav_module_close(struct gxav_device *dev, int module_id)
{
	struct gxav_module *module;

	GXAV_DBGEV("%s: (dev=%p, module_id=%d)\n", __FUNCTION__, dev, module_id);
	GXAV_ASSERT(dev != NULL);

	if (DEVICE_INACITVE(dev))
		return -1;

	if (MODULE_ID_VAILD(dev, module_id)) {
		if ((module = dev->modules_handle[module_id]->module) != NULL) {
			struct gxav_module_ops *ops = gxav_module_getops(module);

			module->module_id = module_id;
			gxav_module_dec(module);
			if( module->inode->refcount < 0)
				gx_printf("inode->refcount < 0\n");

			if (ops && ops->close && module->refcount == 0) {
				if (ops->close(module)) {
					gx_printf("%s: (av module ops->close fail!)\n", __FUNCTION__);
					return -1;
				}
				gx_memset(module->in_channel, 0, sizeof(struct gxav_channel*) * GXAV_MAX_IO_CHANNEL);
				gx_memset(module->out_channel, 0, sizeof(struct gxav_channel*) * GXAV_MAX_IO_CHANNEL);
			}
			gx_free(dev->modules_handle[module_id]);
			dev->modules_handle[module_id] = NULL;

			return 0;
		}
	}

	return -1;
}

inline int gxav_module_inode_set_event(struct gxav_module_inode *inode, unsigned int event_mask)
{
	if (inode == NULL || DEVICE_INACITVE(inode->dev))
		return -1;//return -2;

	GXAV_DBGEV("%s: (inode=%p, event_mask=0x%x)\n", __FUNCTION__, inode, event_mask);
	inode->event_status |= (event_mask & inode->event_mask);

	return 0;
}

inline int gxav_module_inode_clear_event(struct gxav_module_inode *inode, unsigned int event_mask)
{
	if (inode == NULL || DEVICE_INACITVE(inode->dev))
		return -1;//return -2;

	inode->event_status &= ~(event_mask);

	return 0;
}

int gxav_module_wake_event(struct gxav_module* module, enum gxav_module_event event)
{
	int mask;

	if (event == GXAV_MODULE_EVENT_EMPTY) {
		mask = EVENT_MODULE_EMPTY(module->sub);
	} else {
		mask = EVENT_MODULE_FULL(module->sub);
	}

	return gx_wake_event(module, mask);
}

int gxav_module_read(struct gxav_device *dev, int module_id, void *buf, unsigned int size, int timeout_us)
{
	struct gxav_module *module;
	struct gxav_module_ops *ops;

	GXAV_ASSERT(dev != NULL);

	if (!MODULE_ID_VAILD(dev, module_id))
		return -1;

	if (DEVICE_INACITVE(dev))
		return -1;

	module = dev->modules_handle[module_id]->module;
	if (module == NULL)
		return -1;

	module->module_id = module_id;
	ops = gxav_module_getops(module);
	if (ops && ops->read) {
		int len = 0, ret;
// redo:
		gx_mutex_lock(module->inode->lock);
		ret = ops->read(module, (void *)(buf+len), size-len);
		if(ret < 0) {
			gx_printf("%s :: read fail, %d\n", __func__, len);
			gx_mutex_unlock(module->inode->lock);
			return len;
		}
		gx_mutex_unlock(module->inode->lock);

		len += ret;
#if 0
		if (len < size && timeout_us > 0) {
			int ret = gx_wait_event(module, EVENT_MODULE_FULL(module->sub), timeout_us);
			if(ret < 0) {
				gx_printf("%s :: gx_wait_event fail, len = %d\n", __func__, len);
				return len;
			}
			//gx_printf("%s :: gx_wait_event succ, len = %d\n", __func__, len);
			goto redo;
		}
#endif
		return len;
	}

	return -2;
}

int gxav_module_write(struct gxav_device *dev, int module_id, void *buf, unsigned int size, int timeout_us)
{
	struct gxav_module *module;
	struct gxav_module_ops *ops;

	GXAV_ASSERT(dev != NULL);

	if (!MODULE_ID_VAILD(dev, module_id))
		return -1;

	if (DEVICE_INACITVE(dev))
		return -1;

	module = dev->modules_handle[module_id]->module;
	if (module == NULL)
		return -1;

	module->module_id = module_id;
	ops = gxav_module_getops(module);
	if (ops && ops->write) {
		int len = 0, ret;
//redo:
		gx_mutex_lock(module->inode->lock);
		ret = ops->write(module, (void *)((unsigned int)buf+len), size-len);
		if(ret < 0) {
			gx_mutex_unlock(module->inode->lock);
			return -2;
		}
		gx_mutex_unlock(module->inode->lock);

		len += ret;
#if 0
		if (len < size && timeout_us > 0) {
			int ret = gx_wait_event(module, EVENT_MODULE_EMPTY(module->sub), timeout_us);
			if(ret < 0) {
				//gx_printf("%s :: gx_wait_event fail\n", __func__);
				return len;
			}
			goto redo;
		}
#endif
		return len;
	}

	return -2;
}

int gxav_module_lock(struct gxav_device *dev, int module_id)
{
	struct gxav_module *module;

	GXAV_ASSERT(dev != NULL);

	if (!MODULE_ID_VAILD(dev, module_id))
		return -1;

	if (DEVICE_INACITVE(dev))
		return -1;

	module = dev->modules_handle[module_id]->module;
	if (module == NULL)
		return -1;

	gx_mutex_lock(&module->lock);

	return 0;
}

int gxav_module_trylock(struct gxav_device *dev, int module_id)
{
	struct gxav_module *module;

	GXAV_ASSERT(dev != NULL);

	if (!MODULE_ID_VAILD(dev, module_id))
		return -1;

	if (DEVICE_INACITVE(dev))
		return -1;

	module = dev->modules_handle[module_id]->module;
	if (module == NULL)
		return -1;

	return gx_mutex_trylock(&module->lock);
}

int gxav_module_unlock(struct gxav_device *dev, int module_id)
{
	struct gxav_module *module;

	GXAV_ASSERT(dev != NULL);

	if (!MODULE_ID_VAILD(dev, module_id))
		return -1;

	if (DEVICE_INACITVE(dev))
		return -1;

	module = dev->modules_handle[module_id]->module;
	if (module == NULL)
		return -1;

	gx_mutex_unlock(&module->lock);

	return 0;
}


int gxav_module_set_property(struct gxav_device *dev, int module_id, int property_id, void *property, int size)
{
	int retVal = -2;
	struct gxav_module *module;

	GXAV_DBGEV("%s: (dev=%p, module_id=%d, property_id=%d, property=%p, size=%d)\n",
			__FUNCTION__, dev, module_id, property_id, property, size);

	GXAV_ASSERT(dev != NULL);

	if (!MODULE_ID_VAILD(dev, module_id))
		return -1;

	if (DEVICE_INACITVE(dev))
		return -1;

	if ((module = dev->modules_handle[module_id]->module) != NULL) {
		struct gxav_module_ops *ops = gxav_module_getops(module);

		module->module_id = module_id;
		if (ops && ops->set_property)
			retVal = ops->set_property(module, property_id, property, size);

		if (retVal == -2 && dev->interface != NULL && dev->interface->set_property != NULL)
			retVal = dev->interface->set_property(dev, module_id, property_id, property, size);
	}

	return retVal;
}

int gxav_module_write_ctrlinfo(struct gxav_device *dev, int module_id, void* ctrl_info, int ctrl_size)
{
	int retVal = -2;
	struct gxav_module *module;

	GXAV_DBGEV("%s: (dev=%p, module_id=%d, property_id=%d, property=%p, size=%d)\n",
			__FUNCTION__, dev, module_id, property_id, property, size);

	GXAV_ASSERT(dev != NULL);

	if (!MODULE_ID_VAILD(dev, module_id))
		return -1;

	if (DEVICE_INACITVE(dev))
		return -1;

	if ((module = dev->modules_handle[module_id]->module) != NULL) {
		struct gxav_module_ops *ops = gxav_module_getops(module);

		if (ops && ops->write_ctrlinfo)
			retVal = ops->write_ctrlinfo(module, ctrl_info, ctrl_size);

		if (retVal == -2 && dev->interface != NULL && dev->interface->write_ctrlinfo != NULL)
			retVal = dev->interface->write_ctrlinfo(dev, module_id, ctrl_info, ctrl_size);
	}

	return retVal;
}

int gxav_module_get_property(struct gxav_device *dev, int module_id, int property_id, void *property, int size)
{
	int retVal = -2;
	struct gxav_module *module;

	GXAV_DBGEV("%s: (dev=%p, module_id=%d, property_id=%d, property=%p, size=%d)\n",
			__FUNCTION__, dev, module_id, property_id, property, size);

	GXAV_ASSERT(dev != NULL);

	if (!MODULE_ID_VAILD(dev, module_id))
		return -1;

	if (DEVICE_INACITVE(dev))
		return -1;

	if ((module = dev->modules_handle[module_id]->module) != NULL) {
		struct gxav_module_ops *ops = gxav_module_getops(module);

		module->module_id = module_id;
		if (ops && ops->get_property)
			retVal = ops->get_property(module, property_id, property, size);

		if (retVal == -2 && dev->interface != NULL && dev->interface->get_property != NULL)
			retVal = dev->interface->get_property(dev, module_id, property_id, property, size);
	}

	return retVal;
}

int gxav_module_link(struct gxav_device *dev, int module_id, int dir, int pin_id, struct gxav_channel *channel)
{
	int ret = -1;
	struct gxav_module *module;
	GxAvChanInfo property;
	struct gxav_sdc *module_sdc = dev->modules_inode[GXAV_MOD_SDC].priv;

	GXAV_DBGEV("%s: (dev=%p, module_id=%d, dir=%d, pin_id=%d, channel=%p)\n", __FUNCTION__, dev, module_id,\
			dir, pin_id, channel);
	if (!MODULE_ID_VAILD(dev, module_id))
		goto error;

	if (DEVICE_INACITVE(dev) || channel == NULL ||
			(dir != GXAV_PIN_INPUT && dir != GXAV_PIN_OUTPUT) ||
			(pin_id < 0 || pin_id >= GXAV_MAX_IO_CHANNEL) )
	{
		goto error;
	}

	if (channel->channel_id == -1)
		goto error;

	module = dev->modules_handle[module_id]->module;
	if (dir == GXAV_PIN_INPUT) {
		if (module->in_channel[pin_id] != NULL) {
			gx_printf("module->in_channel[%d] is linked to %p\n", pin_id, module->in_channel[pin_id]);
			goto error;
		}
	}
	else if (dir == GXAV_PIN_OUTPUT) {
		if (module->out_channel[pin_id] != NULL){
			gx_printf("module->out_channel[%d] is linked to %p\n", pin_id, module->out_channel[pin_id]);
			goto error;
		}
	}

	property.dir       = dir;
	property.pin_id    = pin_id;
	property.channel   = channel;

	GXAV_DBGEV("LINK: channel=%p, pin_id=%d, dir=%d\n", channel, pin_id, dir);
	ret = gxav_module_set_property(dev, module_id, GxAVGenericPropertyID_ModuleLinkChannel, \
			&property, sizeof(GxAvChanInfo));

	if (ret != 0) {
		gx_printf("gxav_module_set_property Link error\n");
		goto error;
	}

	if (dir == GXAV_PIN_INPUT) {
		module->in_channel[pin_id] = channel;

		channel->outpin_id = pin_id;
		channel->flag &= W;

		if(channel->type & GXAV_PTS_FIFO && channel->pts_channel_id != -1) {
			module_sdc->channel[channel->pts_channel_id].flag &= W;
		}
	}
	else {
		module->out_channel[pin_id] = channel;

		channel->inpin_id = pin_id;
		channel->flag &= R;

		if(channel->type & GXAV_PTS_FIFO && channel->pts_channel_id != -1) {
			module_sdc->channel[channel->pts_channel_id].flag &= R;
		}
	}

	return 0;

error:
	return -1;
}

int gxav_module_unlink(struct gxav_device *dev, int module_id, struct gxav_channel *channel)
{
	struct gxav_module *module;
	int dir, pin_id, ret;
	GxAvChanInfo property;

	GXAV_DBGEV("%s: (dev=%p, module_id=%d, channel=%p)\n", __FUNCTION__, dev, module_id, channel);
	if (!MODULE_ID_VAILD(dev, module_id))
		goto error;

	if (DEVICE_INACITVE(dev) || channel == NULL)
		goto error;

	module = dev->modules_handle[module_id]->module;

	if (module == NULL)
		goto error;

	if (channel == module->in_channel[channel->outpin_id]) {
		dir    = GXAV_PIN_INPUT;
		pin_id = channel->outpin_id;
	}
	else if (channel == module->out_channel[channel->inpin_id]) {
		dir    = GXAV_PIN_OUTPUT;
		pin_id = channel->inpin_id;
	}
	else {
		gx_printf("not unlink\n");
		goto error;
	}

	if ( (dir != GXAV_PIN_INPUT && dir != GXAV_PIN_OUTPUT) || (pin_id < 0 || pin_id >= GXAV_MAX_IO_CHANNEL) )
		goto error;

	property.channel = channel;
	property.pin_id  = pin_id;
	property.dir     = dir;

	GXAV_DBGEV("UNLINK: channel=%p, pin_id=%d, dir=%d\n", channel, pin_id, dir);

	ret = gxav_module_set_property(dev, module_id, GxAVGenericPropertyID_ModuleUnLinkChannel,\
			&property, sizeof(GxAvChanInfo));

	if (ret != 0) {
		gx_printf("gxav_module_set_property unink error\n");
		goto error;
	}

#ifdef CONFIG_AV_MODULE_SDC
	if (dir == GXAV_PIN_INPUT) {
		unsigned long flags;
		struct gxav_sdc *module_sdc = dev->modules_inode[GXAV_MOD_SDC].priv;
		module->in_channel[pin_id] = NULL;

		flags = gx_sdc_spin_lock_irqsave(channel->channel_id);
		channel->flag |= R;
		channel->outpin_id = -1;
		channel->indata = NULL;
		channel->incallback = NULL;

		if(channel->type & GXAV_PTS_FIFO && channel->pts_channel_id != -1) {
			module_sdc->channel[channel->pts_channel_id].flag |= R;
		}
		gx_sdc_spin_unlock_irqrestore(channel->channel_id, flags);
	}
	else {
		unsigned long flags;
		struct gxav_sdc *module_sdc = dev->modules_inode[GXAV_MOD_SDC].priv;
		module->out_channel[pin_id] = NULL;

		flags = gx_sdc_spin_lock_irqsave(channel->channel_id);
		channel->flag |= W;
		channel->inpin_id = -1;
		channel->outdata = NULL;
		channel->outcallback = NULL;

		if(channel->type & GXAV_PTS_FIFO && channel->pts_channel_id != -1) {
			module_sdc->channel[channel->pts_channel_id].flag |= W;
		}
		gx_sdc_spin_unlock_irqrestore(channel->channel_id, flags);
	}
#endif
	//gxav_channel_reset(dev, channel);

	return 0;
error:
	return -1;
}

void gxav_device_spin_lock(struct gxav_device *dev)
{
	gx_spin_lock(&dev->spin_lock);
}

void gxav_device_spin_unlock(struct gxav_device *dev)
{
	gx_spin_unlock(&dev->spin_lock);
}

unsigned long gxav_device_spin_lock_irqsave(struct gxav_device *dev)
{
	unsigned long flag = 0;
	gx_spin_lock_irqsave(&dev->spin_lock, flag);

	return flag;
}

void gxav_device_spin_unlock_irqrestore(struct gxav_device *dev, unsigned long flag)
{
	gx_spin_unlock_irqrestore(&dev->spin_lock, flag);
}

int gxav_device_capability(struct gxav_device *dev, struct gxav_device_capability *cap)
{
	int i, ret = -1;
	GXAV_DBGEV("%s: (dev=%p)\n", __FUNCTION__, dev);
	GXAV_ASSERT(dev != NULL);

	if (dev->refcount > 0) {
		for (i = 0; i < GXAV_MAX_MODULE; i++) {
			struct gxav_module_inode *inode = dev->modules_inode + i;
			cap->module_cnt[i] = inode->count;
		}
		return 0;
	}

	return ret;

}

EXPORT_SYMBOL(gxav_device_spin_lock_irqsave);
EXPORT_SYMBOL(gxav_device_spin_unlock_irqrestore);
