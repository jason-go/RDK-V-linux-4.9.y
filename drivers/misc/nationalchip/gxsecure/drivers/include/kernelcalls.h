#ifndef __KERNEL_CALLS_H__
#define __KERNEL_CALLS_H__

#include <log.h>
#include <gx_user_defines.h>

/************************** SCPU ***************************/
#if defined (CPU_SCPU)

#include <stdio.h>
#include <string.h>

unsigned int   BIG_TO_LITTLE  (unsigned int value);
unsigned int   ENDIAN_SWITCH  (unsigned int value);
unsigned short ENDIAN_SWITCH16(unsigned short value);

typedef int gx_spin_lock_t;
#define gx_spin_lock_init(l)
#define gx_spin_lock_irqsave(l, f)       (void)f
#define gx_spin_unlock_irqrestore(l, f)  (void)f

typedef int gx_mutex_t;
#define gx_mutex_init(m)     (void)m
#define gx_mutex_lock(m)     (void)m
#define gx_mutex_unlock(m)   (void)m
#define gx_mutex_destroy(m)  (void)m

typedef int gx_event_t;
#define gx_event_init(event)
#define gx_event_deinit(event)
#define gx_wake_event(event, bits)
#define gx_mask_event(event, bits)
#define gx_wait_event(event, value, mask) ({0})
#define gx_wait_event_timeout(event, value, mask, timeout) ({gx_msleep(timeout); mask;})

#define gx_virt_to_phys(addr) (unsigned int)(addr)
#define gx_phys_to_virt(addr) (unsigned int)(addr)
#define gx_request_mem_region(pos, size)  (void *)0x12345678
#define gx_release_mem_region(pos, size)
#define gx_ioremap(offset, size)          (void *)(offset)
#define gx_iounmap(p)

#define gx_swab32        ENDIAN_SWITCH
extern void mdelay(unsigned int ms);
#define gx_msleep        mdelay
extern void udelay(unsigned int us);
#define gx_usleep        udelay
#define gx_printf(fmt, args...)          printf(fmt, ##args)

/************************** ACPU ***************************/
#else

/************************** LINUX ***************************/
#if defined   (LINUX_OS)
#    include <linux/fs.h>
#    include <linux/delay.h>
#    include <linux/module.h>
#    include <linux/poll.h>
#    include <linux/interrupt.h>
#    include <linux/slab.h>
#    include <linux/cdev.h>
#    include <linux/mutex.h>
#    include <linux/mm.h>
#    include <linux/firmware.h>
#    include <linux/dma-mapping.h>
#    include <linux/platform_device.h>
#    include <asm/cacheflush.h>
#    include <linux/io.h>
#    include <linux/version.h>
#    include <linux/gx_mem_info.h>
#    if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,25)
#    include <linux/of.h>
#    include <linux/of_irq.h>
#    endif

void *gx_osdep_request_mem_region(phys_addr_t start, unsigned int size);
void *gx_osdep_ioremap(unsigned int start, unsigned int size);
#define gx_msleep  msleep
#define gx_usleep  udelay
#define gx_swab32  __swab32
#define gx_printf(fmt, args...)           printk(fmt, ##args)
#define gx_malloc(size)                   kmalloc(size, GFP_KERNEL | __GFP_REPEAT)
#define gx_free(p)                        kfree(p);
#define gx_ioremap(offset, size)          gx_osdep_ioremap(offset,size)
#define gx_iounmap(p)                     iounmap((void *)(p))
#define gx_request_mem_region(pos, size)  gx_osdep_request_mem_region(pos, size)
#define gx_release_mem_region(pos, size)  release_mem_region(pos, size)
#define gx_time_after_eq(a, b)            time_after_eq(((unsigned long)(a)), ((unsigned long)(b)))

