#ifndef __GX_USER_DEFS_H__
#define __GX_USER_DEFS_H__

#define ENOERR     0
#define ENOENT     2                     /* No such file or directory */
#define EIO        5                     /* I/O error */
#define EAGAIN     11                    /* Try again later */
#define ENOMEM     12                    /* Out of memory */
#define ENODEV     19                    /* No such device */
#define EINVAL     22                    /* Invalid argument */
#define ENOSYS     38                    /* Function not implemented */
#define EREMOTEIO  121                   /* Remote I/O error */

#define CHECK_RET(ret) {if (ret<0) return ret;}
#define CONVERSION_RET(ret) \
	do{ \
		if (ret == -2) \
			return -EINVAL; \
		else if (ret == -3) \
			return -ENODEV; \
		else if (ret == -4) \
			return -ENOSYS; \
		else \
			return ret; \
	} while(0)

#ifdef CPU_SCPU
extern void gxse_misc_scpu_reset(void);
#define CHECK_ISR_TIMEOUT(module, flag, mask, timeout, queue) \
	do{ \
		mdelay(timeout);\
		if (flag == 1) { \
			flag = 0; \
			gxse_misc_scpu_reset(); \
		\
		} else { \
			flag = 0; \
			return -1; \
		} \
	} while(0)

#else
#define CHECK_ISR_TIMEOUT(module, flag, mask, timeout, queue) \
	{ \
		if ((gx_wait_event_timeout(&queue, flag, mask, timeout)) > 0) { \
			gx_mask_event(&queue, mask); \
		} else { \
			gxlog_e(module, "isr timeout\n"); \
			return -1; \
		}\
	}
#endif

#ifndef LINUX_OS
enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

#define ARRAY_SIZE(x)    (sizeof(x) / sizeof((x)[0]))
#define readl(reg)       ({ unsigned int __v = (*(volatile unsigned int *) (reg)); __v; })
#define writel(val, reg) (void)((*(volatile unsigned int *) (unsigned int)(reg)) = (val))
#endif

#endif
