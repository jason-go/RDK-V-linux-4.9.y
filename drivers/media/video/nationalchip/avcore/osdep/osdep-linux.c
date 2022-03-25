#ifdef LINUX_OS

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/poll.h>
#include <linux/agpgart.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/in.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/mm.h>
#include <linux/writeback.h>
#include <linux/sysctl.h>
#include <linux/gfp.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/kfifo.h>

#include "kernelcalls.h"
#include "osdep-dma.h"
#include "avcore.h"
#include "gxav_common.h"

#ifdef GX_DEBUG
static volatile int page_malloc_count = 0;
static volatile int auto_malloc_count = 0;
static volatile int dma_malloc_count = 0;
static volatile int page_malloc_count_max = 0;
static volatile int auto_malloc_count_max = 0;
static volatile int dma_malloc_count_max = 0;
#endif
extern unsigned int gx_chip_id_probe(void);
extern unsigned int gx_chip_id_sub_probe(void);
unsigned int gxcore_chip_probe(void)
{
#ifdef CONFIG_GX3201
	return GXAV_ID_GX3201;
#endif

#ifdef CONFIG_GX3211
	return GXAV_ID_GX3211;
#endif

#ifdef CONFIG_GX6605S
	return GXAV_ID_GX6605S;
#endif

#ifdef CONFIG_SIRIUS
	return GXAV_ID_SIRIUS;
#endif

#ifdef CONFIG_TAURUS
	return GXAV_ID_TAURUS;
#endif

#ifdef CONFIG_GEMINI
	return GXAV_ID_GEMINI;
#endif
	return 0;
	//return gx_chip_id_probe();
}
unsigned int gxcore_chip_sub_probe(void)
{
	return gx_chip_id_sub_probe();
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0)
#define ioremap_cached ioremap
#else
#ifndef CONFIG_ARM
#define ioremap_cached ioremap
#endif
#endif

int	gx_gettimeofday(struct timeval *tv, struct timezone *tz){
	do_gettimeofday(tv);
	return 0;
}
extern unsigned char *__gx_page_malloc(unsigned long size);
unsigned char *gx_page_malloc(unsigned long size)
{
	return __gx_page_malloc(size);
}

void gx_page_free(unsigned char *p, unsigned long size)
{
	unsigned long real_size = size <= 4096 ? 4096 : size;

#ifdef GX_DEBUG
	page_malloc_count--;
#endif
	free_pages((unsigned long)p, get_order(real_size));
}

static void *gx_dma_alloc_coherent(void* dev, size_t size, void* dma_handle, int flag)
{
	unsigned char *node, *ret;
	dma_addr_t  handle;

#define PAGEALIGN  (1 << PAGE_SHIFT)

	size_t ssize = size + PAGEALIGN;

	node = dma_alloc_coherent(dev, ssize, &handle, flag);
	if(node == NULL) {
		gx_printf("%s,malloc (%d) NULL\n",__func__, ssize);
		return NULL;
	}

	gxav_dmainfo_add((unsigned int)node, (unsigned int)handle, ssize);

	ret = (unsigned char *)((unsigned int)(node + PAGEALIGN - 1) & (~(unsigned int)(PAGEALIGN - 1)));

#ifdef GX_DEBUG
	dma_malloc_count++;
	if (dma_malloc_count > dma_malloc_count_max) {
		dma_malloc_count_max = dma_malloc_count;
		gx_printf("%15s: %3d: %d\n", "dma_alloc", dma_malloc_count, size);
	}
#endif
	return ret;
}

static void gx_dma_free_coherent(void* dev, size_t size, void* dma_addr, void* dma_handle)
{
	if(dma_addr) {
		struct gxav_dmainfo *blk = gxav_dmainfo_find((unsigned int)dma_addr);
		if (blk) {
			dma_free_coherent(dev, blk->size, (void *)blk->virt, blk->phys);
			gxav_dmainfo_remove((unsigned int)dma_addr);
#ifdef GX_DEBUG
			dma_malloc_count--;
#endif
		}
	}
}

void* gx_dma_malloc(size_t size)
{
	return gx_dma_alloc_coherent(NULL, size, NULL, GFP_KERNEL);
}

void  gx_dma_free(void *p, size_t size)
{
	gx_dma_free_coherent(NULL, size, p, NULL);
}

