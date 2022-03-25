#include "gxse_core.h"
#include "gxse_hwobj_tfm.h"

#ifdef CPU_ACPU

#if (defined (LINUX_OS) && (defined (CONFIG_SIRIUS))) || (!defined (LINUX_OS))
#if defined (CFG_GXSE_KLM) && (defined (CFG_GXSE_KLM_IRDETO) || defined (CFG_GXSE_KLM_GENERIC))
gx_mutex_t sirius_klm_mutex;
#endif
#endif

#if (defined (LINUX_OS) && (!defined (CONFIG_SIRIUS))) || (!defined (LINUX_OS))
#if defined (CFG_GXSE_CRYPTO) || defined (CFG_GXSE_KLM)
gx_mutex_t gx3211_mtc_mutex;
#endif
#endif

int32_t gxse_hwobj_mutex_static_init(GxSeModuleHwObj *obj)
{
	struct mutex_static_priv *p = obj->priv;
	if (NULL == p)
		return GXSE_ERR_GENERIC;
	gx_mutex_init(&p->mutex);
	obj->mutex = &p->mutex;
	return GXSE_SUCCESS;
}

int32_t gxse_hwobj_mutex_static_deinit(GxSeModuleHwObj *obj)
{
	struct mutex_static_priv *p = obj->priv;
	if (NULL == p)
		return GXSE_ERR_GENERIC;
	gx_mutex_destroy(&p->mutex);
	obj->mutex = NULL;
	return GXSE_SUCCESS;
}

#include "common_dev.h"
static GxSeSecureMem gx_secmem_list[GXSE_MAX_SECMEM];
static void gxse_secmem_register_byname(char *name)
{
	struct gx_mem_info info;
	int ret = gx_mem_info_get(name, &info);
	if (ret == 0 && info.start) {
		ret = gxse_secmem_register(info.start, info.size);
	}
}

static void gxse_secmem_init(void)
{
	static int init = 0;
	if (init == 0) {
		memset(gx_secmem_list, 0, sizeof(GxSeSecureMem) * GXSE_MAX_SECMEM);
		gxse_secmem_register_byname((char *)"vfwmem");
		gxse_secmem_register_byname((char *)"afwmem");
		gxse_secmem_register_byname((char *)"secmem");
		gxse_secmem_register_byname((char *)"tsrmem");
		gxse_secmem_register_byname((char *)"tswmem");
		init = 1;
	}
}

int32_t gxse_secmem_is_illegal(char *name, uint32_t addr, uint32_t size)
{
	struct gx_mem_info info;
	int32_t ret = gx_mem_info_get(name, &info);
	if(ret == 0 && info.start) {
		if (!((INTEGER_MAX_VALUE-addr >= size) &&
			(INTEGER_MAX_VALUE-info.start >= info.size) &&
			(addr >= info.start) && (addr+size <= info.start + info.size)))
			return 1;
	}
	return 0;
}

int32_t gxse_secmem_probe_byname(char *name, uint32_t *addr, uint32_t *size)
{
	struct gx_mem_info info;
	int32_t ret = gx_mem_info_get(name, &info);
	if(ret == 0 && info.start) {
		if ((ret = gxse_secmem_probe(info.start, info.size)) >= 0) {
			if (addr)
				*addr = info.start;
			if (size)
				*size = info.size;
			return ret;
		}
	}
	return GXSE_MEM_OUTSIDE;
}

int32_t gxse_secmem_probe(uint32_t addr, uint32_t size)
{
	int32_t i = 0, idle = 0;
	uint32_t mstart = 0, msize = 0;

	for (i = 0; i < GXSE_MAX_SECMEM; i++) {
		if (gx_secmem_list[i].phys == 0) {
			idle = 1;
			continue;
		}

		mstart = gx_secmem_list[i].phys;
		msize = gx_secmem_list[i].size;

		if (mstart > addr + size || mstart + msize <= addr)
			continue;

		if (mstart <= addr && mstart + msize >= addr + size) {
			if (mstart == addr && mstart + msize == addr + size)
				return i;

			return GXSE_MEM_INSIDE;
		}
		return GXSE_MEM_CROSSOVER;
	}

	if (idle == 0) {
		gxlog_e(GXSE_LOG_MOD_DEV, "secmem is full!\n");
		return GXSE_MEM_FULL;
	}

	return GXSE_MEM_OUTSIDE;
}