typedef struct mutex gx_mutex_t;
#define gx_mutex_init(m)                  mutex_init(m)
#define gx_mutex_lock(m)                  mutex_lock(m)
#define gx_mutex_unlock(m)                mutex_unlock(m)
#define gx_mutex_destroy(m)               mutex_destroy(m)

typedef spinlock_t gx_spin_lock_t;
#define gx_spin_lock_init(l)              spin_lock_init(l)
#define gx_spin_lock_irqsave(l, f)        {if(irqs_disabled()) {f = 0;spin_lock(l);} else spin_lock_irqsave(l, f);}
#define gx_spin_unlock_irqrestore(l, f)   {if(f == 0) spin_unlock(l); else spin_unlock_irqrestore(l, f);}

typedef wait_queue_head_t gx_event_t;
#define gx_event_init(event)              init_waitqueue_head(event)
#define gx_event_deinit(event)
#define gx_mask_event(event, bits)
#define gx_wake_event(event, bits)        wake_up_interruptible(event)
#define gx_wait_event(event, value, mask) wait_event_interruptible(*event, (value) & (mask))
#define gx_wait_event_timeout(event, value, mask, timeout) \
	wait_event_interruptible_timeout(*event, (value) & (mask), msecs_to_jiffies(timeout))

typedef wait_queue_head_t gx_select_t;
#define gx_select_init(event)             gx_event_init(event)
#define gx_select_wake(event, bits)       gx_wake_event(event, bits)

#define gx_dma_alloc(pptr, vptr, size)    {vptr = (unsigned int)dma_alloc_coherent(NULL, size, (dma_addr_t *)&pptr, GFP_KERNEL);}
#define gx_dma_free(pptr, vptr, size)     dma_free_coherent(NULL, size, (void *)vptr, pptr);

extern unsigned int gx_osdep_virt_to_phys(unsigned int addr);
extern unsigned int gx_osdep_phys_to_virt(unsigned int addr);
#define gx_virt_to_phys(addr)             gx_osdep_virt_to_phys((unsigned int)addr)
#define gx_phys_to_virt(addr)             gx_osdep_phys_to_virt((unsigned int)addr)

/************************** ECOS ***************************/
#elif defined (ECOS_OS)
#    include <stdlib.h>
#    include <string.h>
#    include <stdio.h>
#    include <linux/kernel.h>
#    include <cyg/kernel/kapi.h>
#    include <sys/param.h>
#    include <sys/endian.h>
#    include <cyg/hal/gx_mem_info.h>

#    include <ctype.h>
#    include <cyg/io/io.h>
#    include <cyg/io/devtab.h>
#    include <cyg/io/dev_regist.h>
#    include <cyg/fileio/fileio.h>
#    include <cyg/hal/drv_api.h>
#    include <cyg/fs/compat.h>

#define gx_swab32  swap32
#define gx_msleep  cyg_thread_delay
#define gx_usleep  cyg_time_delay_us
#define gx_printf(fmt, args...)           diag_printf(fmt, ##args)
#define gx_malloc(size)                   malloc(size)
#define gx_free(p)                        free(p)
#define gx_time_after_eq(a, b)           ((long long)((a) - (b)) >= 0)

extern void *gx_osdep_ioremap(unsigned int offset, unsigned int size);
#define gx_ioremap(offset, size)          gx_osdep_ioremap(offset, size)
#define gx_iounmap(p)
#define gx_request_mem_region(pos, size)  (void *)0x12345678
#define gx_release_mem_region(pos, size)

typedef struct cyg_mutex_t gx_mutex_t;
#define gx_mutex_init(m)                  cyg_mutex_init(m)
#define gx_mutex_lock(m)                  cyg_mutex_lock(m)
#define gx_mutex_unlock(m)                cyg_mutex_unlock(m)
#define gx_mutex_destroy(m)               cyg_mutex_destroy(m)

