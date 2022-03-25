#ifdef ECOS_OS
#include "kernelcalls.h"
#include "gxse_core.h"

extern cyg_uint32 gx_chip_probe(void);
extern cyg_uint32 gx_chip_sub_probe(void);
unsigned int gx_osdep_chip_probe(void)
{
	static unsigned int chipid = 0;

	if (chipid)
		return chipid;

	chipid = gx_chip_probe();
    return chipid;
}

unsigned int gx_osdep_chip_sub_probe(void)
{
	static unsigned int chipid_sub = 0;

	if (chipid_sub)
		return chipid_sub;

	chipid_sub = gx_chip_sub_probe();
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
    if(node == NULL) {
        gxlog_e(GXSE_LOG_MOD_DEV, "%s,malloc (inode) NULL\n",__func__);
        return NULL;
    }

    tmp = node + sizeof(void *);

    ret = (unsigned char *)((unsigned int)(tmp + PAGE_ALIGN - 1) & (~(unsigned int)( PAGE_ALIGN - 1)));
    p   = (unsigned int *) (ret - 4);
    *p  = (unsigned int)node;

    return ret;
}

void gx_osdep_page_free(unsigned char *p, unsigned long size)
{
    if(p) {
        void *tmp = (void *)(*(unsigned int *)((unsigned int)p - 4));
        if(tmp) gx_free(tmp);
    }
}

void gx_osdep_cache_sync(const void *start, unsigned int size, int direction)
{
	HAL_DCACHE_SYNC();
}

static unsigned int interrupt_disable = 0;
void gx_osdep_interrupt_disable(void)
{
    if(!interrupt_disable) {
        cyg_interrupt_disable();
    }
    interrupt_disable++;
}

void gx_osdep_interrupt_enable(void)
{
    if(interrupt_disable) {
        interrupt_disable--;
        if(!interrupt_disable)
            cyg_interrupt_enable();
    }
}

void gx_osdep_interrupt_mask(unsigned int vector)
{
#ifdef ARCH_ARM
	vector += 32;
#endif
    cyg_interrupt_mask(vector);
}

void gx_osdep_interrupt_unmask(unsigned int vector)
{
#ifdef ARCH_ARM
	vector += 32;
#endif
    cyg_interrupt_unmask(vector);
}

extern cyg_uint32 hal_ioremap_uncached(cyg_uint32 phy_addr);
void *gx_osdep_ioremap(unsigned int offset, unsigned int size)
{
	int32_t ret = 0;
	uint32_t virt = 0;

	if ((ret = gxse_memhole_probe(offset, size, &virt, GXSE_MEM_VIRT)) >= 0)
		return (void *)virt;

	if (ret == GXSE_MEM_INSIDE || ret == GXSE_MEM_CROSSOVER || ret == GXSE_MEM_FULL)
		return NULL;

	virt = hal_ioremap_uncached(offset);
	gxse_memhole_register(offset, virt, size);
	return (void *)virt;
}

static cyg_handle_t  s_interrupt_handle[GXSE_MAX_IRQ];
static cyg_interrupt s_interrupt_data[GXSE_MAX_IRQ];
int32_t gx_osdep_probe_irq_byname(const char *name, int32_t *irq)
{
#ifdef ARCH_ARM
	GxSeDeviceIRQ *devirq = NULL;

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
#endif
	return GXSE_SUCCESS;
}

int gx_osdep_request_irq(uint32_t irq, void *isr_cb, void *dsr_cb, void *priv)
{
	uint32_t id = 0, count = 0;
	GxSeDeviceIRQ *devirq = NULL;

	if (NULL == priv || NULL == isr_cb || NULL == dsr_cb) {
		gxlog_e(GXSE_LOG_MOD_DEV, "irq param error: %d\n", irq);
		return GXSE_ERR_GENERIC;
	}

	if ((devirq = gxse_irqlist_get(irq)) == NULL) {
		gxlog_e(GXSE_LOG_MOD_DEV, "No entry\n");
		return GXSE_ERR_GENERIC;
	}

	if ((count = devirq->device_count) == GXSE_MAX_DEV_IRQ) {
		gxlog_e(GXSE_LOG_MOD_DEV, "request irq too many times : %d\n", irq);
		return GXSE_ERR_GENERIC;
	}

	if (count == 0) {
		id = devirq->id;
		cyg_drv_interrupt_create(
				irq,
				10,
				(cyg_addrword_t)devirq,
				(cyg_ISR_t *)isr_cb,
				(cyg_DSR_t *)dsr_cb,
				&s_interrupt_handle[id],
				&s_interrupt_data[id]);
		cyg_drv_interrupt_attach(s_interrupt_handle[id]);
		cyg_drv_interrupt_unmask(irq);
	}
	devirq->device[count] = priv;
	devirq->device_mask |= 0x1<<count;
	devirq->device_count = count+1;

	return GXSE_SUCCESS;
}

int gx_osdep_free_irq(uint32_t irq, void *priv)
{
	GxSeDeviceIRQ *devirq = NULL;
	uint32_t id = 0, i = 0, count = 0;

	if (NULL == priv) {
		gxlog_e(GXSE_LOG_MOD_DEV, "irq param error: %d\n", irq);
		return GXSE_ERR_GENERIC;
	}

	if ((devirq = gxse_irqlist_get(irq)) == NULL) {
		gxlog_e(GXSE_LOG_MOD_DEV, "No entry\n");
		return GXSE_ERR_GENERIC;
	}

	if ((count = devirq->device_count) == 0) {
		gxlog_e(GXSE_LOG_MOD_DEV, "irq doesn't existed: %d\n", irq);
		return GXSE_ERR_GENERIC;
	}

	for (i = 0; i < GXSE_MAX_DEV_IRQ; i++) {
		if (devirq->device[i] == priv) {
			devirq->device[i] = NULL;
			devirq->device_mask &= ~(0x1<<i);
			count -= 1;
		}
	}

	devirq->device_count = count;
	if (count == 0) {
		id = devirq->id;
		cyg_drv_interrupt_mask(irq);
		cyg_drv_interrupt_detach(s_interrupt_handle[id]);
		cyg_drv_interrupt_delete(s_interrupt_handle[id]);
		gxse_irqlist_remove(id);
	}

	return GXSE_SUCCESS;
}

extern cyg_tick_count_t cyg_current_time(void);
unsigned long long gx_osdep_current_tick(void)
{
    return (unsigned long long)(cyg_current_time());
}

#endif