int32_t gxse_secmem_register(uint32_t addr, uint32_t size)
{
	int32_t i = 0;
	int32_t ret = 0;

	if ((ret = gxse_secmem_probe(addr, size)) >= 0)
		return GXSE_SUCCESS;

	if (ret == GXSE_MEM_INSIDE || ret == GXSE_MEM_CROSSOVER || ret == GXSE_MEM_FULL)
		return GXSE_ERR_GENERIC;

	for (i = 0; i < GXSE_MAX_SECMEM; i++) {
		if (gx_secmem_list[i].phys == 0) {
			gx_secmem_list[i].phys = addr;
			gx_secmem_list[i].size = size;
			break;
		}
	}

	if (i == GXSE_MAX_MEMHOLE)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

static GxSeMemhole gx_memhole_list[GXSE_MAX_MEMHOLE];
static void gxse_memhole_init(void)
{
	static int init = 0;
	if (init == 0) {
		memset(gx_memhole_list, 0, sizeof(GxSeMemhole) * GXSE_MAX_MEMHOLE);
		init = 1;
	}
}

int32_t gxse_memhole_probe(uint32_t addr, uint32_t size, uint32_t *_addr, uint32_t flags)
{
	int32_t i = 0, idle = 0;
	uint32_t mstart = 0, msize = 0;

	gxse_memhole_init();

	for (i = 0; i < GXSE_MAX_MEMHOLE; i++) {
		if (gx_memhole_list[i].phys == 0) {
			idle = 1;
			continue;
		}

		mstart = (flags == GXSE_MEM_VIRT) ? gx_memhole_list[i].virt : gx_memhole_list[i].phys;
		msize = gx_memhole_list[i].size;
		if (mstart > addr + size || mstart + msize <= addr)
			continue;

		if (mstart <= addr && mstart + msize >= addr + size) {
			if (mstart == addr && mstart + msize == addr + size) {
				if (_addr)
					*_addr = (flags == GXSE_MEM_VIRT) ? gx_memhole_list[i].phys : gx_memhole_list[i].virt;
				return i;
			}

			return GXSE_MEM_INSIDE;
		}
		return GXSE_MEM_CROSSOVER;
	}

	if (idle == 0) {
		gxlog_e(GXSE_LOG_MOD_DEV, "memhole is full!\n");
		return GXSE_MEM_FULL;
	}

	return GXSE_MEM_OUTSIDE;
}

int32_t gxse_memhole_probe_addr(uint32_t addr, uint32_t *_addr, uint32_t flags)
{
	int32_t i = 0;
	uint32_t mvirt = 0, mphys = 0, msize = 0;

	gxse_memhole_init();

	for (i = 0; i < GXSE_MAX_MEMHOLE; i++) {
		if (gx_memhole_list[i].phys == 0) {
			continue;
		}

		mvirt = gx_memhole_list[i].virt;
		mphys = gx_memhole_list[i].phys;
		msize = gx_memhole_list[i].size;

		if (flags == GXSE_MEM_VIRT) {
			if (addr >= mvirt && addr < mvirt + msize) {
				*_addr = mphys + (addr - mvirt);
				return i;
			}
		} else {
			if (addr >= mphys && addr < mphys + msize) {
				*_addr = mvirt + (addr - mphys);
				return i;
			}
		}
	}

	return GXSE_MEM_OUTSIDE;
}

int32_t gxse_memhole_register(uint32_t vaddr, uint32_t paddr, uint32_t size)
{
	int32_t i = 0;
	int32_t ret = 0;

	gxse_memhole_init();
	if ((ret = gxse_memhole_probe(vaddr, size, NULL, GXSE_MEM_VIRT)) >= 0)
		return GXSE_SUCCESS;

	if (ret == GXSE_MEM_INSIDE || ret == GXSE_MEM_CROSSOVER || ret == GXSE_MEM_FULL)
		return GXSE_ERR_GENERIC;

	for (i = 0; i < GXSE_MAX_MEMHOLE; i++) {
		if (gx_memhole_list[i].phys == 0) {
			gx_memhole_list[i].phys = paddr;
			gx_memhole_list[i].virt = vaddr;
			gx_memhole_list[i].size = size;
			break;
		}
	}

	if (i == GXSE_MAX_MEMHOLE)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

static GxSeDeviceIRQ gxse_irq_list[GXSE_MAX_IRQ];
void gxse_irqlist_init(void)
{
	static int init = 0;
	if (init == 0) {
		int i = 0;
		for (i = 0; i < GXSE_MAX_IRQ; i++) {
			memset(&gxse_irq_list[i], 0, sizeof(GxSeDeviceIRQ));
			gxse_irq_list[i].irq = -1;
		}
		init = 1;
	}
}

GxSeDeviceIRQ *gxse_irqlist_get(uint16_t irq)
{
	int i = 0, idle = -1;
	for (i = 0; i < GXSE_MAX_IRQ; i++) {
		if (idle < 0 && gxse_irq_list[i].irq == -1)
			idle = i;

		if (gxse_irq_list[i].irq == irq)
			return &gxse_irq_list[i];
	}

	if (idle >= 0) {
		gxse_irq_list[idle].id = idle;
		gxse_irq_list[idle].irq = irq;
		return &gxse_irq_list[idle];
	}

	return NULL;
}

int32_t gxse_irqlist_remove(uint32_t id)
{
	if (id >= GXSE_MAX_IRQ)
		return GXSE_ERR_GENERIC;

	gxse_irq_list[id].irq = -1;
	return GXSE_SUCCESS;
}

int32_t gxse_device_init(GxSeDevice *device)
{
	int32_t i = 0;
	GxSeModuleResource *res = NULL;

	if ((device->module = gxse_module_find_by_devname(device->devname)) == NULL) {
		gxlog_d(GXSE_LOG_MOD_DEV, "Can't find the module (%s).\n", device->devname);
		return GXSE_ERR_GENERIC;
	}

	if (device->refcount)
		return GXSE_ERR_GENERIC;

	res = &device->module->res;
	for (i = 0; i < GXSE_MAX_MOD_IRQ; i++) {
		if (res->irqs[i] < 0)
			break;

		if (gx_osdep_probe_irq_byname(res->irq_names[i], &res->irqs[i]) < 0) {
			res->irqs[i] = -2;
			continue;
		}

		if (res->irqs[i] > 0) {
			if (gx_osdep_request_irq(res->irqs[i], gxse_os_dev_isr, gxse_os_dev_dsr, device) < 0) {
				res->irqs[i] = -2;
				continue;
			}
		}
	}

	gx_mutex_init(&device->mutex);
	device->refcount++;

	return GXSE_SUCCESS;
}

int32_t gxse_device_deinit(GxSeDevice *device)
{
	int32_t i = 0;
	GxSeModuleResource *res = NULL;

	if (NULL == device || NULL == device->module)
		return GXSE_ERR_GENERIC;

	if (device->refcount != 1)
		return GXSE_ERR_GENERIC;

	gx_mutex_lock(&device->mutex);
	gxse_module_unregister(device->module);
	res = &device->module->res;
	if (device->module) {
		for (i = 0; i < GXSE_MAX_MOD_IRQ; i++) {
			if (res->irqs[i] < 0)
				break;

			gx_osdep_free_irq(res->irqs[i], device);
		}
	}
	device->refcount--;
	gx_mutex_unlock(&device->mutex);
	gx_mutex_destroy(&device->mutex);
	return GXSE_SUCCESS;
}

int32_t gxse_device_open(GxSeDevice *device)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == device || NULL == device->module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	if (device->refcount == 0)
		return GXSE_ERR_GENERIC;

	if (device->refcount > 1) {
		device->refcount++;
		return GXSE_SUCCESS;
	}

	gx_mutex_lock(&device->mutex);
	mod = device->module;
	if (mod_ops(mod)->open) {
		if ((ret = mod_ops(mod)->open(mod)) == GXSE_SUCCESS)
			device->refcount++;
	}
	gx_mutex_unlock(&device->mutex);

	return ret;
}

int32_t gxse_device_close(GxSeDevice *device)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == device || NULL == device->module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	if (device->refcount < 2)
		return GXSE_SUCCESS;

	if (device->refcount > 2) {
		device->refcount--;
		return GXSE_SUCCESS;
	}

	gx_mutex_lock(&device->mutex);
	mod = device->module;
	if (mod_ops(mod)->close)
		ret = mod_ops(mod)->close(mod);
	device->refcount--;
	gx_mutex_unlock(&device->mutex);

	return ret;
}

