#ifdef TEE_OS
#include "kernelcalls.h"
#include "gxse_core.h"

unsigned int gx_osdep_chip_probe(void)
{
    return GXSECURE_CHIPID_SIRIUS;
}

unsigned int gx_osdep_chip_sub_probe(void)
{
	return 0;
}

unsigned char *gx_osdep_page_malloc(unsigned long size)
{
    return malloc(size);
}

void gx_osdep_page_free(unsigned char *p, unsigned long size)
{
	(void) size;
	free(p);
}

void gx_osdep_cache_sync(const void *start, unsigned int size, int direction)
{
	(void) direction;
	if (start)
		cache_op_inner(DCACHE_AREA_CLEAN_INV, (void *)start, size);
	else
		cache_op_inner(DCACHE_CLEAN_INV, NULL, 0);
}

void *gx_osdep_ioremap(paddr_t start, unsigned int size)
{
	int32_t ret = 0;
	void *virt = 0;

	if ((ret = gxse_memhole_probe(start, size, (void *)&virt, GXSE_MEM_PHYS)) >= 0)
		return virt;

	if (ret == GXSE_MEM_INSIDE || ret == GXSE_MEM_CROSSOVER || ret == GXSE_MEM_FULL)
		return NULL;

	if (core_mmu_add_mapping(MEM_AREA_IO_SEC, start, size)) {
		virt = phys_to_virt(start, MEM_AREA_IO_SEC);
		gxse_memhole_register((uint32_t)virt, start, size);
		return virt;
	}

	return NULL;
}

int32_t gx_osdep_probe_irq_byname(const char *name, int32_t *irq)
{
	GxSeDeviceIRQ *devirq = NULL;
	(void) name;

	if (*irq > 0) {
		if ((devirq = gxse_irqlist_get(*irq)) == NULL) {
			gxlog_e(GXSE_LOG_MOD_DEV, "No entry\n");
			return GXSE_ERR_GENERIC;
		}

		if (devirq->device_count == 0) {
			*irq += 32;
			devirq->irq = *irq;
		}
	}

	return GXSE_SUCCESS;
}

int gx_osdep_request_irq(uint32_t irq, void *isr_cb, void *dsr_cb, void *priv)
{
	uint32_t id = 0;
	GxSeDeviceIRQ *devirq = NULL;
	GxSeDevice *_dev = (GxSeDevice *)priv;
	(void) dsr_cb;

	if (NULL == priv || NULL == _dev->priv || NULL == isr_cb) {
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
		struct itr_handler *itr_priv = _dev->priv;
		itr_priv->data = devirq;
		itr_priv->handler = isr_cb;
		itr_priv->it = irq;
		itr_add(itr_priv);
		itr_enable(irq);
	}
	devirq->device[id] = priv;
	devirq->device_mask |= 0x1<<id;
	devirq->device_count = id+1;

	return GXSE_SUCCESS;
}

int gx_osdep_free_irq(uint32_t irq, void *priv)
{
	GxSeDeviceIRQ *devirq = NULL;
	uint32_t id = 0, i = 0;

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
	if (id == 0)
		gxse_irqlist_remove(devirq->id);

	return GXSE_SUCCESS;
}

unsigned long long gx_osdep_current_tick(void)
{
	return gx_counter_get_ms()/10;
}

#endif