unsigned int gx_dma_to_phys(unsigned int dma_addr)
{
	if(dma_addr){
		struct gxav_dmainfo *blk = gxav_dmainfo_find((unsigned int)dma_addr);
		if (blk) {
			return (blk->phys + (((unsigned int)dma_addr) - blk->virt));
		}
	}
	return 0;
}

inline int gx_copy_to_user(void *to, const void *from, unsigned int n)
{
	if (copy_to_user(to, from, n))
		return -1;

	return 0;
}

inline int gx_copy_from_user(void *to, const void *from, unsigned int n)
{
	if (copy_from_user(to, from, n))
		return -1;

	return 0;
}

void dbgassert(const char *fcn, int line, const char *expr)
{
	int x;
	printk(KERN_ERR "ASSERTION FAILED, %s:%s:%d %s\n", __FILE__, fcn, line, expr);
	x = *(volatile int *)0;	/* force proc to exit */
}

void *gx_sdc_memcpy(void *dst, const void *src, int len)
{
	return memcpy(dst, src, len);
}

static inline void _dcache_sync(const void *start, unsigned int size, int direction)
{
#ifdef CONFIG_ARM
	flush_cache_all();
	outer_flush_all();
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,25)
	flush_cache_mm(NULL);
#else
	extern void dma_cache_maint(const void *start, size_t size, int direction);
	dma_cache_maint(start, size, direction);
#endif
#endif
}

typedef struct {
	unsigned int start;
	unsigned int end;
} DCacheSyncParam;

static void _dcache_inv_range(DCacheSyncParam *param)
{
	unsigned int start = param->start;
	unsigned int size  = param->end - start;

#ifdef CONFIG_ARM
	if (start == 0) {
		_dcache_sync((void*)start, size, DMA_FROM_DEVICE);
	}
	else {
		phys_addr_t phys_start = gx_virt_to_phys(start);
		__cpuc_flush_dcache_area((void*)start, size);
		outer_inv_range(phys_start, phys_start + size);
	}
#else
	_dcache_sync((void*)start, size, DMA_FROM_DEVICE);
#endif
}

static void _dcache_clean_range(DCacheSyncParam *param)
{
	unsigned int start = param->start;
	unsigned int size  = param->end - start;

#ifdef CONFIG_ARM
	if (start == 0) {
		_dcache_sync((void*)start, size, DMA_TO_DEVICE);
	}
	else {
		phys_addr_t phys_start = gx_virt_to_phys(start);
		__cpuc_flush_dcache_area((void*)start, size);
		outer_clean_range(phys_start, phys_start + size);
	}
#else
	_dcache_sync((void*)start, size, DMA_TO_DEVICE);
#endif
}

static void _dcache_flush_range(DCacheSyncParam *param)
{
	unsigned int start = param->start;
	unsigned int size  = param->end - start;

	_dcache_sync((void*)start, size, DMA_BIDIRECTIONAL);
}

void gx_dcache_inv_range(unsigned int start, unsigned int end)
{
	DCacheSyncParam param = {start, end};

#ifdef CONFIG_SMP
	if(unlikely(in_interrupt()) || irqs_disabled()) {
		_dcache_inv_range(&param);
	}
	else {
		smp_mb();
		on_each_cpu((smp_call_func_t)_dcache_inv_range, &param, true);
	}
#else
	_dcache_inv_range(&param);
#endif
}

void gx_dcache_clean_range(unsigned int start, unsigned int end)
{
	DCacheSyncParam param = {start, end};

#ifdef CONFIG_SMP
	if(unlikely(in_interrupt()) || irqs_disabled()) {
		_dcache_clean_range(&param);
	}
	else {
		smp_mb();
		on_each_cpu((smp_call_func_t)_dcache_clean_range, &param, true);
	}
#else
	_dcache_clean_range(&param);
#endif
}

void gx_dcache_flush_range(unsigned int start, unsigned int end)
{
	DCacheSyncParam param = {start, end};

#ifdef CONFIG_SMP
	if(unlikely(in_interrupt()) || irqs_disabled()) {
		_dcache_flush_range(&param);
	}
	else {
		smp_mb();
		on_each_cpu((smp_call_func_t)_dcache_flush_range, &param, true);
	}
#else
	_dcache_flush_range(&param);
#endif
}