int32_t gxse_device_ioctl(GxSeDevice *device, uint32_t cmd, void *arg, size_t size)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;
	void *param = NULL;

	if (NULL == device || NULL == device->module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	gx_mutex_lock(&device->mutex);
	mod = device->module;

	if (size && mod_ops(mod)->copy_from_usr) {
		if (mod_ops(mod)->copy_from_usr(&param, arg, size, cmd) < 0) {
			gxlog_e(GXSE_LOG_MOD_DEV, "copy from usr error\n");
			goto err;
		}
	} else
		param = arg;

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, cmd, param, size);

err:
	if (size) {
		if (mod_ops(mod)->copy_to_usr)
			mod_ops(mod)->copy_to_usr(arg, &param, size, cmd);
	}
	gx_mutex_unlock(&device->mutex);

	return ret;
}

int32_t gxse_device_poll(GxSeDevice *device, int32_t which, void **r_wait, void **w_wait)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == device || NULL == device->module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	gx_mutex_lock(&device->mutex);
	mod = device->module;

	if (mod_ops(mod)->poll)
		ret = mod_ops(mod)->poll(mod, which, r_wait, w_wait);

	gx_mutex_unlock(&device->mutex);

	return ret;
}

int32_t gxse_device_read(GxSeDevice *device, uint8_t *buf, size_t size)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;
	void *param = NULL;

	if (NULL == device || NULL == device->module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	if (NULL == buf || size == 0) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Buffer is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	gx_mutex_lock(&device->mutex);
	mod = device->module;

	if (size && mod_ops(mod)->copy_from_usr) {
		if (mod_ops(mod)->copy_from_usr(&param, buf, size, GXSE_CMD_RX) < 0) {
			gxlog_e(GXSE_LOG_MOD_DEV, "copy from usr error\n");
			goto err;
		}
	} else
		param = buf;

	if (mod_ops(mod)->read)
		ret = mod_ops(mod)->read(mod, param, size);

err:
	if (size) {
		if (mod_ops(mod)->copy_to_usr)
			mod_ops(mod)->copy_to_usr(buf, &param, size, GXSE_CMD_RX);
	}
	gx_mutex_unlock(&device->mutex);

	return ret;
}

