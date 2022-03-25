#ifdef LINUX_OS
#include "kernelcalls.h"
#include "gxse_core.h"

unsigned int gx_osdep_chip_probe(void)
{
#ifdef CONFIG_GX3201
    return GXSECURE_CHIPID_GX3201;
#endif

#ifdef CONFIG_GX3211
    return GXSECURE_CHIPID_GX3211;
#endif

#ifdef CONFIG_GX6605S
    return GXSECURE_CHIPID_GX6605S;
#endif

#ifdef CONFIG_SIRIUS
    return GXSECURE_CHIPID_SIRIUS;
#endif

#ifdef CONFIG_TAURUS
    return GXSECURE_CHIPID_TAURUS;
#endif

#ifdef CONFIG_GEMINI
    return GXSECURE_CHIPID_GEMINI;
#endif
}

extern unsigned int gx_chip_id_sub_probe(void);
unsigned int gx_osdep_chip_sub_probe(void)
{
	static unsigned int chipid_sub = 0;

	if (chipid_sub)
		return chipid_sub;

	chipid_sub =  gx_chip_id_sub_probe();
	return chipid_sub;
}

extern unsigned char *__gx_page_malloc(unsigned long size);
unsigned char *gx_osdep_page_malloc(unsigned long size)
{
    return __gx_page_malloc(size);
}

void gx_osdep_page_free(unsigned char *p, unsigned long size)
{
    unsigned long real_size = size <= 4096 ? 4096 : size;

    free_pages((unsigned long)p, get_order(real_size));
}

void gx_osdep_cache_sync(const void *start, unsigned int size, int direction)
{
#ifdef CONFIG_ARM
	if (start) {
		phys_addr_t phys_start = gx_virt_to_phys(start);
		__cpuc_flush_dcache_area((void*)start, size);
		if (direction == DMA_TO_DEVICE)
			outer_clean_range(phys_start, phys_start + size);
		else
			outer_inv_range(phys_start, phys_start + size);

	} else {
		flush_cache_all();
		outer_flush_all();
	}
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,25)
	flush_cache_mm(NULL);
#else
	extern void dma_cache_maint(const void *start, size_t size, int direction);
	dma_cache_maint(start, size, direction);
#endif
#endif
}

void *gx_osdep_request_mem_region(phys_addr_t start, unsigned int size)
{
	int32_t ret = 0;
	if ((ret = gxse_memhole_probe(start, size, NULL, GXSE_MEM_PHYS)) <= GXSE_MEM_CROSSOVER)
		return NULL;

	if (ret == GXSE_MEM_INSIDE || ret >= GXSE_MEM_SAME)
		return (void *)0x12345678;

	return request_mem_region(start, size, "gxse");
}

void *gx_osdep_ioremap(unsigned int start, unsigned int size)
{
	int32_t ret = 0;
	void *virt = 0;

	if ((ret = gxse_memhole_probe(start, size, (void *)&virt, GXSE_MEM_PHYS)) >= 0)
		return virt;

	if (ret == GXSE_MEM_INSIDE || ret == GXSE_MEM_CROSSOVER || ret == GXSE_MEM_FULL)
		return NULL;

	if ((virt = ioremap(start, size))) {
		gxse_memhole_register((uint32_t)virt, start, size);
		return virt;
	}

	return NULL;
}

unsigned int gx_osdep_virt_to_phys(unsigned int addr)
{
	unsigned int phys;

	if (gxse_memhole_probe_addr(addr, &phys, GXSE_MEM_VIRT) >= 0)
		return phys;

	return virt_to_phys((unsigned long *)addr);
}

unsigned int gx_osdep_phys_to_virt(unsigned int addr)
{
	unsigned int virt;

	if (gxse_memhole_probe_addr(addr, &virt, GXSE_MEM_PHYS) >= 0)
		return virt;

	return (unsigned int)phys_to_virt(addr);
}

unsigned long long gx_osdep_current_tick(void)
{
    return jiffies;
}

int32_t gx_osdep_probe_irq_byname(const char *name, int32_t *irq)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,25)
    struct device_node *dn = of_find_node_by_path("/soc/gxsecure");
#endif

	if (NULL == name || NULL == irq) {
		gxlog_d(GXSE_LOG_MOD_DEV, "irq probe param is NULL\n");
		return GXSE_ERR_GENERIC;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,25)
	*irq = of_irq_get_byname(dn, name);

	if (*irq > 0) {
		GxSeDeviceIRQ *devirq = gxse_irqlist_get(*irq);
		if (NULL == devirq) {
			gxlog_e(GXSE_LOG_MOD_DEV, "probe (%7s), irq = %d failed. No entry\n", name, *irq);
			return GXSE_ERR_GENERIC;
		}
		if (devirq->device_count == 0)
			gxlog_i(GXSE_LOG_MOD_DEV, "probe (%7s), irq = %d\n", name, *irq);
	}
#endif
	return GXSE_SUCCESS;
}

int gx_osdep_request_irq(uint32_t irq, void *isr_cb, void *dsr_cb, void *priv)
{
	uint32_t id = 0;
	GxSeDeviceIRQ *devirq = NULL;

	if (NULL == priv || NULL == isr_cb) {
		gxlog_e(GXSE_LOG_MOD_DEV, "irq param error: %d\n", irq);
		return GXSE_ERR_GENERIC;
	}

	if ((devirq = gxse_irqlist_get(irq)) == NULL) {
		gxlog_e(GXSE_LOG_MOD_DEV, "No entry\n");
		return GXSE_ERR_GENERIC;
	}

	if ((id = devirq->device_count) == GXSE_MAX_DEV_IRQ) {
		gxlog_e(GXSE_LOG_MOD_DEV, "request irq too many times : %d\n", irq);
		return GXSE_ERR_GENERIC;
	}

	if (id == 0) {
		if (request_irq(irq, isr_cb, IRQF_SHARED, "gxse_isr", devirq) != 0) {
			gxlog_e(GXSE_LOG_MOD_DEV, "request irq failed : %d\n", irq);
			return GXSE_ERR_GENERIC;
		}
	}
	devirq->device[id] = priv;
	devirq->device_mask |= 0x1<<id;
	devirq->device_count = id+1;

	return GXSE_SUCCESS;
}

int gx_osdep_free_irq(uint32_t irq, void *priv)
{
	uint32_t id = 0, i = 0;
	GxSeDeviceIRQ *devirq = NULL;

	if (NULL == priv) {
		gxlog_e(GXSE_LOG_MOD_DEV, "irq param error: %d\n", irq);
		return GXSE_ERR_GENERIC;
	}

	if ((devirq = gxse_irqlist_get(irq)) == NULL) {
		gxlog_e(GXSE_LOG_MOD_DEV, "No entry\n");
		return GXSE_ERR_GENERIC;
	}

	if ((id = devirq->device_count) == 0) {
		gxlog_e(GXSE_LOG_MOD_DEV, "irq doesn't existed: %d\n", irq);
		return GXSE_ERR_GENERIC;
	}

	for (i = 0; i < GXSE_MAX_DEV_IRQ; i++) {
		if (devirq->device[i] == priv) {
			devirq->device[i] = NULL;
			devirq->device_mask &= ~(0x1<<i);
			id -= 1;
		}
	}

	devirq->device_count = id;
	if (id == 0) {
		free_irq(irq, devirq);
		gxse_irqlist_remove(devirq->id);
	}

	return GXSE_SUCCESS;
}

#endif