typedef int gx_spin_lock_t;
#define gx_spin_lock_init(l)              do {} while(0)
#define gx_spin_lock_bh(l)                cyg_scheduler_lock()
#define gx_spin_unlock_bh(l)              cyg_scheduler_unlock()
#define gx_spin_lock_irq(l)               cyg_scheduler_lock()
#define gx_spin_lock(l)                   cyg_scheduler_lock()
#define gx_spin_unlock(l)                 cyg_scheduler_unlock()
#define gx_spin_lock_irqsave(l, f)        gx_osdep_interrupt_disable();cyg_scheduler_lock();   (void)f
#define gx_spin_unlock_irqrestore(l, f)   gx_osdep_interrupt_enable(); cyg_scheduler_unlock(); (void)f

typedef cyg_flag_t gx_event_t;
#define gx_event_init(event)              cyg_flag_init((cyg_flag_t *)event)
#define gx_event_deinit(event)            cyg_flag_destroy((cyg_flag_t *)event)
#define gx_wake_event(event, bits)        cyg_flag_setbits((cyg_flag_t *)event, bits)
#define gx_mask_event(event, bits)        cyg_flag_maskbits((cyg_flag_t *)event, ~(bits))
#define gx_wait_event(event, value, mask) ({!cyg_flag_wait((cyg_flag_t *)event, mask, CYG_FLAG_WAITMODE_OR);})
#define gx_wait_event_timeout(event, value, mask, timeout) \
	cyg_flag_timed_wait((cyg_flag_t *)event, mask, CYG_FLAG_WAITMODE_OR,  timeout + cyg_current_time())

typedef cyg_selinfo gx_select_t;
#define gx_select_init(event)             cyg_selinit((cyg_selinfo *)event)
#define gx_select_wake(event, bits)       cyg_selwakeup((cyg_selinfo *)event)

extern cyg_uint32 hal_cached_to_phys(cyg_uint32 cached_addr);
extern cyg_uint32 hal_ioremap_cached(cyg_uint32 cached_addr);
#define gx_virt_to_phys(addr)            hal_cached_to_phys((cyg_uint32)(addr))
#define gx_phys_to_virt(addr)            hal_ioremap_cached((cyg_uint32 )(addr))

#define gx_dma_alloc(pptr, vptr, size) {vptr = (unsigned int)malloc(size); pptr = vptr;}
#define gx_dma_free(pptr, vptr, size)  {free((void *)vptr); vptr = 0; pptr = 0;}

#define cpu_to_be16(v)   (((v)<< 8) | ((v)>>8))
#define cpu_to_be32(v)   (((v)>>24) | (((v)>>8)&0xff00) | (((v)<<8)&0xff0000) | ((v)<<24))
#define be16_to_cpu(v)   cpu_to_be16(v)
#define be32_to_cpu(v)   cpu_to_be32(v)

#define cpu_to_be16(v)   (((v)<< 8) | ((v)>>8))
#define cpu_to_be32(v)   (((v)>>24) | (((v)>>8)&0xff00) | (((v)<<8)&0xff0000) | ((v)<<24))
#define be16_to_cpu(v)   cpu_to_be16(v)
#define be32_to_cpu(v)   cpu_to_be32(v)

/************************** NOS ***************************/
#elif defined (NOS_OS)
#    include <time.h>
#    include <string.h>
#    include <stdio.h>
#    include <io.h>
#    include <rtc.h>
#    include <interrupt.h>
#    include <common/gx_mem_info.h>

#define gx_printf(fmt, args...)          printf(fmt, ##args)
#define gx_msleep(n)                     gx_rtc_delay_ms(n)
#define gx_usleep(n)                     gx_rtc_delay_us(n)
#define gx_swab32                        swab32
#define gx_time_after_eq(a, b)           ((long)(((unsigned long)(a)) - ((unsigned long)(b))) >= 0)

extern void *gx_osdep_ioremap(unsigned int offset, unsigned int size);
#define gx_ioremap(offset, size)         gx_osdep_ioremap(offset, size)
#define gx_iounmap(p)
#define gx_request_mem_region(pos, size)  (void *)0x12345678
#define gx_release_mem_region(pos, size)