int32_t gxse_device_write(GxSeDevice *device, const uint8_t *buf, size_t size)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;
	void *param = NULL;

	if (NULL == device || NULL == device->module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	if (NULL == buf || size == 0) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Buffer is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	gx_mutex_lock(&device->mutex);
	mod = device->module;
	if (size && mod_ops(mod)->copy_from_usr) {
		if (mod_ops(mod)->copy_from_usr(&param, (void *)buf, size, GXSE_CMD_TX) < 0) {
			gxlog_e(GXSE_LOG_MOD_DEV, "copy from usr error\n");
			goto err;
		}
	} else
		param = (void *)buf;

	if (mod_ops(mod)->write)
		ret = mod_ops(mod)->write(mod, param, size);
err:
	if (size) {
		if (mod_ops(mod)->copy_to_usr)
			mod_ops(mod)->copy_to_usr((void *)buf, &param, size, GXSE_CMD_TX);
	}
	gx_mutex_unlock(&device->mutex);

	return ret;
}

int32_t gxse_device_isr(GxSeDevice *device)
{
	GxSeModule *mod = NULL;

	if (NULL == device || NULL == device->module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	mod = device->module;
	if (mod_ops(mod)->isr)
		mod_ops(mod)->isr(mod);

	return GXSE_SUCCESS;
}

int32_t gxse_device_dsr(GxSeDevice *device)
{
	GxSeModule *mod = NULL;

	if (NULL == device || NULL == device->module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	mod = device->module;
	if (mod_ops(mod)->dsr)
		mod_ops(mod)->dsr(mod);

	return GXSE_SUCCESS;
}

int32_t gxse_module_devname_2_moduleid(const char *devname)
{
	int32_t i = 0, base = 0, sub = 0, sub2nd = 0;

	if (memcmp(devname, GXSE_CRYPTO_DEVNAME, sizeof(GXSE_CRYPTO_DEVNAME) - 1) == 0)
		base = GXSE_MOD_CRYPTO_FIFO;
	else if (memcmp(devname, GXSE_KLM_DEVNAME, sizeof(GXSE_KLM_DEVNAME) - 1) == 0)
		base = GXSE_MOD_KLM_GENERIC;
	else if (memcmp(devname, GXSE_FIREWALL_DEVNAME, sizeof(GXSE_FIREWALL_DEVNAME) - 1) == 0)
		base = GXSE_MOD_MISC_FIREWALL;
	else if (memcmp(devname, GXSE_SECURE_DEVNAME, sizeof(GXSE_SECURE_DEVNAME) - 1) == 0)
		base = GXSE_MOD_SECURE_MBOX;
	else if (memcmp(devname, GXSE_MISC_DEVNAME, sizeof(GXSE_MISC_DEVNAME) - 1) == 0)
		base = GXSE_MOD_MISC_CHIP_CFG;
	else {
		gxlog_e(GXSE_LOG_MOD_DEV, "Unexpected devname %s.\n", devname);
		return GXSE_ERR_GENERIC;
	}

	for (i = 0; i < GXSE_MAX_DEV_NAME; i++) {
		if (devname[i] == '\0')
			break;

		if (devname[i] < '0' || devname[i] > '9')
			continue;

		sub = devname[i] - '0';
		if (i + 4 < GXSE_MAX_DEV_NAME &&
			devname[i+1] == '/' &&
			devname[i+2] == 'c' &&
			devname[i+3] == 'h') {
			sub2nd = devname[i+4] - '0';
			break;
		}
	}
	return base + sub + sub2nd;
}

GxSeModule *gxse_module_find_by_devname(const char *devname)
{
	int id = gxse_module_devname_2_moduleid(devname);
	if (id < 0)
		return NULL;
	return gxse_module_find_by_id(id);
}
#endif

static GxSeModule *gx_module_list[GXSE_MAX_MOD_COUNT] = {0};
GxSeModule *gxse_module_find_by_id(GxSeModuleID id)
{
	if (id >= GXSE_MAX_MOD_COUNT)
		return NULL;
	return gx_module_list[id];
}

GxSeModule *gxse_module_find_by_tfmid(GxTfmModule mod, uint32_t sub)
{
	uint32_t __mod = mod;
	GxSeModuleID id = GXSE_MOD_CRYPTO_MAX;

	switch (__mod) {
	case TFM_MOD_KLM_SCPU_STATIC:      ///< scpu 静态KLM 模块
		id = GXSE_MOD_KLM_SCPU_IRDETO;
		break;

	case TFM_MOD_KLM_SCPU_DYNAMIC:     ///< scpu 动态KLM 模块
		id = GXSE_MOD_KLM_SCPU_IRDETO_GENERIC;
		break;

	case TFM_MOD_KLM_SCPU_NC_DYNAMIC:  ///< scpu 国芯动态KLM 模块
		id = GXSE_MOD_KLM_SCPU_GENERIC;
		break;

	case TFM_MOD_KLM_IRDETO:           ///< acpu irdeto KLM 模块
		id = GXSE_MOD_KLM_IRDETO;
		break;

	case TFM_MOD_KLM_GENERAL:          ///< acpu 通用KLM 模块
	case TFM_MOD_KLM_MTC:              ///< acpu MTC klm模块
		id = GXSE_MOD_KLM_GENERIC;
		break;

	case TFM_MOD_CRYPTO:               ///< acpu crypto 模块
		id = GXSE_MOD_CRYPTO_FIFO + sub;
		break;

	case TFM_MOD_M2M:                  ///< acpu m2m/mtc 模块
		id = GXSE_MOD_CRYPTO_DMA0 + sub;
		break;

	case TFM_MOD_MISC_DGST:            ///< acpu hash 模块
		id = GXSE_MOD_MISC_DGST;
		break;

#ifdef CPU_ACPU
	case TFM_MOD_MISC_AKCIPHER:        ///< acpu akcipher 模块
		id = GXSE_MOD_MISC_AKCIPHER;
		break;
#endif

	default:
		return NULL;
	}

	return gxse_module_find_by_id(id);
}

int32_t gxse_module_ioctl_check(GxSeModule *module, GxSeModuleHwObjType type, uint32_t cmd, uint32_t size)
{
	GxSeModuleHwObj *obj = NULL;

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "module is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	obj = module->hwobj;
    if (obj->type != type) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Object type is error. module = 0x%x\n", module->id);
		return GXSE_ERR_GENERIC;
    }

	if (size != GXSE_IOLEN(cmd)) {
		gxlog_e(GXSE_LOG_MOD_DEV, "The cmd length is error. cmd = 0x%x, size = 0x%x\n", cmd, size);
		return GXSE_ERR_GENERIC;
	}

	return GXSE_SUCCESS;
}

int32_t gxse_module_register(GxSeModule *module)
{
#ifdef CPU_ACPU
	gxse_secmem_init();
	gxse_irqlist_init();
#endif

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "param is NULL\n");
		return GXSE_ERR_GENERIC;
	}

	if (gx_module_list[module->id])
		return GXSE_SUCCESS;

	if (mod_ops(module)->init) {
		if (mod_ops(module)->init(module) < 0) {
			gxlog_e(GXSE_LOG_MOD_DEV, "module init failed, moduleid = %d\n", module->id);
			return GXSE_ERR_GENERIC;
		}
	}

	gx_module_list[module->id] = module;
	return GXSE_SUCCESS;
}

