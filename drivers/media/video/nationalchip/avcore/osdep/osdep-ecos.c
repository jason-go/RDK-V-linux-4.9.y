#ifdef ECOS_OS

#include <stdio.h>
#include <stdlib.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_cache.h>

#include "kernelcalls.h"
#include "osdep-dma.h"
#include "avcore.h"
#include "gxav_common.h"

extern void *realloc(void *mem_address, unsigned int newsize);
static void _dcache_sync(const void *start, size_t size, int direction);

extern cyg_uint32 gx_chip_probe(void);
extern cyg_uint32 gx_chip_sub_probe(void);
unsigned int gxcore_chip_probe(void)
{
	return gx_chip_probe();
}

unsigned int gxcore_chip_sub_probe(void)
{
	return gx_chip_sub_probe();
}

int	gx_gettimeofday(struct timeval *tv, struct timezone *tz){
	return gettimeofday(tv, tz);
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

void *gx_dma_malloc(size_t size)
{
	void *p, *dma;

	p = gx_page_malloc(size);
	if(p == NULL) {
		gx_printf("%s,dma_malloc (inode) NULL\n",__func__);
		return NULL;
	}

	_dcache_sync(0, 0, DMA_FROM_DEVICE);
	dma = (void*)hal_uncached_map((cyg_uint32)p);
	gxav_dmainfo_add((unsigned int)dma, (unsigned int)0, size);
	return dma;
}

void gx_dma_free(void* p, size_t size)
{
	if(p) {
		void* addr = (void*)hal_cached_map((cyg_uint32)p);
		gx_page_free(addr, size);
		gxav_dmainfo_remove((unsigned int)p);
	}
}

unsigned int gx_dma_to_phys(unsigned int dma_addr)
{
	if (dma_addr) {
		struct gxav_dmainfo *blk = gxav_dmainfo_find(dma_addr);
		if (blk) {
			cyg_uint32 addr = hal_cached_map((cyg_uint32)dma_addr);
			return hal_cached_to_phys(addr);
		}
	}
	return 0;
}

unsigned int gx_virt_to_phys(unsigned int addr)
{
	unsigned int phys = gx_dma_to_phys(addr);
	if (phys != 0)
		return phys;

	return hal_cached_to_phys((cyg_uint32)(addr));
}

unsigned int gx_phys_to_virt(unsigned int addr)
{
	return hal_ioremap_cached((cyg_uint32 )(addr));
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

#if 0
void gx_dcache_check(unsigned int start, unsigned int end)
{
	if (start && end) {
		volatile unsigned int *nocached_buffer = (volatile unsigned int *)hal_uncached_map(start);
		volatile unsigned int *cached_buffer = (volatile unsigned int *)start;
		while (*nocached_buffer != *cached_buffer) {
			gx_printf("%s, failed!", __func__);
		}
		nocached_buffer = (volatile unsigned int *)hal_uncached_map(end);
		cached_buffer = (volatile unsigned int *)end;
		while (*nocached_buffer != *cached_buffer) {
			gx_printf("%s, failed!", __func__);
		}
	}
}
#else
#define gx_dcache_check(s, e) (void)0
#endif

void gx_dcache_inv_range(unsigned int start, unsigned int end)
{
	//由于csky的cache不能按line来刷，所以，任何使用到inv
	//cache的地方都只能是先clean,再inv，防止把其他line的cache被误失效掉
	//HAL_DCACHE_INVALIDATE_ADDR(start, end);

	HAL_DCACHE_SYNC_ADDR(start, end);
	gx_dcache_check(start, end);
}

void gx_dcache_clean_range(unsigned int start, unsigned int end)
{
	HAL_DCACHE_CLEAN_ADDR(start, end);
	gx_dcache_check(start, end);
}

void gx_dcache_flush_range(unsigned int start, unsigned int end)
{
	HAL_DCACHE_SYNC_ADDR(start, end);
	gx_dcache_check(start, end);
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

// eCos implements disable count, so needn't it here.
void gx_interrupt_disable(void)
{
	cyg_interrupt_disable();
}

void gx_interrupt_enable(void)
{
	cyg_interrupt_enable();
}

void gx_interrupt_mask(unsigned int vector)
{
#ifdef ARCH_ARM
	vector += 32;
#endif
	cyg_interrupt_mask(vector);
}

void gx_interrupt_unmask(unsigned int vector)
{
#ifdef ARCH_ARM
	vector += 32;
#endif
	cyg_interrupt_unmask(vector);
}
extern int hal_interrupt_maskbit(int src);
int gx_interrupt_masked(unsigned int vector)
{
#ifdef ARCH_ARM
	vector += 32;
#endif
	return hal_interrupt_maskbit(vector);
}

int gx_wait_event(void *module, unsigned int event_mask, int timeout_us)
{
#define GX_TICKS_UNIT 10

	cyg_flag_t *wq = NULL;
	unsigned int ret = 0;
	struct gxav_module *m = (struct gxav_module *)module;

	if(m == NULL || m->inode == NULL || m->inode->wq == NULL) {
		gx_printf("%s,module (inode) NULL\n",__func__);
		return -EINVAL;
	}

	wq = (cyg_flag_t *)m->inode->wq;

	GXAV_DBG("start   %s(),module=%p,event_mask=0x%x \n",__func__, m, event_mask);

	if (timeout_us >= 0)
	{
		cyg_tick_count_t timeout_ticks = timeout_us/1000/GX_TICKS_UNIT;
		ret = cyg_flag_timed_wait(wq, event_mask, CYG_FLAG_WAITMODE_OR,  timeout_ticks + cyg_current_time() );
		if(ret == 0) // error or time_out
			return -EINVAL;
	}
	else
	{
		ret = cyg_flag_wait(wq, event_mask, CYG_FLAG_WAITMODE_OR);
		if(ret == 0) //error
			return -EINVAL;
	}

	//successful
	cyg_flag_maskbits((cyg_flag_t *)wq, (cyg_flag_value_t)(~(ret & event_mask)));

	gxav_module_inode_clear_event(m->inode, event_mask);

	GXAV_DBG("end   %s(),module=%p,event_mask=0x%x \n",__func__, m, event_mask);

	return (ret & event_mask);
}

int gx_wake_event(void *module, unsigned int event)
{
	struct gxav_module *m = (struct gxav_module *)module;

	if(m == NULL || m->inode == NULL || m->inode->wq == NULL) {
		gx_printf("%s,module (inode) NULL\n",__func__);
		return -1;
	}

	cyg_flag_setbits((cyg_flag_t *)m->inode->wq, event);

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

	if (priority > GX_THREAD_PRIORITY_MAX || thread_id == NULL \
			|| entry_func == NULL || thread_name == NULL \
			|| stack_base == NULL || thread_info == NULL)
		return -1;

	cyg_thread_create_ex((cyg_addrword_t)priority, (cyg_thread_entry_t *)entry_func, (cyg_addrword_t)arg,
			(char *)thread_name, stack_base, (cyg_ucount32)stack_size, thread_id, thread_info, 0);

	cyg_thread_resume(*thread_id);

	return 0;
}

int gx_thread_delete (gx_thread_id thread_id)
{
	cyg_thread_kill(thread_id);
	cyg_thread_delete(thread_id);
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

		cyg_thread_delay(loop/10);

		ms_loop -= loop;
	}

	return 0;
}

int gx_sem_create (gx_sem_id *sem_id, unsigned int sem_init_val)
{
	if (sem_id == NULL || sem_init_val > 0x7fffffff)
		return -1;

	cyg_semaphore_init(sem_id, sem_init_val);

	return 0;
}

int gx_sem_delete (gx_sem_id *sem_id)
{
	if (sem_id == NULL)
		return -1;

	cyg_semaphore_destroy(sem_id);

	return 0;
}

int gx_sem_post (gx_sem_id *sem_id)
{
	if (sem_id == NULL)
		return -1;

	cyg_semaphore_post(sem_id);

	return 0;
}

int gx_sem_wait (gx_sem_id *sem_id)
{
	if (sem_id == NULL)
		return -1;

	if (cyg_semaphore_wait(sem_id) == 1)
		return 0;

	return -1;
}

int gx_sem_wait_timeout (gx_sem_id *sem_id,long timeout)
{
	cyg_tick_count_t tickcount;
	cyg_tick_count_t abs_time;

	if (sem_id == NULL)
		return -1;

	tickcount = cyg_current_time();
	abs_time = tickcount + (cyg_tick_count_t)timeout/10;//MILLISECOND_PER_TICK = 10;


	if(cyg_semaphore_timed_wait(sem_id,abs_time)==1)
		return 0;
	return -1;
}

int gx_sem_trywait (gx_sem_id *sem_id)
{
	if (sem_id == NULL)
		return -1;

	if (cyg_semaphore_trywait(sem_id) == 1)
		return 0;

	return -1;
}

int gx_alarm_create(unsigned int *handle,void (*alarmfn)(unsigned int,unsigned int),unsigned int millisecond)
{
	cyg_handle_t counter = 0;
	cyg_alarm *alarm_s = NULL;

	alarm_s = (cyg_alarm *)gx_malloc(sizeof(cyg_alarm));
	if(alarm_s == NULL)
		goto err;

	gx_memset(alarm_s,0,sizeof(cyg_alarm));

	GXAV_DBG("%s(),alarm object : %p\n",__func__,alarm_s);

	cyg_clock_to_counter(cyg_real_time_clock(),&counter);

	cyg_alarm_create(counter,alarmfn,0,handle,alarm_s);
	if(handle == NULL)
		goto err;

	GXAV_DBG("%s(),alarm handle : 0x%x\n",__func__,*handle);

	cyg_alarm_initialize(*handle,cyg_current_time(),(millisecond%10)?(millisecond/10 + 1):(millisecond/10));

	cyg_alarm_disable(*handle);

	return 0;
err:
	if(alarm_s) {
		gx_free(alarm_s);
		alarm_s = NULL;
	}
	return -1;
}

int gx_alarm_delete(unsigned int *handle)
{
	cyg_alarm *alarm_s = (cyg_alarm *)(*handle);

	if(!*handle)
		return -1;

	GXAV_DBG("%s(),alarm handle : 0x%x\n",__func__,*handle);

	cyg_alarm_delete(*handle);

	GXAV_DBG("%s(),alarm object : %p\n",__func__,alarm_s);
	if(alarm_s) {
		gx_free(alarm_s);
		alarm_s = NULL;
	}

	*handle = 0;

	return 0;
}

int gx_alarm_enable(unsigned int *handle)
{
	if(!*handle)
		return -1;

	cyg_alarm_enable(*handle);

	return 0;
}

int gx_alarm_disable(unsigned int *handle)
{
	if(!*handle)
		return -1;

	cyg_alarm_disable(*handle);

	return 0;
}

void *ioremap(unsigned int offset)
{
	return (void *)hal_ioremap_uncached(offset);
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

extern cyg_tick_count_t cyg_current_time(void);
unsigned long long gx_current_tick(void)
{
	return (unsigned long long)(cyg_current_time());
}

unsigned long long gx_tick_start(unsigned long timeout_ms)
{
	return cyg_current_time() * 10 + timeout_ms;
}

int gx_tick_end(unsigned long long tick)
{
	return cyg_current_time() * 10 >= tick;
}

#endif