#ifdef CONFIG_SMP
#include <linux/smp.h>
static unsigned int  av_interrupt_cnt[CONFIG_NR_CPUS] = { 0, 0,  };
static unsigned long av_interrupt_flag[CONFIG_NR_CPUS];
void gx_interrupt_disable(void)
{
	unsigned int id = smp_processor_id();

	if(!av_interrupt_cnt[id]) {
		local_irq_save(av_interrupt_flag[id]);
	}
	av_interrupt_cnt[id]++;
}

void gx_interrupt_enable(void)
{
	unsigned int id = smp_processor_id();

	if(--av_interrupt_cnt[id] == 0) {
		local_irq_restore(av_interrupt_flag[id]);
	}
}

#else
static unsigned int  av_interrupt_cnt = 0;
static unsigned long av_interrupt_flag;
void gx_interrupt_disable(void)
{
	if(!av_interrupt_cnt) {
		local_irq_save(av_interrupt_flag);
	}
	av_interrupt_cnt++;
}

void gx_interrupt_enable(void)
{
	if(--av_interrupt_cnt == 0) {
		local_irq_restore(av_interrupt_flag);
	}
}
#endif

#ifdef GXAV_ENABLE_DTB
extern int gxav_irq_num[GXAV_MAX_IRQ];
void gx_interrupt_mask(unsigned int vector)
{
	disable_irq(gxav_irq_num[vector]);
}

void gx_interrupt_unmask(unsigned int vector)
{
	enable_irq(gxav_irq_num[vector]);
}
#else
void gx_interrupt_mask(unsigned int vector)
{
	disable_irq(vector);
}

void gx_interrupt_unmask(unsigned int vector)
{
	enable_irq(vector);
}
#endif

//接口返回值为事件返回值
int gx_wait_event(void *module, unsigned int event_mask, int timeout_us)
{
	int event = 0;
	struct gxav_module *m = (struct gxav_module *)module;

	if(m == NULL || m->inode == NULL || m->inode->wq == NULL) {
		// TODO:
		if (timeout_us > 0)
			udelay(timeout_us);
		// gx_printf("%s,module (inode) NULL\n",__func__);
		return -EFAULT;
	}

	GXAV_DBG("start   %s(),module=%02x, event_mask=0x%x event_status = 0x%x \n",
			__FUNCTION__, m->module_type, event_mask, m->inode->event_status);

	if (timeout_us >= 0)
		wait_event_interruptible_timeout(*(wait_queue_head_t *)(m->inode->wq), (event_mask&m->inode->event_status), msecs_to_jiffies(timeout_us/1000));
	else
		wait_event_interruptible(*(wait_queue_head_t *)(m->inode->wq), (event_mask&m->inode->event_status));

	GXAV_DBG("end   %s(),module=%02x, event_mask = 0x%x event_status = 0x%x \n",
			__func__, m->module_type, event_mask, m->inode->event_status);

	event = event_mask&m->inode->event_status;
	gxav_module_inode_clear_event(m->inode, event);

	return event;
}

int gx_wake_event(void *module, unsigned int event)
{
	struct gxav_module *m = (struct gxav_module *)module;

	if(m == NULL || m->inode == NULL || m->inode->wq == NULL) {
		gx_printf("%s,module (inode) NULL\n",__func__);
		return -1;
	}
	gxav_module_inode_set_event(m->inode, event);
	wake_up_interruptible((wait_queue_head_t *)m->inode->wq);

	return 0;
}

unsigned long gx_tick_start(unsigned long timeout_ms)
{
	return jiffies + msecs_to_jiffies(timeout_ms);
}

int gx_tick_end(unsigned long tick)
{
	return time_after_eq(jiffies, tick);
}

int gx_thread_create (const char *thread_name, gx_thread_id *thread_id,
		void(*entry_func)(void *), void *arg,
		void *stack_base,
		unsigned int stack_size,
		unsigned int priority,
		gx_thread_info *thread_info)
{
	struct task_struct *task = NULL;

	task = kthread_create((int (*)(void *))entry_func, arg, "%s",thread_name);
	if(task == NULL)
		return -1;

	GXAV_DBG("%s(),task : %p\n",__func__,task);

	*thread_id = (unsigned int)task;

	GXAV_DBG("%s(),thread_id : 0x%x\n",__func__,*thread_id);

	wake_up_process(task);

	return 0;
}

