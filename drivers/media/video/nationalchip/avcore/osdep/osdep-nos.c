#ifdef NO_OS

#include <gxav.h>
#include "avcore.h"

#include "kernelcalls.h"
#include "osdep-dma.h"

#include "interrupt.h"

extern void *realloc(void *mem_address, unsigned int newsize);

static unsigned int gx_dma_to_virt(void*  dma)
{
	if (CHIP_IS_SIRIUS)
		return (unsigned int)dma;
	else
		return (unsigned int )(dma-0x20000000);
}

unsigned int gx_virt_to_dma(unsigned int virt)
{
	if (CHIP_IS_SIRIUS)
		return (unsigned int)virt;
	else
		return (unsigned int )(virt+0x20000000);
}

unsigned int gx_dma_to_phys(unsigned int dma_addr)
{
	if (dma_addr) {
		struct gxav_dmainfo *blk = gxav_dmainfo_find((unsigned int)dma_addr);
		if (blk) {
			if (CHIP_IS_SIRIUS)
				return dma_addr;
			else
				return (dma_addr - 0xa0000000);
		}
		else {
			if (CHIP_IS_SIRIUS)
				return dma_addr;
			else {
				if (dma_addr >= 0xb0000000)
					return dma_addr - 0xa0000000;
				return (dma_addr - 0x80000000);
			}
		}
	}
	return 0;
}

unsigned int gx_virt_to_phys(unsigned int addr)
{
	unsigned int phys = gx_dma_to_phys(addr);
	if (phys != 0)
		return phys;

	if (CHIP_IS_SIRIUS)
		return addr;
	else {
		if (addr >= 0xb0000000)
			return addr - 0xa0000000;
		return (addr - 0x80000000);
	}
}

unsigned int gx_phys_to_virt(unsigned int addr)
{
	if (CHIP_IS_SIRIUS)
		return addr;
	else
		return ((unsigned int)addr + 0x80000000);
}

unsigned char *gx_page_malloc(unsigned long size)
{
	unsigned char *node;
	unsigned char *tmp, *ret;
#define PAGE_ALIGN (1 << PAGE_SHIFT)

	unsigned int ssize = sizeof(void *) + size + PAGE_ALIGN;
	unsigned int *p;

	node = gx_malloc(ssize);
	if(node == NULL){
		gx_printf("%s,malloc (inode) NULL\n",__func__);
		return NULL;
	}

	tmp = node + sizeof(void *);

	ret = (unsigned char *)((unsigned int)(tmp + PAGE_ALIGN - 1) & (~(unsigned int)( PAGE_ALIGN - 1)));
	p = (unsigned int *) (ret - 4);
	*p = (unsigned int)node;

	return ret;
}

void gx_page_free(unsigned char *p, unsigned long size)
{
	void *tmp = (void *)(*(unsigned int *)((unsigned int)(p) - 4));
	gx_free(tmp);
}

void *gx_dma_malloc(size_t size)
{
	void *p, *dma;

	p = gx_page_malloc(size);
	if(p == NULL) {
		gx_printf("%s,dma_malloc (inode) NULL\n",__func__);
		return NULL;
	}

	gx_dcache_inv_range(0, 0);
	dma = (void*)gx_virt_to_dma((unsigned int)p);
	gxav_dmainfo_add((unsigned int)dma, (unsigned int)0, size);
	return dma;
}

void gx_dma_free(void* p, size_t size)
{
	if(p) {
		unsigned char* addr = (unsigned char*)gx_dma_to_virt(p);
		gx_page_free(addr, size);
		gxav_dmainfo_remove((unsigned int)p);
	}
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
	gx_printf("ASSERTION FAILED, %s:%s:%d %s\n", __FILE__, fcn, line, expr);
	x = *(volatile int *)0;	/* force proc to exit */
}

void *gx_sdc_memcpy(void *dst, const void *src, int len)
{
	return memcpy(dst, src, len);
}


void gx_interrupt_disable(void)
{
	gx_disable_irq();
}

void gx_interrupt_enable(void)
{
	gx_enable_irq();
}


