#ifndef __IRR_3XXX_H
#define __IRR_3XXX_H

#include <linux/kfifo.h>

#define MHz (1000U*1000U)

#define IRR_MODULE_EN(_regs_) (_regs_->irr_cntl |= (1 << 3))
#define IRR_MODULE_DIS(_regs_) (_regs_->irr_cntl &= ~(1 << 3))

//IRR protocol ------------------
#define IRR_PROTCOL_TIME                    (562)
#define IRR_STD_UINT_SIG_ZERO               (IRR_PROTCOL_TIME << 2)

#define IRR_STD_UINT_SIG_ONE                (IRR_PROTCOL_TIME << 1)
#define IRR_AVERAGE_PULSE_WIDTH \
	((IRR_STD_UINT_SIG_ZERO + IRR_STD_UINT_SIG_ONE) >> 1)

#define IRR_STD_UINT_SIG_ZERO_STB40         (3*IRR_PROTCOL_TIME )
#define IRR_AVERAGE_PULSE_WIDTH_STB40 \
	((IRR_STD_UINT_SIG_ZERO_STB40 + IRR_STD_UINT_SIG_ONE) >> 1)

/* the follwing macro constants are decide by HARDWARE */
#define IRR_MAX_PULSE_NUM                    ( 64)
#define IRR_MAX_SIMCODE_PULSE_NUM            (  3)
#define IRR_MAX_FULLCODE_DVB40BIT_PULSE_NUM  ( 42)
#define IRR_MAX_FULLCODE_PANASONIC_PULSE_NUM ( 49)
#define IRR_MAX_FULLCODE_PULSE_NUM           ( 33)
#define IRR_MAX_PULSE_NUM_PHILIPS            ( 13)
#define IRR_MIN_PULSE_NUM_PHILIPS            (  6)
#define IRR_MAX_PULSE_NUM_BESCON             ( 16)
#define IRR_MIN_PULSE_NUM_BESCON             (  9)
#define IRR_EMBEDDED_NOISE_ONCE              ( 34)
#define IRR_EMBEDDED_NOISE_MORE              ( 48)

#define IRR_RC5_MAX_PULSE_NUM                ( 20)
#define IRR_RC5_MIN_PULSE_NUM                (  6)
#define IRR_RC5_PROTCOL_TIME                 (889)

// 1M频率下philips1位高电平或低电平的计数值
#define IRR_PROTCOL_TIME_PHILIPS             (888)

// 1M频率下bescon 1位高电平或低电平的计数值,偏大点
#define IRR_PROTCOL_TIME_BESCON              (440)

// 1M频率下panasonic 1位高电平或低电平的计数值
#define IRR_PROTCOL_TIME_PANASONIC           (380)

//Ioctl definitions
#define IRR_SET_MODE 				(0x1000)
#define IRR_SIMPLE_IGNORE_CONFIG          (0x3000)
#define IRR_DISABLE                       (0x4000)
#define IRR_ENABLE                        (0x5000)


struct irr_regs {
	unsigned int irr_cntl;
	unsigned int irr_int;
	unsigned int irr_fifo;
	unsigned int irr_div;
};

typedef enum irr_protocol_e {
	IRR_PROTOCOL_NEC,
	IRR_PROTOCOL_PHILIPS,
}irr_protocol_t;

struct gx_irr_info {
	/* irr info */
	unsigned int irr_base;
	unsigned char irr_isrvec;
	unsigned int sys_clk;
	unsigned int irr_clk;

	struct mutex lock;
	struct completion comp;
	int irr_mode;
	unsigned int key_code;
	unsigned int counter_simple;
	struct timer_list timer;
	unsigned timer_added:1;

	unsigned int pulse_val[IRR_MAX_PULSE_NUM];
	unsigned int pulse_num;

	/* device info */
	struct resource *area;
	struct irr_regs __iomem *regs;

	struct irr_algorithm *algo;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	struct kfifo queue;
#else
	struct kfifo *queue;
	spinlock_t spin_lock;
#endif

	struct input_dev* input;
	unsigned int keymap_num;
	wait_queue_head_t irr_read_queue;

	struct device *dev;
};
struct irr_algorithm {
	int (*functionality) (struct gx_irr_info *);
};

//#define IRR_DEBUG
#ifdef IRR_DEBUG
#define irr_trace(FORMAT, ARGS...) printk("<trace>: ""%s()""[%d]   "FORMAT, __FUNCTION__, __LINE__, ##ARGS)
#define irr_debug(FORMAT, ARGS...) printk("<debug>: ""%s()""[%d]   "FORMAT, __FUNCTION__, __LINE__, ##ARGS)
#define irr_error(FORMAT, ARGS...) printk("<error>: ""%s()""[%d]   "FORMAT, __FUNCTION__, __LINE__, ##ARGS)
#else
#define irr_trace(FORMAT, ARGS...)do {} while (0)
#define irr_debug(FORMAT, ARGS...)do {} while (0)
#define irr_error(FORMAT, ARGS...)do {} while (0)
#endif

#endif

