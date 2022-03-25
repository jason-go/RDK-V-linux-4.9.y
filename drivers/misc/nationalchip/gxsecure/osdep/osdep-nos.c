#ifdef NOS_OS
#include "kernelcalls.h"
#include "gxse_core.h"

extern unsigned int gxcore_chip_probe(void);
extern unsigned int gxcore_chip_sub_probe(void);
unsigned int gx_osdep_chip_probe(void)
{
	static unsigned int chipid = 0;

	if (chipid)
		return chipid;

	chipid = gxcore_chip_probe();
    return chipid;
}

unsigned int gx_osdep_chip_sub_probe(void)
{
	static unsigned int chipid_sub = 0;

	if (chipid_sub)
		return chipid_sub;

	chipid_sub = gxcore_chip_sub_probe();
    return chipid_sub;
}

unsigned char *gx_osdep_page_malloc(unsigned long size)
{
    unsigned char *node;
    unsigned char *tmp, *ret;
#define PAGE_SHIFT (12)
#define PAGE_ALIGN (1 << PAGE_SHIFT)

    unsigned int ssize = sizeof(void *) + size + PAGE_ALIGN;
    unsigned int *p;

    node = gx_malloc(ssize);
    if (node == NULL) {
        gxlog_e(GXSE_LOG_MOD_DEV, "%s,malloc (inode) NULL\n",__func__);
        return NULL;
    }

    tmp = node + sizeof(void *);

    ret = (unsigned char *)((unsigned int)(tmp + PAGE_ALIGN - 1) & (~(unsigned int)( PAGE_ALIGN - 1)));
    p = (unsigned int *) (ret - 4);
    *p = (unsigned int)node;

    return ret;
}

void gx_osdep_page_free(unsigned char *p, unsigned long size)
{
    void *tmp = (void *)(*(unsigned int *)((unsigned int)(p) - 4));
    gx_free(tmp);
}

extern void dcache_flush(void);
void gx_osdep_cache_sync(const void *start, unsigned int size, int direction)
{
	dcache_flush();
}

static void * _ioremap(unsigned int offset)
{
	if (CHIP_IS_SIRIUS)
		return (void *)offset;
	else
		if(offset >= 0x90000000)
			return (void *)offset;
		else
			return (void *)(0xa0000000+offset);
}

void *gx_osdep_ioremap(unsigned int offset, unsigned int size)
{
	int32_t ret = 0;
	uint32_t virt = 0;

	if ((ret = gxse_memhole_probe(offset, size, &virt, GXSE_MEM_VIRT)) >= 0)
		return (void *)virt;

	if (ret == GXSE_MEM_INSIDE || ret == GXSE_MEM_CROSSOVER || ret == GXSE_MEM_FULL)
		return NULL;

	virt = (uint32_t)_ioremap(offset);
	gxse_memhole_register(offset, virt, size);
	return (void *)virt;
}

int32_t gx_osdep_probe_irq_byname(const char *name, int32_t *irq)
{
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
		gx_request_interrupt(irq, IRQ, isr_cb, devirq);
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
		gx_free_interrupt(irq);
		gxse_irqlist_remove(devirq->id);
	}

	return GXSE_SUCCESS;
}

unsigned long long gx_osdep_current_tick(void)
{
    return gxos_get_rtc_time_ms()/10;
}

#endif