extern void *malloc(size_t size);
extern void free(void *ptr);
#define gx_malloc(size)                  malloc(size)
#define gx_free(p)                       free(p)

typedef int gx_mutex_t;
#define gx_mutex_init(m)                 (*m=0)
#define gx_mutex_lock(m)                 (*m=0)
#define gx_mutex_unlock(m)               (*m=0)
#define gx_mutex_destroy(m)              (*m=0)

typedef int gx_spin_lock_t;
#define gx_spin_lock_init(l)
#define gx_spin_lock_irqsave(l, f)       (void)f
#define gx_spin_unlock_irqrestore(l, f)  (void)f

typedef int gx_event_t;
#define gx_event_init(event)
#define gx_event_deinit(event)
#define gx_wake_event(event, bits)
#define gx_mask_event(event, bits)

// return 0: wait event ok
#define gx_wait_event(event, value, mask) ({ while (!((value) & (mask))); 0;})

// return 0: wait event timeout failed
// return 1: wait event timeout ok
#define gx_wait_event_timeout(event, value, mask, timeout) \
	({ \
		int __ret = 0; \
		unsigned long cur; \
		unsigned long tick = gx_osdep_current_tick(); \
		(void) event; \
		\
		while (!((value) & (mask))) { \
			cur = gx_osdep_current_tick(); \
			if (gx_time_after_eq(cur, tick + timeout)) \
				break; \
		} \
		if ((value) & (mask)) \
			__ret = 1; \
		__ret; \
	})

typedef int gx_select_t;
#define gx_select_init(event)
#define gx_select_wake(event, bits)

#define gx_dma_alloc(pptr, vptr, size) {vptr = (unsigned int)malloc(size); pptr = vptr;}
#define gx_dma_free(pptr, vptr, size)  {free((void *)vptr); vptr = 0; pptr = 0;}

#define gx_virt_to_phys(addr) ((unsigned int)addr)
#define gx_phys_to_virt(addr) ((unsigned int)addr)

#define cpu_to_be16(v)   (((v)<< 8) | ((v)>>8))
#define cpu_to_be32(v)   (((v)>>24) | (((v)>>8)&0xff00) | (((v)<<8)&0xff0000) | ((v)<<24))
#define be16_to_cpu(v)   cpu_to_be16(v)
#define be32_to_cpu(v)   cpu_to_be32(v)

#define cpu_to_be16(v)   (((v)<< 8) | ((v)>>8))
#define cpu_to_be32(v)   (((v)>>24) | (((v)>>8)&0xff00) | (((v)<<8)&0xff0000) | ((v)<<24))
#define be16_to_cpu(v)   cpu_to_be16(v)
#define be32_to_cpu(v)   cpu_to_be32(v)

/************************** TEE_OS ***************************/
#elif defined (TEE_OS)
#    include <string.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <initcall.h>
#    include <io.h>
#    include <keep.h>
#    include <trace.h>
#    include <kernel/interrupt.h>
#    include <kernel/tee_xdev.h>
#    include <gxbitops.h>
#    include <cmdline.h>

#    include <kernel/delay.h>
#    include <kernel/mutex.h>
#    include <kernel/tee_common_otp.h>
#    include <mm/core_mmu.h>
#    include <mm/core_memprot.h>
#    include <tee_api_defines.h>
#    include <gxsecure_dev_init.hxx>

extern void gx_counter_delay_ms(unsigned int delay);
extern void gx_counter_delay_us(unsigned int delay);
extern unsigned int gx_counter_get_ms(void);
#define gx_printf(fmt, args...)  DMSG(fmt, ##args)
#define gx_msleep(n)     gx_counter_delay_ms(n);
#define gx_usleep(n)     gx_counter_delay_us(n);
#define gx_swab32        __compiler_bswap32
#define gx_time_after_eq(a, b)  ((long)(((unsigned long)(a)) - ((unsigned long)(b))) >= 0)

