#ifdef UCOS_OS

#include "kernelcalls.h"
#include "avcore.h"
#include "gxav_common.h"
#include "gxav_ioctl.h"
#include <gxos/ucos_ck.h>

unsigned int interrupt_disable = 0;
extern void *realloc(void *mem_address, unsigned int newsize);

extern unsigned int gx_chip_probe(void);
unsigned int gxcore_chip_probe(void)
{
	return gx_chip_probe();
}

int	gx_gettimeofday(struct timeval *tv, struct timezone *tz){
	tv->tv_sec = drv_time_get()/100;
	tv->tv_usec = (drv_time_get()%100)*10000;
	return 0;
}

void gx_msleep(UINT32 ms)
{
	if (ms < 10) ms = 10;
	drv_os_delay(ms/10);
}

unsigned char *gx_page_malloc(unsigned long size)
{
	unsigned char *node;
	unsigned char *tmp, *ret;
#define PAGE_ALIGN (1 << PAGE_SHIFT)

	unsigned int ssize = sizeof(void *) + size + PAGE_ALIGN;
	unsigned int *p;

	node = gx_malloc(ssize);
	if(node == NULL) {
		gx_printf("%s,malloc (inode) NULL\n",__func__);
		return NULL;
	}

	tmp = node + sizeof(void *);

	ret = (unsigned char *)((unsigned int)(tmp + PAGE_ALIGN - 1) & (~(unsigned int)( PAGE_ALIGN - 1)));
	p   = (unsigned int *) (ret - 4);
	*p  = (unsigned int)node;

	return ret;
}

void gx_page_free(unsigned char *p, unsigned long size)
{
	if(p) {
		void *tmp = (void *)(*(unsigned int *)((unsigned int)p - 4));
		if(tmp) gx_free(tmp);
	}
}

extern void* gx_dma_malloc(size_t size);
extern void  gx_dma_free(void*, size_t);
extern void* gx_dma_to_phys(void* addr);

#define gx_virt_to_phys(addr)            drv_cached_to_phys((unsigned int)addr)
#define gx_phys_to_virt(addr)            drv_ioremap_cached((unsigned int)addr);

void *gx_dma_malloc(size_t size)
{
	void *p, *dma;

	p = gx_page_malloc(size);
	if(p == NULL) {
		gx_printf("%s,dma_malloc (inode) NULL\n",__func__);
		return NULL;
	}

	gx_cache_sync(0, 0, DMA_FROM_DEVICE);
	dma = (void*)drv_uncached_map((unsigned int)p);
	gxav_dmainfo_add((unsigned int)dma, (unsigned int)0, size);
	return dma;
}

void gx_dma_free(void* p, size_t size)
{
	if(p) {
		void* addr = (void*)drv_cached_map((unsigned int)p);
		gx_page_free(addr, size);
		gxav_dmainfo_remove((unsigned int)p);
	}
}

unsigned int gx_dma_to_phys(unsigned int dma_addr)
{
	if (dma_addr) {
		struct gxav_dmainfo *blk = gxav_dmainfo_find(dma_addr);
		if (blk) {
			return drv_uncached_to_phys(dma_addr);
		}
	}

	return 0;
}

unsigned int gx_virt_to_phys(unsigned int addr)
{
	unsigned int phys = gx_dma_to_phys(addr);
	if (phys != 0)
		return phys;

	return drv_cached_to_phys((unsigned int)(addr));
}

unsigned int gx_phys_to_virt(unsigned int addr)
{
	return drv_ioremap_cached((unsigned int)(addr));
}

int gx_copy_to_user(void *to, const void *from, unsigned int n)
{
	memcpy(to, from, n);

	return 0;
}

int gx_copy_from_user(void *to, const void *from, unsigned int n)
{
	memcpy(to, from, n);

	return 0;
}

void dbgassert(const char *fcn, int line, const char *expr)
{
	int x;
	//	gx_printf("ASSERTION FAILED, %s:%s:%d %s\n", __FILE__, fcn, line, expr);
	x = *(volatile int *)0;	/* force proc to exit */
}

void *gx_sdc_memcpy(void *dst, const void *src, int len)
{
	return memcpy(dst, src, len);
}

void gx_dcache_inv_range(unsigned int start, unsigned int end) {
	//由于csky的cache不能按line来刷，所以，任何使用到inv
	//cache的地方都只能是先clean,再inv，防止把其他line的cache被误失效掉
	//drv_dcache_invalidate_addr(start, start+end);
	drv_dcache_sync_addr(start, start+end);
}

void gx_dcache_clean_range(unsigned int start, unsigned int end) {
	drv_dcache_clean_addr(start, start+end);
}

void gx_dcache_flush_range(unsigned int start, unsigned int end) {
	drv_dcache_sync_addr(start, start+end);
}

static void _dcache_sync(const void *start, size_t size, int direction)
{
	unsigned int ustart = (unsigned int) start;
	unsigned int uend = ustart + size;

	switch (direction) {
	case DMA_FROM_DEVICE:            /* invalidate only */
		gx_dcache_inv_range(ustart, uend);
		break;
	case DMA_TO_DEVICE:             /* writeback only */
		gx_dcache_clean_range(ustart, uend);
		break;
	case DMA_BIDIRECTIONAL:         /* writeback and invalidate */
		gx_dcache_flush_range(ustart, uend);
		break;
	default:
		gx_printf("BUG(%s)\n", __func__);
	}
}


void gx_interrupt_disable(void)
{
	if(!interrupt_disable) {
		OS_ENTER_CRITICAL();
	}
	interrupt_disable++;
}

void gx_interrupt_enable(void)
{
	if(interrupt_disable) {
		interrupt_disable--;
		if(!interrupt_disable) {
			OS_EXIT_CRITICAL();
		}
	}
}