int gx_thread_delete (gx_thread_id thread_id)
{
	struct task_struct *task = (struct task_struct *)thread_id;

	GXAV_DBG("%s(),thread_id : 0x%x\n",__func__,thread_id);

	GXAV_DBG("%s(),task : %p\n",__func__,task);
	if(task)
	{
		kthread_stop(task);
		task = NULL;
	}

	return 0;
}

int gx_thread_should_stop(void)
{
	return kthread_should_stop();
}

int gx_thread_delay(unsigned int millisecond)
{
	unsigned int ms_loop;
	unsigned int loop;

	ms_loop = millisecond;

	while (ms_loop > 0)
	{
		loop = (ms_loop <= 999) ? ms_loop : 999;

		msleep(loop);

		ms_loop -= loop;
	}

	return 0;
}

int gx_sem_create (gx_sem_id *sem_id, unsigned int sem_init_val)
{
	sema_init(sem_id, (int)sem_init_val);

	return 0;
}

int gx_sem_delete (gx_sem_id *sem_id)
{
	return -1;
}

int gx_sem_post (gx_sem_id *sem_id)
{
	up(sem_id);

	return 0;
}

int gx_sem_wait (gx_sem_id *sem_id)
{
#if 0
	if(down_interruptible(sem_id))
		return -1;
#else
	down(sem_id);
#endif

	return 0;
}

int gx_sem_wait_timeout (gx_sem_id *sem_id,long timeout)
{
	/*
	 * if return value is not zero, may be time out
	 */
	return down_timeout(sem_id,timeout/10);
}

int gx_sem_trywait (gx_sem_id *sem_id)
{
	return (!down_trylock(sem_id) ? 0 : -1);
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

struct list_head *list_get(struct list_head *head)
{
	struct list_head *first = head->next;

	if (first != head)  {
		__list_del(first->prev, first->next);
		return first;
	}
	return 0;
}

void *gx_realloc(void *mem_address, unsigned int old_size, unsigned int new_size)
{
	void *p;

	if(new_size < old_size) {
		return mem_address;
	}

	p = gx_malloc(new_size);
	gx_memcpy(p, mem_address, old_size);
	gx_free(mem_address);

	return p;
}

// CACHE
void *kcache_create(const char *name, size_t size, size_t align)
{
	return kmem_cache_create(name, size, align, SLAB_PANIC, NULL);
}

void kcache_destroy(void* pcache)
{
	kmem_cache_destroy((struct kmem_cache*)pcache);
}

void *kcache_alloc(void* cache, size_t size)
{
	return kmem_cache_alloc((struct kmem_cache*)cache, GFP_KERNEL | __GFP_REPEAT);
}

void kcache_free(void* cache, void *p)
{
	return kmem_cache_free((struct kmem_cache*)cache, p);
}

unsigned long long gx_current_tick(void)
{
	return jiffies;
}

struct proc_dir_entry * gx_proc_create(const char *name, int mode, struct proc_dir_entry *base, void *proc_ops, void *data)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,25)
	// TODO:
	//const struct file_operations proc_fops = {
	//	.owner      = THIS_MODULE,
	//	.open       = NULL,
	//	.read       = NULL,
	//	.llseek     = NULL,
	//	.release    = NULL,
	//};
	return NULL;
#else
	return create_proc_read_entry(name, mode, base, proc_ops, data);
#endif
}

void gx_proc_remove(const char *name, struct proc_dir_entry *entry)
{
	return remove_proc_entry(name, entry);
}

#define AV_MEMHOLE_MAX (10)
static struct {
	unsigned int phys;
	unsigned int virt;
	unsigned int size;
} av_memhole[AV_MEMHOLE_MAX];

static void gxav_memhole_probe(char *name)
{
	int i;

	for (i=0; i<AV_MEMHOLE_MAX; i++) {
		if (av_memhole[i].virt == 0) {
			struct gx_mem_info info;
			int ret = gx_mem_info_get(name, &info);
			if(ret == 0 && info.start) {
				gx_request_mem_region(info.start, info.size);
				av_memhole[i].phys = info.start;
				av_memhole[i].size = info.size;
				av_memhole[i].virt = (unsigned int)ioremap_cached(info.start, info.size);
			}
			return;
		}
	}
}