extern void *gx_osdep_ioremap(paddr_t start, unsigned int size);
#define gx_ioremap(offset, size) gx_osdep_ioremap(offset, size)
#define gx_iounmap(p)
#define gx_request_mem_region(pos, size)  (void *)0x12345678
#define gx_release_mem_region(pos, size)

#define gx_malloc(size)  malloc(size)
#define gx_free(p)       free(p)
#define likely(x)        __builtin_expect(!!(x), 1)
#define unlikely(x)      __builtin_expect(!!(x), 0)

#define cpu_to_be16(v)   (((v)<< 8) | ((v)>>8))
#define cpu_to_be32(v)   (((v)>>24) | (((v)>>8)&0xff00) | (((v)<<8)&0xff0000) | ((v)<<24))
#define be16_to_cpu(v)   cpu_to_be16(v)
#define be32_to_cpu(v)   cpu_to_be32(v)

typedef struct mutex gx_mutex_t;
#define gx_mutex_init(m)    mutex_init(m)
#define gx_mutex_lock(m)    mutex_lock(m)
#define gx_mutex_unlock(m)  mutex_unlock(m)
#define gx_mutex_destroy(m) mutex_destroy(m)

typedef int gx_spin_lock_t;
#define gx_spin_lock_init(l)
#define gx_spin_lock_irqsave(l, f)       (void)f
#define gx_spin_unlock_irqrestore(l, f)  (void)f

typedef int gx_event_t;

#define gx_event_init(event)             (void)(event)
#define gx_event_deinit(event)           (void)(event)
#define gx_wake_event(event, bits)       (void)(event)
#define gx_mask_event(event, bits)       (void)(event)
// return 0: wait event ok
#define gx_wait_event(event, value, mask) ({ while (!((value) & (mask))); 0;})

// return 0: wait event timeout failed
// return 1: wait event timeout ok
#define gx_wait_event_timeout(event, value, mask, timeout) \
	({ \
		int __ret = 0; \
		unsigned long cur; \
		unsigned long tick = gx_osdep_current_tick(); \
		(void) event; \
		\
		while (!((value) & (mask))) { \
			cur = gx_osdep_current_tick(); \
			if (gx_time_after_eq(cur, tick + timeout)) \
				break; \
		} \
		if ((value) & (mask)) \
			__ret = 1; \
		__ret; \
	})

typedef int gx_select_t;
#define gx_select_init(event)       (void)(event)
#define gx_select_wake(event, bits) (void)(event)

#define gx_virt_to_phys(addr) (unsigned int)virt_to_phys(addr)
#define gx_phys_to_virt(addr) (unsigned int)phys_to_virt(addr, MEM_AREA_IO_SEC)

#define gx_dma_alloc(pptr, vptr, size) {pptr = vptr = (unsigned int)malloc(size);}
#define gx_dma_free(pptr, vptr, size)  {free((void *)vptr); vptr = 0; pptr = 0;}

#define min(X, Y)               \
	    ({ typeof (X) __x = (X), __y = (Y); \
		      (__x < __y) ? __x : __y; })

#endif /* end of OS   */
#endif /* end of ACPU */

