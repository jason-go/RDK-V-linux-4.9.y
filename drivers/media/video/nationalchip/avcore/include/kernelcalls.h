#ifndef __KERNELCALLS_H__
#define __KERNELCALLS_H__

#ifdef __cplusplus
extern "C" {
#endif
	/****************************************************************/
	//#define GX_DEBUG

#if (!defined(LINUX_OS) && !defined(ECOS_OS) && !defined(NO_OS) && !defined(UCOS_OS) && !defined(TEE_OS) )
#error "No defined OS"
#endif

#include "autoconf.h"

#ifdef LINUX_OS
#    include <linux/kernel.h>
#    include <linux/slab.h>
#    include <linux/mm.h>
#    include <linux/ioport.h>
#    include <linux/errno.h>
#    include <linux/vmalloc.h>
#    include <asm/io.h>
#    include <asm/uaccess.h>
#    include <asm/cacheflush.h>
#    include <linux/memory.h>
#    include <linux/interrupt.h>
#    include <linux/delay.h>
#    include <linux/kthread.h>
#    include <asm/div64.h>
#    include <linux/version.h>
#    include <linux/list.h>
#    include <linux/proc_fs.h>
#    include <linux/gx_mem_info.h>
#    include <linux/time.h>
#    include <linux/platform_device.h>
#    if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)
#        include <linux/kconfig.h>
#        include <linux/of_irq.h>
#        define GXAV_ENABLE_DTB 1
#    endif

#    if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
#        include <linux/semaphore.h>
#        include <asm/page.h>
#        include <linux/swab.h>
#    elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#        include <linux/semaphore.h>
#        include <linux/byteorder/swab.h>
#        include <asm-generic/page.h>
#    else
#        include <asm/semaphore.h>
#        include <linux/byteorder/swab.h>
#        include <asm-generic/page.h>
#    endif

#define MAX_KMALLOC_SIZE                    (128*1024 - 16)

#define gx_ioremap(offset,size)             ioremap(offset,size)
#define gx_iounmap(addr)                    iounmap((void *)(addr))

#define gx_request_mem_region(start,len)    request_mem_region(start,len,"av")
#define gx_release_mem_region(start,len)    release_mem_region(start,len)

#define gx_trace(fmt, args...)              printk(fmt, ##args)
#if (GX_DEBUG_PRINTF_LEVEL >= 2)
#define gx_printf(fmt, args...)             printk(fmt, ##args)
#else
#define gx_printf(fmt, args...)             ((void)0)
#endif
#define gx_sprintf(buf, fmt, args...)       sprintf(buf, fmt, ##args)
#define gx_snprintf(buf, len, fmt, args...) snprintf(buf, len, fmt, ##args)
#define gx_sscanf(buf, fmt, args...)        sscanf(buf, fmt, ##args)
#define gx_vsprintf(buf, fmt, args)         vsprintf(buf, fmt, args)

	// memory
#define _malloc(size)                    kmalloc(size, GFP_KERNEL | __GFP_REPEAT)
#define _free(p)                         kfree(p)
#define _vmalloc(size)                   vmalloc(size)
#define _vfree(p)                        vfree(p)

	extern struct proc_dir_entry * gx_proc_create(const char *name, int mode, struct proc_dir_entry *base, void *proc_ops, void *data);
	extern void gx_proc_remove(const char *name, struct proc_dir_entry *entry);

	// time
#define gx_mdelay(n)                     mdelay((n))
#define gx_udelay(n)                     udelay(n)
#define gx_msleep(n)                     msleep((n))

	// spinlock
	typedef spinlock_t gx_spin_lock_t;
#define gx_spin_lock_init(l)              spin_lock_init(l)
#define gx_spin_lock_bh(l)                spin_lock_bh(l)
#define gx_spin_unlock_bh(l)              spin_unlock_bh(l)
#define gx_spin_lock_irq(l)               spin_lock_irq(l)
#define gx_spin_unlock_irq(l)             spin_unlock_irq(l)
#define gx_spin_lock(l)                   spin_lock(l)
#define gx_spin_unlock(l)                 spin_unlock(l)
#define gx_spin_lock_irqsave(l, f)        {if(irqs_disabled()) {f = 0;spin_lock(l);} else spin_lock_irqsave(l, f);}
#define gx_spin_unlock_irqrestore(l, f)   {if(f == 0) spin_unlock(l); else spin_unlock_irqrestore(l, f);}

	// mutex lock
	typedef struct mutex gx_mutex_t;
#define gx_mutex_init(m)                  mutex_init(m)
#define gx_mutex_lock(m)                  mutex_lock(m)
#define gx_mutex_trylock(m)               mutex_trylock(m)
#define gx_mutex_unlock(m)                mutex_unlock(m)
#define gx_mutex_destroy(m)               mutex_destroy(m)

	typedef unsigned int gx_thread_id;

	typedef int gx_thread_info;

	typedef struct semaphore gx_sem_id;

#else
#define EXPORT_SYMBOL(a)
#endif // end LINUX_OS

	/****************************************************************/

#ifdef ECOS_OS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_cache.h>
	//#include <cyg/posix/sys/time.h>
#include <cyg/hal/gx_mem_info.h>
#include <sys/time.h>
#include <linux/list.h>
#include <ctype.h>
#include <string.h>
#include <cyg/fs/compat.h>

#define PAGE_MAX   (1024)

#define PAGE_SHIFT (12)
#define MAX_KMALLOC_SIZE (0xffffffff)

#define gx_trace(fmt, args...)              printk(fmt, ##args)
#if (GX_DEBUG_PRINTF_LEVEL >= 2)
#define gx_printf(fmt, args...)             printk(fmt, ##args)
#else
#define gx_printf(fmt, args...)             ((void)0)
#endif
#define gx_sprintf(buf, fmt, args...)       sprintf(buf, fmt, ##args)
#define gx_snprintf(buf, len, fmt, args...) sprintf(buf, fmt, ##args)
#define gx_sscanf(buf, fmt, args...)        sscanf(buf, fmt, ##args)
#define gx_vsprintf(buf, fmt, args)         vsprintf(buf, fmt, args)

#define gx_ioremap(offset,size)          ioremap(offset)
#define gx_iounmap(addr)

#define gx_request_mem_region(start,len) request_mem_region(len)
#define gx_release_mem_region(start,len)

	// memory
#define _malloc(size)                    malloc(size)
#define _free(p)                         free(p)
#define _vmalloc(size)                   malloc(size)
#define _vfree(p)                        free(p)

	extern cyg_uint32 hal_cached_to_phys(cyg_uint32 cached_addr);
	extern cyg_uint32 hal_ioremap_cached(cyg_uint32 cached_addr);

#define gx_mdelay(n)                      cyg_time_delay_us((n)*1000)
#define gx_udelay(n)                      cyg_time_delay_us(n)
#define gx_msleep(n)                      cyg_thread_delay(n>=10?n/10:1)

	// spinlock
	typedef int gx_spin_lock_t;
#define gx_spin_lock_init(l)              do {} while(0)
#define gx_spin_lock_bh(l)                cyg_scheduler_lock()
#define gx_spin_unlock_bh(l)              cyg_scheduler_unlock()
#define gx_spin_lock_irq(l)               cyg_scheduler_lock()
#define gx_spin_lock(l)                   cyg_scheduler_lock()
#define gx_spin_unlock(l)                 cyg_scheduler_unlock()
#define gx_spin_lock_irqsave(l, f)        gx_interrupt_disable();cyg_scheduler_lock();   (void)f
#define gx_spin_unlock_irqrestore(l, f)   gx_interrupt_enable(); cyg_scheduler_unlock(); (void)f

	// mutex lock
	typedef struct cyg_mutex_t gx_mutex_t;
#define gx_mutex_init(m)                  cyg_mutex_init(m)
#define gx_mutex_lock(m)                  cyg_mutex_lock(m)
#define gx_mutex_trylock(m)               cyg_mutex_trylock(m)
#define gx_mutex_unlock(m)                cyg_mutex_unlock(m)
#define gx_mutex_destroy(m)               cyg_mutex_destroy(m)

#define do_div(a,b) a = (a)/(b)

	/* is x a power of 2? */
#define is_power_of_2(x)    ((x) != 0 && (((x) & ((x) - 1)) == 0))

	typedef cyg_handle_t gx_thread_id;

	typedef cyg_thread gx_thread_info;

	typedef cyg_sem_t gx_sem_id;

	extern void *ioremap(unsigned int offset);
	extern void *request_mem_region(unsigned int len);
	extern int test_bit (int nr, const volatile long unsigned int *);

	extern cyg_uint32 hal_ioremap_cached(cyg_uint32 phy_addr);
	extern cyg_uint32 hal_ioremap_uncached(cyg_uint32 phy_addr);
	extern cyg_uint32 hal_uncached_map(cyg_uint32 addr);
	extern cyg_uint32 hal_cached_map(cyg_uint32 addr);
#endif // END ECOS_OS

	/****************************************************************/

#ifdef NO_OS
#include <common/gx_mem_info.h>
#include <div64.h>
#include <rtc.h>
#include <stdio.h>

#ifndef NULL
#define NULL            ((void*)(0))
#endif

#ifndef size_t
#define size_t           unsigned int
#endif

	struct timeval {
		long    tv_sec;        /* seconds */
		long    tv_usec;    /* and microseconds */
	};

	struct timezone {
		int    tz_minuteswest;    /* minutes west of Greenwich */
		int    tz_dsttime;    /* type of dst correction */
	};

	typedef struct {
		void *sem;
		volatile unsigned int mask;
	} gx_flag_t;

	extern int isxdigit(int ch);
	extern int islower(int ch);
	extern int isdigit(int ch);
	extern int toupper(int ch);
	extern void *malloc(unsigned int size);
	extern void free(void *ptr);
	extern void *memset(void *s, int c, unsigned int count);
	extern int printf(const char *fmt, ...);
	extern void *memcpy(void *dest, const void *src, unsigned int count);
	extern int memcmp(void *dest, const void *src, unsigned int count);
	extern void *memmove(void *dest, const void *src, unsigned int count);
	extern int sprintf(char *str, const char *format, ...);
	extern int abs(int j);
	extern size_t strlen(const char *s);
	extern char *strcpy(char *dest, const char *src);
	extern char *strcat(char *dest, const char *src);
	extern int strcasecmp(const char *s1, const char *s2);
	extern char *strncpy(char *dest, const char *src, size_t n);
	extern char *strsep(char **s, const char *ct);

#define PAGE_MAX   (1024)

#define PAGE_SHIFT (12)
#define MAX_KMALLOC_SIZE (0xffffffff)

#define gx_trace(fmt, args...)              printf(fmt, ##args)
#if (GX_DEBUG_PRINTF_LEVEL >= 2)
#define gx_printf(fmt, args...)             printf(fmt, ##args)
#else
#define gx_printf(fmt, args...)             ((void)0)
#endif
#define gx_sprintf(buf, fmt, args...)       sprintf(buf, fmt, ##args)
#define gx_snprintf(buf, len, fmt, args...) sprintf(buf, fmt, ##args)
#define gx_vsprintf(buf, fmt, args)         vsprintf(buf, fmt, args)

#define gx_ioremap(offset, size)            ioremap(offset)
#define gx_iounmap(addr)

#define gx_request_mem_region(start,len) request_mem_region(len)
#define gx_release_mem_region(start,len)

	// memory
#define _malloc(size)                    malloc(size)
#define _free(p)                         free(p)
#define _vmalloc(size)                   malloc(size)
#define _vfree(p)                        free(p)

	/* is x a power of 2? */
#define is_power_of_2(x)    ((x) != 0 && (((x) & ((x) - 1)) == 0))

	typedef int gx_thread_id;

	typedef int gx_thread_info;

	typedef int gx_sem_id;
	typedef int cyg_flag_t ;

	extern void mdelay(unsigned int msec);
	extern void udelay(unsigned int usec);
	extern void *ioremap(unsigned int offset);
	extern void *request_mem_region(unsigned int len);
	extern int test_bit (int nr, const volatile long unsigned int *);

#define gx_mdelay(n) gx_rtc_delay_ms(n)
#define gx_udelay(n) gx_rtc_delay_us(n)
#define gx_msleep(n) gx_rtc_delay_ms(n)

	// spinlock
	typedef int gx_spin_lock_t;
#define gx_spin_lock_init(l)
#define gx_spin_lock_bh(l)
#define gx_spin_unlock_bh(l)
#define gx_spin_lock_irq(l)
#define gx_spin_lock(l)
#define gx_spin_unlock(l)
#define gx_spin_lock_irqsave(l, f) (void)f
#define gx_spin_unlock_irqrestore(l, f) (void)f

#define cyg_flag_init(wq)
#define cyg_flag_destroy(wq)
#define cyg_flag_maskbits(wq, mask)
#define cyg_flag_setbits(wq, mask)

	// mutex lock
	typedef int gx_mutex_t;
	extern int gx_mutex_init(int* m);
	extern int gx_mutex_lock(int* m);
	extern int gx_mutex_trylock(int* m);
	extern int gx_mutex_unlock(int* m);
	extern int gx_mutex_destroy(int* m);

#endif // END NO_OS

	/****************************************************************/

#ifdef TEE_OS

#include <gxcommon.h>
#include <cmdline.h>
#include "chip_init.hxx"

#ifndef NULL
#define NULL            ((void*)(0))
#endif

#ifndef size_t
#define size_t           unsigned int
#endif

	struct timeval {
		long    tv_sec;        /* seconds */
		long    tv_usec;    /* and microseconds */
	};

	struct timezone {
		int    tz_minuteswest;    /* minutes west of Greenwich */
		int    tz_dsttime;    /* type of dst correction */
	};

	typedef struct {
		void *sem;
		volatile unsigned int mask;
	} gx_flag_t;

#define PAGE_MAX   (1024)

#define PAGE_SHIFT (12)
#define MAX_KMALLOC_SIZE (0xffffffff)

#define gx_trace(fmt, args...)              MSG(fmt, ##args)
#if (GX_DEBUG_PRINTF_LEVEL >= 2)
#define gx_printf(fmt, args...)             MSG(fmt, ##args)
#else
#define gx_printf(fmt, args...)             ((void)0)
#endif
#define gx_snprintf(buf, len, fmt, args...) snprintf(buf, len, fmt, ##args)
#define gx_vsprintf(buf, fmt, args)         vsprintf(buf, fmt, args)

#define gx_ioremap(offset, size)            ioremap(offset)
#define gx_iounmap(addr)

#define gx_request_mem_region(start,len) request_mem_region(len)
#define gx_release_mem_region(start,len)

	// memory
#define _malloc(size)                    malloc(size)
#define _free(p)                         free(p)
#define _vmalloc(size)                   malloc(size)
#define _vfree(p)                        free(p)

#define do_div(a,b) a = (a)/(b)

	/* is x a power of 2? */
#define is_power_of_2(x)    ((x) != 0 && (((x) & ((x) - 1)) == 0))

	// mutex lock
	typedef struct mutex gx_mutex_t;
#define gx_mutex_init(m)                  mutex_init(m)
#define gx_mutex_lock(m)                  mutex_lock(m)
#define gx_mutex_trylock(m)               mutex_trylock(m)
#define gx_mutex_unlock(m)                mutex_unlock(m)
#define gx_mutex_destroy(m)               mutex_destroy(m)

	typedef int gx_thread_id;

	typedef int gx_thread_info;

	typedef int gx_sem_id;

	extern void *ioremap(unsigned int offset);
	extern void *request_mem_region(unsigned int len);
	extern int test_bit (int nr, const volatile long unsigned int *);

#define gx_mdelay(n) mdelay(n)
#define gx_udelay(n) udelay(n)

	// spinlock
	typedef int gx_spin_lock_t;
#define gx_spin_lock_init(l)
#define gx_spin_lock_bh(l)
#define gx_spin_unlock_bh(l)
#define gx_spin_lock_irq(l)
#define gx_spin_lock(l)
#define gx_spin_unlock(l)
#define gx_spin_lock_irqsave(l, f) (void)f
#define gx_spin_unlock_irqrestore(l, f) (void)f

#endif // END TEE_OS

	/****************************************************************/


#ifdef UCOS_OS
#include <drv/gxos.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <drv/api_serial.h>
#include <stdio.h>

#ifndef NULL
#define NULL            ((void*)(0))
#endif

#ifndef size_t
#define size_t           unsigned int
#endif

	struct timeval {
		long    tv_sec;        /* seconds */
		long    tv_usec;    /* and microseconds */
	};

	struct timezone {
		int    tz_minuteswest;    /* minutes west of Greenwich */
		int    tz_dsttime;    /* type of dst correction */
	};

	typedef struct {
		void *sem;
		volatile unsigned int mask;
	} gx_flag_t;


#define EINVAL     2
#define PAGE_MAX   (1024)

#define PAGE_SHIFT (12)
#define MAX_KMALLOC_SIZE (0xffffffff)

#define gx_trace(fmt, args...)              printf(fmt, ##args)
#if (GX_DEBUG_PRINTF_LEVEL >= 2)
#define gx_printf(fmt, args...)             printf(fmt, ##args)
#else
#define gx_printf(fmt, args...)             ((void)0)
#endif
#define gx_sprintf(buf, fmt, args...)       sprintf(buf, fmt, ##args)
#define gx_snprintf(buf, len, fmt, args...) sprintf(buf, fmt, ##args)
#define gx_sscanf(buf, fmt, args...)        sscanf(buf, fmt, ##args)
#define gx_vsprintf(buf, fmt, args)         vsprintf(buf, fmt, args)

#define gx_ioremap(offset,size)          ioremap(offset)
#define gx_iounmap(addr)

#define gx_request_mem_region(start,len) request_mem_region(len)
#define gx_release_mem_region(start,len)

	// memory
#define _malloc(size)                    malloc(size)
#define _free(p)                         free(p)
#define _vmalloc(size)                   malloc(size)
#define _vfree(p)                        free(p)

	extern void udelay(UINT32 us);
	extern void mdelay(UINT32 ms);
	extern void gx_msleep(UINT32 ms);
#define gx_mdelay(n)                     mdelay(n)
#define gx_udelay(n)                     udelay(n)

	// spinlock
	typedef int gx_spin_lock_t;
#define gx_spin_lock_init(l)             do {} while(0)
#define gx_spin_lock_bh(l)               drv_sched_lock()
#define gx_spin_unlock_bh(l)             drv_sched_unlock()
#define gx_spin_lock_irq(l)              drv_sched_lock()
#define gx_spin_lock(l)                  drv_sched_lock()
#define gx_spin_unlock(l)                drv_sched_unlock()
#define gx_spin_lock_irqsave(l, f)       drv_sched_lock(); (void)f
#define gx_spin_unlock_irqrestore(l, f)  drv_sched_unlock(); (void)f

	// mutex lock
	typedef void* gx_mutex_t;
#define gx_mutex_init(m)                 *(gx_mutex_t*)m = drv_mutex_create()
#define gx_mutex_lock(m)                 drv_mutex_wait((*(gx_mutex_t*)m), 0)
#define gx_mutex_trylock(m)              drv_mutex_wait((*(gx_mutex_t*)m), 0)
#define gx_mutex_unlock(m)               drv_mutex_post((*(gx_mutex_t*)m))
#define gx_mutex_destroy(m)

#define do_div(a,b) a = (a)/(b)

	/* is x a power of 2? */
#define is_power_of_2(x)    ((x) != 0 && (((x) & ((x) - 1)) == 0))

	typedef unsigned char gx_thread_id;
	typedef unsigned int  gx_thread_info;

	typedef void*         gx_sem_id;

	extern void *ioremap(unsigned int offset);
	extern void *request_mem_region(unsigned int len);
	extern int test_bit (int nr, const volatile long unsigned int *);

#endif // END UCOS_OS

	/****************************************************************/

	//reg
#define gx_ioread32(addr)          (*(volatile unsigned int *)(addr))
#define gx_iowrite32(value, addr)  (*(volatile unsigned int *)(addr)=(value))
#define gx_clear_bit(nr, addr)     (*(volatile unsigned int *)addr) &= ~(1<<(nr))
#define gx_set_bit(nr, addr)       (*(volatile unsigned int *)addr) |= 1<<(nr)
#ifndef TEE_OS
#define gx_test_bit(nr, addr)      test_bit(nr,(const volatile long unsigned int *)addr)
#endif

	// string
#define gx_memcpy                 memcpy
#define gx_memset(dest,v,len)     memset(dest,v,len)
#define gx_memcmp(dest,src,len)   memcmp(dest,src,len)
#define gx_strncpy(dest,src,len)  strncpy(dest,src,len)
#define gx_strcat(dest,src)       strcat(dest,src)
#define gx_strcpy(dest,src)       strcpy(dest,src)
#define gx_strlen(str)            strlen(str)

#ifdef GX_DEBUG
	extern void dbgassert(const char *fcn, int line, const char *expr);
#define GXAV_ASSERT(e)           ((e) ? (void) 0 : dbgassert(__FUNCTION__, __LINE__, #e))
#define GXAV_DBGEV(fmt, args...) gx_printf(fmt, ##args)
#define GXAV_DBG(fmt, args...)   gx_printf(fmt, ##args)
#else
#define GXAV_ASSERT(e)           ((void)0)
#define GXAV_DBGEV(fmt, args...) ((void)0)
#define GXAV_DBG(fmt, args...)   ((void)0)
#endif

	/************************************************************************************/
	extern unsigned int gxcore_chip_probe(void);
	extern unsigned int gxcore_chip_sub_probe(void);
	extern void *gx_sdc_memcpy(void *dst, const void *src, int len);

	/* DMA Cache Coherency
	 *
	 * gx_dcache_inv_range(start, end)
	 *
	 *      Invalidate (discard) the specified virtual address range.
	 *      May not write back any entries.  If 'start' or 'end'
	 *      are not cache line aligned, those lines must be written
	 *      back.
	 *      - start  - virtual start address
	 *      - end    - virtual end address
	 *
	 * gx_dcache_clean_range(start, end)
	 *
	 *      Clean (write back) the specified virtual address range.
	 *      - start  - virtual start address
	 *      - end    - virtual end address
	 *
	 * gx_dcache_flush_range(start, end)
	 *
	 *      Clean and invalidate the specified virtual address range.
	 *      - start  - virtual start address
	 *      - end    - virtual end address
	 */

#ifndef LINUX_OS
	enum dma_data_direction {
		DMA_BIDIRECTIONAL = 0,
		DMA_TO_DEVICE = 1,
		DMA_FROM_DEVICE = 2,
		DMA_NONE = 3,
	};
#endif

	// CACHE
	extern void *kcache_create(const char *, size_t, size_t);
	extern void kcache_destroy(void*);
	extern void *kcache_alloc(void*, size_t);
	extern void kcache_free(void*, void *);

	extern void gx_dcache_inv_range(unsigned int start, unsigned int end);
	extern void gx_dcache_clean_range(unsigned int start, unsigned int end);
	extern void gx_dcache_flush_range(unsigned int start, unsigned int end);

	extern void *gx_dma_malloc(size_t size);
	extern void  gx_dma_free(void* p, size_t size);
	extern unsigned int gx_dma_to_phys(unsigned int addr);
	extern unsigned int gx_virt_to_phys(unsigned int addr);
	extern unsigned int gx_phys_to_virt(unsigned int addr);

	extern void gx_interrupt_disable(void);
	extern void gx_interrupt_enable(void);
	extern void gx_interrupt_mask(unsigned int vector);
	extern void gx_interrupt_unmask(unsigned int vector);
	extern int  gx_interrupt_masked(unsigned int vector);

	extern void *gx_realloc(void *mem_address, unsigned int old_size, unsigned int new_size);

	extern unsigned char *gx_page_malloc(unsigned long size);
	extern void gx_page_free(unsigned char * p, unsigned long size);

	extern int gx_copy_to_user  (void *to, const void *from, unsigned int n);
	extern int gx_copy_from_user(void *to, const void *from, unsigned int n);

	extern int gx_wait_event(void *module, unsigned int event_mask, int timeout_us);
	extern int gx_wake_event(void *module, unsigned int event);

	extern int gx_thread_create(const char *thread_name, gx_thread_id *thread_id,
			void(*entry_func)(void *), void *arg,
			void *stack_base,
			unsigned int stack_size,
			unsigned int priority,
			gx_thread_info *thread_info);
	extern int gx_thread_delete (gx_thread_id thread_id);
	extern int gx_thread_should_stop(void);
	extern int gx_thread_delay(unsigned int millisecond);

	extern int gx_sem_create(gx_sem_id *sem_id, unsigned int sem_init_val);
	extern int gx_sem_delete(gx_sem_id *sem_id);
	extern int gx_sem_post  (gx_sem_id *sem_id);
	extern int gx_sem_wait  (gx_sem_id *sem_id);
	extern int gx_sem_wait_timeout (gx_sem_id *sem_id,long timeout);
	extern int gx_sem_trywait  (gx_sem_id *sem_id);

	extern int gx_queue_create(unsigned int queue_depth, unsigned int data_size);
	extern int gx_queue_delete (int queue_id);
	extern int gx_queue_get(int queue_id, char *data, unsigned int size, int timeout);
	extern int gx_queue_put(int queue_id, char *data, unsigned int size);

	extern int gx_alarm_create (unsigned int *handle,void (*alarmfn)(unsigned int,unsigned int),unsigned int millisecond);
	extern int gx_alarm_delete (unsigned int *handle);
	extern int gx_alarm_enable (unsigned int *handle);
	extern int gx_alarm_disable(unsigned int *handle);

	extern unsigned long long gx_current_tick(void);
	extern int gx_gettimeofday(struct timeval *tv, struct timezone *tz);
#if defined ECOS_OS || defined UCOS_OS
	extern unsigned long long gx_tick_start(unsigned long timeout_ms);
	extern int gx_tick_end(unsigned long long tick);
#else
	extern unsigned long gx_tick_start(unsigned long timeout_ms);
	extern int gx_tick_end(unsigned long tick);
#endif

#ifdef AV_CONFIG_MEMORY_DEBUG
	void *gxav_malloc_debug(const char *file, int line, size_t size);
	void *gxav_mallocz_debug(const char *file, int line, size_t size);
	void  gxav_free_debug(const char *file, int line, void *ptr);
	void  gxav_memory_show_debug(int nofree_only);
	void  gxav_memory_check_debug(void);

#define gx_malloc(size)              gxav_malloc_debug(__FILE__, __LINE__, size)
#define gx_mallocz(size)             gxav_mallocz_debug(__FILE__, __LINE__, size)
#define gx_free(ptr)                 gxav_free_debug(__FILE__, __LINE__, ptr)
#define gx_memory_check()            gxav_memory_check_debug()
#define gx_memory_show(stat)         gxav_memory_show_debug(stat)
#else
	void *gxav_malloc_release(size_t size);
	void *gxav_mallocz_release(size_t size);
	void  gxav_free_release(void *ptr);

#ifndef TEE_OS
#define gx_malloc(size)              gxav_malloc_release(size)
#define gx_free(ptr)                 gxav_free_release(ptr)
#define gx_memory_check()
#define gx_memory_show(stat)
#endif

#define gx_mallocz(size)             gxav_mallocz_release(size)
#endif

#ifdef CONFIG_SMP
#include <linux/smp.h>
#include <asm/barrier.h>
#define gx_smp_mb() mb()
#else
#define gx_smp_mb() ((void)0)
#endif

#define __weak    __attribute__((weak))
#define __unused  __attribute__((unused))

	/*******************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // __KERNELCALLS_H__