void gxav_memhole_init(void)
{
	memset(&av_memhole, 0, sizeof(av_memhole));

	gxav_memhole_probe("videomem");
	gxav_memhole_probe("fbmem");
	gxav_memhole_probe("vfwmem");
	gxav_memhole_probe("afwmem");
	gxav_memhole_probe("esamem");
	gxav_memhole_probe("esvmem");
	gxav_memhole_probe("pcmmem");
	gxav_memhole_probe("svpumem");
	gxav_memhole_probe("tsrmem");
	gxav_memhole_probe("tswmem");
}

void gxav_memhole_cleanup(void)
{
	int i;

	for (i=0; i<AV_MEMHOLE_MAX; i++) {
		if (av_memhole[i].virt != 0) {
			gx_iounmap(av_memhole[i].virt);
			gx_release_mem_region(av_memhole[i].phys, av_memhole[i].size);
		}
	}
}

unsigned int gx_virt_to_phys(unsigned int addr)
{
	unsigned int i, phys;

	phys = gx_dma_to_phys(addr);
	if (phys != 0)
		return phys;

	for (i=0; i<AV_MEMHOLE_MAX; i++) {
		if (av_memhole[i].phys == 0)
			break;
		if (addr >= av_memhole[i].virt && addr < av_memhole[i].virt + av_memhole[i].size) {
			return (unsigned int)(av_memhole[i].phys + (addr - av_memhole[i].virt));
		}
	}

	return virt_to_phys((unsigned long *)addr);
}

unsigned int gx_phys_to_virt(unsigned int addr)
{
	int i;
	for (i=0; i<AV_MEMHOLE_MAX; i++) {
		if (av_memhole[i].phys == 0)
			break;
		if (addr >= av_memhole[i].phys && addr < av_memhole[i].phys + av_memhole[i].size) {
			return (av_memhole[i].virt + (addr - av_memhole[i].phys));
		}
	}

	return (unsigned int)phys_to_virt(addr);
}

#if defined CONFIG_AV_ENABLE_TEE && defined CONFIG_OPTEE
#include <linux/tee_drv.h>
static struct file *teefp = NULL;
static unsigned long xdevh = 0;

extern unsigned long gx_xdev_open(struct file *filp, void *path);
extern int gx_xdev_close(struct file *filp, unsigned long h);
extern int gx_xdev_ioctl(struct file *filp, unsigned long h, unsigned int cmd, void *data, unsigned int size);

struct tee_msg {
	unsigned int cmd;
	void *data;
	size_t size;
};

static struct task_struct *tee_task;
static struct kfifo tee_fifo;
static spinlock_t tee_spinlock;
DECLARE_WAIT_QUEUE_HEAD(tee_msg_wait);

static int optee_kthread_func(void *args)
{
	int ret;
	struct tee_msg msg;

	while (1) {
		wait_event_interruptible(tee_msg_wait, kfifo_len(&tee_fifo) != 0);

		ret = 0;
		memset(&msg, 0, sizeof(msg));

		if (!kfifo_is_empty(&tee_fifo)) {
			while (ret != sizeof(struct tee_msg)) {
				ret += kfifo_out_spinlocked(&tee_fifo, (unsigned char *)&msg+ret, sizeof(struct tee_msg)-ret, &tee_spinlock);
			}
		}

		if (msg.cmd == 0) {
			printk("cmd error %d\n", msg.cmd);
			continue;
		}

		//printk("%s: cmd = 0x%x, kfifo_len = %d\n", __func__, msg.cmd, kfifo_len(&tee_fifo));

		gx_xdev_ioctl(teefp, xdevh, msg.cmd, msg.data, msg.size);

		kfree(msg.data);

		if (kthread_should_stop())
			break;
	}

	return 0;
}

static int optee_kthread_init(void)
{
	int ret = 0;

	ret = kfifo_alloc(&tee_fifo, 4096, GFP_KERNEL);
	if (ret)
		return ret;

	spin_lock_init(&tee_spinlock);

	tee_task = kthread_run(optee_kthread_func, NULL, "tee task");
	if (IS_ERR(tee_task)) {
		printk("cant't start tee kthread\n");

		kthread_stop(tee_task);
		kfifo_free(&tee_fifo);
		return -ENOMEM;
	}

	return ret;
}

