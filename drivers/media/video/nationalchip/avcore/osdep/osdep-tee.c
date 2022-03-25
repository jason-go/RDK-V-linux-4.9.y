#ifdef TEE_OS

#include <gxav.h>
#include "avcore.h"

#include "kernelcalls.h"
#include "osdep-dma.h"

unsigned int gxcore_chip_probe(void)
{
	return GXAV_ID_SIRIUS;
}

unsigned int gx_virt_to_dma(unsigned int virt)
{
	return 0;
}

unsigned int gx_dma_to_phys(unsigned int dma_addr)
{
	return 0;
}

unsigned int gx_virt_to_phys(unsigned int addr)
{
	return 0;
}

unsigned int gx_phys_to_virt(unsigned int addr)
{
	return 0;
}

unsigned char *gx_page_malloc(unsigned long size)
{
	return gx_malloc(size);
}

void gx_page_free(unsigned char *p, unsigned long size)
{
	gx_free(p);
}

void *gx_dma_malloc(size_t size)
{
	return gx_malloc(size);
}

void gx_dma_free(void* p, size_t size)
{
	gx_free(p);
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
}

void *gx_sdc_memcpy(void *dst, const void *src, int len)
{
	return memcpy(dst, src, len);
}


void gx_interrupt_disable(void)
{
	return;
}

void gx_interrupt_enable(void)
{
	return;
}


int gx_wait_event(void *module, unsigned int event_mask, int timeout_us)
{
	return -1;
}

int gx_wake_event(void *module, unsigned int event)
{
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
	return 0;
}

int gx_sem_delete (gx_sem_id *sem_id)
{
	return 0;
}

int gx_sem_post (gx_sem_id *sem_id)
{
	return 0;
}

int gx_sem_wait (gx_sem_id *sem_id)
{
	return 0;
}

int gx_sem_wait_timeout (gx_sem_id *sem_id,long timeout)
{
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
	return (void *)phys_to_virt(offset, MEM_AREA_IO_SEC);
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

void gx_dcache_inv_range(unsigned int start, unsigned int end)
{
	unsigned int size  = end - start;

	if (start)
		cache_op_inner(DCACHE_AREA_CLEAN_INV, (void *)start, size);
	else
		cache_op_inner(DCACHE_CLEAN_INV, NULL, 0);
}

void gx_dcache_clean_range(unsigned int start, unsigned int end)
{
	unsigned int size  = end - start;

	if (start)
		cache_op_inner(DCACHE_AREA_CLEAN_INV, (void *)start, size);
	else
		cache_op_inner(DCACHE_CLEAN_INV, NULL, 0);
}

void gx_dcache_flush_range(unsigned int start, unsigned int end)
{
	unsigned int size  = end - start;

	if (start)
		cache_op_inner(DCACHE_AREA_CLEAN_INV, (void *)start, size);
	else
		cache_op_inner(DCACHE_CLEAN_INV, NULL, 0);
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
	itr_disable(vector+32);
}

void gx_interrupt_unmask(unsigned int vector)
{
	itr_enable(vector+32);
}

unsigned int* gx_av_extend(void)
{
	return NULL;
}
unsigned long long gx_current_tick(void)
{
	return 0;
}

int gx_gettimeofday(struct timeval *tv, struct timezone *tz)
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