int32_t gxse_module_init(GxSeModule *module)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleDevOps *ops = NULL;
	GxSeModule *mod = (GxSeModule *)module;

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "module is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	obj = mod->hwobj;
	ops = (GxSeModuleDevOps *)GXSE_OBJ_DEVOPS(obj);

	if (mod->res.reg_base && mod->res.reg_len) {
		if (!gx_request_mem_region(mod->res.reg_base, mod->res.reg_len)) {
			gxlog_e(GXSE_LOG_MOD_DEV, "request_mem_region failed. %x %x\n", mod->res.reg_base, mod->res.reg_len);
			return GXSE_ERR_GENERIC;
		}

		if (NULL == (obj->reg = gx_ioremap(mod->res.reg_base, mod->res.reg_len))) {
			gxlog_e(GXSE_LOG_MOD_DEV, "ioremap address failed.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	if (ops->init) {
		if ((ret = ops->init(obj)) < 0) {
			gxlog_e(GXSE_LOG_MOD_DEV, "module init failed, id = %d.\n", module->id);
			return ret;
		}
	}

#ifdef CPU_ACPU
	if (ops->setup)
		ret = ops->setup(obj, &module->res);
#endif

	return ret;
}

#ifdef CPU_ACPU
int32_t gxse_module_deinit(GxSeModule *module)
{
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleDevOps *ops = NULL;
	GxSeModule *mod = (GxSeModule *)module;

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "module is NULL.\n");
		return GXSE_ERR_GENERIC;
	}

	obj = mod->hwobj;
	ops = (GxSeModuleDevOps *)GXSE_OBJ_DEVOPS(obj);

	if (ops->deinit)
		ops->deinit(obj);

	if (obj->reg) {
		gx_iounmap(obj->reg);
		gx_release_mem_region(mod->res.reg_base, mod->res.reg_len);
	}

	return GXSE_SUCCESS;
}