static int optee_kthread_destroy(void)
{
	kthread_stop(tee_task);
	kfifo_free(&tee_fifo);

	return 0;
}

static int optee_kthread_tigger(unsigned int cmd, void *data, size_t size)
{
	int ret = 0;
	struct tee_msg msg;

	while (kfifo_is_full(&tee_fifo)) {
		printk("[error]: tee msg fifo full!\n");
		wake_up_interruptible(&tee_msg_wait);
	}

	msg.data = kmalloc(size, GFP_KERNEL);
	if (!msg.data) {
		printk("[error]: msg data malloc error!\n");
		return -1;
	}
	msg.cmd = cmd;
	memcpy(msg.data, data, size);
	msg.size = size;

	kfifo_in_spinlocked(&tee_fifo, &msg, sizeof(struct tee_msg), &tee_spinlock);
	wake_up_interruptible(&tee_msg_wait);

	return ret;
}

int gxav_tee_probe(struct platform_device *pdev)
{
	mm_segment_t fs;

	teefp = filp_open("/dev/tee0", O_RDONLY, 0);
	if (IS_ERR(teefp))  {
		printk("@@@@@ %s %d\n", __func__, __LINE__);
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	xdevh = gx_xdev_open(teefp, "/dev/gxav0");

	optee_kthread_init();

	return 0;
}

int gxav_tee_remove(struct platform_device *pdev)
{
	if (teefp) {
		optee_kthread_destroy();
		gx_xdev_close(teefp, xdevh);
		filp_close(teefp, NULL);
		teefp = NULL;
	}

	return 0;
}

int gxav_tee_ioctl(unsigned int cmd, void *data, size_t size)
{
	if (teefp) {
		if(unlikely(in_interrupt()) || irqs_disabled()) {
			return optee_kthread_tigger(cmd, data, size);
		}
		else {
			return gx_xdev_ioctl(teefp, xdevh, cmd, data, size);
		}
	}

	return -1;
}

#else
int gxav_tee_probe(struct platform_device *pdev)
{
	return 0;
}

int gxav_tee_remove(struct platform_device *pdev)
{
	return 0;
}

int gxav_tee_ioctl(unsigned int cmd, void *data, size_t size)
{
	return 0;
}
#endif

EXPORT_SYMBOL(gx_page_malloc);
EXPORT_SYMBOL(gx_page_free);
EXPORT_SYMBOL(gx_dma_alloc_coherent);
EXPORT_SYMBOL(gx_dma_free_coherent);
EXPORT_SYMBOL(gx_dma_to_phys);
EXPORT_SYMBOL(gx_copy_to_user);
EXPORT_SYMBOL(gx_copy_from_user);
EXPORT_SYMBOL(gx_dcache_inv_range);
EXPORT_SYMBOL(gx_dcache_clean_range);
EXPORT_SYMBOL(gx_dcache_flush_range);
EXPORT_SYMBOL(gx_interrupt_disable);
EXPORT_SYMBOL(gx_interrupt_enable);
EXPORT_SYMBOL(gx_interrupt_mask);
EXPORT_SYMBOL(gx_interrupt_unmask);
EXPORT_SYMBOL(gx_wait_event);
EXPORT_SYMBOL(gx_wake_event);
EXPORT_SYMBOL(gx_tick_start);
EXPORT_SYMBOL(gx_tick_end);
EXPORT_SYMBOL(gx_thread_create);
EXPORT_SYMBOL(gx_thread_delete);
EXPORT_SYMBOL(gx_thread_delay);
EXPORT_SYMBOL(gx_sem_create);
EXPORT_SYMBOL(gx_sem_delete);
EXPORT_SYMBOL(gx_sem_post);
EXPORT_SYMBOL(gx_sem_wait);
EXPORT_SYMBOL(gx_sem_wait_timeout);
EXPORT_SYMBOL(gx_sem_trywait);
EXPORT_SYMBOL(gx_virt_to_phys);
EXPORT_SYMBOL(gx_phys_to_virt);


#endif