int gx_wait_event(void *module, unsigned int event_mask, int timeout_us)
{
	gx_flag_t *wq = NULL;
	unsigned int ret = 0;
	struct gxav_module *m = (struct gxav_module *)module;

	if(m == NULL || m->inode == NULL || m->inode->wq == NULL) {
		gx_printf("%s,module (inode) NULL\n",__func__);
		return -1;
	}

	wq = (gx_flag_t *)m->inode->wq;

	GXAV_DBG("start   %s(),module=%p,event_mask=0x%x \n",__func__, m, event_mask);

	if (timeout_us >= 0)
	{
		unsigned int retry = timeout_us/(10*1000) + 1;
		while ((event_mask & wq->mask) == 0) {
			gx_msleep(10);
			retry--;
			if (retry == 0)
				break;
		}
		ret = event_mask & wq->mask;
		if(ret == 0) // error or time_out
			return -1;
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
	return 0;
}

int gx_thread_delete (gx_thread_id thread_id)
{
	return 0;
}

int gx_thread_should_stop(void)
{
    return 0;
}

int gx_thread_delay(unsigned int millisecond)
{
	return 0;
}

volatile gx_sem_id sem_nos[256];
int sem_init_flag = 0;

int gx_sem_create (gx_sem_id *sem_id, unsigned int sem_init_val)
{
	int i = 0;
	if(sem_init_flag == 0)
	{
		memset((void*)sem_nos,-1,256*sizeof(gx_sem_id));
		sem_init_flag = 1;
	}
	for(i=0; i<256; i++)
	{
		if(sem_nos[i] == -1)
		{
			sem_nos[i] = sem_init_val;
			*sem_id = i;
			return 0;
		}
	}
	return 0;
}

int gx_sem_delete (gx_sem_id *sem_id)
{
	sem_nos[*sem_id] = -1;
	return 0;
}

int gx_sem_post (gx_sem_id *sem_id)
{
	sem_nos[*sem_id] +=1;
	return 0;
}

int gx_sem_wait (gx_sem_id *sem_id)
{
	while(sem_nos[*sem_id] <= 0);

	sem_nos[*sem_id] -=1;
	return 0;
}

int gx_sem_wait_timeout (gx_sem_id *sem_id,long timeout)
{
	printf("[ERROR]: %s, timeout not support !\n", __func__);
	while(sem_nos[*sem_id] <= 0);

	sem_nos[*sem_id] -=1;
	return 0;
}
int gx_alarm_create(unsigned int *handle,void (*alarmfn)(unsigned int,unsigned int),unsigned int millisecond)
{
    return 0;
}

int gx_alarm_delete(unsigned int *handle)
{
    return 0;
}

int gx_alarm_enable(unsigned int *handle)
{
    return 0;
}

int gx_alarm_disable(unsigned int *handle)
{
    return 0;
}

void *ioremap(unsigned int offset)
{
	if (CHIP_IS_SIRIUS)
		return (void *)offset;
	else
		if(offset >= 0x90000000)
			return (void *)offset;
		else
			return (void *)(0xa0000000+offset);
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

extern void dcache_flush(void);
void gx_dcache_inv_range(unsigned int start, unsigned int end)
{
    dcache_flush();
}

void gx_dcache_clean_range(unsigned int start, unsigned int end)
{
    dcache_flush();
}

void gx_dcache_flush_range(unsigned int start, unsigned int end)
{
    dcache_flush();
}

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
void gx_interrupt_mask(unsigned int vector)
{
	gx_mask_interrupt(vector);
}
void gx_interrupt_unmask(unsigned int vector)
{
	gx_unmask_interrupt(vector);
}

int gx_interrupt_masked(unsigned int vector)
{
	return 0;
}

unsigned long long gx_current_tick(void)
{
	return gxos_get_rtc_time_ms()/10;
}

int	gx_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	unsigned long tick = gxos_get_rtc_time_ms();
	tv->tv_sec  = tick/1000;
	tv->tv_usec = (tick%1000)*1000;
	return 0;
}

int gx_mutex_init(int* m)
{
	return 0;
}
int gx_mutex_lock(int* m)
{
	return 0;
}
int gx_mutex_trylock(int* m)
{
	return 0;
}
int gx_mutex_unlock(int* m)
{
	return 0;
}
int gx_mutex_destroy(int* m)
{
	return 0;
}

int gx_sem_trywait (gx_sem_id *sem_id)
{
	return 0;
}

int abs(int value)
{
	if (value > 0)
		return value;
	else
		return (-value);
}

#endif