int32_t gxse_module_unregister(GxSeModule *module)
{
	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "param is NULL\n");
		return GXSE_ERR_GENERIC;
	}

	if (NULL == gx_module_list[module->id]) {
		gxlog_e(GXSE_LOG_MOD_DEV, "The module %d has been registered.\n", module->id);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(module)->deinit) {
		if (mod_ops(module)->deinit(module) < 0) {
			gxlog_e(GXSE_LOG_MOD_DEV, "param is NULL\n");
			return GXSE_ERR_GENERIC;
		}
	}

	gx_module_list[module->id] = NULL;
	return GXSE_SUCCESS;
}

int32_t gxse_module_open(GxSeModule *module)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleDevOps *ops = NULL;
	GxSeModule *mod = (GxSeModule *)module;

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "module is NULL.\n");
		return -1;
	}

	obj = mod->hwobj;
	ops = (GxSeModuleDevOps *)GXSE_OBJ_DEVOPS(obj);

	gx_mutex_lock(obj->mutex);
	if (ops->open)
		ret = ops->open(obj);
	gx_mutex_unlock(obj->mutex);

	return ret;
}

int32_t gxse_module_close(GxSeModule *module)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleDevOps *ops = NULL;
	GxSeModule *mod = (GxSeModule *)module;

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "module is NULL.\n");
		return -1;
	}

	obj = mod->hwobj;
	ops = (GxSeModuleDevOps *)GXSE_OBJ_DEVOPS(obj);

	gx_mutex_lock(obj->mutex);
	if (ops->close)
		ret = ops->close(obj);
	gx_mutex_unlock(obj->mutex);

	return ret;
}