extern unsigned int gx_osdep_chip_probe(void);
extern unsigned int gx_osdep_chip_sub_probe(void);
extern unsigned char *gx_osdep_page_malloc(unsigned long size);
extern void gx_osdep_page_free(unsigned char *p, unsigned long size);
extern void gx_osdep_cache_sync(const void *start, unsigned int size, int direction);
extern void gx_osdep_interrupt_disable(void);
extern void gx_osdep_interrupt_enable(void);
extern void gx_osdep_interrupt_mask(unsigned int vector);
extern void gx_osdep_interrupt_unmask(unsigned int vector);
extern int32_t gx_osdep_probe_irq_byname(const char *name, int32_t *irq);
extern int gx_osdep_request_irq(uint32_t irq, void *isr_cb, void *dsr_cb, void *priv);
extern int gx_osdep_free_irq(uint32_t irq, void *priv);
extern unsigned long long gx_osdep_current_tick(void);
extern void gx_fw_debug_print(unsigned int value);
enum {
	GXSECURE_CHIPID_GX3211  = 0x3211,
	GXSECURE_CHIPID_GX3201  = 0x3201,
	GXSECURE_CHIPID_GX3113C = 0x6131,
	GXSECURE_CHIPID_GX6605S = 0x6605,
	GXSECURE_CHIPID_SIRIUS  = 0x6612,
	GXSECURE_CHIPID_TAURUS  = 0x6616,
	GXSECURE_CHIPID_GEMINI  = 0x6701,
	GXSECURE_CHIPID_CYGNUS  = 0x6705,
} ;

#define INTEGER_MAX_VALUE (0xFFFFFFFF)
#define CHIP_IS_GX3201  (gx_osdep_chip_probe() == GXSECURE_CHIPID_GX3201)
#define CHIP_IS_GX3211  (gx_osdep_chip_probe() == GXSECURE_CHIPID_GX3211)
#define CHIP_IS_GX3113C (gx_osdep_chip_probe() == GXSECURE_CHIPID_GX3113C)
#define CHIP_IS_GX6605S (gx_osdep_chip_probe() == GXSECURE_CHIPID_GX6605S)
#define CHIP_IS_SIRIUS  (gx_osdep_chip_probe() == GXSECURE_CHIPID_SIRIUS)
#define CHIP_IS_TAURUS  (gx_osdep_chip_probe() == GXSECURE_CHIPID_TAURUS)
#define CHIP_IS_GEMINI  (gx_osdep_chip_probe() == GXSECURE_CHIPID_GEMINI)
#define CHIP_IS_CYGNUS  (gx_osdep_chip_probe() == GXSECURE_CHIPID_CYGNUS)

#define GXSE_IOC_DIR(x) ((x) & 0xff000000)
#define GXSE_IOC_NUM(x) ((x) & 0xff)
#define GXSE_IOC_SIZE(x)((x>>16) & 0x0fff)

#define UCHAR_BUF(buf, pos) (((uint8_t *)buf)[pos])
#define gx_swab(ptr) ((UCHAR_BUF(ptr, 0) << 24) | (UCHAR_BUF(ptr, 1) << 16) | (UCHAR_BUF(ptr, 2) << 8) | UCHAR_BUF(ptr, 3))
#define gx_u32(ptr)  ((UCHAR_BUF(ptr, 3) << 24) | (UCHAR_BUF(ptr, 2) << 16) | (UCHAR_BUF(ptr, 1) << 8) | UCHAR_BUF(ptr, 0))
#define gx_endian_set(ptr, big_endian) (uint32_t)(big_endian ? gx_u32(ptr) : gx_swab(ptr))
#define gx_u32_get_val(ptr, big_endian) gx_endian_set(ptr, big_endian)
#define gx_u32_set_val(out, in, big_endian) \
	do{ \
		uint32_t __temp; \
		if (big_endian) \
			memcpy((void *)(out), (void *)(in), 4); \
		else { \
			__temp = gx_u32_get_val(in, big_endian); \
			memcpy((void *)(out), (void *)(&__temp), 4); \
		} \
	} while(0);

static inline void gx_buf_reverse(uint8_t *in, uint32_t inlen, uint8_t *out)
{
	uint32_t i = 0;
	uint8_t tmp_val;

	if (in == out) {
		for(i = 0; i < inlen / 2; i++) {
			tmp_val = out[i];
			out[i] = in[inlen-i-1];
			in[inlen-i-1] = tmp_val;

		}

	} else {
		for(i = 0; i < inlen; i++)
			out[i] = in[inlen-i-1];
	}
}

#endif