extern int gxav_irq_num[GXAV_MAX_IRQ];
void gx_interrupt_mask(unsigned int vector)
{
	drv_interrupt_mask(gxav_irq_num[vector]);
}

void gx_interrupt_unmask(unsigned int vector)
{
	drv_interrupt_unmask(gxav_irq_num[vector]);
}

int gx_interrupt_masked(int src)
{
	return 0;
}

unsigned long long gx_tick_start(unsigned long timeout_ms)
{
	return drv_time_get() * 10 + timeout_ms;
}

int gx_tick_end(unsigned long long ms)
{
	return drv_time_get() * 10 >= ms;
}

int gx_wait_event(void *module, unsigned int event_mask, int timeout_us)
{
	gx_flag_t *wq = NULL;
	unsigned int ret = 0;
	struct gxav_module *m = (struct gxav_module *)module;

	if(m == NULL || m->inode == NULL || m->inode->wq == NULL) {
		gx_printf("%s,module (inode) NULL\n",__func__);
		return -EINVAL;
	}

	wq = (gx_flag_t *)m->inode->wq;

	GXAV_DBG("start   %s(),module=%p,event_mask=0x%x \n",__func__, m, event_mask);

	if (timeout_us >= 0)
	{
		unsigned long long tick_start = gx_tick_start(timeout_us/1000);
		while ((event_mask & wq->mask) == 0) {
			gx_msleep(20);
			if (gx_tick_end(tick_start))
				break;
		}
		ret = event_mask & wq->mask;
		if(ret == 0) // error or time_out
			return -EINVAL;
	}
	else
	{
		while ((event_mask & wq->mask) == 0) {
			gx_msleep(20);
		}
		ret = event_mask & wq->mask;
	}

	wq->mask &=(~(event_mask));

	gxav_module_inode_clear_event(m->inode, event_mask);

	GXAV_DBG("end   %s(),module=%p,event_mask=0x%x \n",__func__, m, event_mask);

	return ret;
}

int gx_wake_event(void *module, unsigned int event)
{
	struct gxav_module *m = (struct gxav_module *)module;

	if(m == NULL || m->inode == NULL || m->inode->wq == NULL) {
		gx_printf("%s,module (inode) NULL\n",__func__);
		return -1;
	}

	((gx_flag_t *)(m->inode->wq))->mask |= (~event);

	return 0;
}

int gx_thread_create (const char *thread_name, gx_thread_id *thread_id,
		void(*entry_func)(void *), void *arg,
		void *stack_base,
		unsigned int stack_size,
		unsigned int priority,
		gx_thread_info *thread_info)
{
#define GX_THREAD_PRIORITY_MAX  255

	return 0;
	if (priority > GX_THREAD_PRIORITY_MAX || thread_id == NULL \
			|| entry_func == NULL || thread_name == NULL \
			|| stack_base == NULL || thread_info == NULL)
		return -1;

	drv_task_create(entry_func, arg, (char*)stack_base+stack_size, priority);

	return 0;
}

int gx_thread_delete (gx_thread_id thread_id)
{
	drv_task_delete(thread_id);

	return 0;
}

int gx_thread_should_stop(void)
{
	return 0;
}

int gx_thread_delay(unsigned int millisecond)
{
	int ms_loop;
	unsigned int loop;

	ms_loop = millisecond;

	while (ms_loop > 0)
	{
		loop = ms_loop < 999 ? ms_loop : 999;

		drv_os_delay(loop/10);

		ms_loop -= loop;
	}

	return 0;
}

int gx_sem_create (gx_sem_id *sem_id, unsigned int sem_init_val)
{
	if (sem_id == NULL || sem_init_val > 0x7fffffff)
		return -1;

	*sem_id = drv_sem_create(sem_init_val);

	return 0;
}

int gx_sem_delete (gx_sem_id *sem_id)
{
	if (sem_id == NULL)
		return -1;

	//cyg_semaphore_destroy(sem_id);

	return 0;
}

int gx_sem_post (gx_sem_id *sem_id)
{
	if (sem_id == NULL)
		return -1;

	drv_sem_post(*sem_id);

	return 0;
}

int gx_sem_wait (gx_sem_id *sem_id)
{
	if (sem_id == NULL)
		return -1;

	if (drv_sem_wait(*sem_id) == 1)
		return 0;

	return -1;
}

int gx_sem_wait_timeout (gx_sem_id *sem_id, long timeout)
{
	if (sem_id == NULL)
		return -1;

	if(drv_sem_wait_timeout(*sem_id, timeout/10) == 0)
		return 0;

	return -1;
}

int gx_sem_trywait (gx_sem_id *sem_id)
{
	if (sem_id == NULL)
		return -1;

	if(drv_sem_wait_timeout(*sem_id, 1) == 0)
		return 0;

	return -1;
}

void *ioremap(unsigned int offset)
{
	return (void *)drv_ioremap_uncached(offset);
}

void *request_mem_region(unsigned int len)
{
	return (void *)0x12345678;
}

int test_bit (int nr, const volatile long unsigned int * addr)
{
	return 1UL & (((const volatile unsigned int *) addr)[nr >> 5] >> (nr & 31));
}

void *gx_realloc(void *mem_address,unsigned int old_size,unsigned int new_size)
{
	return realloc(mem_address, new_size);
}

// CACHE
void *kcache_create(const char *name, size_t size, size_t align)
{
	return (void*)name;
}

void kcache_destroy(void* pcache)
{
	;
}

void *kcache_alloc(void* cache, size_t size)
{
	return gx_malloc(size);
}

void kcache_free(void* cache, void *p)
{
	gx_free(p);
}

unsigned long long gx_current_tick(void)
{
	return (unsigned long long)(drv_time_get());
}

#endif