int32_t gxse_module_read(GxSeModule *module, uint8_t *buf, uint32_t len)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleDevOps *ops = NULL;
	GxSeModule *mod = (GxSeModule *)module;

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "module is NULL.\n");
		return -1;
	}

	obj = mod->hwobj;
	ops = (GxSeModuleDevOps *)GXSE_OBJ_DEVOPS(obj);

	gx_mutex_lock(obj->mutex);
	if (ops->read)
		ret = ops->read(obj, buf, len);
	gx_mutex_unlock(obj->mutex);

	return ret;
}

int32_t gxse_module_write(GxSeModule *module, const uint8_t *buf, uint32_t len)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleDevOps *ops = NULL;
	GxSeModule *mod = (GxSeModule *)module;

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "module is NULL.\n");
		return -1;
	}

	obj = mod->hwobj;
	ops = (GxSeModuleDevOps *)GXSE_OBJ_DEVOPS(obj);

	gx_mutex_lock(obj->mutex);
	if (ops->write)
		ret = ops->write(obj, buf, len);
	gx_mutex_unlock(obj->mutex);

	return ret;
}

int32_t gxse_module_isr(GxSeModule *module)
{
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleDevOps *ops = NULL;
	GxSeModule *mod = (GxSeModule *)module;

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "module is NULL.\n");
		return -1;
	}

	obj = mod->hwobj;
	ops = (GxSeModuleDevOps *)GXSE_OBJ_DEVOPS(obj);

	if (ops->isr)
		ops->isr(obj);

	return GXSE_SUCCESS;
}

int32_t gxse_module_dsr(GxSeModule *module)
{
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleDevOps *ops = NULL;
	GxSeModule *mod = (GxSeModule *)module;

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "module is NULL.\n");
		return -1;
	}

	obj = mod->hwobj;
	ops = (GxSeModuleDevOps *)GXSE_OBJ_DEVOPS(obj);

	if (ops->dsr)
		ops->dsr(obj);

	return GXSE_SUCCESS;
}

int32_t gxse_module_poll(GxSeModule *module, int32_t which, void **r_wait, void **w_wait)
{
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleDevOps *ops = NULL;
	GxSeModule *mod = (GxSeModule *)module;

	if (NULL == module) {
		gxlog_e(GXSE_LOG_MOD_DEV, "module is NULL.\n");
		return -1;
	}

	obj = mod->hwobj;
	ops = (GxSeModuleDevOps *)GXSE_OBJ_DEVOPS(obj);

	if (ops->poll)
		return ops->poll(obj, which, r_wait, w_wait);

	return GXSE_POLL_NONE;
}
#endif
