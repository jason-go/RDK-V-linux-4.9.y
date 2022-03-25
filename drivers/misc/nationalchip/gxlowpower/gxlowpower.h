#ifndef __GXLOWPOWER_H__
#define __GXLOWPOWER_H__

// pmu mode
#define  DPRAM_BASE		(0x000)
#define  DPRAM_SIZE		(0x200)
#define  CPU_GPIO_FUNSEL01	(0x200)
#define  CPU_GPIO_FUNSEL10	(0x204)
#define  DRIVER			(0x208)
#define  CPU_CONFIG_REGS	(0x300)
#define  CPU_WR_SRAM		(0x304)
#define  CPU_RD_FSM		(0x308)
#define  MCU_INTR_REG		(0x30C)
#define  CPU_STATE_REG0		(0x310)
#define  CPU_STATE_REG1		(0x314)
#define  CPU_STATE_REG2		(0x318)
#define  CPU_STATE_REG3		(0x31C)

#define  DW8051_SRAM_SIZE       (0x2000)
#define  DW8051_SRAM_BASE       (0x0000)

// mem mode
#define FIRMWARE_OFFSET         (1024)

enum {
	NOT_WAKE_FLAG = 0,
	MANUAL_WAKE_FLAG = 1,
	AUTO_WAKE_FLAG = 2
};

struct gx_tag {
	unsigned int	tag_header;
	char		cmdline[1];     /* this is the minimum size */
};

struct gx_tag_pmu { /*note: The address of each member of the structure must be a multiple of 4 */
	unsigned int	tag_header;
	unsigned int	waketime;
	unsigned int	gpiomask;
	unsigned int	gpiodata;
	unsigned int	keybuf[20];
	unsigned int	keynum;
	unsigned int    powerio[2];
	unsigned int	utctime;	/* utc time day of second */
	unsigned char	wakeflag;
	unsigned char	reserved[3];
	unsigned int	clock_speed;
	unsigned int	panelio[2]; //lowpower panel control io, panelio[0]: clk, panelio[1]: dat
	int             timeshowflag;
	int             timezone;
	int             timesummer;
	/* add new param after this */
};

struct lowpower_info_s {
	u32 WakeTime;
	u32 GpioMask;
	u32 GpioData;
	u32 key;
	u8 *cmdline;
};

struct gx_lowpower_info {
	void __iomem *regs;
	unsigned int reg_addr;
	unsigned int reg_size;
	unsigned int irq;
	struct device *dev;
};

struct gx_lowpower_ops {
	int (*write_cmdline)(unsigned long arg, struct gx_lowpower_info *info);
	int (*load_firmware)(const char *firmware_mem, unsigned int firmware_len, char *load_mem, struct gx_lowpower_info *info);
	int (*set_utctime)(struct gx_lowpower_info *info);
	int (*get_wakeflag)(struct gx_lowpower_info *info);
	int (*enter_lowpower)(struct gx_lowpower_info *info);
};

#define MISC_LOWPOWER_MINOR (MISC_DYNAMIC_MINOR)
#define GXLOWPOWER_FW_NAME "gxlowpower.fw"

#define	LOWPOWER_IOCTL_BASE	'L'

#define LOWPOWER_ENTRY     _IOR(LOWPOWER_IOCTL_BASE, 0, struct lowpower_info_s)
#define LOWPOWER_GETWAKEFLAG     _IOR(LOWPOWER_IOCTL_BASE, 0, unsigned int)

/* Register access macros */
#define lowpower_readl(base, reg)         __raw_readl(base + reg)
#define lowpower_writel(base, reg, value) __raw_writel((value), base + reg)

#define GXLOWPOWER_MAGIC_NUMBER 0x54410003

extern char *next_arg(char *args, char **param, char **val);
#endif
